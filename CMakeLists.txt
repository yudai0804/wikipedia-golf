cmake_minimum_required(VERSION 3.10)

project(wikipedia-golf)

# Version 1.0
set(MAJOR_VERSION 1)
set(MINOR_VERSION 0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -O3 -pthread")

add_definitions(-DMAJOR_VERSION=${MAJOR_VERSION})
add_definitions(-DMINOR_VERSION=${MINOR_VERSION})

add_executable(wikipedia-golf src/main.cpp)
add_executable(create_graph_file src/create_graph_file.cpp)

target_include_directories(wikipedia-golf PRIVATE /usr/include/mysql-cppconn/jdbc)
target_include_directories(create_graph_file PRIVATE /usr/include/mysql-cppconn/jdbc)

target_link_libraries(wikipedia-golf PRIVATE mysqlcppconn)
target_link_libraries(create_graph_file PRIVATE mysqlcppconn)
