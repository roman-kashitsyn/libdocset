#ifndef DOCSET_STRING_BUF_H
#define DOCSET_STRING_BUF_H

#include <stddef.h>

typedef struct {
    char * data;
    size_t size;
    size_t capacity;
} DocSetStringBuf;

int  docset_sb_init(DocSetStringBuf *buf,
                    size_t capacity);

void docset_sb_destroy(DocSetStringBuf *buf);

int  docset_sb_assign(DocSetStringBuf *buf,
                      const char      *data,
                      size_t           len);

int  docset_sb_reserve(DocSetStringBuf *buf,
                       size_t len);

char *docset_sb_new_string(DocSetStringBuf *buf);

char *docset_strdup(const char *str);

#endif
