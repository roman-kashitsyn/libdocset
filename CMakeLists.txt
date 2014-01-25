cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

option(build_examples "Build all the docset examples" ON)

set(MAJOR_VER 0)
set(MINOR_VER 1)
set(PATCH_VER 0)

project(libdocset C)

if(CMAKE_COMPILER_IS_GNUCC)
  add_definitions("-Wall -Werror -Wpedantic -std=c89")
endif()

include_directories(src)

find_package(SQLite3 REQUIRED)
find_package(LibXml2 REQUIRED)

include_directories(${LIBXML2_INCLUDE_DIR})
include_directories(${SQLITE3_INCLUDE_DIR})

add_library(docset SHARED
  src/docset.c
  src/type_names.c
  src/prop_parser.c
  src/stringbuf.c)

target_link_libraries(
  docset
  ${SQLITE3_LIBRARIES}
  ${LIBXML2_LIBRARIES})

install(TARGETS docset
  LIBRARY DESTINATION lib)

install(FILES src/docset.h DESTINATION include/docset)

if(build_examples)
  add_executable(docgrep examples/docgrep.c)
  target_link_libraries(docgrep docset)

  add_executable(docfile examples/docfile.c)
  target_link_libraries(docfile docset)
endif()

set(CPACK_PACKAGE_NAME "libdocset")
set(CPACK_GENERATOR "DEB")

set(CPACK_PACKAGE_VERSION_MAJOR ${MAJOR_VER})
set(CPACK_PACKAGE_VERSION_MINOR ${MINOR_VER})
set(CPACK_PACKAGE_VERSION_PATCH ${PATCH_VER})
set(CPACK_PACKAGE_VERSION "${MAJOR_VER}-${MINOR_VER}-${PATCH_VER}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Library to read docset bundles")

set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE.txt)
set(CPACK_PACKAGE_CONTACT "Roman Kashitsyn <roman.kashitsyn@gmail.com>")

set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsqlite3-0, libxml2")
set(CPACK_DEBIAN_PACKAGE_SECTION "Libraries")

include(CPack)