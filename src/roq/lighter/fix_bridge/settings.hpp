/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/server/fix_bridge/settings.hpp"

#include "roq/lighter/flags/settings.hpp"

namespace roq {
namespace lighter {
namespace fix_bridge {

struct Settings final : public flags::Settings {
  explicit Settings(args::Parser const &);

  server::fix_bridge::Settings fix_bridge;
};

}  // namespace fix_bridge
}  // namespace lighter
}  // namespace roq

template <>
struct fmt::formatter<roq::lighter::fix_bridge::Settings> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::lighter::fix_bridge::Settings const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(gateway={}, )"
        R"(fix_bridge={})"
        R"(}})"sv,
        static_cast<roq::lighter::flags::Settings const &>(value),
        value.fix_bridge);
  }
};
