cmake_minimum_required(VERSION 3.23)

#set(CMAKE_VERBOSE_MAKEFILE ON)

project(bloggy
        HOMEPAGE_URL "https://hogsy.me/"
        DESCRIPTION "A very simple tool for generating a very simple blog."
        LANGUAGES C CXX
)

add_subdirectory(src/hei/plcore)

set(CMAKE_C_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin")

add_executable(bloggy
        src/bloggy.c
)

target_link_libraries(bloggy plcore)
