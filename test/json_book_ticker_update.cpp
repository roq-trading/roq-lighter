/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::lighter;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = protocol::json::BookTickerUpdate;

TEST_CASE("simple", "[json_book_ticker_update]") {
  auto message = R"({)"
                 R"("method": "bookTicker_update", )"
                 R"("params": [[)"
                 R"(1785732678.2802551, )"
                 R"(1785732678.2810941, )"
                 R"("BTC_PERP", )"
                 R"(22217102542, )"
                 R"("62965.8", )"
                 R"("0.005", )"
                 R"("62974.2", )"
                 R"("0.017")"
                 R"(])"
                 R"(], )"
                 R"("id": null)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.method == protocol::json::Method::BOOK_TICKER_UPDATE);
    REQUIRE(std::size(obj.params) == 1);
    auto &p0 = obj.params[0];
    CHECK(p0.transaction_time == 1785732678280255us);
    CHECK(p0.message_time == 1785732678281100us);  // XXX FIXME TODO why is this rounded ???
    CHECK(p0.market == "BTC_PERP"sv);
    CHECK(p0.update_id == 22217102542);
    CHECK(p0.best_bid_price == 62965.8_a);
    CHECK(p0.best_bid_amount == 0.005_a);
    CHECK(p0.best_ask_price == 62974.2_a);
    CHECK(p0.best_ask_amount == 0.017_a);
    CHECK(obj.id == 0);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 2);
}
