cmake_minimum_required(VERSION 2.8)

project(libdocset C)

if(CMAKE_COMPILER_IS_GNUCC)
  add_definitions("-Wall -Werror -Wpedantic -std=c89")
endif()

include_directories(src)

add_library(docset
  src/vector.c
  src/docset.c
  src/type_names.c
  src/stringbuf.c)

add_executable(docgrep examples/docgrep.c)

target_link_libraries(docset sqlite3)
target_link_libraries(docgrep docset)
