/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/lighter/protocol/json/parser.hpp"

#include "roq/logging.hpp"

#include "roq/utils/hash/fnv.hpp"

#include "roq/utils/charconv/from_chars.hpp"

#include "roq/core/json/array_parser.hpp"

#include "roq/lighter/protocol/json/utils.hpp"

#include "roq/lighter/protocol/json/method.hpp"

using namespace std::literals;

namespace roq {
namespace lighter {
namespace protocol {
namespace json {

// === CONSTANTS ===

namespace {
constexpr auto const KEY_METHOD = "method"sv;
constexpr auto const KEY_RESULT = "result"sv;
constexpr auto const RESULT_PONG = "pong"sv;
}  // namespace

// === HELPERS ===

namespace {
template <typename T, typename... Args>
auto dispatch_helper(auto &handler, auto &message, auto &buffer_stack, auto &trace_info, Args &&...args) {
  T obj{message, buffer_stack};
  create_trace_and_dispatch(handler, trace_info, obj, std::forward<Args>(args)...);
  return true;
}
}  // namespace

// === IMPLEMENTATION ===

bool Parser::dispatch(
    Handler &handler, std::string_view const &message, core::json::BufferStack &buffer_stack, TraceInfo const &trace_info, bool allow_unknown_event_types) {
  auto result = false;
  auto has_result = false;
  auto helper = [&](auto &key, auto &value) {
    auto key_2 = utils::hash::FNV::compute(key);
    switch (key_2) {
      case utils::hash::FNV::compute(KEY_METHOD): {
        Method method{value};
        switch (method) {
          using enum Method::type_t;
          case UNDEFINED_INTERNAL:
            log::fatal("Unexpected"sv);
          case UNKNOWN_INTERNAL:
            return true;
          case BOOK_TICKER_UPDATE:
            result = dispatch_helper<BookTickerUpdate>(handler, message, buffer_stack, trace_info);
            return true;
          case DEPTH_UPDATE:
            result = dispatch_helper<DepthUpdate>(handler, message, buffer_stack, trace_info);
            return true;
          case TRADES_UPDATE:
            result = dispatch_helper<TradesUpdate>(handler, message, buffer_stack, trace_info);
            return true;
          case MARKET_UPDATE:
            result = dispatch_helper<MarketUpdate>(handler, message, buffer_stack, trace_info);
            return true;
          case MARKET_TODAY_UPDATE:
            result = dispatch_helper<MarketTodayUpdate>(handler, message, buffer_stack, trace_info);
            return true;
        }
        return true;
      }
      case utils::hash::FNV::compute(KEY_RESULT):
        if (std::holds_alternative<std::string_view>(value) && std::get<std::string_view>(value) == RESULT_PONG) {
          result = dispatch_helper<Pong>(handler, message, buffer_stack, trace_info);
          return true;
        }
        has_result = true;
        break;
    }
    if (has_result) {
      result = dispatch_helper<Response>(handler, message, buffer_stack, trace_info);
    }
    return result;
  };
  core::json::Parser::dispatch<core::json::Object>(helper, message);
  if (result || allow_unknown_event_types) {
    return result;
  }
  log::fatal(R"(Unexpected: message="{}")"sv, message);
}

}  // namespace json
}  // namespace protocol
}  // namespace lighter
}  // namespace roq
