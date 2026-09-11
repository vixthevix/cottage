/*
Functions and data for making cottage easier to use.

Code is part of the cottage framework (https://github.com/vixthevix/cottage)
*/

#ifndef MANAGER_COT 
#define MANAGER_COT

// #include "tcpsetup_cot.h"
// #include "stringmap_cot.h"
// #include "sitevar_cot.h"
// #include "boolcalc_cot.h"
// #include "fopen_cot.h"
// #include "error_cot.h"
// #include "hashfunc_cot.h"
// #include "dependencies_cot.h"
// #include "conversion_cot.h"
// #include "httpsplit_cot.h"

#include "init_cot.h"
#include "dependencies_cot.h"
#include "routemap_cot.h"
#include "stringmap_cot.h"

// Function Prototypes
bool newRoute(char* path, RouteEntry route);
RouteEntry getRoute(char* path);
bool cottageInit(void);
char* prependAssetFolder(char* path);
char* cleanupPath(char* path);

#if defined(COTTAGE_START)

//Global default RouteMap 
RouteMap* globalRoutes;// = RouteMapInit();
#define GLOBALROUTES globalRoutes

/*
Adds a new route to the global RouteMap.
@arg path -> name of route.
@arg route -> encapsulated route functions.
@return status of insert.
*/
bool newRoute(char* path, RouteEntry route) {
    return RouteMapInsert(&globalRoutes, path, route);
}

/*
Gets a route from the global RouteMap.
@arg path -> name of route.
@return route functions associated with path.
*/
RouteEntry getRoute(char* path) {
    return RouteMapGet(globalRoutes, path);
}

/*
Initialises cottage to work on all cottage-compatible platforms succesfully.
@return status of init.
*/
bool cottageInit() {
    if (!cottageInitialised) {
        cottageInitialised = true;
        globalRoutes = RouteMapInit();
        if (!globalRoutes) cottageInitialised = false;
        return cottageInitialised;
    }
    return false;
}

//Starting path of assets to be used with cottage.
const char* globalAssetFolder = "./assets/";

/*
Prepends the globalAssetFolder string to a given path string.
@arg path -> path string.
@return new prepended path string. 
*/
char* prependAssetFolder(char* path) {
    cottageCheck(NULL);
    char* newPath = (char*) calloc(strlen(globalAssetFolder) + strlen(path) + 1, sizeof(char));
    if (newPath) {
        strcpy(newPath, globalAssetFolder);
        strcat(newPath, path);
    }
    return newPath;
}

/*
Makes a path safe to read a file from.
@arg path -> unsafe path.
@return cleaned up safe path.
*/
char* cleanupPath(char* path) {
    cottageCheck(NULL);
    if (!path) return NULL;
    
    //Path to build
    char* newPath = (char*) calloc(strlen(path) + 1, sizeof(char));
    //Current part of path to analyze
    char* buffer = (char*) calloc(strlen(path) + 1, sizeof(char)); 
    
    if (!buffer || !newPath) return NULL;

    size_t j = 0;
    for (size_t i = 0; i < strlen(path); i++) {
        if (path[i] == '/') {
            buffer[j] = 0; //null terminator to close off buffer
            if (j > 0 && strcmp(buffer, ".") != 0 && strcmp(buffer, "..") != 0) {
                //write the contents of buffer to newPath
                if (buffer[0]) {
                    if (!newPath[0]) strcpy(newPath, buffer);
                    else strcat(newPath, buffer);
                }
                //also append a /
                //this may result in security vulnerabilities, so check later
                strcat(newPath, "/");
            }
            //reset j
            j = 0;
            continue;
        }

        buffer[j++] = path[i];
    }
    // we may have data left over in buffer. copy it over
    buffer[j] = 0;
    if (j > 0 && strcmp(buffer, ".") != 0 && strcmp(buffer, "..") != 0) {
        //write the contents of buffer to newPath
        if (buffer[0]) {
            if (!newPath[0]) strcpy(newPath, buffer);
            else strcat(newPath, buffer);
        }
    }
    if (buffer) free(buffer);
    return newPath;
}

#endif
#endif