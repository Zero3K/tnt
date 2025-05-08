
cmake_minimum_required(VERSION 4.0.0)
project(tnt)

set(CMAKE_CXX_STANDARD 20)

# |-------------------------------------------------

include(FetchContent)

# openssl
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

# libcpr
set(CPR_USE_SYSTEM_CURL ON)
FetchContent_Declare(cpr 
    QUIET
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.11.2
)
FetchContent_MakeAvailable(cpr)

# |-------------------------------------------------

add_library(bencode 
    tnt/bencode/decoding.cpp
    tnt/bencode/encoding.cpp
)
add_library(torrent_file
    tnt/torrent_file/parser.cpp
)
add_library(torrent_tracker
    tnt/torrent_tracker/torrent_tracker.cpp
)
target_link_libraries(
    torrent_tracker PRIVATE cpr::cpr
)
add_library(tcp_connection
    tnt/tcp_connection/tcp_connection.cpp
)

add_executable(${PROJECT_NAME} tnt/main.cpp)

target_link_libraries(
    ${PROJECT_NAME} 
    cpr::cpr 
    ${OPENSSL_LIBRARIES}

    bencode 
    torrent_file 
    torrent_tracker
)