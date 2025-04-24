cmake_minimum_required(VERSION 3.31)
project(NoteBox C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_subdirectory(src)

add_executable(NoteBox main.c)

target_link_directories(NoteBox PRIVATE pthread)
target_link_directories(NoteBox PRIVATE NoteBoxLib)