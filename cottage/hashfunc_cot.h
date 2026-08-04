#ifndef HASHFUNC_COT
#define HASHFUNC_COT

#include "dependencies_cot.h"

//will use djb2 hashing algorithm
size_t stringHash(char* key) {
    unsigned int hash = 5381; //magic number
    int c;
    
    //printf("key is %s\n", key);

    //printf("begin hash\n");
    while ((c = *key++)) //for each character in the string
        hash = ((hash << 5) + hash) + c; //hash * 33 + c

    //printf("hash got: %u\n", hash);
    return hash;
}

#endif