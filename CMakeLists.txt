cmake_minimum_required(VERSION 3.20)

project(pulsejson VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(pulsejson INTERFACE)
add_library(PulseJSON::pulsejson ALIAS pulsejson)

target_include_directories(pulsejson INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if (NOT TARGET RapidJSON::RapidJSON)
    if (TARGET RapidJSON)
        add_library(RapidJSON::RapidJSON INTERFACE IMPORTED)
        target_link_libraries(RapidJSON::RapidJSON INTERFACE RapidJSON)
    else()
        set(PULSEJSON_RAPIDJSON_INCLUDE_DIR
            "${CMAKE_CURRENT_LIST_DIR}/../rapidjson/include"
            CACHE PATH "path to rapidjson/include")

        if (NOT EXISTS "${PULSEJSON_RAPIDJSON_INCLUDE_DIR}/rapidjson/document.h")
            message(FATAL_ERROR
                "pulsejson requires RapidJSON::RapidJSON or "
                "PULSEJSON_RAPIDJSON_INCLUDE_DIR")
        endif()

        add_library(RapidJSON::RapidJSON INTERFACE IMPORTED)
        set_target_properties(RapidJSON::RapidJSON PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${PULSEJSON_RAPIDJSON_INCLUDE_DIR}")
    endif()
endif()

target_link_libraries(pulsejson INTERFACE RapidJSON::RapidJSON)

target_compile_features(pulsejson INTERFACE cxx_std_20)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR OR PULSEMOU_WITH_BUNDLED_PULSEJSON)
    install(TARGETS pulsejson
        EXPORT pulsejsonTargets
        COMPONENT pulsejson_Development
    )

    install(DIRECTORY include/pulsejson/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/pulsejson
        COMPONENT pulsejson_Development
    )

    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/pulsejson-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/pulsejson-config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pulsejson
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/pulsejson-config-version.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/pulsejson-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/pulsejson-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pulsejson
        COMPONENT pulsejson_Development
    )

    install(EXPORT pulsejsonTargets
        FILE pulsejson-targets.cmake
        NAMESPACE PulseJSON::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pulsejson
        COMPONENT pulsejson_Development
    )
endif()

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(CTest)
endif()

option(PULSEJSON_BUILD_TESTS "build pulsejson tests" ${BUILD_TESTING})

if (BUILD_TESTING AND PULSEJSON_BUILD_TESTS)
    add_executable(pulsejson-smoke
        tests/smoke.cc
    )

    target_link_libraries(pulsejson-smoke PRIVATE PulseJSON::pulsejson)
    target_compile_options(pulsejson-smoke PRIVATE
        -Wall -Wextra -Wpedantic
    )

    add_test(NAME pulsejson_smoke COMMAND pulsejson-smoke)
endif()
