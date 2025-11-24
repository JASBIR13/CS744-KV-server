cmake_minimum_required(VERSION 3.10)
project(KVServer)
set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

add_executable(kv_server main.cpp)
target_include_directories(kv_server PRIVATE ${PQXX_INCLUDE_DIRS})
target_link_libraries(kv_server ${PQXX_LIBRARIES} pthread)
add_subdirectory(client)
