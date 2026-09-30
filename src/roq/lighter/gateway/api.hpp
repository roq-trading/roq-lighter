/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/lighter/gateway/settings.hpp"

#include "roq/lighter/tools/api.hpp"

namespace roq {
namespace lighter {
namespace gateway {

struct API final {
  struct {
    std::string_view asset_details;
    std::string_view order_book_details;
  } market_data;

  struct {
    std::string_view account_info;
    std::string_view account_wallet_balance;
    std::string_view position_list;
    std::string_view order_realtime;
    std::string_view execution_list;
    std::string_view order_create;
    std::string_view order_amend;
    std::string_view order_cancel;
    std::string_view order_cancel_all;
  } simple;

  tools::API api;

  // factory
  static API create(Settings const &);

  static tools::API parse_api(std::string_view const &api);
};

}  // namespace gateway
}  // namespace lighter
}  // namespace roq
