
cmake_minimum_required(VERSION 4.0.0)
project(tnt)

set(CMAKE_CXX_STANDARD 20)

# add_compile_options(-fsanitize=thread)
# add_link_options(-fsanitize=thread)

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

# cxxopts
FetchContent_Declare(
  cxxopts
  GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
  GIT_TAG        v3.2.0
)
FetchContent_MakeAvailable(cxxopts)

# |-------------------------------------------------

add_library(bencode 
    tnt/bencode/decoding.cpp
    tnt/bencode/encoding.cpp
)
add_library(torrent_file
    tnt/torrent_file/parser.cpp
)
add_library(torrent_tracker
    tnt/torrent_tracker.cpp
)
target_link_libraries(
    torrent_tracker PRIVATE cpr::cpr
)
add_library(tcp_connection
    tnt/tcp_connection/tcp_connection.cpp
)
add_library(peer_connection
    tnt/peer_connection/peer_connection.cpp
    tnt/peer_connection/message.h
    tnt/peer_connection/message.cpp
)
add_library(piece_storage
    tnt/piece_storage/piece.cpp
    tnt/piece_storage/piece_storage.cpp
)
add_library(peer_downloader
    tnt/peer_downloader/peer_downloader.cpp
)
add_library(conductor
    tnt/conductor/conductor.cpp
)
add_library(visuals
    tnt/visuals/visuals.cpp
)

add_executable(${PROJECT_NAME} tnt/main.cpp)

target_link_libraries(
    ${PROJECT_NAME} 
    cpr::cpr 
    ${OPENSSL_LIBRARIES}
    cxxopts

    bencode 
    torrent_file 
    torrent_tracker
    tcp_connection
    peer_connection
    piece_storage
    peer_downloader
    conductor
    visuals
)