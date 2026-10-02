/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/lighter/gateway/market_data.hpp"

#include "roq/logging.hpp"

#include "roq/mask.hpp"

#include "roq/utils/safe_cast.hpp"
#include "roq/utils/update.hpp"

#include "roq/utils/charconv/from_chars.hpp"

#include "roq/utils/hash/fnv.hpp"

#include "roq/utils/exceptions/unhandled.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/lighter/protocol/json/map.hpp"

using namespace std::literals;

namespace roq {
namespace lighter {
namespace gateway {

// === CONSTANTS ===

namespace {
auto const NAME = "md"sv;

auto const SUPPORTS = Mask{
    // SupportType::MARKET_STATUS,
    SupportType::TOP_OF_BOOK,
    SupportType::MARKET_BY_PRICE,
    SupportType::TRADE_SUMMARY,
    SupportType::STATISTICS,
};

size_t const MAX_DECODE_BUFFER_DEPTH = 1;
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id) {
  return fmt::format("{}:{}"sv, stream_id, NAME);
}

auto create_connection(auto &handler, auto &settings, auto &context, auto &shared) {
  auto uri = settings.ws.uri;
  auto config = web::socket::Client::Config{
      // connection
      .interface = {},
      .uris = {&uri, 1},
      .host = settings.ws.host,
      .validate_certificate = settings.net.tls_validate_certificate,
      // connection manager
      .connection_timeout = settings.net.connection_timeout,
      .disconnect_on_idle_timeout = settings.net.disconnect_on_idle_timeout,
      .always_reconnect = true,
      // proxy
      .proxy = {},
      // http
      .user_agent = ROQ_PACKAGE_NAME,
      .request_timeout = {},
      .ping_frequency = settings.ws.ping_freq,
      // implementation
      .decode_buffer_size = settings.misc.decode_buffer_size,
      .encode_buffer_size = settings.misc.encode_buffer_size,
  };
  return web::socket::Client::create(handler, context, config, shared.throttle, []() { return std::string(); });
}

// channel => market_id

constexpr auto extract_market_id(std::string_view const &channel) -> std::string_view {
  auto pos = channel.find(':');
  if (pos == std::string_view::npos) {
    return {};
  }
  return channel.substr(pos + 1);
}

static_assert(extract_market_id(""sv) == ""sv);
static_assert(extract_market_id("order_book:1"sv) == "1"sv);

auto parse_market_id(std::string_view const &channel) -> int32_t {
  auto value = extract_market_id(channel);
  if (!std::empty(value)) {
    return utils::charconv::from_string_relaxed<int32_t>(value);  // note! not constexpr
  }
  return -1;
}

// static_assert(parse_market_id(""sv) == -1);
// static_assert(parse_market_id("order_book:1"sv) == 1);

// type => update_type

constexpr auto extract_update_type(std::string_view const &type) -> std::string_view {
  auto pos = type.find('/');
  if (pos == std::string_view::npos) {
    return {};
  }
  return type.substr(0, pos);
}

static_assert(extract_update_type(""sv) == ""sv);
static_assert(extract_update_type("subscribed/ticker"sv) == "subscribed"sv);
static_assert(extract_update_type("update/ticker"sv) == "update"sv);

constexpr auto parse_update_type(std::string_view const &type) -> UpdateType {
  auto value = extract_update_type(type);
  auto key = utils::hash::FNV::compute(value);
  switch (key) {
    case utils::hash::FNV::compute("subscribed"sv):
      return UpdateType::SNAPSHOT;
    case utils::hash::FNV::compute("update"sv):
      return UpdateType::INCREMENTAL;
  }
  return {};
}

static_assert(parse_update_type(""sv) == UpdateType::UNDEFINED);
static_assert(parse_update_type("subscribed/ticker"sv) == UpdateType::SNAPSHOT);
static_assert(parse_update_type("update/ticker"sv) == UpdateType::INCREMENTAL);

struct create_metrics final : public utils::metrics::Factory {
  create_metrics(auto &settings, auto &group, auto const &function) : utils::metrics::Factory{settings.app.name, group, function} {}
};
}  // namespace

// === IMPLEMENTATION ===

MarketData::MarketData(Handler &handler, io::Context &context, uint16_t stream_id, Shared &shared, size_t index)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_)}, index_{index}, ping_frequency_{shared.settings.ws.ping_freq},
      connection_{create_connection(*this, shared.settings, context, shared)}, decode_buffer_{shared.settings.misc.decode_buffer_size, MAX_DECODE_BUFFER_DEPTH},
      counter_{
          .disconnect = create_metrics(shared.settings, name_, "disconnect"sv),
      },
      profile_{
          .parse = create_metrics(shared.settings, name_, "parse"sv),
          .connected = create_metrics(shared.settings, name_, "connected"sv),
          .pong = create_metrics(shared.settings, name_, "pong"sv),
          .error = create_metrics(shared.settings, name_, "error"sv),
          .order_book = create_metrics(shared.settings, name_, "order_book"sv),
          .ticker = create_metrics(shared.settings, name_, "ticker"sv),
          .trade = create_metrics(shared.settings, name_, "trade"sv),
          .market_stats = create_metrics(shared.settings, name_, "market_stats"sv),
      },
      latency_{
          .ping = create_metrics(shared.settings, name_, "ping"sv),
          .heartbeat = create_metrics(shared.settings, name_, "heartbeat"sv),
      },
      shared_{shared} {
}

