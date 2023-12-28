#include <stdbool.h>
#include <stdio.h>

#ifndef URL_VECTOR_H
#define URL_VECTOR_H

#define MAX_SIZE 2500

typedef struct url_vector {
    char* urls[MAX_SIZE];
    int size;
} URL_VECTOR;

bool push(URL_VECTOR* p_vec, char* p_URL);
char* pop(URL_VECTOR* p_vec);

#endif // ULR_VECTOR_H

