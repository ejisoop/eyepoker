cmake_minimum_required(VERSION 3.20)
project(pokereye VERSION 0.2.0 LANGUAGES CXX)

option(POKEREYE_NATIVE "Tune generated code for this build machine (not portable)" OFF)
option(POKEREYE_SANITIZERS "Enable address/undefined sanitizers on GCC/Clang" OFF)
option(POKEREYE_BENCHMARKS "Build reproducible benchmarks" ON)
option(POKEREYE_LEGACY_TESTS "Also build original GoogleTest tests (requires installed GTest)" OFF)

find_package(Threads REQUIRED)
add_library(poker_core STATIC
  src/core/random.cpp
  src/core/deck.cpp
  src/core/hand_evaluator.cpp
  src/core/engine.cpp
  src/core/pot.cpp
  src/core/equity.cpp
)
add_library(PokerEye::core ALIAS poker_core)
target_compile_features(poker_core PUBLIC cxx_std_20)
set_target_properties(poker_core PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(poker_core PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(poker_core PUBLIC Threads::Threads)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(poker_core PRIVATE -Wall -Wextra -Wpedantic)
  if(POKEREYE_NATIVE)
    target_compile_options(poker_core PRIVATE -march=native)
  endif()
  if(POKEREYE_SANITIZERS)
    target_compile_options(poker_core PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(poker_core PUBLIC -fsanitize=address,undefined)
  endif()
endif()

add_executable(poker_sim src/cli/main.cpp)
target_link_libraries(poker_sim PRIVATE PokerEye::core)
add_executable(poker_example examples/custom_variant.cpp)
target_link_libraries(poker_example PRIVATE PokerEye::core)
if(POKEREYE_BENCHMARKS)
  add_executable(poker_bench benchmarks/benchmark.cpp)
  target_link_libraries(poker_bench PRIVATE PokerEye::core)
endif()
include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