void MarketData::operator()(Event<Start> const &) {
  (*connection_).start();
}

void MarketData::operator()(Event<Stop> const &) {
  (*connection_).stop();
}

void MarketData::operator()(Event<Timer> const &event) {
  auto now = event.value.now;
  (*connection_).refresh(now);
  if (ready() && next_ping_ < now) {
    send_ping(now);
  }
}

void MarketData::operator()(metrics::Writer &writer) const {
  writer
      // counter
      .write(counter_.disconnect, metrics::Type::COUNTER)
      // profile
      .write(profile_.parse, metrics::Type::PROFILE)
      .write(profile_.connected, metrics::Type::PROFILE)
      .write(profile_.pong, metrics::Type::PROFILE)
      .write(profile_.error, metrics::Type::PROFILE)
      .write(profile_.order_book, metrics::Type::PROFILE)
      .write(profile_.ticker, metrics::Type::PROFILE)
      .write(profile_.trade, metrics::Type::PROFILE)
      .write(profile_.market_stats, metrics::Type::PROFILE)
      // latency
      .write(latency_.ping, metrics::Type::LATENCY)
      .write(latency_.heartbeat, metrics::Type::LATENCY);
}

void MarketData::subscribe(size_t start_from) {
  if (ready()) {
    subscribe(shared_.symbols.get_slice(index_, start_from));
  }
}

void MarketData::operator()(Trace<web::socket::Connected> const &) {
}

void MarketData::operator()(Trace<web::socket::Disconnected> const &event) {
  auto &[trace_info, disconnected] = event;
  ++counter_.disconnect;
  Trace event_2{trace_info, ConnectionStatus::DISCONNECTED};
  (*this)(event_2);
}

void MarketData::operator()(Trace<web::socket::Ready> const &event) {
  auto &[trace_info, ready] = event;
  Trace event_2{trace_info, ConnectionStatus::READY};
  (*this)(event_2);
  subscribe();
}

void MarketData::operator()(Trace<web::socket::Close> const &) {
}

