/*
Definitions for a default string-to-string hashmap.

Code is part of the cottage framework (https://github.com/vixthevix/cottage)
*/

#ifndef STRINGMAP_COT
#define STRINGMAP_COT

#include "dependencies_cot.h"
#include "hashfunc_cot.h"
#include "init_cot.h"

/*
Base pair to be stored in a stringMap.
*/
typedef struct stringPair {
    char* key;
    char* value;
    int pd; //for round robin hashing
} stringPair;

/*
Map that stores stringPairs
*/
typedef struct stringMap {
    stringPair** items;
    unsigned int count;
    unsigned int capacity;
} stringMap;

// Function prototypes
stringPair* strPairInit(char* key, char* value);
void strPairFree(stringPair* pair);
stringMap* strMapNewSize(const unsigned int oldSize);
stringMap* strMapInit(void);
void strMapFree(stringMap* map);
stringMap* strMapResize(stringMap* strMap);
bool strMapInsert(stringMap** map, char* key, char* value);
char* strMapGet(stringMap* map, char* key);

#if defined(COTTAGE_START)

/*
Initialises a basic stringPair.
@arg key -> string key of map.
@arg value -> string value associated with key.
@return dynamically allocated pairing.
*/
stringPair* strPairInit(char* key, char* value) {
    cottageCheck(NULL);
    stringPair* newpair = (stringPair*) calloc(1, sizeof(stringPair));
    //stringPair consists of a copy of key and value
    newpair->key = (char*) calloc(strlen(key) + 1, sizeof(char));
    newpair->value = (char*) calloc(strlen(value) + 1, sizeof(char));
    strcpy(newpair->key, key);
    strcpy(newpair->value, value);
    newpair->pd = 0;
    return newpair;
}

/*
Frees a stringPair from memory.
@arg pair -> target to free.
*/
void strPairFree(stringPair* pair) {
    cottageCheck();
    if (pair->key) free(pair->key);
    if (pair->value) free(pair->value);
    free(pair);
    pair = NULL;
}

/*
Creates a stringMap given an initial size. Used in Init and Resize.
@arg oldSize -> initial size seed. In Resize, it is the capacity of the old map.
@return dynamically created stringMap. 
*/
stringMap* strMapNewSize(const unsigned int oldSize) {
    cottageCheck(NULL);
    const unsigned int newSize = oldSize << 1; //doubled
    stringMap* newmap = (stringMap*)malloc(sizeof(stringMap));
    newmap->count = 0;
    newmap->capacity = newSize;
    newmap->items = (stringPair**) calloc(newSize, sizeof(stringPair*));

    return newmap;
}

/*
Initialises a base stringMap.
@return dynamically created stringMap. 
*/
stringMap* strMapInit() {
    cottageCheck(NULL);
    const unsigned int initSize = 8 >> 1; //start with size of 8
    return strMapNewSize(initSize);
}

/*
Frees a stringMap from memory.
@arg map -> target to free.
*/
void strMapFree(stringMap* map) {
    cottageCheck();
    //Free each pair in the map.
    for (unsigned int i = 0; i < map->capacity; i++) {
        if (map->items[i]) strPairFree(map->items[i]);
    }
    free(map->items);
    free(map);
    
}

bool strMapInsert(stringMap** strMap, char* key, char* value);

/*
Creates a new bigger stringMap from a base map.
@arg map -> base map for new stringMap.
@return new bigger stringMap.
*/
stringMap* strMapResize(stringMap* strMap) {
    cottageCheck(NULL);
    stringMap* newmap = strMapNewSize(strMap->capacity);
    newmap->count = strMap->count;

    //Reinsert values from old map
    for (unsigned int i = 0; i < strMap->capacity; i++) {
        if (strMap->items[i]) { 
            strMapInsert(&newmap, strMap->items[i]->key, strMap->items[i]->value);
        }
    }
    
    strMapFree(strMap);

    return newmap;
}

