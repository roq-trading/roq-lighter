/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::lighter;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = protocol::json::OrderBook;

// note! truncated
TEST_CASE("simple", "[json_order_book]") {
  auto message = R"({)"
                 R"("channel":"order_book:1",)"
                 R"("last_updated_at":1790775306313578,)"
                 R"("offset":1128252,)"
                 R"("order_book":{)"
                 R"("code":0,)"
                 R"("asks":[)"
                 R"({"price":"85150.1","size":"2.01682"},)"
                 R"({"price":"85150.5","size":"0.09565"},)"
                 R"({"price":"138816.1","size":"0.00187"},)"
                 R"({"price":"250759.2","size":"0.00360"})"
                 R"(],)"
                 R"("bids":[)"
                 R"({"price":"85150.0","size":"27.43087"},)"
                 R"({"price":"85149.7","size":"0.00012"},)"
                 R"({"price":"20000.0","size":"0.10000"},)"
                 R"({"price":"8351.5","size":"0.21480"})"
                 R"(],)"
                 R"("offset":1128252,)"
                 R"("nonce":23411089863,)"
                 R"("last_updated_at":1790775306313578,)"
                 R"("begin_nonce":0)"
                 R"(},)"
                 R"("timestamp":1790775306359,)"
                 R"("type":"subscribed/order_book")"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == "order_book:1"sv);
    //
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
