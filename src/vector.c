#include "vector.h"

#include <stdlib.h>
#include <string.h>

static int realloc_vector(DocSetVector * vec);

int docset_vector_init(DocSetVector * vec,
                       unsigned long capacity)
{
    elem_type * data = calloc(capacity, sizeof(*data));

    if (!data) return 0;

    vec->size = 0;
    vec->capacity = capacity;
    vec->data = data;
    return 1;
}

int docset_vector_push(DocSetVector * vec,
                       elem_type item)
{
    if (vec->capacity <= vec->size) {
        if (!realloc_vector(vec)) return 0;
    }
    vec->data[vec->size++] = item;
    return 1;
}

void docset_vector_destroy(DocSetVector * vec)
{
    free(vec->data);
    memset(vec, 0, sizeof(*vec));
}


static int realloc_vector(DocSetVector * vec)
{
    unsigned long new_capacity = vec->capacity * 2;
    elem_type * new_data;

    if (new_capacity < vec->capacity) return 1;

    new_data = realloc(vec->data, new_capacity);

    if (new_data == NULL) return 1;

    vec->capacity = new_capacity;
    vec->data = new_data;
    return 0;
}
