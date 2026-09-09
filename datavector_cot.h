/*
Code for creating dynamically managed arrays.

Code is part of the cottage framework (https://github.com/vixthevix/cottage)
*/

#ifndef DATAVECTOR_COT
#define DATAVECTOR_COT

#include "init_cot.h"
#include "error_cot.h"
#include "dependencies_cot.h"

/*
Struct for dynamically storing and resizing a character array.
*/
typedef struct dataVector {
    char* data;
    uint32_t capacity;
    uint32_t index;
} dataVector;

// Function prototypes
dataVector dataVectorInit(uint32_t capacity);
bool dataVectorPush(dataVector* target, char c);
bool dataVectorPushString(dataVector* target, const char* s);

#if defined(COTTAGE_START)

/*
Initialises a dataVector.
@arg capacity -> initial capacity to give.
@return new dataVector.
*/
dataVector dataVectorInit(uint32_t capacity) {
    if (capacity == 0) {
        return (dataVector){0, 0, 0};
    }
    
    dataVector target = {
        .data = (char*) calloc(capacity, sizeof(char)),
        .capacity = capacity,
        .index = 0,
    };

    return target;
}

/*
Pushes a character onto a dataVector.
@arg target -> dataVector to push character onto.
@arg c -> character to push.
@return status of push.
*/
bool dataVectorPush(dataVector* target, char c) {
    uint32_t load = target->index / target->capacity * 100;
    if (load > 60) {
        target->capacity <<= 1;
        target->data = (char*) realloc(target->data, target->capacity * sizeof(char));
        if (!target->data) return false;
    }

    target->data[target->index++] = c;
    return true;
}

/*
Pushes characters from string onto a dataVector.
@arg target -> dataVector to push characters onto.
@arg s -> string to push.
@return status of push
*/
bool dataVectorPushString(dataVector* target, const char* s) {
    if (!s) return false;
    size_t size = strlen(s);
    for (size_t i = 0; i < size; i++) {
        if (!dataVectorPush(target, s[i])) return false;
    }
    return true;
}

#endif
#endif