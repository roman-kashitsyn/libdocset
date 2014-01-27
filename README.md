libdocset - docset bundles reader
=================================

libdocset is a pure-C (C89-compiant) library for .docset bundles
reading. The bundles are used by the handy
[Dash](http://kapeli.com/dash) application for MacOS.

What you can do
---------------

* Extract basic docset meta-information (name, bundle identifier,
  platform family, is js enabled, etc).
* Enumerate all the docset entries.
* Perform simple queries using sql-like patterns.

What you can't do (yet?)
------------------------

* Access table of contents.
* Create docsets using this library.

Example
=======

    DocSet *docset = docset_open("~/.docsets/C.docset");
    DocSetCursor *cursor = docset_find(docset, "%printf");
    while (docset_cursor_step(cursor)) {
        DocSetEntry *e = docset_cursor_entry(cursor);
        printf("%s (%s): %s",
               docset_entry_name(cursor),
               docset_entry_canonical_type(cursor),
               docset_entry_path(cursor));
    }
    docset_cursor_dispose(cursor);
    docset_close(docset);

Please find more examples in the `/examples` directory and some basic
documentation in the `src/docset.h` header file.

Building
========

Currently only [CMake](http://www.cmake.org/) builds are supported.
You can use the following actions to build the library:

    mkdir somewhere/docset-build
    cd somwhere/docset-build
    cmake /path/to/libdocset
    make
    make install

Thread Safety
=============

All the library functions are reenterable. All the data structures
used by the library are not thread-safe and require external
synchronization when accessing them from multiple threads.

Status
======

The library is in active development and is not quite ready for
production use.

Dependencies
============

The library depends on following libraries:

* `libsqlite3` is required to read the docset index
  database;

* `libxml2` is used to parse docset property lists.

License
=======

The library is distributed under the MIT license.
