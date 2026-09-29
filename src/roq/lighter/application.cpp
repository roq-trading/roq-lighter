/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/lighter/application.hpp"

#include "roq/lighter/flags/settings.hpp"

#include "roq/lighter/gateway/config.hpp"
#include "roq/lighter/gateway/controller.hpp"

using namespace std::literals;

namespace roq {
namespace lighter {

// === IMPLEMENTATION ===

int Application::main(args::Parser const &args) {
  flags::Settings settings{args};
  gateway::Config config{settings};
  auto context = server::create_io_context(settings);
  server::Trading<gateway::Controller>{settings, config, *context}.dispatch();
  return EXIT_SUCCESS;
}

}  // namespace lighter
}  // namespace roq
