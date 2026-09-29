/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/lighter/protocol/json/markets_ack.hpp"

using namespace roq;
using namespace roq::lighter;

using namespace std::literals;
using namespace std::chrono_literals;

using value_type = protocol::json::MarketsAck;

// note! truncated
TEST_CASE("simple", "[markets_ack]") {
  auto message = R"([{)"
                 R"("symbol":"0G",)"
                 R"("market_index":84)"
                 R"(},{)"
                 R"("symbol":"AZTEC/USDC",)"
                 R"("market_index":2055)"
                 R"(},{)"
                 R"("symbol":"ARM",)"
                 R"("market_index":206)"
                 R"(},{)"
                 R"("symbol":"AVGO",)"
                 R"("market_index":210)"
                 R"(})"
                 R"(])"sv;
  auto helper = [](value_type const &obj) {
    REQUIRE(std::size(obj.data) == 4);
    //
  };
  core::json::BufferStack buffers{8192, 1};
  value_type obj{message, buffers};
  helper(obj);
}
