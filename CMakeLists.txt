cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(HypeRate VERSION 1.0.0)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE src)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# ---------------------------------------------------------------------------
# HypeRate API key
#
# The key is never committed. Sources, in order:
#   1. -DHYPERATE_API_KEY=... when configuring
#   2. HYPERATE_API_KEY environment variable (CI secret)
#   3. api-key.txt next to this file (local builds, gitignored)
# ---------------------------------------------------------------------------
if (NOT DEFINED HYPERATE_API_KEY)
    set(HYPERATE_API_KEY "$ENV{HYPERATE_API_KEY}")
endif()
if (HYPERATE_API_KEY STREQUAL "" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/api-key.txt")
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/api-key.txt" HYPERATE_API_KEY)
    string(STRIP "${HYPERATE_API_KEY}" HYPERATE_API_KEY)
endif()
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/api-key.txt")
    # Re-configure when the key file changes.
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/api-key.txt")
endif()
if (NOT HYPERATE_API_KEY MATCHES "^[A-Za-z0-9_-]*$")
    message(FATAL_ERROR "HYPERATE_API_KEY contains unexpected characters.")
endif()
if (HYPERATE_API_KEY STREQUAL "")
    message(WARNING "HYPERATE_API_KEY is not set - the mod will build, but cannot connect to HypeRate.")
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE HYPERATE_API_KEY="${HYPERATE_API_KEY}")

# ---------------------------------------------------------------------------
# WebSocket (IXWebSocket)
#
# TLS backend per platform:
#   macOS / iOS      -> Apple Secure Transport (system trust store)
#   Windows / Android -> mbedTLS (statically linked)
# ---------------------------------------------------------------------------
set(USE_TLS ON CACHE BOOL "" FORCE)
set(USE_ZLIB OFF CACHE BOOL "" FORCE)
set(IXWEBSOCKET_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

if (NOT APPLE)
    CPMAddPackage(
        NAME mbedtls
        URL https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-3.6.7/mbedtls-3.6.7.tar.bz2
        PATCH_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch-mbedtls.cmake
        OPTIONS
            "ENABLE_PROGRAMS OFF"
            "ENABLE_TESTING OFF"
            "GEN_FILES OFF"
            "MBEDTLS_FATAL_WARNINGS OFF"
            "USE_SHARED_MBEDTLS_LIBRARY OFF"
            "USE_STATIC_MBEDTLS_LIBRARY ON"
    )
    # Tell IXWebSocket to use the mbedTLS targets above instead of find_package().
    set(USE_MBED_TLS ON CACHE BOOL "" FORCE)
    set(MBEDTLS_FOUND TRUE)
    set(MBEDTLS_INCLUDE_DIRS "${mbedtls_SOURCE_DIR}/include")
    set(MBEDTLS_LIBRARIES mbedtls mbedx509 mbedcrypto)
endif()

CPMAddPackage(
    NAME ixwebsocket
    GITHUB_REPOSITORY machinezone/IXWebSocket
    GIT_TAG v12.0.1
)

target_link_libraries(${PROJECT_NAME} ixwebsocket)

# Geode's precompiled header includes <windows.h>, which conflicts with the
# <winsock2.h> IXWebSocket needs. Build the only file using it without the PCH.
set_source_files_properties(src/HypeRateClient.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

setup_geode_mod(${PROJECT_NAME})
