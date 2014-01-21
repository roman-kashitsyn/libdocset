#ifndef DOCSET_VECTOR_H
#define DOCSET_VECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef long elem_type;

typedef struct {
    unsigned long capacity;
    unsigned long size;
    elem_type * data;
} DocSetVector;

int docset_vector_init(DocSetVector *vec, unsigned long capacity);

int docset_vector_push(DocSetVector *vec, elem_type item);

void docset_vector_destroy(DocSetVector * vec);

#ifdef __cplusplus
}
#endif

#endif
