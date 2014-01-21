#include "docset.h"

#include <stdlib.h>
#include <stdio.h>

void usage(const char *progname) {
    fprintf(stderr, "%s: QUERY DOCSET_PATH\n", progname);
    exit(1);
}

int main(int argc, const char *argv[])
{
    DocSet * docset;
    DocSetSearch * search;

    if (argc != 3) {
        usage(argv[0]);
    }

    docset = docset_open(argv[2]);
    if (!docset) {
        fprintf(stderr, "Unable to create a docset\n");
        return 1;
    }

    search = docset_search(docset, argv[1]);
    if (!search) {
        fprintf(stderr, "Unable to run a search\n");
        docset_close(docset);
        return 1;
    }

    while (docset_search_has_more(search)) {
        printf("%-30s (%s)[%s] at %s\n",
               docset_search_entry_name(search),
               docset_search_entry_type_name(search),
               docset_search_entry_canonical_type(search),
               docset_search_entry_path(search)
               );
        docset_search_advance(search);
    }

    docset_dispose_search(search);
    docset_close(docset);

    return 0;
}
