/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/lighter/gateway/rest.hpp"

#include "roq/mask.hpp"

#include "roq/utils/safe_cast.hpp"
#include "roq/utils/update.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/lighter/protocol/json/map.hpp"
#include "roq/lighter/protocol/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace lighter {
namespace gateway {

// === CONSTANTS ===

namespace {
auto const NAME = "rest"sv;

auto const SUPPORTS = Mask{
    SupportType::REFERENCE_DATA,
    SupportType::MARKET_STATUS,
};

size_t const MAX_DECODE_BUFFER_DEPTH = 2;
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id) {
  return fmt::format("{}:{}"sv, stream_id, NAME);
}

auto create_connection(auto &handler, auto &settings, auto &context, auto &shared) {
  auto uri = settings.rest.uri;
  auto config = web::rest::Client::Config{
      // connection
      .interface = {},
      .proxy = settings.rest.proxy,
      .uris = {&uri, 1},
      .host = settings.rest.host,
      .validate_certificate = settings.net.tls_validate_certificate,
      // connection manager
      .connection_timeout = {},
      .disconnect_on_idle_timeout = {},
      .connection = web::http::Connection::KEEP_ALIVE,
      // request
      .allow_pipelining = true,
      .request_timeout = settings.rest.request_timeout,
      // response
      .suspend_on_retry_after = {},
      // http
      .query = {},
      .user_agent = ROQ_PACKAGE_NAME,
      .ping_frequency = settings.rest.ping_freq,
      .ping_path = settings.rest.ping_path,
      // implementation
      .decode_buffer_size = settings.misc.decode_buffer_size,
      .encode_buffer_size = settings.misc.encode_buffer_size,
  };
  return web::rest::Client::create(handler, context, config, shared.throttle);
}

struct create_metrics final : public utils::metrics::Factory {
  create_metrics(auto &settings, auto &group, auto const &function) : utils::metrics::Factory{settings.app.name, group, function} {}
};
}  // namespace

// === IMPLEMENTATION ===

Rest::Rest(Handler &handler, io::Context &context, uint16_t stream_id, Shared &shared)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_)}, connection_{create_connection(*this, shared.settings, context, shared)},
      decode_buffer_{shared.settings.misc.decode_buffer_size, MAX_DECODE_BUFFER_DEPTH},
      counter_{
          .disconnect = create_metrics(shared.settings, name_, "disconnect"sv),
      },
      profile_{
          .asset_details = create_metrics(shared.settings, name_, "asset_details"sv),
          .asset_details_ack = create_metrics(shared.settings, name_, "asset_details_ack"sv),
          .order_book_details = create_metrics(shared.settings, name_, "order_book_details"sv),
          .order_book_details_ack = create_metrics(shared.settings, name_, "order_book_details_ack"sv),
      },
      latency_{
          .ping = create_metrics(shared.settings, name_, "ping"sv),
      },
      shared_{shared}, download_{shared.settings.rest.request_timeout, [this](auto &event) { return download(event); }} {
}

// server::Stream

void Rest::operator()(Trace<Start> const &) {
  (*connection_).start();
}

void Rest::operator()(Trace<Stop> const &) {
  (*connection_).stop();
}

void Rest::operator()(Trace<Timer> const &event) {
  auto &[trace_info, timer] = event;
  (*connection_).refresh(timer.now);
}

void Rest::operator()(metrics::Writer &writer) const {
  writer
      // counter
      .write(counter_.disconnect, metrics::Type::COUNTER)
      // profile
      .write(profile_.asset_details, metrics::Type::PROFILE)
      .write(profile_.asset_details_ack, metrics::Type::PROFILE)
      .write(profile_.order_book_details, metrics::Type::PROFILE)
      .write(profile_.order_book_details_ack, metrics::Type::PROFILE)
      // latency
      .write(latency_.ping, metrics::Type::LATENCY);
}

