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
  subscribe("order_book"sv, symbols);
  subscribe("ticker"sv, symbols);
  subscribe("trade"sv, symbols);
  subscribe("market_stats"sv, symbols);
}

void MarketData::subscribe(std::string_view const &channel, std::span<Symbol const> const &symbols) {
  assert(!std::empty(symbols));
  for (auto &item : symbols) {
    auto message = fmt::format(
        R"({{)"
        R"("type":"subscribe",)"
        R"("channel":"{}/1")"
        R"(}})"sv,
        channel);
    log::warn("DEBUG {}"sv, message);
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
  // log::warn(R"(DEBUG message="{}")"sv, message);
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
    log::warn("DEBUG connected={}"sv, connected);
    (*connection_).touch(trace_info.source_receive_time);
  });
}

void MarketData::operator()(Trace<protocol::json::Pong> const &event) {
  profile_.pong([&]() {
    auto &[trace_info, pong] = event;
    log::info<3>("pong={}"sv, pong);
    log::warn("DEBUG pong={}"sv, pong);
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
  profile_.trade([&]() {
    auto &[trace_info, order_book] = event;
    log::info<3>("order_book={}"sv, order_book);
    log::warn("DEBUG order_book={}"sv, order_book);
    (*connection_).touch(trace_info.source_receive_time);
    /*
    for (auto &item : order_book.params) {
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
    */
  });
}

void MarketData::operator()(Trace<protocol::json::Ticker> const &event) {
  profile_.ticker([&]() {
    auto &[trace_info, ticker] = event;
    log::info<3>("ticker={}"sv, ticker);
    log::warn("DEBUG ticker={}"sv, ticker);
    (*connection_).touch(trace_info.source_receive_time);
    /*
    for (auto &item : ticker.params) {
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
    */
  });
}

void MarketData::operator()(Trace<protocol::json::Trade> const &event) {
  profile_.market_stats([&]() {
    auto &[trace_info, trade] = event;
    log::info<3>("trade={}"sv, trade);
    log::warn("DEBUG trade={}"sv, trade);
    (*connection_).touch(trace_info.source_receive_time);
    /*
    for (auto &item : trade.params) {
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
    */
  });
}

void MarketData::operator()(Trace<protocol::json::MarketStats> const &event) {
  profile_.trade([&]() {
    auto &[trace_info, market_stats] = event;
    log::info<3>("market_stats={}"sv, market_stats);
    log::warn("DEBUG market_stats={}"sv, market_stats);
    (*connection_).touch(trace_info.source_receive_time);
    /*
    for (auto &item : market_stats.params) {
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
    */
  });
}

}  // namespace gateway
}  // namespace lighter
}  // namespace roq
