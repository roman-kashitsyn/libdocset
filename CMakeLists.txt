cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

option(build_examples "Build all the docset examples" ON)
option(build_tests "Build all the docset tests" ON)

set(MAJOR_VER 0)
set(MINOR_VER 2)
set(PATCH_VER 0)
set(VERSION "${MAJOR_VER}.${MINOR_VER}.${PATCH_VER}")
set(SOVERSION 0)

project(libdocset)

if(CMAKE_COMPILER_IS_GNUCC OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  add_definitions("-Wall -Werror")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic -std=c89")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

include_directories(src)

find_package(SQLite3 REQUIRED)
find_package(LibXml2 REQUIRED)

include_directories(${LIBXML2_INCLUDE_DIR})
include_directories(${SQLITE3_INCLUDE_DIR})

# The library
add_library(docset SHARED
  src/docset.c
  src/type_names.c
  src/prop_parser.c
  src/stringbuf.c)

set_target_properties(
  docset PROPERTIES
  VERSION   ${VERSION}
  SOVERSION ${SOVERSION}
)

target_link_libraries(
  docset
  ${SQLITE3_LIBRARIES}
  ${LIBXML2_LIBRARIES})

# C++ bindings
add_library(docset++ SHARED
  src/docset.cpp
)

set_target_properties(
  docset++ PROPERTIES
  VERSION   ${VERSION}
  SOVERSION ${SOVERSION}
)

target_link_libraries(docset++ docset)

# Install rules
install(
  TARGETS docset docset++
  LIBRARY DESTINATION lib)

install(
  FILES src/docset.h src/docset.hpp
  DESTINATION include/docset)

if(UNIX AND NOT APPLE)
  configure_file("data/libdocset.pc.in" "libdocset.pc" @ONLY)
  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/libdocset.pc
    DESTINATION lib/pkgconfig)

  configure_file("data/libdocset++.pc.in" "libdocset++.pc" @ONLY)
  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/libdocset++.pc
    DESTINATION lib/pkgconfig)
endif()

if(build_examples)
  add_executable(docgrep examples/docgrep.c)
  target_link_libraries(docgrep docset)

  add_executable(docfile examples/docfile.c)
  target_link_libraries(docfile docset)

  add_executable(docgrep++ examples/docgrep.cpp)
  target_link_libraries(docgrep++ docset++)
endif()

if (build_tests)
  enable_testing()

  add_executable(test_type_names test/test_type_names.c)
  target_link_libraries(test_type_names docset)

  add_test("TestTypeNameSearch" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_type_names)
endif()