void Rest::operator()(Trace<ConnectionStatus> const &event, std::string_view const &reason) {
  auto &[trace_info, connection_status] = event;
  if (utils::update(connection_status_, connection_status)) {
    auto stream_status = StreamStatus{
        .stream_id = stream_id_,
        .account = {},
        .supports = SUPPORTS,
        .transport = Transport::TCP,
        .protocol = Protocol::HTTP,
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

// web::rest::Client::Handler

void Rest::operator()(Trace<web::rest::Connected> const &event) {
  auto &[trace_info, connected] = event;
  if (download_.downloading()) {
    download_.bump(trace_info);
  } else {
    download_.begin(trace_info);
  }
}

void Rest::operator()(Trace<web::rest::Disconnected> const &event) {
  auto &[trace_info, disconnected] = event;
  ++counter_.disconnect;
  create_trace_and_dispatch_2(trace_info, ConnectionStatus::DISCONNECTED);
  if (!download_.downloading()) {
    download_.reset();
  }
}

void Rest::operator()(Trace<web::rest::Latency> const &event) {
  auto &[trace_info, latency] = event;
  auto external_latency = ExternalLatency{
      .stream_id = stream_id_,
      .account = {},
      .latency = latency.sample,
  };
  create_trace_and_dispatch(shared_.dispatcher, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

// core::Download

int32_t Rest::download(Trace<State> const &event) {
  auto &[trace_info, state] = event;
  switch (state) {
    using enum State;
    case UNDEFINED:
      assert(false);
      break;
    case GET_ASSET_DETAILS:
      create_trace_and_dispatch_2(trace_info, ConnectionStatus::DOWNLOADING, "get-asset-details"sv);
      get_asset_details();
      return 1;
    case GET_ORDER_BOOK_DETAILS:
      create_trace_and_dispatch_2(trace_info, ConnectionStatus::DOWNLOADING, "get-order-book-details"sv);
      get_order_book_details();
      return 1;
    case DONE:
      create_trace_and_dispatch_2(trace_info, ConnectionStatus::READY);
      return 0;
  }
  assert(false);
  return 0;
}

// asset-details

void Rest::get_asset_details() {
  profile_.asset_details([&]() {
    auto request = web::rest::Request{
        .method = web::http::Method::GET,
        .path = shared_.api.market_data.asset_details,
        .query = {},
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = {},
        .headers = {},
        .body = {},
        .quality_of_service = {},
    };
    auto callback = [this, sequence = download_.sequence()](auto &event, [[maybe_unused]] auto &request_id) { get_asset_details_ack(event, sequence); };
    (*connection_)(request, callback, "asset-details"sv);
  });
}

void Rest::get_asset_details_ack(Trace<web::rest::Response> const &event, uint32_t sequence) {
  auto const STATE = State::GET_ASSET_DETAILS;
  profile_.asset_details_ack([&]() {
    auto &[trace_info, response] = event;
    auto handle_error = [&](auto origin, auto status, auto error, auto const &text) {
      log::warn(R"(origin={}, error={}, status={}, text="{}")"sv, origin, error, status, text);
      download_.retry(STATE);
    };
    auto handle_success = [&](auto &body) {
      if (download_.skip(sequence, STATE)) {
        log::info("Download state={} has already been processed"sv, STATE);
      } else {
        protocol::json::AssetDetailsAck asset_details_ack{body, decode_buffer_};
        create_trace_and_dispatch_2(trace_info, asset_details_ack);
        download_.check(trace_info, STATE);
      }
    };
    process_response(event, handle_error, handle_success);
  });
}

void Rest::operator()(Trace<protocol::json::AssetDetailsAck> const &event) {
  auto &[trace_info, asset_details_ack] = event;
  log::info<4>("asset_details_ack={}"sv, asset_details_ack);
  for (auto &item : asset_details_ack.asset_details) {
    shared_.assets[item.asset_id] = std::string{item.symbol};
  }
}

// order-book-details

void Rest::get_order_book_details() {
  profile_.order_book_details([&]() {
    auto request = web::rest::Request{
        .method = web::http::Method::GET,
        .path = shared_.api.market_data.order_book_details,
        .query = {},
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = {},
        .headers = {},
        .body = {},
        .quality_of_service = {},
    };
    auto callback = [this, sequence = download_.sequence()](auto &event, [[maybe_unused]] auto &request_id) { get_order_book_details_ack(event, sequence); };
    (*connection_)(request, callback, "order-book-details"sv);
  });
}

void Rest::get_order_book_details_ack(Trace<web::rest::Response> const &event, uint32_t sequence) {
  auto const STATE = State::GET_ORDER_BOOK_DETAILS;
  profile_.order_book_details_ack([&]() {
    auto &[trace_info, response] = event;
    auto handle_error = [&](auto origin, auto status, auto error, auto const &text) {
      log::warn(R"(origin={}, error={}, status={}, text="{}")"sv, origin, error, status, text);
      download_.retry(STATE);
    };
    auto handle_success = [&](auto &body) {
      if (download_.skip(sequence, STATE)) {
        log::info("Download state={} has already been processed"sv, STATE);
      } else {
        protocol::json::OrderBookDetailsAck order_book_details_ack{body, decode_buffer_};
        create_trace_and_dispatch_2(trace_info, order_book_details_ack);
        download_.check(trace_info, STATE);
      }
    };
    process_response(event, handle_error, handle_success);
  });
}

void Rest::operator()(Trace<protocol::json::OrderBookDetailsAck> const &event) {
  auto &[trace_info, order_book_details_ack] = event;
  log::info<4>("order_book_details_ack={}"sv, order_book_details_ack);
  std::vector<Symbol> symbols;
  symbols.reserve(std::size(order_book_details_ack.spot_order_book_details) + std::size(order_book_details_ack.order_book_details));  // alloc
  size_t counter = 0;
  auto helper = [&](auto &item) {
    log::info<2>("item={}"sv, item);
    auto discard = shared_.dispatcher.discard_symbol(item.symbol);
    shared_.markets[item.market_id] = std::string{item.symbol};
    shared_.reverse_markets[item.symbol] = item.market_id;
    auto base_currency = shared_.get_symbol_from_asset_id(item.base_asset_id);
    auto quote_currency = shared_.get_symbol_from_asset_id(item.quote_asset_id);
    auto tick_size = std::pow(10.0, -item.supported_price_decimals);           // ???
    auto trade_vol_step_size = std::pow(10.0, -item.supported_size_decimals);  // ???
    auto reference_data = ReferenceData{
        .stream_id = stream_id_,
        .exchange = shared_.settings.exchange,
        .symbol = item.symbol,
        .description = {},
        .security_type = map(item.market_type),
        .external_security_id = utils::safe_cast(item.market_id),
        .market_segment = {},
        .cfi_code = {},
        .base_currency = base_currency,
        .quote_currency = quote_currency,
        .settlement_currency = {},
        .margin_currency = {},
        .commission_currency = {},
        .tick_size = tick_size,
        .tick_size_steps = {},
        .multiplier = NaN,
        .min_notional = NaN,
        .min_trade_vol = item.min_quote_amount,  // ???
        .max_trade_vol = NaN,
        .trade_vol_step_size = trade_vol_step_size,
        .option_type = {},
        .strike_currency = {},
        .strike_price = NaN,
        .underlying = {},
        .time_zone = {},
        .issue_date = {},
        .settlement_date = {},
        .expiry_datetime = {},
        .expiry_datetime_utc = {},
        .exchange_time_utc = {},
        .exchange_sequence = {},
        .sending_time_utc = {},
        .discard = discard,
    };
    create_trace_and_dispatch(shared_.dispatcher, trace_info, reference_data, true);
    if (discard) {
      log::info<1>(R"(Drop symbol="{}")"sv, item.symbol);
      return;
    }
    if (shared_.all_symbols.emplace(item.symbol).second) {  // only include new
      symbols.emplace_back(item.symbol);
    }
    ++counter;
  };
  for (auto &item : order_book_details_ack.spot_order_book_details) {
    helper(item);
  }
  // XXX FIXME TODO not sure if we should create spot as well -- perhaps we can't subscribe market data ???
  for (auto &item : order_book_details_ack.order_book_details) {
    helper(item);
  }
  if (!std::empty(symbols)) {
    auto symbols_update = SymbolsUpdate{
        .symbols = symbols,
    };
    handler_(symbols_update);
  }
  if (counter > 0) {
    log::info("Symbols {} / {}"sv, counter, std::size(order_book_details_ack.order_book_details));
  }
}

// helpers

void Rest::process_response(Trace<web::rest::Response> const &event, auto error_handler, auto success_handler) {
  auto &[trace_info, response] = event;
  try {
    auto [status, category, body] = response.result();
    switch (category) {
      using enum web::http::Category;
      case UNKNOWN:
      case INFORMATIONAL_RESPONSE:
        response.expect(web::http::Status::OK);  // throws
        break;
      case SUCCESS:
        success_handler(body);
        break;
      case REDIRECTION:
        log::fatal("Unexpected: URL is being redirected"sv);
      case CLIENT_ERROR: {
        auto message = fmt::format("{}"sv, status);
        error_handler(Origin::EXCHANGE, RequestStatus::REJECTED, Error::UNKNOWN, message);
        break;
      }
      case SERVER_ERROR: {
        auto message = fmt::format("{}"sv, status);
        error_handler(Origin::EXCHANGE, RequestStatus::REJECTED, Error::UNKNOWN, message);
        break;
      }
    }
  } catch (NetworkError &e) {
    log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
    error_handler(Origin::GATEWAY, e.request_status(), e.error(), e.what());
  } catch (std::exception &e) {
    log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
    error_handler(Origin::EXCHANGE, RequestStatus::ERROR, Error::UNKNOWN, e.what());
  }
}

}  // namespace gateway
}  // namespace lighter
}  // namespace roq
