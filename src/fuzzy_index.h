#ifndef DOCSET_FUZZY_INDEX_H
#define DOCSET_FUZZY_INDEX_H

#include "vector.h"

#define DOCSET_FUZZY_MIN_WORD 3

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char key[DOCSET_FUZZY_MIN_WORD];
    DocSetVector ids;
} DocSetIndexEntry;

/**
 * The index structure. The structure is effectively an open
 * hash-table with a vector of word id's in each bucket. Keys are
 * integers composed from 3-grams.
 */
typedef struct {
    int capacity;
    int size;
    DocSetIndexEntry * buckets;
} DocSetFuzzyIndex;

/**
 * @brief Creates new fuzzy search index.
 */
DocSetFuzzyIndex * docset_fuzzy_index_create();

/**
 * @brief Adds a word to a fuzzy search index.
 *
 * @param index the fuzzy index
 * @param word_id unique word identifier
 * @param word the word to add ( only ascii is supported :( )
 * @param len  len of the word in bytes
 */
int docset_fuzzy_index_add_word(DocSetFuzzyIndex * index,
                                long word_id,
                                const char * word,
                                int len);

/**
 * @brief Runs search and contructs vector with found identifiers.
 *
 * @param index the fuzzy index
 * @param input the string to search
 * @param limit max results number
 */
DocSetVector docset_run_fuzzy_search(DocSetFuzzyIndex * index,
                                     const char * input,
                                     unsigned limit);

#ifdef __cplusplus
}
#endif

#endif
