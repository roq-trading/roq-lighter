/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <chrono>

#include "roq/error.hpp"

#include "roq/utils/datetime.hpp"

#include "roq/utils/charconv/from_chars.hpp"

#include "roq/core/json/parser.hpp"

namespace roq {
namespace lighter {
namespace protocol {
namespace json {

template <typename T>
inline void update(T &result, core::json::Value const &value) {
  result = core::json::get<T>(value);
}

// note! using microseconds due to double representation
template <>
inline void update(std::chrono::microseconds &result, core::json::Value const &value) {
  using result_type = std::remove_cvref_t<decltype(result)>;
  std::visit(
      utils::overloaded{
          [&](core::json::Null const &) { result = result_type{}; },
          [](bool) { throw std::bad_cast{}; },
          [&](int64_t value) { result = result_type{value}; },
          [&](double value) { result = std::chrono::duration_cast<result_type>(utils::from_double(value)); },
          [&](std::string_view const &value) { result = utils::charconv::from_chars<result_type>(value, utils::charconv::Format::DATETIME); },
          [](core::json::Object const &) { throw std::bad_cast{}; },
          [](core::json::Array const &) { throw std::bad_cast{}; },
      },
      value);
}

extern roq::Error map_error(int32_t ret_code);

}  // namespace json
}  // namespace protocol
}  // namespace lighter
}  // namespace roq
