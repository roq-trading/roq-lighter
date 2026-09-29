set(TARGET_NAME ${PROJECT_NAME}-tools)

set(SOURCES crypto.cpp throttle.cpp)

add_library(${TARGET_NAME} OBJECT ${SOURCES})

include(RoqAutogen)

set(AUTOGEN_SCHEMAS api.json)

roq_autogen_hpp(
  OUTPUT
  AUTOGEN_HEADERS
  SOURCES
  ${AUTOGEN_SCHEMAS}
  TEMPLATE_DIR
  ${TEMPLATE_DIR}
  TEMPLATE_TYPE
  "state")

add_custom_target(${TARGET_NAME}-autogen-headers ALL DEPENDS ${AUTOGEN_HEADERS})

roq_gitignore(OUTPUT .gitignore SOURCES ${TARGET_NAME} ${AUTOGEN_HEADERS})

add_dependencies(${TARGET_NAME} ${TARGET_NAME}-autogen-headers)

target_link_libraries(${TARGET_NAME} PRIVATE roq-server::roq-server roq-web::roq-web roq-utils::roq-utils fmt::fmt)
