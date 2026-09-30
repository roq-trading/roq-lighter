/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::lighter;

using namespace std::literals;
using namespace std::chrono_literals;

using value_type = protocol::json::Connected;

TEST_CASE("simple", "[json_connected]") {
  auto message = R"({)"
                 R"("session_id":"25754518-2da6-42a3-8b96-70d6cd103cb2",)"
                 R"("type":"connected")"
                 R"(})"sv;
  auto helper = [](value_type const &obj) { CHECK(obj.session_id == "25754518-2da6-42a3-8b96-70d6cd103cb2"sv); };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
