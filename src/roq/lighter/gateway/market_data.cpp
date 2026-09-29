/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/lighter/gateway/market_data.hpp"

#include "roq/logging.hpp"

#include "roq/mask.hpp"

#include "roq/utils/safe_cast.hpp"
#include "roq/utils/update.hpp"

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
    SupportType::MARKET_STATUS,
    SupportType::TOP_OF_BOOK,
    SupportType::MARKET_BY_PRICE,
    SupportType::TRADE_SUMMARY,
    SupportType::STATISTICS,
};

uint64_t const REQUEST_ID = 1'000'000;

size_t const DEPTH_25 = 25;
size_t const DEPTH_50 = 50;

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

auto is_spot(auto api) {
  return api == tools::API::SPOT;
}

auto get_mbp_depth(auto &settings, auto api) -> size_t {
  auto result = settings.ws.mbp_depth;
  if (!result) {
    switch (api) {
      using enum tools::API;
      case UNDEFINED:
        break;
      case SPOT:
        return DEPTH_50;
      case LINEAR:
        return DEPTH_50;
      case INVERSE:
        return DEPTH_50;
      case OPTION:
        return DEPTH_25;
    }
    log::fatal("Unexpected"sv);
  }
  return result;
}

auto create_mbp_topic(size_t depth) {
  return fmt::format("orderbook.{}"sv, depth);
}

struct create_metrics final : public utils::metrics::Factory {
  create_metrics(auto &settings, auto &group, auto const &function) : utils::metrics::Factory{settings.app.name, group, function} {}
};
}  // namespace

// === IMPLEMENTATION ===

