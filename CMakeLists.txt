cmake_minimum_required(VERSION 3.8)
project(Hangman)

set(CMAKE_CXX_STANDARD 11)

set(SERVER_FILES
        server/client.h
        server/main.c
        server/server.h)

add_executable(server ${SERVER_FILES})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(server PRIVATE Threads::Threads)

target_compile_options(server PRIVATE -Wall)
