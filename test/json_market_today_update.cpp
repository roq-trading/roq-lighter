/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::lighter;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = protocol::json::MarketTodayUpdate;

TEST_CASE("update", "[json_market_today_update]") {
  auto message = R"({)"
                 R"("method": "marketToday_update", )"
                 R"("params": [)"
                 R"("BTC_PERP", {)"
                 R"("open": "63629.3", )"
                 R"("high": "64022.6", )"
                 R"("low": "62346.8", )"
                 R"("volume": "42230.637", )"
                 R"("deal": "2670380757.0266", )"
                 R"("last": "63873.1")"
                 R"(})"
                 R"(], )"
                 R"("id": null)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.method == protocol::json::Method::MARKET_TODAY_UPDATE);
    CHECK(obj.params.name == "BTC_PERP"sv);
    CHECK(obj.params.data.open == 63629.3_a);
    CHECK(obj.params.data.high == 64022.6_a);
    CHECK(obj.params.data.low == 62346.8_a);
    CHECK(obj.params.data.volume == 42230.637_a);
    CHECK(obj.params.data.deal == 2670380757.0266_a);
    CHECK(obj.params.data.last == 63873.1_a);
    CHECK(obj.id == 0);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
