/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/lighter/protocol/json/market_info_ack.hpp"

using namespace roq;
using namespace roq::lighter;

using namespace std::literals;
using namespace std::chrono_literals;

using value_type = protocol::json::MarketInfoAck;

// note! truncated
TEST_CASE("simple", "[market_info_ack]") {
  auto message = R"([{)"
                 R"("name":"ETH_BTC",)"
                 R"("stock":"ETH",)"
                 R"("money":"BTC",)"
                 R"("stockPrec":"4",)"
                 R"("moneyPrec":"6",)"
                 R"("feePrec":"6",)"
                 R"("makerFee":"0.1",)"
                 R"("takerFee":"0.1",)"
                 R"("minAmount":"0.0001",)"
                 R"("minTotal":"0.000012",)"
                 R"("tradesEnabled":true,)"
                 R"("type":"spot",)"
                 R"("isCollateral":true,)"
                 R"("maxTotal":"100000",)"
                 R"("isTradFiFutures":false,)"
                 R"("delistedAt":null)"
                 R"(},{"name":"BTC_USD",)"
                 R"("stock":"BTC",)"
                 R"("money":"USD",)"
                 R"("stockPrec":"6",)"
                 R"("moneyPrec":"2",)"
                 R"("feePrec":"6",)"
                 R"("makerFee":"0.1",)"
                 R"("takerFee":"0.1",)"
                 R"("minAmount":"0.00001",)"
                 R"("minTotal":"1",)"
                 R"("tradesEnabled":true,)"
                 R"("type":"spot",)"
                 R"("isCollateral":false,)"
                 R"("maxTotal":"10000000",)"
                 R"("isTradFiFutures":false,)"
                 R"("delistedAt":null)"
                 R"(})"
                 R"(])"sv;
  auto helper = [](value_type const &obj) {
    REQUIRE(std::size(obj.data) == 2);
    //
  };
  core::json::BufferStack buffers{8192, 1};
  value_type obj{message, buffers};
  helper(obj);
}
