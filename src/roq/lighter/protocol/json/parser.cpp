/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/lighter/protocol/json/parser.hpp"

#include "roq/logging.hpp"

#include "roq/utils/hash/fnv.hpp"

#include "roq/utils/charconv/from_chars.hpp"

#include "roq/core/json/array_parser.hpp"

#include "roq/lighter/protocol/json/utils.hpp"

#include "roq/lighter/protocol/json/type.hpp"

using namespace std::literals;

namespace roq {
namespace lighter {
namespace protocol {
namespace json {

// === CONSTANTS ===

namespace {
constexpr auto const KEY_TYPE = "type"sv;
constexpr auto const KEY_ERROR = "error"sv;
}  // namespace

// === HELPERS ===

namespace {
template <typename T, typename... Args>
auto dispatch_helper(auto &handler, auto &message, auto &buffer_stack, auto &trace_info, Args &&...args) {
  T obj{message, buffer_stack};
  create_trace_and_dispatch(handler, trace_info, obj, std::forward<Args>(args)...);
  return true;
}

constexpr std::string_view extract_type(std::string_view const &text) {
  auto pos = text.find('/');
  if (pos == std::string_view::npos) {
    return text;
  }
  return text.substr(pos + 1);
}

static_assert(extract_type(""sv) == ""sv);
static_assert(extract_type("foo"sv) == "foo"sv);
static_assert(extract_type("foo/bar"sv) == "bar"sv);
}  // namespace

// === IMPLEMENTATION ===

bool Parser::dispatch(
    Handler &handler, std::string_view const &message, core::json::BufferStack &buffer_stack, TraceInfo const &trace_info, bool allow_unknown_event_types) {
  auto result = false;
  auto helper = [&](auto &key, auto &value) {
    auto key_2 = utils::hash::FNV::compute(key);
    switch (key_2) {
      case utils::hash::FNV::compute(KEY_TYPE): {
        auto tmp = std::get<std::string_view>(value);
        Type type{extract_type(tmp)};
        switch (type) {
          using enum Type::type_t;
          case UNDEFINED_INTERNAL:
            log::fatal("Unexpected"sv);
          case UNKNOWN_INTERNAL:
            return true;
          case CONNECTED:
            result = dispatch_helper<Connected>(handler, message, buffer_stack, trace_info);
            return true;
          case PONG:
            result = dispatch_helper<Pong>(handler, message, buffer_stack, trace_info);
            return true;
          case ORDER_BOOK:
            result = dispatch_helper<OrderBook>(handler, message, buffer_stack, trace_info);
            return true;
          case TICKER:
            result = dispatch_helper<Ticker>(handler, message, buffer_stack, trace_info);
            return true;
          case TRADE:
            result = dispatch_helper<Trade>(handler, message, buffer_stack, trace_info);
            return true;
          case MARKET_STATS:
            result = dispatch_helper<MarketStats>(handler, message, buffer_stack, trace_info);
            return true;
        }
        return true;
      }
      case utils::hash::FNV::compute(KEY_ERROR): {
        result = dispatch_helper<Error>(handler, message, buffer_stack, trace_info);
        return true;
      }
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
