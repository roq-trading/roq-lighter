/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::lighter;

using namespace std::literals;
using namespace std::chrono_literals;

using value_type = protocol::json::Response;

TEST_CASE("success", "[json_response]") {
  auto message = R"({)"
                 R"("error": null, )"
                 R"("result": {)"
                 R"("status": "success")"
                 R"(}, )"
                 R"("id": 2000001)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.error.code == 0);
    CHECK(std::empty(obj.error.message));
    CHECK(obj.result.status == "success"sv);
    CHECK(obj.id == 2000001);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

TEST_CASE("failure", "[json_response]") {
  auto message = R"({)"
                 R"("error": {)"
                 R"("code": 9, )"
                 R"("message": "market BTC_PERPx does not exist")"
                 R"(}, )"
                 R"("result": null, )"
                 R"("id": 2000001)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.error.code == 9);
    CHECK(obj.error.message == "market BTC_PERPx does not exist"sv);
    CHECK(std::empty(obj.result.status));
    CHECK(obj.id == 2000001);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
