libdocset - docset bundles reader
=================================

libdocset is a pure-C library for readind of .docset bundles used by
Dash application for MacOS.  It allows to extract docset bundle
meta-information and perform various queries (substring and fuzzy search).

Example
=======

    DocSet * docset = docset_open("~/.docsets/C.docset");
    DocSetSearch * search = docset_fuzzy_find(docset, "prntf");
    while (docset_search_has_more(search)) {
        docset_search_advance(search);
        printf("%s (%s): %s",
               docset_search_entry_name(search),
               docset_search_entry_type_name(search),
               docset_search_entry_path(search));
    }
    docset_dispose_search(search);
    docset_close(docset);

Thread Safety
=============

All the library functions are reenterable. All the data structures
used by the library are not thread-safe and require external
synchronization when accessing them from multiple threads.

Dependencies
============

The library depends on libsqlite3 required to read the docset index
database.


License
=======
The library is distributed under MIT license.
