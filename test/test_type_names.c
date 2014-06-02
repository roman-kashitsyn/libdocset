#include "docset.h"
#include <stdio.h>

int main()
{
    int t;
    DocSetEntryType etype, ftype;
    const char *name;

    for (t = DOCSET_TYPE_UNKNOWN; t <= DOCSET_TYPE_LAST; ++t) {
        etype = (DocSetEntryType)t;
        name = docset_canonical_type_name(etype);
        ftype = docset_type_by_name(name);
        if (ftype != etype) {
            fprintf(stderr,
                    "Found type %s not equal to expected %s",
                    docset_canonical_type_name(etype),
                    docset_canonical_type_name(ftype));
            return 1;
        }
    }

    return 0;
}
