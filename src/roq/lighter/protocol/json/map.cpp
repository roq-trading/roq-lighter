/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/lighter/protocol/json/map.hpp"

using namespace std::literals;

namespace roq {

namespace {
template <typename... Args>
using Helper = detail::MapHelper<Args...>;
}

// lighter::json => roq

// lighter::protocol::json::MarketType ==> roq::SecurityType

template <>
template <>
constexpr Helper<lighter::protocol::json::MarketType>::operator std::optional<roq::SecurityType>() const {
  switch (std::get<0>(args_)) {
    using enum lighter::protocol::json::MarketType::type_t;
    case UNDEFINED_INTERNAL:
      return SecurityType::UNDEFINED;
    case UNKNOWN_INTERNAL:
      return SecurityType::UNDEFINED;
    case SPOT:
      return SecurityType::SPOT;
    case PERP:
      return SecurityType::FUTURES;
  }
  return {};
}

static_assert(Helper{lighter::protocol::json::MarketType{lighter::protocol::json::MarketType::UNDEFINED_INTERNAL}} == roq::SecurityType::UNDEFINED);
static_assert(Helper{lighter::protocol::json::MarketType{lighter::protocol::json::MarketType::SPOT}} == roq::SecurityType::SPOT);
static_assert(Helper{lighter::protocol::json::MarketType{lighter::protocol::json::MarketType::PERP}} == roq::SecurityType::FUTURES);

template <>
template <>
std::optional<roq::SecurityType> Map<lighter::protocol::json::MarketType>::helper() const {
  return Helper{args_};
}

// roq ==>

}  // namespace roq