void MarketData::operator()(Trace<web::socket::Latency> const &event) {
  auto &[trace_info, latency] = event;
  auto external_latency = ExternalLatency{
      .stream_id = stream_id_,
      .account = {},
      .latency = latency.sample,
  };
  create_trace_and_dispatch(shared_.dispatcher, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void MarketData::operator()(Trace<web::socket::Text> const &event) {
  auto &[trace_info, text] = event;
  parse(text.payload);
}

void MarketData::operator()(Trace<web::socket::Binary> const &) {
  log::fatal("Unexpected"sv);
}

void MarketData::operator()(Trace<ConnectionStatus> const &event, std::string_view const &reason) {
  auto &[trace_info, connection_status] = event;
  if (utils::update(connection_status_, connection_status)) {
    auto stream_status = StreamStatus{
        .stream_id = stream_id_,
        .account = {},
        .supports = SUPPORTS,
        .transport = Transport::TCP,
        .protocol = Protocol::WS,
        .encoding = {Encoding::JSON},
        .priority = Priority::PRIMARY,
        .connection_status = connection_status_,
        .reason = reason,
        .interface = (*connection_).get_interface(),
        .authority = (*connection_).get_current_authority(),
        .path = (*connection_).get_current_path(),
        .proxy = (*connection_).get_proxy(),
    };
    log::info("stream_status={}"sv, stream_status);
    create_trace_and_dispatch(shared_.dispatcher, trace_info, stream_status);
  }
}

void MarketData::subscribe(std::span<Symbol const> const &symbols) {
  if (std::empty(symbols)) {
    return;
  }
  subscribe("order_book"sv, symbols);
  subscribe("ticker"sv, symbols);
  subscribe("trade"sv, symbols);
  subscribe("market_stats"sv, symbols);
}

void MarketData::subscribe(std::string_view const &channel, std::span<Symbol const> const &symbols) {
  assert(!std::empty(symbols));
  for (auto &item : symbols) {
    auto market_id = shared_.get_market_id_from_symbol(item);
    if (market_id < 0) {
      log::fatal("Unexpected: internal error"sv);
    }
    auto message = fmt::format(
        R"({{)"
        R"("type":"subscribe",)"
        R"("channel":"{}/{}")"
        R"(}})"sv,
        channel,
        market_id);
    (*connection_).send_text(message);
  }
}

void MarketData::send_ping(std::chrono::nanoseconds now) {
  assert(ping_frequency_.count() > 0);
  next_ping_ = now + ping_frequency_;
  auto message = fmt::format(
      R"({{)"
      R"("type":"ping")"
      R"(}})"sv);
  (*connection_).send_text(message);
}

void MarketData::parse(std::string_view const &message) {
  profile_.parse([&]() {
    auto log_message = [&]() { log::warn(R"(*** PLEASE REPORT *** message="{}")"sv, message); };
    try {
      TraceInfo trace_info;
      if (!protocol::json::Parser::dispatch(*this, message, decode_buffer_, trace_info, shared_.settings.experimental.allow_unknown_event_types)) {
        log_message();
      }
    } catch (...) {
      log_message();
      utils::exceptions::Unhandled::terminate();
    }
  });
}

void MarketData::operator()(Trace<protocol::json::Connected> const &event) {
  profile_.connected([&]() {
    auto &[trace_info, connected] = event;
    log::info<3>("connected={}"sv, connected);
    (*connection_).touch(trace_info.source_receive_time);
  });
}

void MarketData::operator()(Trace<protocol::json::Pong> const &event) {
  profile_.pong([&]() {
    auto &[trace_info, pong] = event;
    log::info<3>("pong={}"sv, pong);
    (*connection_).touch(trace_info.source_receive_time);
  });
}

void MarketData::operator()(Trace<protocol::json::Error> const &event) {
  profile_.error([&]() {
    auto &[trace_info, error] = event;
    log::error("error={}"sv, error);
    (*connection_).touch(trace_info.source_receive_time);
  });
}

void MarketData::operator()(Trace<protocol::json::OrderBook> const &event) {
  profile_.order_book([&]() {
    auto &[trace_info, order_book] = event;
    log::info<3>("order_book={}"sv, order_book);
    (*connection_).touch(trace_info.source_receive_time);
    auto market_id = parse_market_id(order_book.channel);
    if (market_id < 0) [[unlikely]] {
      log::fatal("Unexpected"sv);
    }
    auto symbol = shared_.get_symbol_from_market_id(market_id);
    auto update_type = parse_update_type(order_book.type);
    auto emplace_back = [](auto &result, auto &item) {
      auto mbp_update = MBPUpdate{
          .price = item.price,
          .quantity = item.size,
          .implied_quantity = NaN,
          .number_of_orders = {},
          .update_action = {},
          .price_level = {},
      };
      result.emplace_back(std::move(mbp_update));
    };
    shared_.bids.clear();
    shared_.asks.clear();
    for (auto &item : order_book.order_book.bids) {
      emplace_back(shared_.bids, item);
    }
    for (auto &item : order_book.order_book.asks) {
      emplace_back(shared_.asks, item);
    }
    auto market_by_price_update = MarketByPriceUpdate{
        .stream_id = stream_id_,
        .exchange = shared_.settings.exchange,
        .symbol = symbol,
        .bids = shared_.bids,
        .asks = shared_.asks,
        .update_type = update_type,
        .exchange_time_utc = order_book.order_book.last_updated_at,
        .exchange_sequence = utils::safe_cast(order_book.order_book.nonce),
        .sending_time_utc = order_book.timestamp,
        .price_precision = {},
        .quantity_precision = {},
        .checksum = {},
    };
    try {
      create_trace_and_dispatch(shared_.dispatcher, trace_info, market_by_price_update, true, shared_.final_bids, shared_.final_asks);
    } catch (BadState &) {
      // resubscribe(symbol);
    }
  });
}

void MarketData::operator()(Trace<protocol::json::Ticker> const &event) {
  profile_.ticker([&]() {
    auto &[trace_info, ticker] = event;
    log::info<3>("ticker={}"sv, ticker);
    (*connection_).touch(trace_info.source_receive_time);
    auto update_type = parse_update_type(ticker.type);
    auto top_of_book = TopOfBook{
        .stream_id = stream_id_,
        .exchange = shared_.settings.exchange,
        .symbol = ticker.ticker.symbol,
        .layer{
            .bid_price = ticker.ticker.bid.price,
            .bid_quantity = ticker.ticker.bid.size,
            .ask_price = ticker.ticker.ask.price,
            .ask_quantity = ticker.ticker.ask.size,
        },
        .update_type = update_type,
        .exchange_time_utc = ticker.last_updated_at,
        .exchange_sequence = utils::safe_cast(ticker.nonce),  // ???
        .sending_time_utc = ticker.timestamp,
    };
    create_trace_and_dispatch(shared_.dispatcher, trace_info, top_of_book, true);
  });
}

void MarketData::operator()(Trace<protocol::json::Trade> const &event) {
  profile_.trade([&]() {
    auto &[trace_info, trade] = event;
    log::info<3>("trade={}"sv, trade);
    (*connection_).touch(trace_info.source_receive_time);
    auto update_type = parse_update_type(trade.type);
    if (update_type != UpdateType::INCREMENTAL) {
      return;
    }
    shared_.trades.clear();
    uint32_t market_id = {};
    std::chrono::nanoseconds transaction_time = {};
    for (auto &item : trade.trades) {
      auto side = [&]() {  // note! we need taker's side
        if (item.is_maker_ask) {
          return Side::BUY;
        }
        return Side::SELL;
      }();
      auto trade_2 = Trade{
          .trade_conditions = {},
          .trade_type = {},
          .side = side,
          .price = item.price,
          .quantity = item.size,
          .trade_id = item.trade_id_str,
          .taker_order_id = {},
          .maker_order_id = {},
      };
      shared_.trades.emplace_back(std::move(trade_2));
      utils::update_if_not_empty(market_id, item.market_id);
      utils::update_max(transaction_time, item.transaction_time);
    }
    auto symbol = shared_.get_symbol_from_market_id(market_id);
    if (!std::empty(symbol) && !std::empty(shared_.trades)) {
      auto trade_summary = TradeSummary{
          .stream_id = stream_id_,
          .exchange = shared_.settings.exchange,
          .symbol = symbol,
          .trades = shared_.trades,
          .exchange_time_utc = transaction_time,
          .exchange_sequence = utils::safe_cast(trade.nonce),
          .sending_time_utc = {},
      };
      create_trace_and_dispatch(shared_.dispatcher, trace_info, trade_summary, true);
    }
  });
}

void MarketData::operator()(Trace<protocol::json::MarketStats> const &event) {
  profile_.market_stats([&]() {
    auto &[trace_info, market_stats] = event;
    log::info<3>("market_stats={}"sv, market_stats);
    (*connection_).touch(trace_info.source_receive_time);
    auto update_type = parse_update_type(market_stats.type);
    std::array<Statistics, 5> statistics{{
        {
            .type = StatisticsType::HIGHEST_TRADED_PRICE,
            .value = market_stats.market_stats.daily_price_high,
            .begin_time_utc = {},
            .end_time_utc = {},
        },
        {
            .type = StatisticsType::LOWEST_TRADED_PRICE,
            .value = market_stats.market_stats.daily_price_low,
            .begin_time_utc = {},
            .end_time_utc = {},
        },
        {
            .type = StatisticsType::OPEN_INTEREST,
            .value = market_stats.market_stats.open_interest,
            .begin_time_utc = {},
            .end_time_utc = {},
        },
        {
            .type = StatisticsType::FUNDING_RATE,
            .value = market_stats.market_stats.funding_rate,
            .begin_time_utc = {},
            .end_time_utc = {},
        },
        {
            .type = StatisticsType::FUNDING_RATE_PREDICTION,
            .value = market_stats.market_stats.current_funding_rate,
            .begin_time_utc = {},
            .end_time_utc = {},
        },
    }};
    auto statistics_update = StatisticsUpdate{
        .stream_id = stream_id_,
        .exchange = shared_.settings.exchange,
        .symbol = market_stats.market_stats.symbol,
        .statistics = statistics,
        .update_type = update_type,
        .exchange_time_utc = {},
        .exchange_sequence = {},
        .sending_time_utc = market_stats.timestamp,
    };
    create_trace_and_dispatch(shared_.dispatcher, trace_info, statistics_update, true);
  });
}

}  // namespace gateway
}  // namespace lighter
}  // namespace roq
