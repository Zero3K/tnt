
cmake_minimum_required(VERSION 3.5.0)
project(tnt)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

add_executable(${PROJECT_NAME} tnt/main.cpp)
add_library(bencode 
	tnt/bencode/decoding.cpp
	tnt/bencode/encoding.cpp
)
add_library(torrent_file
	tnt/torrent_file/parser.cpp
)

target_link_libraries(${PROJECT_NAME} bencode torrent_file ${OPENSSL_LIBRARIES})