/*
This file will produce a helper script, for making route functions
You pass in the name of the route, and it will generate a route table for you,
under the routes folder
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

const char* format = 
"#define COTTAGE_START\n"
"#include \"../cottage/cottage.h\"\n"
"\n"
"NewRouteFunction(%1$sGet) {\n"
"   return false;\n"
"}\n"
"NewRouteFunction(%1$sPost) {\n"
"   return false;\n"
"}\n"
"NewRouteFunction(%1$sPut) {\n"
"   return false;\n"
"}\n"
"NewRouteFunction(%1$sDelete) {\n"
"   return false;\n"
"}\n"
"\n"

"/*\n"
"Enter the following into your main code, under where you setup your routes:"
"\n"
"RouteEntry %1$s = {\n"
"    .routeGet = %1$sGet,\n"
"    .routePost = %1$sPost,\n"
"    .routePut = %1$sPut,\n"
"    .routeDelete = %1$sDelete\n"
"};\n"
"\n"
"\n"
"newRoute(YOUR SITE PATH HERE, %1$s);\n"
"*/\n"
;
const int routeCount = 4;

int main(int argc, char** argv) {
    if (argc <= 1) {
        printf("Please enter the name of the route. Thank you\n");
        return 1;
    }
    char* name = argv[1];
    char* contents = (char*) calloc(strlen(format) + ((strlen(name) * routeCount)) + 10, sizeof(char));
    sprintf(contents, format, name);

    printf("%s\n", contents);

    //make the routes folder
    int result = mkdir("routes", 0777);
    if (result != 0) {
        printf("Could not create routes folder (may already exist).\n");
    }

    //print this into a file
    char* filename = (char*) calloc(strlen("routes/") + strlen(name) + strlen("_routes.h") + 1, sizeof(char));
    sprintf(filename, "routes/%s_routes.h", name);
    FILE* file = fopen(filename, "w");
    if (!file) {
        printf("%s does not exist\n", filename);
        goto end;
    }
    
    fprintf(file, contents);
    fclose(file);

    end:
    free(filename);
    free(contents);
    return 0;
}