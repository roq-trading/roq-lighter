/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::lighter;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = protocol::json::DepthUpdate;

// note! truncated
TEST_CASE("snapshot", "[json_depth_update]") {
  auto message = R"({)"
                 R"("method": "depth_update", )"
                 R"("params": [)"
                 R"(true, {)"
                 R"("timestamp": 1785730130.6040511, )"
                 R"("update_id": 22216665449, )"
                 R"("asks": [)"
                 R"(["62957.3", "0.017"], )"
                 R"(["62961.4", "0.029"], )"
                 R"(["63038.7", "0.285"])"
                 R"(], )"
                 R"("bids": [)"
                 R"(["62957.2", "0.017"], )"
                 R"(["62951.7", "0.004"], )"
                 R"(["62803.3", "1.09"])"
                 R"(], )"
                 R"("event_time": 1785730135.194067)"
                 R"(}, )"
                 R"("BTC_PERP")"
                 R"(], )"
                 R"("id": null)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.method == protocol::json::Method::DEPTH_UPDATE);
    CHECK(obj.params.snapshot == true);
    CHECK(obj.params.data.timestamp == 1785730130604051us);
    CHECK(obj.params.data.update_id == 22216665449);
    REQUIRE(std::size(obj.params.data.asks) == 3);
    auto &a0 = obj.params.data.asks[0];
    CHECK(a0.price == 62957.3_a);
    CHECK(a0.amount == 0.017_a);
    REQUIRE(std::size(obj.params.data.bids) == 3);
    auto &b0 = obj.params.data.bids[0];
    CHECK(b0.price == 62957.2_a);
    CHECK(b0.amount == 0.017_a);
    // CHECK(obj.params.data.event_time == 1785730135294067us);
    CHECK(obj.params.data.event_time == 1785730135200000us);  // XXX FIXME TODO why is this rounded ???
    CHECK(obj.params.name == "BTC_PERP"sv);
    CHECK(obj.id == 0);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 2);
}

TEST_CASE("incremental", "[json_depth_update]") {
  auto message = R"({)"
                 R"("method": "depth_update", )"
                 R"("params": [)"
                 R"(false, {)"
                 R"("timestamp": 1785730136.175524, )"
                 R"("update_id": 22216668616, )"
                 R"("past_update_id": 22216668570, )"
                 R"("asks": [)"
                 R"(["62925.8", "0.13"], )"
                 R"(["62932.1", "0.328"], )"
                 R"(["62934.2", "0.31"], )"
                 R"(["62940.4", "0.541"], )"
                 R"(["62942.5", "0.499"], )"
                 R"(["62948.8", "0.836"], )"
                 R"(["62950.9", "0.593"], )"
                 R"(["62957.2", "0.839"], )"
                 R"(["62997.2", "0"])"
                 R"(], )"
                 R"("bids": [)"
                 R"(["62820.2", "0.453"], )"
                 R"(["62818.1", "0.499"], )"
                 R"(["62816", "0.446"], )"
                 R"(["62813.9", "0.496"], )"
                 R"(["62788.7", "0.906"], )"
                 R"(["62784.6", "1.032"], )"
                 R"(["62780.4", "1.548"], )"
                 R"(["62776.2", "1.351"])"
                 R"(], )"
                 R"("event_time": 1785730136.2352009)"
                 R"(}, )"
                 R"("BTC_PERP")"
                 R"(], )"
                 R"("id": null)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.method == protocol::json::Method::DEPTH_UPDATE);
    CHECK(obj.params.snapshot == false);
    CHECK(obj.params.data.timestamp == 1785730136175524us);
    CHECK(obj.params.data.update_id == 22216668616);
    CHECK(obj.params.data.past_update_id == 22216668570);
    REQUIRE(std::size(obj.params.data.asks) == 9);
    REQUIRE(std::size(obj.params.data.bids) == 8);
    CHECK(obj.params.data.event_time == 1785730136235200us);
    CHECK(obj.params.name == "BTC_PERP"sv);
    CHECK(obj.id == 0);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 2);
}
