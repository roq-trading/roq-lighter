/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/lighter/gateway/account.hpp"

#include "roq/logging.hpp"

#include "roq/utils/safe_cast.hpp"

#include "roq/clock.hpp"

using namespace std::literals;
using namespace std::chrono_literals;

namespace roq {
namespace lighter {
namespace gateway {

// === HELPERS ===

namespace {
auto create_crypto(auto &settings, auto &config, auto &name) -> tools::Crypto {
  auto ready = true;
  auto key = config.get_api_key(name);
  if (std::empty(key)) {
    ready = false;
    log::warn(R"(Unexpected: missing key for name="{}")"sv, name);
  }
  auto secret = config.get_secret(name);
  if (std::empty(secret)) {
    ready = false;
    log::warn(R"(Unexpected: missing secret for name="{}")"sv, name);
  }
  if (!ready) {
    log::fatal("Invalid config"sv);
  }
  return {key, secret, settings.rest.recv_window};
}
}  // namespace

// === IMPLEMENTATION ===

Account::Account(Settings const &settings, Config const &config, std::string_view const &name)
    : name{name}, crypto_{create_crypto(settings, config, name)}, rate_limiter{settings.request.limit, settings.request.limit_interval},
      request_queue{rate_limiter} {
}

std::string Account::create_signature(std::chrono::milliseconds expires_utc) {
  return crypto_.create_signature_v2(expires_utc);
}

std::string Account::create_headers(std::string_view const &path, std::string_view const &query, std::string_view const &body) {
  auto now_utc = clock::get_realtime<std::chrono::milliseconds>();
  return crypto_.create_headers_v2(path, query, body, now_utc);
}

}  // namespace gateway
}  // namespace lighter
}  // namespace roq
