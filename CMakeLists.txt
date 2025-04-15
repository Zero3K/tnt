
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
add_library(tf_utils
	tnt/tf_utils/parser.cpp
)

target_link_libraries(${PROJECT_NAME} bencoding tf_utils ${OPENSSL_LIBRARIES})