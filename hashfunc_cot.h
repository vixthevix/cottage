/*
Hashing function used for hashmaps.

Code is part of the cottage framework (https://github.com/vixthevix/cottage)
*/

#ifndef HASHFUNC_COT
#define HASHFUNC_COT

#include "dependencies_cot.h"
#include "init_cot.h"

// Function prototypes
uint32_t stringHash(char* key);

#if defined(COTTAGE_START)

/*
Hash function, using djb2 algorithm.
@arg key -> string key to hash.
@return hash value.
*/
uint32_t stringHash(char* key) {
    cottageCheck(0);
    uint32_t hash = 5381; //magic number
    int c;

    while ((c = *key++)) //for each character in the string
        hash = ((hash << 5) + hash) + c; //hash * 33 + c

    return hash;
}

#endif
#endif