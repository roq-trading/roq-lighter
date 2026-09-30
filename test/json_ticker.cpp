/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::lighter;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = protocol::json::Ticker;

TEST_CASE("simple", "[json_ticker]") {
  auto message = R"({)"
                 R"("channel":"ticker:1",)"
                 R"("last_updated_at":1790769272386001,)"
                 R"("nonce":23404090787,)"
                 R"("ticker":{)"
                 R"("s":"BTC",)"
                 R"("a":{)"
                 R"("price":"83791.8",)"
                 R"("size":"0.01790")"
                 R"(},)"
                 R"("b":{)"
                 R"("price":"83786.0",)"
                 R"("size":"0.01482")"
                 R"(},)"
                 R"("last_updated_at":1790769272386001)"
                 R"(},)"
                 R"("timestamp":1790769272388,)"
                 R"("type":"subscribed/ticker")"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == "ticker:1"sv);
    CHECK(obj.last_updated_at == 1790769272386001us);
    CHECK(obj.nonce == 23404090787);
    CHECK(obj.ticker.symbol == "BTC"sv);
    CHECK(obj.ticker.ask.price == 83791.8_a);
    CHECK(obj.ticker.ask.size == 0.01790_a);
    CHECK(obj.ticker.bid.price == 83786.0_a);
    CHECK(obj.ticker.bid.size == 0.01482_a);
    CHECK(obj.ticker.last_updated_at == 1790769272386001us);
    CHECK(obj.timestamp == 1790769272388ms);
    CHECK(obj.type == "subscribed/ticker"sv);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
