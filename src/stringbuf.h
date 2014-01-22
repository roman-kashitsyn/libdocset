#ifndef DOCSET_STRING_BUF_H
#define DOCSET_STRING_BUF_H

#include <stddef.h>

/**
 * @brief Structure representing string chunk.
 */
typedef struct {
    const char * data;
    size_t size;
} DocSetStringRef;

typedef struct {
    char * data;
    size_t size;
    size_t capacity;
} DocSetStringBuf;

int docset_sb_init(DocSetStringBuf *buf, size_t capacity);

void docset_sb_destroy(DocSetStringBuf *buf);

int docset_sb_assign(DocSetStringBuf *buf, const char *data, size_t len);


void docset_sr_copy(DocSetStringRef *ref, const char *data);

void docset_sr_copy_n(DocSetStringRef *ref, const char *data, size_t len);

void docset_sr_free(DocSetStringRef *ref);

#endif
