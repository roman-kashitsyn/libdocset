/**
 * @file
 *
 * This file provides auxiliary types and functions to work with
 * variable-length string buffers.
 *
 * This file is part of the docset library implementation and is not a
 * public API.
 *
 */
#ifndef DOCSET_STRING_BUF_H
#define DOCSET_STRING_BUF_H

#include <stddef.h>

typedef struct {
    char * data;
    size_t size;
    size_t capacity;
} DocSetStringBuf;


/**
 * @brief Initializes a string buffer.
 *
 * After the initialization the buffer data pointer is either points
 * to a storage of given capacity or is null.
 */
int
docset_sb_init(DocSetStringBuf *buf,
               size_t           capacity);

/**
 * @brief Deallocates the memory owned by a buffer.
 * The buffer data pointer is allowed to be NULL.
 *
 * The operation is idempotent. Deallocation of previously deallocated
 * buffer is no-op.
 */
void
docset_sb_destroy(DocSetStringBuf *buf);

/**
 * @brief Assigns a string data of size len to a buffer.
 */
int
docset_sb_assign(DocSetStringBuf *buf,
                 const char      *data,
                 size_t           len);

/**
 * @brief Ensures that a buffer could place at least len bytes in
 * total.
 */
int
docset_sb_reserve(DocSetStringBuf *buf,
                  size_t len);

/**
 * @brief Allocates a new string equal to buffer data.
 * Returns NULL if memory could not be allocated.
 */
char *
docset_sb_new_string(DocSetStringBuf *buf);

#endif
