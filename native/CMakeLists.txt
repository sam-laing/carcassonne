cmake_minimum_required(VERSION 3.16)
project(carcassonne_native LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(carcassonne_engine STATIC src/engine.cpp)
target_include_directories(carcassonne_engine PUBLIC include)
target_compile_options(carcassonne_engine PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(carcassonne_engine PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(carcassonne_native SHARED src/c_api.cpp)
target_link_libraries(carcassonne_native PRIVATE carcassonne_engine)
target_include_directories(carcassonne_native PUBLIC include)
set_target_properties(carcassonne_native PROPERTIES OUTPUT_NAME carcassonne_native)

add_executable(carcassonne_native_tests tests/engine_test.cpp)
target_link_libraries(carcassonne_native_tests PRIVATE carcassonne_engine)

add_executable(carcassonne_benchmark tools/benchmark.cpp)
target_link_libraries(carcassonne_benchmark PRIVATE carcassonne_engine)

enable_testing()
add_test(NAME native_engine COMMAND carcassonne_native_tests)
