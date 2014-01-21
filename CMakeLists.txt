cmake_minimum_required(VERSION 2.8)

project(libdocset C)

include_directories(src)

add_library(docset
  src/vector.c
  src/docset.c
  src/type_names.c)

add_executable(docgrep examples/docgrep.c)

target_link_libraries(docset sqlite3)
target_link_libraries(docgrep docset)
