#include "docset.h"

#include <stdlib.h>
#include <stdio.h>

static void usage(const char *progname)
{
    fprintf(stderr, "%s: QUERY DOCSET_PATH\n", progname);
}

static void print_error(void *ctx, const char *msg)
{
    fprintf(stderr, "%s: %s\n", (const char*)ctx, msg);
}

int main(int argc, const char *argv[])
{
    DocSet       *docset;
    DocSetCursor *cursor;
    DocSetEntry  *entry;
    const char   *query;
    const char  **path;
    const char  **end;

    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }

    query = argv[1];

    for (path = argv + 2, end = argv + argc;
         path != end;
         ++path) {

        docset = docset_open(*path);
        if (!docset) {
            fprintf(stderr,
                    "Unable to create a docset %s\n",
                    *path);
            continue;
        }

        docset_set_error_handler(docset, print_error, (void*)*path);

        cursor = docset_find(docset, query);
        if (!cursor) {
            fprintf(stderr,
                    "%s: Unable to create a cursor\n",
                    *path);
            docset_close(docset);
            continue;
        }

        while (docset_cursor_step(cursor)) {
            entry = docset_cursor_entry(cursor);
            printf("%10s: (%c) %-25s: %s\n",
                   docset_name(docset),
                   docset_entry_canonical_type(entry)[0],
                   docset_entry_name(entry),
                   docset_entry_path(entry)
                   );
        }

        docset_cursor_dispose(cursor);
        docset_close(docset);
    }

    return 0;
}
