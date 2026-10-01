/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string>
#include <vector>

#include "roq/api.hpp"

#include "roq/server.hpp"

#include "roq/utils/container.hpp"

#include "roq/core/symbols.hpp"
#include "roq/core/timer_queue.hpp"

#include "roq/core/limit/rate_limiter.hpp"

#include "roq/lighter/gateway/api.hpp"
#include "roq/lighter/gateway/settings.hpp"

#include "roq/lighter/tools/throttle.hpp"

namespace roq {
namespace lighter {
namespace gateway {

struct Shared final {
  Shared(server::Dispatcher &, Settings const &);

  Shared(Shared const &) = delete;

  server::Dispatcher &dispatcher;

  Settings const &settings;
  API const api;

  tools::Throttle throttle;

  core::limit::RateLimiter rate_limiter;

  core::Symbols symbols;
  utils::unordered_set<std::string> all_symbols;

  std::vector<MBPUpdate> bids, asks, final_bids, final_asks;
  std::vector<Trade> trades;
  std::vector<Bar> bars;
  std::vector<Fill> fills;

  utils::unordered_map<int32_t, std::string> assets;

  std::string_view get_symbol_from_asset_id(int32_t asset_id) const {
    auto iter = assets.find(asset_id);
    return iter != std::end(assets) ? (*iter).second : std::string_view{};
  }

  utils::unordered_map<int32_t, std::string> markets;

  std::string_view get_symbol_from_market_id(int32_t market_id) const {
    auto iter = markets.find(market_id);
    return iter != std::end(markets) ? (*iter).second : std::string_view{};
  }

  utils::unordered_map<std::string, int32_t> reverse_markets;

  int32_t get_market_id_from_symbol(std::string_view const &symbol) {
    auto iter = reverse_markets.find(symbol);
    return iter != std::end(reverse_markets) ? (*iter).second : -1;
  }
};

}  // namespace gateway
}  // namespace lighter
}  // namespace roq
