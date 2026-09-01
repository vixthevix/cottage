/*
I think I will have cottage do things for you, behind the scenes, by default.
At least, for now. I can always lock these things behind a define key.

What should cottage do for you?
    manage the routemap
    manage the stylesheet map
    manage that assets map (images, video, sound etc.)
put these into a separate file called "manager_cot.h"

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

#include "dependencies_cot.h"
#include "routemap_cot.h"
#include "stringmap_cot.h"
#include "init_cot.h"

/*
    ROUTEMAP
*/
RouteMap* globalRoutes;// = RouteMapInit();
#define GLOBALROUTES globalRoutes

bool newRoute(char* path, RouteEntry route) {
    return RouteMapInsert(&globalRoutes, path, route);
}
RouteEntry getRoute(char* path) {
    return RouteMapGet(globalRoutes, path);
}

bool cottageInit() {
    if (!cottageInitialised) {
        cottageInitialised = true;
        globalRoutes = RouteMapInit();
        if (!globalRoutes) cottageInitialised = false;
        return cottageInitialised;
    }
    return false;
}


/*
An alternative idea with style and asset storage.
Do what NextJS does, and have a public folder.
if, in the html, there is an href for "mystyle.css", 
cottage will actually look inside "public/mystyle.css".
this adds security, as we can strip all "." and ".." from the request resource
and it will ensure everything is looked at in public.

one issue, what if a file in public has the same name as a route?
right now, the only solution I have is to enforce that every asset
has a file extension, and that every route does not.
this can easily be done with a "validate routes and assets" function,
but yeah.

i guess another way is to have a rule of priority: route checks happen first.
this is the standard, and it would mean less boilerplate in the html and c code.

*/

/*
    ASSET STORAGE FOLDER
    its just a string really
*/

const char* globalAssetFolder = "./assets/";

char* prependAssetFolder(char* path) {
    cottageCheck(NULL);
    char* newPath = (char*) calloc(strlen(globalAssetFolder) + strlen(path) + 1, sizeof(char));
    if (newPath) {
        strcpy(newPath, globalAssetFolder);
        strcat(newPath, path);
    }
    return newPath;
}

//we can also make a cleanup path function

char* cleanupPath(char* path) {
    cottageCheck(NULL);
    if (!path) return NULL;
    //we have to read the string until the next '/' character
    //if equal to . or .., remove it
    char* newPath = (char*) calloc(strlen(path) + 1, sizeof(char));
    char* buffer = (char*) calloc(strlen(path) + 1, sizeof(char)); 
    
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
    free(buffer);
    return newPath;
}


/*
    STYLEMAP
*/

// stringMap* globalStyles = strMapInit();

// int newStyle(char* path, char* name) {
//     return strMapInsert(&globalStyles, name, path);
// }
// char* getStyle(char* key) {
//     return strMapGet(globalStyles, key);
// }

// /*
//     ASSETMAP
// */

// stringMap* globalAssets = strMapInit();

// int newAsset(char* path, char* name) {
//     return strMapInsert(&globalAssets, name, path);
// }
// char* getAsset(char* key) {
//     return strMapGet(globalAssets, key);
// }

#endif
