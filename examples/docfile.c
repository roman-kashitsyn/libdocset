#include "docset.h"
#include <stdio.h>

#define FIELD_FORMAT "%-16s %s\n"

static void usage(const char *progname)
{
    fprintf(stderr, "%s: DOCSET_DIR\n", progname);
}

static const char *is_set(DocSet     *docset,
                          DocSetFlags flag)
{
    return docset_flags(docset) & flag ? "yes" : "no";
}

int main(int argc, const char* argv[])
{
    const char *basedir;
    DocSet     *docset;

    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }

    basedir = argv[1];
    docset = docset_open(basedir);

    if (!docset) {
        fprintf(stderr,
                "Unable to open docset at %s\n",
                basedir);
        return 1;
    }

    printf(FIELD_FORMAT, "Bundle Id:",
           docset_bundle_identifier(docset));
    printf(FIELD_FORMAT, "Name:",
           docset_name(docset));
    printf(FIELD_FORMAT, "Platform Family:",
           docset_platform_family(docset));
    printf(FIELD_FORMAT, "Kind:",
           docset_kind_name(docset_kind(docset)));
    printf(FIELD_FORMAT, "Dash Docset?",
           is_set(docset, DOCSET_IS_DASH));
    printf(FIELD_FORMAT, "JS Enabled?",
           is_set(docset, DOCSET_IS_JS_ENABLED));

    docset_close(docset);

    return 0;
}
