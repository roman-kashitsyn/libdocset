#include "stringbuf.h"
#include <stdlib.h>
#include <string.h>

int docset_sb_init(DocSetStringBuf *buf, size_t capacity)
{
    char * data = calloc(capacity, sizeof(*data));
    if (!data) return 0;

    buf->data = data;
    buf->size = 0;
    buf->capacity = capacity;
    return 1;
}

void docset_sb_destroy(DocSetStringBuf *buf)
{
    if (buf) {
        free(buf->data);
        memset(buf, 0, sizeof(*buf));
    }
}

int docset_sb_assign(DocSetStringBuf *buf, const char *data, size_t len)
{
    size_t req_cap = len + 1;
    if (buf->capacity < req_cap) {
        size_t capx2 = buf->capacity * 2;
        size_t new_cap = capx2 > req_cap ? capx2 : req_cap;
        char * n = realloc(buf->data, new_cap);
        if (!n) return 0;
        buf->capacity = new_cap;
        buf->data = n;
    }
    memcpy(buf->data, data, len);
    buf->data[len] = '\0';
    buf->size = len;
    return 1;
}
/*
DocSetStringRef docset_sr_copy(const char *data)
{
    return docset_sr_copy_n(data, strlen(data));
}

DocSetStringRef docset_sr_copy_n(const char *data, unsigned size)
{
    char * ref = malloc(size);
    memcpy(ref, data, size);
    return {ref, size};
}

void docset_sr_free(DocSetStringRef *ref)
{
    free((void*)ref->data);
}



*/