MarketData::MarketData(Handler &handler, io::Context &context, uint16_t stream_id, Shared &shared, size_t index)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_)}, index_{index}, ping_frequency_{shared.settings.ws.ping_freq},
      spot_{is_spot(shared.api.api)}, mbp_depth_{get_mbp_depth(shared.settings, shared.api.api)}, mbp_topic_{create_mbp_topic(mbp_depth_)},
      connection_{create_connection(*this, shared.settings, context, shared)}, decode_buffer_{shared.settings.misc.decode_buffer_size, MAX_DECODE_BUFFER_DEPTH},
      request_id_{stream_id_ * REQUEST_ID},
      counter_{
          .disconnect = create_metrics(shared.settings, name_, "disconnect"sv),
      },
      profile_{
          .parse = create_metrics(shared.settings, name_, "parse"sv),
          .response = create_metrics(shared.settings, name_, "response"sv),
          .book_ticker_update = create_metrics(shared.settings, name_, "book_ticker_update"sv),
          .depth_update = create_metrics(shared.settings, name_, "depth_update"sv),
          .trades_update = create_metrics(shared.settings, name_, "trades_update"sv),
          .market_update = create_metrics(shared.settings, name_, "market_update"sv),
          .market_today_update = create_metrics(shared.settings, name_, "market_today_update"sv),
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
      .write(profile_.response, metrics::Type::PROFILE)
      .write(profile_.book_ticker_update, metrics::Type::PROFILE)
      .write(profile_.depth_update, metrics::Type::PROFILE)
      .write(profile_.trades_update, metrics::Type::PROFILE)
      .write(profile_.market_update, metrics::Type::PROFILE)
      .write(profile_.market_today_update, metrics::Type::PROFILE)
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

void MarketData::operator()(Trace<web::socket::Disconnected> const &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
}

void MarketData::operator()(Trace<web::socket::Ready> const &) {
  (*this)(ConnectionStatus::READY);
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

void MarketData::operator()(ConnectionStatus connection_status, std::string_view const &reason) {
  connection_status_ = connection_status;
  TraceInfo trace_info;
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

void MarketData::subscribe(std::span<Symbol const> const &symbols) {
  if (std::empty(symbols)) {
    return;
  }
  subscribe("bookTicker_subscribe"sv, symbols);
  subscribe("depth_subscribe"sv, symbols, 100);  // XXX FIXME TODO limit=100 from settings
  subscribe("trades_subscribe"sv, symbols);
  subscribe("market_subscribe"sv, symbols);
  subscribe("marketToday_subscribe"sv, symbols);
}

void MarketData::subscribe(std::string_view const &method, std::span<Symbol const> const &symbols) {
  assert(!std::empty(symbols));
  auto message = fmt::format(
      R"({{)"
      R"("id":{},)"
      R"("method":"{}",)"
      R"("params":["{}"])"
      R"(}})"sv,
      ++request_id_,
      method,
      fmt::join(symbols, R"(",")"sv));
  log::debug("{}"sv, message);
  (*connection_).send_text(message);
}

void MarketData::subscribe(std::string_view const &method, std::span<Symbol const> const &symbols, size_t limit) {
  assert(!std::empty(symbols));
  for (auto &symbol : symbols) {
    auto message = fmt::format(
        R"({{)"
        R"("id":{},)"
        R"("method":"{}",)"
        R"("params":[)"
        R"("{}",)"
        R"({},)"
        R"("0",)"  // price_interval
        R"(true)"  // subscription is **added**
        R"(])"
        R"(}})"sv,
        ++request_id_,
        method,
        symbol,
        limit);
    log::debug("{}"sv, message);
    (*connection_).send_text(message);
  }
}

void MarketData::send_ping(std::chrono::nanoseconds now) {
  assert(ping_frequency_.count() > 0);
  next_ping_ = now + ping_frequency_;
  auto message = fmt::format(
      R"({{)"
      R"("id":0,)"
      R"("method":"ping",)"
      R"("params":[])"
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

void MarketData::operator()(Trace<protocol::json::Pong> const &event) {
  auto &[trace_info, pong] = event;
  log::info<4>("pong={}"sv, pong);
  (*connection_).touch(trace_info.source_receive_time);
}

void MarketData::operator()(Trace<protocol::json::Response> const &event) {
  profile_.response([&]() {
    auto &[trace_info, response] = event;
    log::info<3>("response={}"sv, response);
    (*connection_).touch(trace_info.source_receive_time);
    if (response.error.code) {
      log::error("response={}"sv, response);
    }
  });
}

void MarketData::operator()(Trace<protocol::json::BookTickerUpdate> const &event) {
  profile_.book_ticker_update([&]() {
    auto &[trace_info, book_ticker_update] = event;
    log::info<3>("book_ticker_update={}"sv, book_ticker_update);
    (*connection_).touch(trace_info.source_receive_time);
    for (auto &item : book_ticker_update.params) {
      auto top_of_book = TopOfBook{
          .stream_id = stream_id_,
          .exchange = shared_.settings.exchange,
          .symbol = item.market,
          .layer{
              .bid_price = item.best_bid_price,
              .bid_quantity = item.best_bid_amount,
              .ask_price = item.best_ask_price,
              .ask_quantity = item.best_ask_amount,
          },
          .update_type = UpdateType::INCREMENTAL,
          .exchange_time_utc = item.transaction_time,
          .exchange_sequence = utils::safe_cast(item.update_id),
          .sending_time_utc = item.message_time,
      };
      create_trace_and_dispatch(shared_.dispatcher, trace_info, top_of_book, true);
    }
  });
}

void MarketData::operator()(Trace<protocol::json::DepthUpdate> const &event) {
  profile_.depth_update([&]() {
    auto &[trace_info, depth_update] = event;
    log::info<3>("depth_update={}"sv, depth_update);
    (*connection_).touch(trace_info.source_receive_time);
    auto helper = [&](auto &result, auto &item) {
      auto mbp_update = MBPUpdate{
          .price = item.price,
          .quantity = item.amount,
          .implied_quantity = NaN,
          .number_of_orders = {},
          .update_action = {},
          .price_level = {},
      };
      result.emplace_back(std::move(mbp_update));
    };
    shared_.bids.clear();
    shared_.asks.clear();
    for (auto &item : depth_update.params.data.bids) {
      helper(shared_.bids, item);
    }
    for (auto &item : depth_update.params.data.asks) {
      helper(shared_.asks, item);
    }
    auto update_type = [&]() {
      if (depth_update.params.snapshot) {
        return UpdateType::SNAPSHOT;
      }
      return UpdateType::INCREMENTAL;
    }();
    auto market_by_price_update = MarketByPriceUpdate{
        .stream_id = stream_id_,
        .exchange = shared_.settings.exchange,
        .symbol = depth_update.params.name,
        .bids = shared_.bids,
        .asks = shared_.asks,
        .update_type = update_type,
        .exchange_time_utc = depth_update.params.data.timestamp,
        .exchange_sequence = utils::safe_cast(depth_update.params.data.update_id),
        .sending_time_utc = {},
        .price_precision = {},
        .quantity_precision = {},
        .checksum = {},
    };
    try {
      create_trace_and_dispatch(shared_.dispatcher, trace_info, market_by_price_update, true, shared_.final_bids, shared_.final_asks);
    } catch (BadState &) {
      log::fatal("HERE"sv);
    }
  });
}

// XXX FIXME TODO drop snapshot
void MarketData::operator()(Trace<protocol::json::TradesUpdate> const &event) {
  profile_.trades_update([&]() {
    auto &[trace_info, trades_update] = event;
    log::info<3>("trades_update={}"sv, trades_update);
    (*connection_).touch(trace_info.source_receive_time);
    auto &trades = shared_.trades;
    trades.clear();
    for (auto &item : trades_update.params.data) {
      auto trade = Trade{
          .side = map(item.type),
          .price = item.price,
          .quantity = item.amount,
          .trade_id = {},  // note!
          .taker_order_id = {},
          .maker_order_id = {},
      };
      fmt::format_to(std::back_inserter(trade.trade_id), "{}"sv, item.id);
      trades.emplace_back(std::move(trade));
    }
    if (!std::empty(trades)) {
      auto trade_summary = TradeSummary{
          .stream_id = stream_id_,
          .exchange = shared_.settings.exchange,
          .symbol = trades_update.params.name,
          .trades = trades,
          .exchange_time_utc = {},
          .exchange_sequence = {},
          .sending_time_utc = {},
      };
      create_trace_and_dispatch(shared_.dispatcher, trace_info, trade_summary, true);
      trades.clear();
    }
  });
}

void MarketData::operator()(Trace<protocol::json::MarketUpdate> const &event) {
  profile_.market_update([&]() {
    auto &[trace_info, market_update] = event;
    log::info<3>("market_update={}"sv, market_update);
    (*connection_).touch(trace_info.source_receive_time);
    std::array<Statistics, 5> statistics{{
        {
            .type = StatisticsType::OPEN_PRICE,
            .value = market_update.params.data.high,
            .begin_time_utc = {},
            .end_time_utc = {},
        },
        {
            .type = StatisticsType::HIGHEST_TRADED_PRICE,
            .value = market_update.params.data.high,
            .begin_time_utc = {},
            .end_time_utc = {},
        },
        {
            .type = StatisticsType::LOWEST_TRADED_PRICE,
            .value = market_update.params.data.low,
            .begin_time_utc = {},
            .end_time_utc = {},
        },
        {
            .type = StatisticsType::CLOSE_PRICE,
            .value = market_update.params.data.close,
            .begin_time_utc = {},
            .end_time_utc = {},
        },
        {
            .type = StatisticsType::TRADE_VOLUME,
            .value = market_update.params.data.volume,
            .begin_time_utc = {},
            .end_time_utc = {},
        },
    }};
    auto statistics_update = StatisticsUpdate{
        .stream_id = stream_id_,
        .exchange = shared_.settings.exchange,
        .symbol = market_update.params.name,
        .statistics = statistics,
        .update_type = UpdateType::INCREMENTAL,
        .exchange_time_utc = {},
        .exchange_sequence = {},
        .sending_time_utc = {},
    };
    create_trace_and_dispatch(shared_.dispatcher, trace_info, statistics_update, true);
  });
}

void MarketData::operator()(Trace<protocol::json::MarketTodayUpdate> const &event) {
  profile_.market_today_update([&]() {
    auto &[trace_info, market_today_update] = event;
    log::info<3>("market_today_update={}"sv, market_today_update);
    (*connection_).touch(trace_info.source_receive_time);
  });
}

}  // namespace gateway
}  // namespace lighter
}  // namespace roq
