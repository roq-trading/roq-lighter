/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/lighter/protocol/json/market_type.hpp"
#include "roq/lighter/protocol/json/status.hpp"

#include "roq/security_type.hpp"
#include "roq/side.hpp"

#include "roq/map.hpp"

namespace roq {

template <>
template <>
std::optional<SecurityType> Map<lighter::protocol::json::MarketType>::helper() const;

// ===

}  // namespace roq
