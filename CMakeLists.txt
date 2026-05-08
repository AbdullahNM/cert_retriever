cmake_minimum_required(VERSION 3.10)
project(CertificateStore VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_library(CASSANDRA_LIB cassandra HINTS /usr/local/lib)
find_path(CASSANDRA_INCLUDE cassandra.h HINTS /usr/local/include)

find_package(OpenSSL REQUIRED)

add_library(db_client STATIC db_client.cpp db_client.h)
target_include_directories(db_client PUBLIC ${CASSANDRA_INCLUDE} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(db_client PUBLIC ${CASSANDRA_LIB} OpenSSL::SSL OpenSSL::Crypto uv pthread)

add_executable(cert_store main.cpp)
target_link_libraries(cert_store PRIVATE db_client)

add_executable(cert_tests tests.cpp)
target_link_libraries(cert_tests PRIVATE db_client)
