#include "docset.h"

#include <stdlib.h>
#include <stdio.h>

static void usage(const char *progname)
{
    fprintf(stderr, "%s: QUERY DOCSET_PATH\n", progname);
    exit(1);
}

static void print_error(void * ctx, const char *msg)
{
    fprintf(stderr, "%s", msg);
}

int main(int argc, const char *argv[])
{
    DocSet * docset;
    DocSetCursor * cursor;
    const DocSetEntry * entry;

    if (argc != 3) {
        usage(argv[0]);
    }

    docset = docset_open(argv[2]);
    if (!docset) {
        fprintf(stderr, "Unable to create a docset\n");
        return 1;
    }

    docset_set_error_handler(docset, print_error, NULL);

    cursor = docset_find(docset, argv[1]);
    if (!cursor) {
        fprintf(stderr, "Unable to create a cursor\n");
        docset_close(docset);
        return 1;
    }

    while (docset_cursor_step(cursor)) {
        entry = docset_cursor_entry(cursor);
        printf("%-30s (%s)[%s] at %s\n",
               docset_entry_name(entry),
               docset_entry_type_name(entry),
               docset_entry_canonical_type(entry),
               docset_entry_path(entry)
               );
    }

    docset_cursor_dispose(cursor);
    docset_close(docset);

    return 0;
}
