cmake_minimum_required(VERSION 3.12)
project(task_graph_when_all_sample)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "-Wall -Wno-long-long -pedantic")

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_executable(task_graph_when_all_sample main.cpp)

target_link_libraries(task_graph_when_all_sample Threads::Threads)