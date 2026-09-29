/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/lighter/tools/crypto.hpp"

#include <fmt/format.h>

#include <cassert>
#include <iterator>

#include "roq/logging.hpp"

#include "roq/utils/codec/hex.hpp"

#include <roq/utils/mac/hmac.hpp>

using namespace std::literals;

namespace roq {
namespace lighter {
namespace tools {

// === IMPLEMENTATION ===

Crypto::Crypto(std::string_view const &key, std::string_view const &secret, std::chrono::milliseconds recv_window)
    : key{key}, mac_{secret}, recv_window_{recv_window} {
}

std::string Crypto::create_signature_v2(std::chrono::milliseconds expires_utc) {
  auto tmp = fmt::format("GET/realtime{}"sv, expires_utc.count());
  mac_.clear();
  mac_.update(tmp);
  auto digest = mac_.final(digest_);
  std::string result;
  utils::codec::Hex::encode(result, digest);
  return result;
}

std::string Crypto::create_headers_v2(
    [[maybe_unused]] std::string_view const &path, std::string_view const &query, std::string_view const &body, std::chrono::milliseconds now_utc) {
  assert(!std::empty(path));
  auto query_or_body = [&]() {
    if (std::empty(query)) {
      return body;
    }
    assert(std::empty(body));
    assert(query[0] == '?');
    return query.substr(1);
  }();
  auto tmp = fmt::format("{}{}{}{}"sv, now_utc.count(), key, recv_window_.count(), query_or_body);
  mac_.clear();
  mac_.update(tmp);
  auto digest = mac_.final(digest_);
  std::string signature;
  utils::codec::Hex::encode(signature, digest);
  auto result = fmt::format(
      "X-BAPI-API-KEY: {}\r\n"
      "X-BAPI-SIGN: {}\r\n"
      "X-BAPI-TIMESTAMP: {}\r\n"
      "X-BAPI-RECV-WINDOW: {}\r\n",
      key,
      signature,
      now_utc.count(),
      recv_window_.count());
  return result;
}

}  // namespace tools
}  // namespace lighter
}  // namespace roq
