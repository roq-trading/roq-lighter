/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/trace_info.hpp"

#include "roq/core/json/buffer_stack.hpp"

#include "roq/lighter/protocol/json/pong.hpp"

#include "roq/lighter/protocol/json/response.hpp"

// public
#include "roq/lighter/protocol/json/book_ticker_update.hpp"
#include "roq/lighter/protocol/json/depth_update.hpp"
#include "roq/lighter/protocol/json/market_today_update.hpp"
#include "roq/lighter/protocol/json/market_update.hpp"
#include "roq/lighter/protocol/json/trades_update.hpp"

// private

namespace roq {
namespace lighter {
namespace protocol {
namespace json {

struct Parser final {
  struct Handler {
    virtual void operator()(Trace<protocol::json::Pong> const &) = 0;
    // response
    virtual void operator()(Trace<protocol::json::Response> const &) = 0;
    // public stream
    virtual void operator()(Trace<protocol::json::BookTickerUpdate> const &) = 0;
    virtual void operator()(Trace<protocol::json::DepthUpdate> const &) = 0;
    virtual void operator()(Trace<protocol::json::TradesUpdate> const &) = 0;
    virtual void operator()(Trace<protocol::json::MarketUpdate> const &) = 0;
    virtual void operator()(Trace<protocol::json::MarketTodayUpdate> const &) = 0;
    // private stream
    /*
    virtual void operator()(Trace<protocol::json::Wallet> const &) = 0;
    virtual void operator()(Trace<protocol::json::Position> const &) = 0;
    virtual void operator()(Trace<protocol::json::Order> const &) = 0;
    virtual void operator()(Trace<protocol::json::Execution> const &) = 0;
    */
  };

  static bool dispatch(Handler &, std::string_view const &message, core::json::BufferStack &, TraceInfo const &, bool allow_unknown_event_types);
};

}  // namespace json
}  // namespace protocol
}  // namespace lighter
}  // namespace roq
