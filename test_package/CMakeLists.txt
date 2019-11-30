# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.8.2)
project(test_graphql VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 17)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
            conan_basic_setup(TARGETS)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/unified)

function(add_bigobj_flag target)
  if(MSVC)
    # MSVC requires the /bigobj flag if the number of sections gets too big.
    target_compile_options(${target} PRIVATE /bigobj)
  endif()
endfunction()
include_directories(${CONAN_INCLUDE_DIRS_CPPGRAPHQLGEN})
find_program (SCHEMAGEN schemagen PATHS ${CONAN_BIN_DIRS_CPPGRAPHQLGEN})
# todaygraphql
add_custom_command(
  OUTPUT
    unified/TodaySchema.cpp
    unified/TodaySchema.h
  COMMAND ${SCHEMAGEN} --schema="${CMAKE_CURRENT_SOURCE_DIR}/today/schema.today.graphql" --prefix="Today" --namespace="today"
  DEPENDS ${SCHEMAGEN} today/schema.today.graphql
  WORKING_DIRECTORY unified
  COMMENT "Generating mock TodaySchema files"
)

# force the generation of samples on the default build target
add_custom_target(update_samples ALL
  DEPENDS
    unified/TodaySchema.cpp
)

add_library(unifiedschema STATIC unified/TodaySchema.cpp)
# target_link_libraries(unifiedschema PUBLIC ) # CONAN_PKG::cppgraphqlgen)
target_include_directories(unifiedschema PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include
  ${CMAKE_CURRENT_BINARY_DIR}/unified)
add_bigobj_flag(unifiedschema)

add_library(unifiedgraphql today/UnifiedToday.cpp)
target_link_libraries(unifiedgraphql PUBLIC unifiedschema)
target_include_directories(unifiedgraphql PUBLIC today)

# test_today
find_package(Threads MODULE REQUIRED)
add_executable(sample today/sample.cpp)
target_link_libraries(sample PRIVATE
  unifiedgraphql
CONAN_PKG::cppgraphqlgen
Threads::Threads)
target_include_directories(sample PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../PEGTL/include
  ${CMAKE_CURRENT_SOURCE_DIR}/today)

if(GRAPHQL_UPDATE_SAMPLES)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/unified
    DESTINATION ${CMAKE_CURRENT_SOURCE_DIR})
endif()
