set(TARGET_NAME susptest)

include_directories(${gtest_SOURCE_DIR}/include)

set(test_sources
  test1.cpp
  suspension.t.cpp
)

add_executable(${TARGET_NAME} ${test_sources})

target_link_libraries(${TARGET_NAME} gtest_main corosusp)
