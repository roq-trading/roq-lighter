/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/lighter/gateway/controller.hpp"

#include "roq/logging.hpp"

#include "roq/server/oms/exceptions.hpp"

#include "roq/lighter/gateway/api.hpp"

using namespace std::literals;

namespace roq {
namespace lighter {
namespace gateway {

// === CONSTANTS ===

namespace {
uint8_t const API_SPOT = 0x1;
uint8_t const API_LINEAR = 0x2;
uint8_t const API_INVERSE = 0x3;
uint8_t const API_OPTION = 0x4;
}  // namespace

// === HELPERS ===

namespace {
template <typename R>
R create_accounts(auto &settings, auto &config) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  for (auto &[_, account] : config.accounts) {
    auto obj = std::make_unique<Account>(settings, config, account.name);
    result.try_emplace(account.name, std::move(obj));
  }
  return result;
}

auto create_rest(auto &gateway, auto &context, auto &stream_id, auto &shared) {
  return std::make_unique<Rest>(gateway, context, ++stream_id, shared);
}
}  // namespace

// === IMPLEMENTATION ===

std::unique_ptr<server::Handler> Controller::create(server::Dispatcher &dispatcher, Settings const &settings, Config const &config, io::Context &context) {
  return std::make_unique<Controller>(dispatcher, settings, config, context);
}

uint8_t Controller::parse_api(Settings const &settings) {
  auto api = API::parse_api(settings.app.api);
  switch (api) {
    using enum tools::API;
    case UNDEFINED:
      break;
    case SPOT:
      return API_SPOT;
    case LINEAR:
      return API_LINEAR;
    case INVERSE:
      return API_INVERSE;
    case OPTION:
      return API_OPTION;
  }
  log::fatal(R"(Unexpected: api="{}")"sv, settings.app.api);
}

Controller::Controller(server::Dispatcher &dispatcher, Settings const &settings, Config const &config, io::Context &context)
    : dispatcher_{dispatcher}, context_{context}, shared_{dispatcher, settings}, accounts_{create_accounts<decltype(accounts_)>(settings, config)},
      rest_{create_rest(*this, context_, stream_id_, shared_)} {
}

// server::Handler

void Controller::operator()(Trace<Start> const &event) {
  log::info("Starting..."sv);
  assert(std::empty(market_data_));
  dispatch(event);
}

void Controller::operator()(Trace<Stop> const &event) {
  log::info("Stopping..."sv);
  dispatch(event);
}

void Controller::operator()(Trace<Timer> const &event) {
  dispatch(event);
}

void Controller::operator()(Trace<Connected> const &, [[maybe_unused]] uint8_t user_id) {
}

void Controller::operator()(Trace<Disconnected> const &, [[maybe_unused]] uint8_t user_id) {
}

void Controller::operator()(Trace<Subscribe> const &event) {
  auto &[trace_info, subscribe] = event;
  std::vector<Symbol> symbols;
  for (auto &item : subscribe.symbols) {
    if (shared_.all_symbols.emplace(item).second) {
      symbols.emplace_back(item);
    } else {
      log::warn(R"(*** DUPLICATE SUBSCRIPTION *** symbol="{}")"sv, item);
    }
  }
  auto symbols_update = SymbolsUpdate{
      .symbols = symbols,
  };
  create_trace_and_dispatch_2(trace_info, symbols_update);
}

void Controller::operator()(Trace<Control> const &event) {
  auto &[trace_info, control] = event;
  switch (control.action) {
    using enum Action;
    case UNDEFINED:
      assert(false);
      break;
    case ENABLE:
      dispatcher_(State::ENABLED);
      break;
    case DISABLE:
      dispatcher_(State::DISABLED);
      break;
  }
}

void Controller::operator()(Trace<server::oms::OrderActionBatchBegin> const &) {
  throw server::oms::NotSupported{"not supported"sv};
}

void Controller::operator()(Trace<server::oms::OrderActionBatchCommit> const &) {
  throw server::oms::NotSupported{"not supported"sv};
}

void Controller::operator()(Trace<server::oms::OrderActionBatchRollback> const &) {
  throw server::oms::NotSupported{"not supported"sv};
}

uint16_t Controller::operator()(
    Trace<CreateOrder> const &, server::oms::Order const &, server::oms::RefData const &, [[maybe_unused]] std::string_view const &request_id) {
  throw server::oms::NotSupported{"not supported"sv};
}

uint16_t Controller::operator()(
    Trace<ModifyOrder> const &,
    server::oms::Order const &,
    server::oms::RefData const &,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  throw server::oms::NotSupported{"not supported"sv};
}

uint16_t Controller::operator()(
    Trace<CancelOrder> const &,
    server::oms::Order const &,
    server::oms::RefData const &,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  throw server::oms::NotSupported{"not supported"sv};
}

uint16_t Controller::operator()(Trace<CancelAllOrders> const &, [[maybe_unused]] std::string_view const &request_id) {
  throw server::oms::NotSupported{"not supported"sv};
}

uint16_t Controller::operator()(Trace<MassQuote> const &) {
  throw server::oms::NotSupported{"not supported"sv};
}

uint16_t Controller::operator()(Trace<CancelQuotes> const &) {
  throw server::oms::NotSupported{"not supported"sv};
}

void Controller::operator()(metrics::Writer &writer) const {
  dispatch_helper(*this, writer);
}

// streams

void Controller::operator()(Trace<SymbolsUpdate> const &event) {
  auto &[trace_info, symbols_update] = event;
  auto [size, start_from] = shared_.symbols(symbols_update.symbols);
  ensure_symbol_slices(trace_info, size);
  for (auto &item : market_data_) {
    dynamic_cast<server::MarketDataStream &>(*item).subscribe(start_from);
  }
}

// utilities

void Controller::ensure_symbol_slices(TraceInfo const &trace_info, size_t size) {
  while (std::size(market_data_) < size) {
    log::info("Create market-data (user-stream)"sv);
    auto obj = std::make_unique<MarketData>(*this, context_, ++stream_id_, shared_, std::size(market_data_));
    Start start;
    create_trace_and_dispatch(dynamic_cast<server::Stream &>(*obj), trace_info, start);
    market_data_.emplace_back(std::move(obj));
  }
}

template <typename... Args>
void Controller::dispatch(Args &&...args) {
  dispatch_helper(*this, std::forward<Args>(args)...);
}

template <typename... Args>
void Controller::dispatch_helper(auto &self, Args &&...args) {
  auto helper = [&](auto &target) {
    if constexpr (std::is_const_v<decltype(self)>) {
      dynamic_cast<server::Stream const &>(target)(std::forward<Args>(args)...);
    } else {
      dynamic_cast<server::Stream &>(target)(std::forward<Args>(args)...);
    }
  };
  helper(*self.rest_);
  for (auto &item : self.market_data_) {
    helper(*item);
  }
}

}  // namespace gateway
}  // namespace lighter
}  // namespace roq
