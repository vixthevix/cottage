/*
Definitions for a hashmap that maps route names to RouteEntries. 

Code is part of the cottage framework (https://github.com/vixthevix/cottage)
*/

#ifndef ROUTEMAP_COT
#define ROUTEMAP_COT

#include "dependencies_cot.h"
#include "routefunction_cot.h"
#include "sitevar_cot.h"
#include "init_cot.h"

/*
Base pair to be stored in a RouteMap.
*/
typedef struct RoutePair {
    char* key;
    RouteEntry route;
    int pd; //for round-robin hashing.
} RoutePair;

/*
Map that stores RoutePairs.
*/
typedef struct RouteMap {
    RoutePair** items;
    size_t count;
    size_t capacity;
} RouteMap;

// Function prototypes
RoutePair* RoutePairInit(char* key, RouteEntry route);
void RoutePairFree(RoutePair* pair);
RouteMap* RouteMapNewSize(const size_t oldSize);
RouteMap* RouteMapInit(void);
void RouteMapFree(RouteMap* map);
RouteMap* RouteMapResize(RouteMap* map);
bool RouteMapInsert(RouteMap** map, char* key, RouteEntry route);
RouteEntry RouteMapGet(RouteMap* map, char* key);

#if defined(COTTAGE_START)

/*
Initialises a basic RoutePair.
@arg key -> string key of map.
@arg route -> routes corresponding to key.
@return dynamically allocated pairing.
*/
RoutePair* RoutePairInit(char* key, RouteEntry route) {
    cottageCheck(NULL);    
    RoutePair* newpair = (RoutePair*) malloc(sizeof(RoutePair));
    newpair->key = (char*) malloc(strlen(key) + 1);
    strcpy(newpair->key, key);
    newpair->route = route;
    newpair->pd = 0;

    return newpair;
}

/*
Frees a RoutePair from memory.
@arg pair -> target to free.
*/
void RoutePairFree(RoutePair* pair) {
    cottageCheck();
    if (pair->key) free(pair->key);
    free(pair);
    pair = NULL;
}

/*
Creates a RouteMap given an initial size. Used in Init and Resize.
@arg oldSize -> initial size seed. In Resize, it is the capacity of the old map.
@return dynamically created RouteMap. 
*/
RouteMap* RouteMapNewSize(const size_t oldSize) {
    cottageCheck(NULL);
    const size_t newSize = oldSize << 1; //doubled
    RouteMap* newmap = (RouteMap*) malloc(sizeof(RouteMap));
    newmap->count = 0;
    newmap->capacity = newSize;
    newmap->items = (RoutePair**) calloc(newSize, sizeof(RoutePair*));

    return newmap;
}

/*
Initialises a base RouteMap.
@return dynamically created RouteMap. 
*/
RouteMap* RouteMapInit() {
    cottageCheck(NULL);
    const size_t initSize = 8 >> 1; //initial size of 8
    return RouteMapNewSize(initSize);
}

/*
Frees a RouteMap from memory.
@arg map -> target to free.
*/
void RouteMapFree(RouteMap* map) {
    cottageCheck();
    if (!map) return;
    //Free each pair in the map.
    for (size_t i = 0; i < map->capacity; i++) {
        if (map->items[i]) RoutePairFree(map->items[i]);
    }
    if (map->items) free(map->items);
    if (map) free(map);
    map = NULL;
}

bool RouteMapInsert(RouteMap** map, char* key, RouteEntry route);

/*
Creates a new bigger RouteMap from a base map.
@arg map -> base map for new RouteMap.
@return new bigger RouteMap.
*/
RouteMap* RouteMapResize(RouteMap* map) {
    cottageCheck(NULL);
    RouteMap* newmap = RouteMapNewSize(map->capacity);
    newmap->count = map->count;

    //Reinsert values from old map
    for (size_t i = 0; i < map->capacity; i++) {
        if (map->items[i]) {
            RouteMapInsert(&newmap, map->items[i]->key, map->items[i]->route);
        }
    }
    
    RouteMapFree(map);

    return newmap;
}

/*
Inserts a new route definition into a RouteMap.
@arg map -> pointer to target RouteMap to insert into.
@arg key -> name of route.
@arg route -> route functions tied to the route.
@return status of insert.
*/
bool RouteMapInsert(RouteMap** map, char* key, RouteEntry route) {
    cottageCheck(false);
    if (!key || !map || !(*map)) return false;

    const size_t load = (*map)->count * 100 / (*map)->capacity;
    if (load > 60) *map = RouteMapResize(*map);

    RoutePair* newPair = RoutePairInit(key, route);
    size_t initpos = stringHash(key) % (*map)->capacity;
    size_t index;
    RoutePair* curPair;

    for (size_t i = 0; i < (*map)->capacity; i++) {
        index = (initpos + i) % (*map)->capacity;
        curPair = (*map)->items[index];

        if (curPair == NULL) {
            (*map)->items[index] = newPair;
            (*map)->count++;
            return true;
        }
        if (strcmp(curPair->key, key) == 0) {
            RoutePairFree((*map)->items[index]);
            (*map)->items[index] = newPair;
            return true;
        }

        //Round robin hashing implementation
        if (newPair->pd > curPair->pd) {
            (*map)->items[index] = newPair;
            newPair = curPair;
        }

        newPair->pd++;
    }

    RoutePairFree(newPair);
    return false;
}

/*
Gets the RouteEntry from a RouteMap.
@arg map -> RouteMap to search.
@arg key -> route name.
@return RouteEntry associated with key.
*/
RouteEntry RouteMapGet(RouteMap* map, char* key) {
    RouteEntry error;
    memset(&error, 0, sizeof(RouteEntry));
    cottageCheck(error);
    
    if (!map || !key) return error;

    size_t initpos = stringHash(key) % map->capacity;
    size_t index;
    RoutePair* curPair;
    int curpd = 0;

    for (size_t i = 0; i < map->capacity; i++) {
        index = (initpos + i) % map->capacity;
        curPair = map->items[index];

        //Round-robin check.
        if (curPair == NULL || curPair->pd < curpd) return error;
        //Normal check
        if (strcmp(curPair->key, key) == 0) return curPair->route;

        curpd++;
    }

    return error;
}

#endif
#endif