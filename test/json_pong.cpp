/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::lighter;

using namespace std::literals;
using namespace std::chrono_literals;

using value_type = protocol::json::Pong;

TEST_CASE("simple", "[json_pong]") {
  auto message = R"({)"
                 R"("error": null, )"
                 R"("result": "pong", )"
                 R"("id": 177472028786210)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.error.code == 0);
    CHECK(std::empty(obj.error.message));
    CHECK(obj.result == "pong"sv);
    CHECK(obj.id == 177472028786210);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
