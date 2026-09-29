/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/lighter/protocol/json/trade_type.hpp"
#include "roq/lighter/protocol/json/type.hpp"

#include "roq/security_type.hpp"
#include "roq/side.hpp"

#include "roq/map.hpp"

namespace roq {

template <>
template <>
std::optional<SecurityType> Map<lighter::protocol::json::Type>::helper() const;

template <>
template <>
std::optional<Side> Map<lighter::protocol::json::TradeType>::helper() const;

// ===

template <>
template <>
std::optional<lighter::protocol::json::TradeType> Map<Side>::helper() const;

}  // namespace roq
