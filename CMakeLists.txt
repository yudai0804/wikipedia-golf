cmake_minimum_required(VERSION 3.10)
project(wikipedia-golf)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -O3")

add_executable(wikipedia-golf src/main.cpp)

target_include_directories(wikipedia-golf PRIVATE /usr/include/mysql-cppconn/jdbc)
target_link_libraries(wikipedia-golf PRIVATE mysqlcppconn)