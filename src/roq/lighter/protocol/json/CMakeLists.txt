set(TARGET_NAME ${PROJECT_NAME}-protocol-json)

set(SOURCES encoder.cpp map.cpp parser.cpp utils.cpp)

include(RoqAutogen)

set(AUTOGEN_SCHEMAS
    order_book.json
    order_book_order_book.json
    order_book_order_book_level.json
    market_stats.json
    market_stats_market_stats.json
    connected.json
    asset_details_ack.json
    asset_details_ack_asset_details_item.json
    order_book_details_ack.json
    order_book_details_ack_order_book_details_item.json
    order_book_details_ack_spot_order_book_details_item.json
    market_type.json
    status.json
    type.json
    ticker.json
    ticker_ticker.json
    ticker_ticker_level.json
    pong.json
    error.json
    error_error.json
    trade.json
    trade_trades_item.json
    market_kind.json
    trade_type.json
    #
    method.json
    result.json
    trade_type.json)

roq_autogen_hpp(
  OUTPUT
  AUTOGEN_HEADERS
  SOURCES
  ${AUTOGEN_SCHEMAS}
  TEMPLATE_DIR
  ${TEMPLATE_DIR}
  TEMPLATE_TYPE
  "json")

add_custom_target(protocol-json-autogen-headers ALL DEPENDS ${AUTOGEN_HEADERS})

roq_autogen_cpp(
  OUTPUT
  AUTOGEN_SOURCES
  SOURCES
  ${AUTOGEN_SCHEMAS}
  TEMPLATE_DIR
  ${TEMPLATE_DIR}
  TEMPLATE_TYPE
  "json")

roq_gitignore(OUTPUT .gitignore SOURCES ${TARGET_NAME} ${AUTOGEN_HEADERS} ${AUTOGEN_SOURCES})

add_library(${TARGET_NAME} OBJECT ${SOURCES} ${AUTOGEN_SOURCES})

add_dependencies(${TARGET_NAME} protocol-json-autogen-headers)

target_link_libraries(${TARGET_NAME} PRIVATE roq-core::roq-core roq-codec::roq-codec roq-utils::roq-utils roq-logging::roq-logging fmt::fmt)
