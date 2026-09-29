/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/lighter/tools/throttle.hpp"

#include "roq/utils/compare.hpp"
#include "roq/utils/update.hpp"

#include "roq/utils/hash/fnv.hpp"

#include "roq/utils/charconv/from_chars.hpp"

using namespace std::literals;

namespace roq {
namespace lighter {
namespace tools {

// === CONSTANTS ===

namespace {
auto const DEFAULT_BACKOFF = 5s;     // note! maybe as low as 1 second
auto const BLOCKED_BACKOFF = 10min;  // note! very serious
}  // namespace

// === HELPERS ===

namespace {
// note! std::tolower is not constexpr gcc16 + clang23
constexpr auto lower(auto value) {
  return utils::detail::ascii_to_lower(value);
}

enum class Header {
  UNKNOWN,
  X_BAPI_LIMIT,
  X_BAPI_LIMIT_STATUS,
  X_BAPI_LIMIT_RESET_TIMESTAMP,
};

constexpr auto parse_header(std::string_view const &text) {
  std::string value;
  value.reserve(std::size(text));
  std::transform(std::begin(text), std::end(text), std::back_inserter(value), [](auto c) { return lower(c); });
  auto key = utils::hash::FNV::compute(value);
  switch (key) {
    case utils::hash::FNV::compute("x-bapi-limit"sv):
      return Header::X_BAPI_LIMIT;
    case utils::hash::FNV::compute("x-bapi-limit-status"sv):
      return Header::X_BAPI_LIMIT_STATUS;
    case utils::hash::FNV::compute("x-bapi-limit-reset-timestamp"sv):
      return Header::X_BAPI_LIMIT_RESET_TIMESTAMP;
  }
  return Header::UNKNOWN;
}

static_assert(parse_header("X-Bapi-Limit"sv) == Header::X_BAPI_LIMIT);
static_assert(parse_header("X-Bapi-Limit-Status"sv) == Header::X_BAPI_LIMIT_STATUS);
static_assert(parse_header("X-Bapi-Limit-Reset-Timestamp"sv) == Header::X_BAPI_LIMIT_RESET_TIMESTAMP);
}  // namespace

// === IMPLEMENTATION ===

Throttle::Throttle(server::Settings const &settings) : enabled_{settings.experimental.enable_rate_limit} {
}

// web::rest::Interceptor

void Throttle::operator()(Trace<web::rest::MessageBegin> const &) {
}

void Throttle::operator()(Trace<web::rest::MessageHeader> const &event) {
  auto &[trace_info, header] = event;
  auto update_value = [&](auto &result) {
    using value_type = std::remove_cvref_t<decltype(result)>;
    auto value = utils::charconv::from_chars<value_type>(header.value);
    return utils::update(result, value);
  };
  auto update_suspend_until = [&]() {
    if (!enabled_) {
      return;
    }
    if (params_.limit_status > 0 || params_.limit_reset_timestamp == 0) {
      suspend_until_ = {};
    } else {
      auto now = clock::get_system();
      auto now_utc = clock::get_realtime();
      auto timestamp = std::chrono::milliseconds{params_.limit_reset_timestamp};
      if (now_utc < timestamp) {
        auto period = timestamp - now_utc;
        suspend_until_ = std::max(suspend_until_, now + period);
      } else {
        suspend_until_ = std::max(suspend_until_, now + DEFAULT_BACKOFF);
      }
    }
  };
  auto key = parse_header(header.name);
  switch (key) {
    using enum Header;
    [[likely]] case UNKNOWN:
      return;
    case X_BAPI_LIMIT:
      update_value(params_.limit);
      break;
    case X_BAPI_LIMIT_STATUS:
      if (update_value(params_.limit_status)) {
        update_suspend_until();
      }
      break;
    case X_BAPI_LIMIT_RESET_TIMESTAMP:
      if (update_value(params_.limit_reset_timestamp)) {
        update_suspend_until();
      }
      break;
  }
}

void Throttle::operator()(Trace<web::rest::MessageEnd> const &event) {
  auto &[trace_info, message_end] = event;
  if (!enabled_) {
    return;
  }
  switch (message_end.status) {
    using enum web::http::Status;
    [[unlikely]] case FORBIDDEN: {  // 403
      auto now = clock::get_system();
      suspend_until_ = std::max(suspend_until_, now + BLOCKED_BACKOFF);
      break;
    }
    [[unlikely]] case TOO_MANY_REQUESTS: {  // 429
      if (suspend_until_.count() == 0) {
        auto now = clock::get_system();
        suspend_until_ = now + DEFAULT_BACKOFF;
      }
      break;
    }
    default:
      break;
  }
}

// web::socket::Interceptor

}  // namespace tools
}  // namespace lighter
}  // namespace roq
