/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::lighter;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = protocol::json::MarketUpdate;

TEST_CASE("update", "[json_market_update]") {
  auto message = R"({)"
                 R"("method": "market_update", )"
                 R"("params": [)"
                 R"("BTC_PERP", {)"
                 R"("open": "63170", )"
                 R"("close": "63776", )"
                 R"("high": "64022.6", )"
                 R"("low": "62346.8", )"
                 R"("volume": "51109.179", )"
                 R"("deal": "3234693364.6174", )"
                 R"("last": "63776", )"
                 R"("period": 86400)"
                 R"(})"
                 R"(], )"
                 R"("id": null)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.method == protocol::json::Method::MARKET_UPDATE);
    CHECK(obj.params.name == "BTC_PERP"sv);
    CHECK(obj.params.data.open == 63170_a);
    CHECK(obj.params.data.close == 63776_a);
    CHECK(obj.params.data.high == 64022.6_a);
    CHECK(obj.params.data.low == 62346.8_a);
    CHECK(obj.params.data.volume == 51109.179_a);
    CHECK(obj.params.data.deal == 3234693364.6174_a);
    CHECK(obj.params.data.last == 63776_a);
    CHECK(obj.params.data.period == 86400);
    CHECK(obj.id == 0);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
