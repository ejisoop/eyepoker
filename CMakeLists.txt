cmake_minimum_required(VERSION 3.20)

project(pokereye)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(poker_core STATIC
  src/core/random.cpp
  src/core/deck.cpp
)

target_include_directories(poker_core
  PUBLIC ${PROJECT_SOURCE_DIR}/include
)

include(CTest)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
