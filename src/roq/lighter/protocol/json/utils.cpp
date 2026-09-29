/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/lighter/protocol/json/utils.hpp"

#include "roq/lighter/protocol/json/map.hpp"

using namespace std::literals;

namespace roq {
namespace lighter {
namespace protocol {
namespace json {

// NOLINTBEGIN(readability-magic-numbers)

roq::Error map_error([[maybe_unused]] int32_t ret_code) {
  return Error::UNKNOWN;
}

// NOLINTEND(readability-magic-numbers)

}  // namespace json
}  // namespace protocol
}  // namespace lighter
}  // namespace roq
