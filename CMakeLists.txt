cmake_minimum_required(VERSION 3.16)
project(pokerbot_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_library(pokerbot_core SHARED
  cpp/pokerbot/core/c_api.cpp
  cpp/pokerbot/core/hand_evaluator.cpp
  cpp/pokerbot/core/limit_holdem_game.cpp
)

target_include_directories(pokerbot_core
  PUBLIC
    ${PROJECT_SOURCE_DIR}/cpp
)

target_compile_features(pokerbot_core PUBLIC cxx_std_17)

install(TARGETS pokerbot_core
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
)
