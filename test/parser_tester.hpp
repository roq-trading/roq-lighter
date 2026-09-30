/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/lighter/protocol/json/parser.hpp"

namespace roq {
namespace lighter {

template <typename T>
struct ParserTester final : public protocol::json::Parser::Handler {
  using value_type = std::remove_cvref_t<T>;
  using callback_type = std::function<void(value_type const &)>;  // XXX FIXME TODO doesn't accept multiple arguments...

  static void dispatch(callback_type const &callback, std::string_view const &message, size_t buffer_size, size_t max_depth) {
    core::json::BufferStack buffers{buffer_size, max_depth};
    // simple
    // XXX FIXME TODO catch2 block ???
    T obj{message, buffers};
    callback(obj);
    // parser
    // XXX FIXME TODO catch2 block ???
    ParserTester handler{callback};
    auto res = protocol::json::Parser::dispatch(handler, message, buffers, {}, false);
    CHECK(res == true);
    CHECK(handler.found_ == true);
  }

 protected:
  explicit ParserTester(callback_type const &callback) : callback_{callback} {}

  void operator()(Trace<protocol::json::Connected> const &event) override { dispatch_helper(event); }
  void operator()(Trace<protocol::json::Pong> const &event) override { dispatch_helper(event); }
  void operator()(Trace<protocol::json::Error> const &event) override { dispatch_helper(event); }
  void operator()(Trace<protocol::json::OrderBook> const &event) override { dispatch_helper(event); }
  void operator()(Trace<protocol::json::Ticker> const &event) override { dispatch_helper(event); }
  void operator()(Trace<protocol::json::Trade> const &event) override { dispatch_helper(event); }
  void operator()(Trace<protocol::json::MarketStats> const &event) override { dispatch_helper(event); }

  /* XXX FIXME TODO
  template <typename U, typename... Args>
  void dispatch_helper(Trace<U> const &event, Args &&...args) {
    if constexpr (std::is_invocable_v<callback_type, U, Args...>) {
      found_ = true;
      callback_(event, std::forward<Args>(args)...);
    } else {
      FAIL();
    }
  }
  */
  template <typename U>
  void dispatch_helper(Trace<U> const &event) {
    if constexpr (std::is_invocable_v<callback_type, U>) {
      found_ = true;
      callback_(event);
    } else {
      FAIL();
    }
  }

 private:
  callback_type const callback_;
  bool found_ = false;
};

}  // namespace lighter
}  // namespace roq
