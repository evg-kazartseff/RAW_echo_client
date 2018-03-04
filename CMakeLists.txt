cmake_minimum_required(VERSION 3.9)
project(Echo C)

set(CMAKE_C_STANDARD 11)

add_subdirectory(Server)
add_subdirectory(Client)