cmake_minimum_required(VERSION 3.10)
project(gold_price_prediction CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(predictor GoldPricePredictor.cpp)
add_executable(gold_price_prediction main.cpp)
target_link_libraries(gold_price_prediction predictor)

enable_testing()
add_executable(test_predictor test_predictor.cpp)
target_link_libraries(test_predictor predictor)
add_test(NAME predictor COMMAND test_predictor)
# Les tests utilisent assert() : on les garde actifs même en Release.
if(NOT MSVC)
  target_compile_options(test_predictor PRIVATE -UNDEBUG)
endif()
