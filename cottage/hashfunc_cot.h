#ifndef HASHFUNC_COT
#define HASHFUNC_COT

#include "dependencies_cot.h"
#include "init_cot.h"

//will use djb2 hashing algorithm
size_t stringHash(char* key) {
    cottageCheck(0);
    unsigned int hash = 5381; //magic number
    int c;

    while ((c = *key++)) //for each character in the string
        hash = ((hash << 5) + hash) + c; //hash * 33 + c

    return hash;
}

#endif