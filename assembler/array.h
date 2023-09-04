#ifndef H_ARRAY
#define H_ARRAY

#include <stdbool.h>
#include <assert.h>

typedef struct {
    void **data;
    s64 allocated;
    s64 count;
} Array;

Array array_create(size_t start_size)
{
    Array array = {0};
    array.data = (void *)malloc(sizeof(void *) * start_size);
    array.allocated = start_size;
    array.count = 0;

    return array;
}

void array_add(Array* array, void* item)
{
    if (array->count >= array->allocated) {
        s64 reserve = 2 * array->allocated;
        if (reserve < 8) reserve = 8;

        array->data = realloc(array->data, sizeof(void *) * reserve);
        array->allocated = reserve;
    }

    array->data[array->count] = item;
    array->count += 1;
}

void *array_last_item(Array *array)
{
    if (array->count == 0) return NULL;

    return array->data[array->count-1];
}

void *array_pop(Array *array)
{
    if (array->count == 0) return NULL;

    void *r = array->data+(array->count-1);
    array->count -= 1;

    return (void *)r;
}

#endif