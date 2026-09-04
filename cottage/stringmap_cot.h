#ifndef STRINGMAP_COT
#define STRINGMAP_COT

#include "dependencies_cot.h"
#include "hashfunc_cot.h"
#include "init_cot.h"

typedef struct stringPair {
    char* key;
    char* value;
    int pd; //used in round robin hashing
} stringPair;

typedef struct stringMap {
    stringPair** items;
    unsigned int count;
    unsigned int capacity;
} stringMap;


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

void strPairFree(stringPair* query) {
    cottageCheck();
    if (query->key) free(query->key);
    if (query->value) free(query->value);
    query->pd = -1;
    free(query);
}


stringMap* strMapNewSize(const unsigned int oldSize) {
    cottageCheck(NULL);
    const unsigned int newSize = oldSize << 1; //doubled
    stringMap* newmap = (stringMap*)malloc(sizeof(stringMap));
    newmap->count = 0;
    newmap->capacity = newSize;
    newmap->items = (stringPair**) calloc(newSize, sizeof(stringPair*));

    return newmap;
}

stringMap* strMapInit() {
    cottageCheck(NULL);
    const unsigned int initSize = 8 >> 1; //start with size of 8
    return strMapNewSize(initSize);
}

bool strMapFree(stringMap* strMap) {
    cottageCheck(false);
    //free every query
    for (unsigned int i = 0; i < strMap->capacity; i++) {
        if (strMap->items[i]) strPairFree(strMap->items[i]);
    }
    free(strMap->items);
    free(strMap);
    
    return false;
}

bool strMapInsert(stringMap** strMap, char* key, char* value);


stringMap* strMapResize(stringMap* strMap) { //currently only resize upwards, since deleting items isnt in the current scope
    cottageCheck(NULL);
    stringMap* newmap = strMapNewSize(strMap->capacity);

    newmap->count = strMap->count;

    for (unsigned int i = 0; i < strMap->capacity; i++) {
        if (strMap->items[i]) { 
            char* newkey = (char*) malloc(strlen(strMap->items[i]->key));
            strcpy(newkey, strMap->items[i]->key);
            
            char* newval = (char*) malloc(strlen(strMap->items[i]->value));
            strcpy(newval, strMap->items[i]->value);

            strMapInsert(&newmap, newkey, newval);
    
        }
    }
    strMapFree(strMap);

    return newmap;
}


bool strMapInsert(stringMap** strMap, char* key, char* value) {
    cottageCheck(false);
    if (!key || !value || !strMap) return 1;
    
    const unsigned int load = (*strMap)->count * 100 / (*strMap)->capacity;
    if (load > 60) {
        (*strMap) = strMapResize((*strMap));
    }
    
    stringPair* newpair = strPairInit(key, value);
    unsigned int initpos = stringHash(key) % (*strMap)->capacity;
    unsigned int index;
    stringPair* curpair;

    for (unsigned int i = 0; i < (*strMap)->capacity; i++) {
        index = (initpos + i) % (*strMap)->capacity;
        curpair = (*strMap)->items[index];

        if (curpair == NULL) { //empty
            (*strMap)->items[index] = newpair;
            (*strMap)->count++;
            return true;
        }

        if (strcmp(curpair->key, key) == 0) {//value with same key so replace
            strPairFree((*strMap)->items[index]);
            (*strMap)->items[index] = newpair;
            return true;
        }

        if (newpair->pd > curpair->pd) { //round robin
            (*strMap)->items[index] = newpair;
            newpair = curpair;
        }

        newpair->pd++;


    }
    
    //in the case things do go wrong
    strPairFree(newpair);
    return false;

}

char* strMapGet(stringMap* strMap, char* key) {
    cottageCheck(NULL);

    unsigned int initpos = stringHash(key) % strMap->capacity;
    
    unsigned int index;
    stringPair* curpair;
    int curpd = 0;
    
    for (unsigned int i = 0; i < strMap->capacity; i++) {
        index = (initpos + i) % strMap->capacity;
        curpair = strMap->items[index];

        if (curpair == NULL || curpair->pd < curpd) return NULL;
        
        if (strcmp(curpair->key, key) == 0) return curpair->value;

        curpd++;
    }

    return NULL;
}

//combining two stringMaps
stringMap* strMapCombine(stringMap* intruder, stringMap* home) {
    cottageCheck(NULL);

    //we create the new map first, then populate with the right data if only intruder or home is valid. this is to ensure unique pointers
    stringMap* new = strMapInit();

    if (!home && !intruder) {
        strMapFree(new);
        return NULL;
    }
    
    if (!intruder && home) {
        for (int i = 0; i < home->capacity; i++) {
            stringPair* cur = home->items[i];
            if (!cur) continue;
            //will create new strings, so two new pointers. no worry of deletion
            strMapInsert(&new, cur->key, cur->value);
        }
        return new;
    }
    if (!home && intruder) {
        for (int i = 0; i < intruder->capacity; i++) {
            stringPair* cur = intruder->items[i];
            if (!cur) continue;

            strMapInsert(&new, cur->key, cur->value);
        }
        return new;
    }

    //strMapInsert works off existing pointers, so we have to make new ones here.
    //we insert the intruder into home
    //using a new strMap

    //stringMap* new = strMapInit();

    //first we populate new with values from home
    for (int i = 0; i < home->capacity; i++) {
        stringPair* cur = home->items[i];
        if (!cur) continue;
        //will create new strings, so two new pointers. no worry of deletion
        strMapInsert(&new, cur->key, cur->value);
    }

    //then we insert the intruder into home
    for (int i = 0; i < intruder->capacity; i++) {
        stringPair* cur = intruder->items[i];
        if (!cur) continue;

        strMapInsert(&new, cur->key, cur->value);
    }

    //finally we free both original maps
    //or do we? for now no lets give the programmer some control here

    return new;
}

#endif