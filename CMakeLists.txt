
cmake_minimum_required(VERSION 3.5.0)
project(tnt)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

add_executable(${PROJECT_NAME} tnt/main.cpp)
add_library(bencoding 
	tnt/bencoding/decoding.cpp
	tnt/bencoding/encoding.cpp
)
add_library(metainfo 
	tnt/metainfo/parser.cpp
)

target_link_libraries(${PROJECT_NAME} bencoding metainfo ${OPENSSL_LIBRARIES})