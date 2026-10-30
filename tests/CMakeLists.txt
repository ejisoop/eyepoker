include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY
    https://github.com/google/googletest.git
  GIT_TAG v1.16.0
)

FetchContent_MakeAvailable(googletest)

add_executable(poker_tests
  unit/random_test.cpp
  unit/deck_test.cpp
)

target_link_libraries(poker_tests
  PRIVATE
    poker_core
    GTest::gtest_main
)

include(GoogleTest)

gtest_discover_tests(poker_tests)
