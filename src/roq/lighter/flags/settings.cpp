/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/lighter/flags/settings.hpp"

#include "roq/logging.hpp"

using namespace std::literals;

namespace roq {
namespace lighter {
namespace flags {

Settings::Settings(args::Parser const &args) : Settings{args, flags::Flags::create()} {
}

Settings::Settings(args::Parser const &args, flags::Flags const &flags)
    : server::flags::Settings{args, ROQ_PACKAGE_NAME, ROQ_BUILD_NUMBER, ROQ_GIT_DESCRIBE_HASH, flags.api}, exchange{flags.exchange}, ws_api{flags.ws_api},
      misc{flags::Misc::create()}, rest{flags::REST::create()}, ws{flags::WS::create()}, mbp{flags::MBP::create()}, request{flags::Request::create()} {
  log::info("settings={}"sv, *this);
}

}  // namespace flags
}  // namespace lighter
}  // namespace roq
