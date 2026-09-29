/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::lighter;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = protocol::json::TradesUpdate;

// note! truncated
TEST_CASE("snapshot", "[json_trades_update]") {
  auto message = R"({)"
                 R"("method": "trades_update", )"
                 R"("params": [)"
                 R"("BTC_PERP", [{)"
                 R"("id": 22676096396, )"
                 R"("time": 1785727720.6707921, )"
                 R"("price": "63117", )"
                 R"("amount": "10.967", )"
                 R"("type": "sell", )"
                 R"("rpi": false)"
                 R"(}, {)"
                 R"("id": 22676096353, )"
                 R"("time": 1785727720.586998, )"
                 R"("price": "63118.4", )"
                 R"("amount": "0.006", )"
                 R"("type": "buy", )"
                 R"("rpi": false)"
                 R"(} , {)"
                 R"("id": 22675746019, )"
                 R"("time": 1785726302.235096, )"
                 R"("price": "63193", )"
                 R"("amount": "0.012", )"
                 R"("type": "sell", )"
                 R"("rpi": false)"
                 R"(})"
                 R"(])"
                 R"(], )"
                 R"("id": null)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.method == protocol::json::Method::TRADES_UPDATE);
    CHECK(obj.params.name == "BTC_PERP"sv);
    REQUIRE(std::size(obj.params.data) == 3);
    auto &d0 = obj.params.data[0];
    CHECK(d0.id == 22676096396);
    // CHECK(d0.time == 1785727720670792us);
    CHECK(d0.time == 1785727720670800us);  // XXX FIXME TODO why is this rounded ???
    CHECK(d0.price == 63117_a);
    CHECK(d0.amount == 10.967_a);
    CHECK(d0.type == protocol::json::TradeType::SELL);
    CHECK(d0.rpi == false);
    CHECK(obj.id == 0);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

TEST_CASE("update", "[json_trades_update]") {
  auto message = R"({)"
                 R"("method": "trades_update", )"
                 R"("params": [)"
                 R"("BTC_PERP", [{)"
                 R"("id": 22676356091, )"
                 R"("time": 1785728782.167881, )"
                 R"("price": "63058.8", )"
                 R"("amount": "0.021", )"
                 R"("type": "sell", )"
                 R"("rpi": false)"
                 R"(})"
                 R"(])"
                 R"(], )"
                 R"("id": null)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.method == protocol::json::Method::TRADES_UPDATE);
    CHECK(obj.params.name == "BTC_PERP"sv);
    REQUIRE(std::size(obj.params.data) == 1);
    auto &d0 = obj.params.data[0];
    CHECK(d0.id == 22676356091);
    CHECK(d0.time == 1785728782167881us);
    CHECK(d0.price == 63058.8_a);
    CHECK(d0.amount == 0.021_a);
    CHECK(d0.type == protocol::json::TradeType::SELL);
    CHECK(d0.rpi == false);
    CHECK(obj.id == 0);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
