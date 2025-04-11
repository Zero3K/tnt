
cmake_minimum_required(VERSION 3.5.0)
project(tnt)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

add_executable(${PROJECT_NAME} tnt/main.cpp)
add_library(testlib tnt/parser/pp.cpp)

target_link_libraries(${PROJECT_NAME} testlib ${OPENSSL_LIBRARIES})