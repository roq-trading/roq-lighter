/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/lighter/protocol/json/encoder.hpp"

#include <fmt/format.h>

#include "roq/decimal.hpp"

#include "roq/lighter/protocol/json/map.hpp"

using namespace std::literals;

namespace roq {
namespace lighter {
namespace protocol {
namespace json {

// REST

std::string_view Encoder::place_order(
    std::string &buffer,
    roq::CreateOrder const &,
    server::oms::Order const &,
    server::oms::RefData const &,
    [[maybe_unused]] std::string_view const &request_id) {
  buffer.clear();
  /*
  auto side = map(create_order.side).template get<Side>();
  auto order_type = map(create_order.order_type).template get<OrderType>();
  auto time_in_force = map(create_order.time_in_force).template get<TimeInForce>();
  auto reduce_only = create_order.execution_instructions.has(ExecutionInstruction::DO_NOT_INCREASE);
  fmt::format_to(
      std::back_inserter(buffer),
      R"({{)"
      R"("category":"{}",)"
      R"("symbol":"{}",)"
      R"("side":"{}",)"
      R"("orderType":"{}",)"
      R"("qty":"{}",)"
      R"("timeInForce":"{}",)"
      R"("reduceOnly":{})"sv,
      category.as_raw_text(),
      create_order.symbol,
      side.as_raw_text(),
      order_type.as_raw_text(),
      Decimal{create_order.quantity, ref_data.quantity.precision},
      time_in_force.as_raw_text(),
      reduce_only);
  if (!std::isnan(create_order.price)) {
    fmt::format_to(std::back_inserter(buffer), R"(,"price":"{}")"sv, Decimal{create_order.price, ref_data.price.precision});
  }
  fmt::format_to(
      std::back_inserter(buffer),
      R"(,"orderLinkId":"{}")"
      R"(}})"sv,
      request_id);
  */
  return buffer;
}

std::string_view Encoder::amend_order(
    std::string &buffer,
    roq::ModifyOrder const &,
    server::oms::Order const &,
    server::oms::RefData const &,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  buffer.clear();
  /*
  fmt::format_to(
      std::back_inserter(buffer),
      R"({{)"
      R"("category":"{}",)"
      R"("symbol":"{}")"sv,
      category.as_raw_text(),
      order.symbol);
  if (!std::isnan(modify_order.price)) {
    fmt::format_to(std::back_inserter(buffer), R"(,"price":"{}")"sv, Decimal{modify_order.price, ref_data.price.precision});
  }
  if (!std::isnan(modify_order.quantity)) {
    fmt::format_to(std::back_inserter(buffer), R"(,"qty":"{}")"sv, Decimal{modify_order.quantity, ref_data.quantity.precision});
  }
  if (!std::empty(order.external_order_id)) {
    fmt::format_to(
        std::back_inserter(buffer),
        R"(,"orderId":"{}")"
        R"(}})"sv,
        order.external_order_id);
  } else {
    fmt::format_to(
        std::back_inserter(buffer),
        R"(,"orderLinkId":"{}")"
        R"(}})"sv,
        previous_request_id);  // XXX not correct -- we need original request_id
  }
  */
  return buffer;
}

std::string_view Encoder::cancel_order(
    std::string &buffer,
    roq::CancelOrder const &,
    server::oms::Order const &,
    server::oms::RefData const &,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  buffer.clear();
  /*
  fmt::format_to(
      std::back_inserter(buffer),
      R"({{)"
      R"("category":"{}",)"
      R"("symbol":"{}")"sv,
      category.as_raw_text(),
      order.symbol);
  if (!std::empty(order.external_order_id)) {
    fmt::format_to(
        std::back_inserter(buffer),
        R"(,"orderId":"{}")"
        R"(}})"sv,
        order.external_order_id);
  } else {
    fmt::format_to(
        std::back_inserter(buffer),
        R"(,"orderLinkId":"{}")"
        R"(}})"sv,
        previous_request_id);  // XXX not correct -- we need original request_id
  }
  */
  return buffer;
}

std::string_view Encoder::cancel_all_orders(
    std::string &buffer, roq::CancelAllOrders const &, [[maybe_unused]] std::string_view const &request_id, [[maybe_unused]] std::string_view const &symbol) {
  buffer.clear();
  /*
  fmt::format_to(
      std::back_inserter(buffer),
      R"({{)"
      R"("category":"{}",)"
      R"("symbol":"{}")"
      R"(}})"sv,
      category.as_raw_text(),
      symbol);
  */
  return buffer;
}

// WS

std::string_view Encoder::place_order_ws(
    std::string &buffer,
    roq::CreateOrder const &,
    server::oms::Order const &,
    server::oms::RefData const &,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::chrono::milliseconds now_utc,
    [[maybe_unused]] std::chrono::milliseconds recv_window) {
  buffer.clear();
  /*
  auto side = map(create_order.side).template get<Side>();
  auto order_type = map(create_order.order_type).template get<OrderType>();
  auto time_in_force = map(create_order.time_in_force).template get<TimeInForce>();
  auto reduce_only = create_order.execution_instructions.has(ExecutionInstruction::DO_NOT_INCREASE);
  fmt::format_to(
      std::back_inserter(buffer),
      R"({{)"
      R"("op":"order.create",)"
      R"("header":{{)"
      R"("X-BAPI-TIMESTAMP":"{}",)"
      R"("X-BAPI-RECV-WINDOW":"{}")"
      R"(}},)"
      R"("args":[{{)"
      R"("category":"{}",)"
      R"("symbol":"{}",)"
      R"("side":"{}",)"
      R"("orderType":"{}",)"
      R"("qty":"{}",)"
      R"("timeInForce":"{}",)"
      R"("reduceOnly":{})"sv,
      now_utc.count(),
      recv_window.count(),
      category.as_raw_text(),
      create_order.symbol,
      side.as_raw_text(),
      order_type.as_raw_text(),
      Decimal{create_order.quantity, ref_data.quantity.precision},
      time_in_force.as_raw_text(),
      reduce_only);
  if (!std::isnan(create_order.price)) {
    fmt::format_to(std::back_inserter(buffer), R"(,"price":"{}")"sv, Decimal{create_order.price, ref_data.price.precision});
  }
  fmt::format_to(
      std::back_inserter(buffer),
      R"(,"orderLinkId":"{}")"
      R"(}})"
      R"(],)"
      R"("reqId":"{}")"
      R"(}})"sv,
      request_id,
      request_id);
  */
  return buffer;
}

std::string_view Encoder::amend_order_ws(
    std::string &buffer,
    roq::ModifyOrder const &,
    server::oms::Order const &,
    server::oms::RefData const &,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id,
    [[maybe_unused]] std::chrono::milliseconds now_utc,
    [[maybe_unused]] std::chrono::milliseconds recv_window) {
  buffer.clear();
  /*
  fmt::format_to(
      std::back_inserter(buffer),
      R"({{)"
      R"("op":"order.amend",)"
      R"("header":{{)"
      R"("X-BAPI-TIMESTAMP":"{}",)"
      R"("X-BAPI-RECV-WINDOW":"{}")"
      R"(}},)"
      R"("args":[{{)"
      R"("category":"{}",)"
      R"("symbol":"{}")"sv,
      now_utc.count(),
      recv_window.count(),
      category.as_raw_text(),
      order.symbol);
  if (!std::isnan(modify_order.price)) {
    fmt::format_to(std::back_inserter(buffer), R"(,"price":"{}")"sv, Decimal{modify_order.price, ref_data.price.precision});
  }
  if (!std::isnan(modify_order.quantity)) {
    fmt::format_to(std::back_inserter(buffer), R"(,"qty":"{}")"sv, Decimal{modify_order.quantity, ref_data.quantity.precision});
  }
  if (!std::empty(order.external_order_id)) {
    fmt::format_to(std::back_inserter(buffer), R"(,"orderId":"{}")"sv, order.external_order_id);
  } else {
    fmt::format_to(std::back_inserter(buffer), R"(,"orderLinkId":"{}")"sv,
                   previous_request_id);  // XXX not correct -- we need original request_id
  }
  fmt::format_to(
      std::back_inserter(buffer),
      R"(}})"
      R"(],)"
      R"("reqId":"{}")"
      R"(}})"sv,
      request_id);
  */
  return buffer;
}

std::string_view Encoder::cancel_order_ws(
    std::string &buffer,
    roq::CancelOrder const &,
    server::oms::Order const &,
    server::oms::RefData const &,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id,
    [[maybe_unused]] std::chrono::milliseconds now_utc,
    [[maybe_unused]] std::chrono::milliseconds recv_window) {
  buffer.clear();
  /*
  fmt::format_to(
      std::back_inserter(buffer),
      R"({{)"
      R"("op":"order.cancel",)"
      R"("header":{{)"
      R"("X-BAPI-TIMESTAMP":"{}",)"
      R"("X-BAPI-RECV-WINDOW":"{}")"
      R"(}},)"
      R"("args":[{{)"
      R"("category":"{}",)"
      R"("symbol":"{}")"sv,
      now_utc.count(),
      recv_window.count(),
      category.as_raw_text(),
      order.symbol);
  if (!std::empty(order.external_order_id)) {
    fmt::format_to(std::back_inserter(buffer), R"(,"orderId":"{}")"sv, order.external_order_id);
  } else {
    fmt::format_to(std::back_inserter(buffer), R"(,"orderLinkId":"{}")"sv,
                   previous_request_id);  // XXX not correct -- we need original request_id
  }
  fmt::format_to(
      std::back_inserter(buffer),
      R"(}})"
      R"(],)"
      R"("reqId":"{}")"
      R"(}})"sv,
      request_id);
  */
  return buffer;
}

}  // namespace json
}  // namespace protocol
}  // namespace lighter
}  // namespace roq
