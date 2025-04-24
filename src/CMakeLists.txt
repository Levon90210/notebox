file(GLOB SRC_FILES *.c)

add_library(NoteBoxLib ${SRC_FILES})

target_include_directories(NoteBoxLib PUBLIC ${CMAKE_SOURCE_DIR}/include)