#ifndef DOCSET_PROP_PARSER_H
#define DOCSET_PROP_PARSER_H

typedef enum {
    DOCSET_PROP_STRING,
    DOCSET_PROP_BOOL
} DocSetPropType;

typedef struct {
    DocSetPropType type;
    const char    *name;
    union {
        char **str_target;
        int   *bool_target;
    } target;
} DocSetProp;

int docset_parse_properties(const char *path,
                            DocSetProp *begin,
                            DocSetProp *end);

#endif
