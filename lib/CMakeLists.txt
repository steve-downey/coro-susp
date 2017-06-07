set(TARGET_NAME corosusp)

add_library(${TARGET_NAME} STATIC suspension.cpp)

target_include_directories(${TARGET_NAME}
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
