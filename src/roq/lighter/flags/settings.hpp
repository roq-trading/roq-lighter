/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <fmt/format.h>

#include "roq/server/flags/settings.hpp"

#include "roq/lighter/flags/flags.hpp"
#include "roq/lighter/flags/mbp.hpp"
#include "roq/lighter/flags/misc.hpp"
#include "roq/lighter/flags/request.hpp"
#include "roq/lighter/flags/rest.hpp"
#include "roq/lighter/flags/ws.hpp"

namespace roq {
namespace lighter {
namespace flags {

struct ROQ_PUBLIC Settings : public server::flags::Settings {
  explicit Settings(args::Parser const &);

  std::string_view exchange;
  bool ws_api;

  flags::Misc misc;
  flags::REST rest;
  flags::WS ws;
  flags::MBP mbp;
  flags::Request request;

 private:
  Settings(args::Parser const &, flags::Flags const &);
};

}  // namespace flags
}  // namespace lighter
}  // namespace roq

template <>
struct fmt::formatter<roq::lighter::flags::Settings> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::lighter::flags::Settings const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(exchange="{}", )"
        R"(ws_api={}, )"
        R"(misc={}, )"
        R"(rest={}, )"
        R"(ws={}, )"
        R"(mbp={}, )"
        R"(request={}, )"
        R"(server={})"
        R"(}})"sv,
        value.exchange,
        value.ws_api,
        value.misc,
        value.rest,
        value.ws,
        value.mbp,
        value.request,
        static_cast<roq::server::Settings const &>(value));
  }
};
