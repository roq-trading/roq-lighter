/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string>
#include <string_view>

#include "roq/cancel_all_orders.hpp"
#include "roq/cancel_order.hpp"
#include "roq/create_order.hpp"
#include "roq/modify_order.hpp"

#include "roq/server/oms/order.hpp"
#include "roq/server/oms/ref_data.hpp"

namespace roq {
namespace lighter {
namespace protocol {
namespace json {

struct Encoder final {
  // REST

  static std::string_view place_order(
      std::string &buffer, roq::CreateOrder const &, server::oms::Order const &, server::oms::RefData const &, std::string_view const &request_id);

  static std::string_view amend_order(
      std::string &buffer,
      roq::ModifyOrder const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id);

  static std::string_view cancel_order(
      std::string &buffer,
      roq::CancelOrder const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id);

  static std::string_view cancel_all_orders(
      std::string &buffer, roq::CancelAllOrders const &, std::string_view const &request_id, std::string_view const &symbol);

  // WS

  static std::string_view place_order_ws(
      std::string &buffer,
      roq::CreateOrder const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::chrono::milliseconds now_utc,
      std::chrono::milliseconds recv_window);

  static std::string_view amend_order_ws(
      std::string &buffer,
      roq::ModifyOrder const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id,
      std::chrono::milliseconds now_utc,
      std::chrono::milliseconds recv_window);

  static std::string_view cancel_order_ws(
      std::string &buffer,
      roq::CancelOrder const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id,
      std::chrono::milliseconds now_utc,
      std::chrono::milliseconds recv_window);
};

}  // namespace json
}  // namespace protocol
}  // namespace lighter
}  // namespace roq