/*
Inserts a new entry into a stringMap.
@arg map -> pointer to target stringMap to insert into.
@arg key -> string key.
@arg value -> string value associated with key.
@return status of insert.
*/
bool strMapInsert(stringMap** map, char* key, char* value) {
    cottageCheck(false);
    if (!key || !value || !map) return false;
    
    const unsigned int load = (*map)->count * 100 / (*map)->capacity;
    if (load > 60) {
        (*map) = strMapResize((*map));
    }
    
    stringPair* newpair = strPairInit(key, value);
    unsigned int initpos = stringHash(key) % (*map)->capacity;
    unsigned int index;
    stringPair* curpair;

    for (unsigned int i = 0; i < (*map)->capacity; i++) {
        index = (initpos + i) % (*map)->capacity;
        curpair = (*map)->items[index];

        if (curpair == NULL) { //empty
            (*map)->items[index] = newpair;
            (*map)->count++;
            return true;
        }

        if (strcmp(curpair->key, key) == 0) {//value with same key so replace
            strPairFree((*map)->items[index]);
            (*map)->items[index] = newpair;
            return true;
        }

        if (newpair->pd > curpair->pd) { //round robin
            (*map)->items[index] = newpair;
            newpair = curpair;
        }

        newpair->pd++;


    }
    
    //in the case things do go wrong
    strPairFree(newpair);
    return false;

}

/*
Gets a value associted with a key in a stringMap.
@arg map -> stringMap to search.
@arg key -> string key.
@return value associated with key.
*/
char* strMapGet(stringMap* map, char* key) {
    cottageCheck(NULL);

    unsigned int initpos = stringHash(key) % map->capacity;
    
    unsigned int index;
    stringPair* curpair;
    int curpd = 0;
    
    for (unsigned int i = 0; i < map->capacity; i++) {
        index = (initpos + i) % map->capacity;
        curpair = map->items[index];

        if (curpair == NULL || curpair->pd < curpd) return NULL;
        
        if (strcmp(curpair->key, key) == 0) return curpair->value;

        curpd++;
    }

    return NULL;
}

/*
Unused stringMap combine function.
*/
// stringMap* strMapCombine(stringMap* intruder, stringMap* home) {
//     cottageCheck(NULL);

//     //we create the new map first, then populate with the right data if only intruder or home is valid. this is to ensure unique pointers
//     stringMap* new = strMapInit();

//     if (!home && !intruder) {
//         strMapFree(new);
//         return NULL;
//     }
    
//     if (!intruder && home) {
//         for (int i = 0; i < home->capacity; i++) {
//             stringPair* cur = home->items[i];
//             if (!cur) continue;
//             //will create new strings, so two new pointers. no worry of deletion
//             strMapInsert(&new, cur->key, cur->value);
//         }
//         return new;
//     }
//     if (!home && intruder) {
//         for (int i = 0; i < intruder->capacity; i++) {
//             stringPair* cur = intruder->items[i];
//             if (!cur) continue;

//             strMapInsert(&new, cur->key, cur->value);
//         }
//         return new;
//     }

//     //strMapInsert works off existing pointers, so we have to make new ones here.
//     //we insert the intruder into home
//     //using a new strMap

//     //stringMap* new = strMapInit();

//     //first we populate new with values from home
//     for (int i = 0; i < home->capacity; i++) {
//         stringPair* cur = home->items[i];
//         if (!cur) continue;
//         //will create new strings, so two new pointers. no worry of deletion
//         strMapInsert(&new, cur->key, cur->value);
//     }

//     //then we insert the intruder into home
//     for (int i = 0; i < intruder->capacity; i++) {
//         stringPair* cur = intruder->items[i];
//         if (!cur) continue;

//         strMapInsert(&new, cur->key, cur->value);
//     }

//     //finally we free both original maps
//     //or do we? for now no lets give the programmer some control here

//     return new;
// }

#endif
#endif