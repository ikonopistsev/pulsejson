cmake_minimum_required(VERSION 3.20)

project(pulsejson LANGUAGES CXX)

add_library(pulsejson INTERFACE)

set(PULSEJSON_RAPIDJSON_INCLUDE_DIR
    "${CMAKE_CURRENT_LIST_DIR}/../rapidjson/include"
    CACHE PATH "path to rapidjson/include")

if (NOT EXISTS "${PULSEJSON_RAPIDJSON_INCLUDE_DIR}/rapidjson/document.h")
    message(FATAL_ERROR
        "pulsejson requires rapidjson; set PULSEJSON_RAPIDJSON_INCLUDE_DIR")
endif()

target_include_directories(pulsejson INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    $<BUILD_INTERFACE:${PULSEJSON_RAPIDJSON_INCLUDE_DIR}>
)

target_compile_features(pulsejson INTERFACE cxx_std_20)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(CTest)
endif()

option(PULSEJSON_BUILD_TESTS "build pulsejson tests" ${BUILD_TESTING})

if (BUILD_TESTING AND PULSEJSON_BUILD_TESTS)
    add_executable(pulsejson-smoke
        tests/smoke.cc
    )

    target_link_libraries(pulsejson-smoke PRIVATE pulsejson)
    target_compile_options(pulsejson-smoke PRIVATE
        -Wall -Wextra -Wpedantic
    )

    add_test(NAME pulsejson_smoke COMMAND pulsejson-smoke)
endif()
