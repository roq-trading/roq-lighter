/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string>
#include <string_view>

#include "roq/core/timer_queue.hpp"

#include "roq/core/limit/queue.hpp"
#include "roq/core/limit/rate_limiter.hpp"

#include "roq/lighter/gateway/config.hpp"
#include "roq/lighter/gateway/settings.hpp"

#include "roq/lighter/tools/crypto.hpp"

namespace roq {
namespace lighter {
namespace gateway {

struct Account final {
  Account(Settings const &, Config const &, std::string_view const &name);

  Account(Account const &) = delete;

  std::string_view get_key() const { return crypto_.key; }

  std::string create_signature(std::chrono::milliseconds expires_utc);
  std::string create_headers(std::string_view const &path, std::string_view const &query, std::string_view const &body);

  std::string const name;

 private:
  tools::Crypto crypto_;

 public:
  core::limit::RateLimiter rate_limiter;
  core::limit::Queue<std::pair<std::string, std::string>> request_queue;
};

}  // namespace gateway
}  // namespace lighter
}  // namespace roq
