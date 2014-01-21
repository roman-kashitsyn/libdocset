cmake_minimum_required(VERSION 2.8)

project(libdocset C)

include_directories(src)

add_library(docset
  src/vector.c
  src/docset.c)

target_link_libraries(docset sqlite3)