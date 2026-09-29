/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <chrono>

#include <fmt/format.h>

#include "roq/web/rest/interceptor.hpp"

#include "roq/web/socket/interceptor.hpp"

#include "roq/server/settings.hpp"

namespace roq {
namespace lighter {
namespace tools {

struct Throttle final : public web::rest::Interceptor, public web::socket::Interceptor {
  explicit Throttle(server::Settings const &);

  struct Params {
    int32_t limit = {};
    int32_t limit_status = {};
    int64_t limit_reset_timestamp = {};  // msec
  };

 protected:
  // web::Interceptor

  operator std::chrono::nanoseconds() const override { return suspend_until_; }

  // web::rest::Interceptor

  void operator()(Trace<web::rest::MessageBegin> const &) override;
  void operator()(Trace<web::rest::MessageHeader> const &) override;
  void operator()(Trace<web::rest::MessageEnd> const &) override;

  // web::socket::Interceptor

 private:
  bool const enabled_;

  Params params_;

  std::chrono::nanoseconds suspend_until_ = {};
};

}  // namespace tools
}  // namespace lighter
}  // namespace roq

template <>
struct fmt::formatter<roq::lighter::tools::Throttle::Params> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::lighter::tools::Throttle::Params const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(limit={}, )"
        R"(limit_status={}, )"
        R"(limit_reset_timestamp={})"
        R"(}})"sv,
        value.limit,
        value.limit_status,
        value.limit_reset_timestamp);
  }
};
