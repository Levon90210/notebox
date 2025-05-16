cmake_minimum_required(VERSION 3.31)
project(NoteBox C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_library(NoteBoxLib src/notebox.c)

add_executable(NoteBox main.c)

target_link_libraries(NoteBox
    PRIVATE
    pthread
    ncurses
    NoteBoxLib
)