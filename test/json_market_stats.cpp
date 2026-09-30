/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::lighter;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = protocol::json::MarketStats;

TEST_CASE("simple", "[json_market_stats]") {
  auto message = R"({)"
                 R"("channel":"market_stats:1",)"
                 R"("market_stats":{)"
                 R"("symbol":"BTC",)"
                 R"("market_id":1,)"
                 R"("index_price":"85312.1",)"
                 R"("mark_price":"85265.7",)"
                 R"("mid_price":"85252.8",)"
                 R"("best_ask_price":"85254.4",)"
                 R"("best_bid_price":"85251.2",)"
                 R"("open_interest":"175114545.631038",)"
                 R"("open_interest_limit":"72057594037927936.000000",)"
                 R"("funding_clamp_small":"0.0500",)"
                 R"("funding_clamp_big":"4.0000",)"
                 R"("last_trade_price":"85251.1",)"
                 R"("current_funding_rate":"0.0003",)"
                 R"("funding_rate":"0.0001",)"
                 R"("funding_timestamp":1790773200002,)"
                 R"("daily_base_token_volume":9044.15085,)"
                 R"("daily_quote_token_volume":756158572.821753,)"
                 R"("daily_price_low":82868.7,)"
                 R"("daily_price_high":85617.2,)"
                 R"("daily_price_change":1.8560611942384344,)"
                 R"("base_interest_rate":"0.0100",)"
                 R"("premium":"-0.0475")"
                 R"(},)"
                 R"("timestamp":1790774392527,)"
                 R"("type":"subscribed/market_stats")"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == "market_stats:1"sv);
    //
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
