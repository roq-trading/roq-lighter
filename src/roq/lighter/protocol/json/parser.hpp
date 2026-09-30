/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/trace_info.hpp"

#include "roq/core/json/buffer_stack.hpp"

#include "roq/lighter/protocol/json/connected.hpp"
#include "roq/lighter/protocol/json/error.hpp"
#include "roq/lighter/protocol/json/pong.hpp"

// public

#include "roq/lighter/protocol/json/market_stats.hpp"
#include "roq/lighter/protocol/json/order_book.hpp"
#include "roq/lighter/protocol/json/ticker.hpp"
#include "roq/lighter/protocol/json/trade.hpp"

// private

namespace roq {
namespace lighter {
namespace protocol {
namespace json {

struct Parser final {
  struct Handler {
    virtual void operator()(Trace<protocol::json::Connected> const &) = 0;
    virtual void operator()(Trace<protocol::json::Pong> const &) = 0;
    virtual void operator()(Trace<protocol::json::Error> const &) = 0;
    // public stream
    virtual void operator()(Trace<protocol::json::OrderBook> const &) = 0;
    virtual void operator()(Trace<protocol::json::Ticker> const &) = 0;
    virtual void operator()(Trace<protocol::json::Trade> const &) = 0;
    virtual void operator()(Trace<protocol::json::MarketStats> const &) = 0;
    // private stream
  };

  static bool dispatch(Handler &, std::string_view const &message, core::json::BufferStack &, TraceInfo const &, bool allow_unknown_event_types);
};

}  // namespace json
}  // namespace protocol
}  // namespace lighter
}  // namespace roq
