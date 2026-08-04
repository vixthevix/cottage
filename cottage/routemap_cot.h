#ifndef ROUTEMAP_COT
#define ROUTEMAP_COT

/*
A routeMap is a kind of mapping from a string link, to a special struct
containing function pointers for the different http requests.

in this way, the programmer can define what each link does for each request,
and if not defined, default behaviour can occur, using the handle functions


*/


#include "dependencies_cot.h"
#include "routefunction_cot.h"
#include "sitevar_cot.h"
#include "init_cot.h"


typedef struct RoutePair {
    char* key;
    RouteEntry route;
    int pd;
} RoutePair;

typedef struct RouteMap {
    RoutePair** items;
    size_t count;
    size_t capacity;
} RouteMap;

RoutePair* RoutePairInit(char* key, RouteEntry route) {
    cottageCheck(NULL);    
    RoutePair* newpair = (RoutePair*) malloc(sizeof(RoutePair));
    newpair->key = (char*) malloc(strlen(key) + 1);
    strcpy(newpair->key, key);
    newpair->route = route;
    newpair->pd = 0;

    return newpair;
}

void RoutePairFree(RoutePair* pair) {
    cottageCheck();
    if (pair->key) free(pair->key);
    free(pair);
    pair = NULL;
}

RouteMap* RouteMapNewSize(const size_t oldSize) {
    cottageCheck(NULL);
    const size_t newSize = oldSize << 1;
    RouteMap* newmap = (RouteMap*) malloc(sizeof(RouteMap));
    newmap->count = 0;
    newmap->capacity = newSize;
    newmap->items = (RoutePair**) calloc(newSize, sizeof(RoutePair*));

    return newmap;
}

RouteMap* RouteMapInit() {
    cottageCheck(NULL);
    const size_t initSize = 8 >> 1; //initial size of 8
    return RouteMapNewSize(initSize);
}

void RouteMapFree(RouteMap* map) {
    cottageCheck();
    for (size_t i = 0; i < map->capacity; i++) {
        if (map->items[i]) RoutePairFree(map->items[i]);
    }
    free(map->items);
    free(map);
    map = NULL;
}

bool RouteMapInsert(RouteMap* map, char* key, RouteEntry route);

RouteMap* RouteMapResize(RouteMap* map) {
    cottageCheck(NULL);
    RouteMap* newmap = RouteMapNewSize(map->capacity);
    newmap->count = map->count;

    for (size_t i = 0; i < map->capacity; i++) {
        if (map->items[i]) {
            RouteMapInsert(newmap, map->items[i]->key, map->items[i]->route);
        }
    }
    
    RouteMapFree(map);

    return newmap;
}

bool RouteMapInsert(RouteMap* map, char* key, RouteEntry route) {
    cottageCheck(false);
    if (!key || !map) return false;

    const size_t load = map->count * 100 / map->capacity;
    if (load > 60) map = RouteMapResize(map);

    RoutePair* newPair = RoutePairInit(key, route);
    size_t initpos = stringHash(key) % map->capacity;
    size_t index;
    RoutePair* curPair;

    for (size_t i = 0; i < map->capacity; i++) {
        index = (initpos + i) % map->capacity;
        curPair = map->items[index];

        if (curPair == NULL) {
            map->items[index] = newPair;
            map->count++;
            return true;
        }
        if (strcmp(curPair->key, key) == 0) {
            RoutePairFree(map->items[index]);
            map->items[index] = newPair;
            return true;
        }
        if (newPair->pd > curPair->pd) {
            map->items[index] = newPair;
            newPair = curPair;
        }

        newPair->pd++;
    }

    RoutePairFree(newPair);
    return false;
}

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

        if (curPair == NULL || curPair->pd < curpd) return error;
        if (strcmp(curPair->key, key) == 0) return curPair->route;

        curpd++;
    }

    return error;
}

#endif