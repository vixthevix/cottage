/*
Struct definitions for HTTP requests and routes.

Code is part of the cottage framework (https://github.com/vixthevix/cottage)
*/

#ifndef ROUTEFUNCTION_COT
#define ROUTEFUNCTION_COT

#include "sitevar_cot.h"
#include "stringmap_cot.h"
#include "dependencies_cot.h"
#include "init_cot.h"

/*
Enum for each valid HTTP request type.
*/
typedef enum HTTPTYPE {
    UNKNOWN = -1,
    GET,
    PUT,
    POST,
    DELETE,
    PATCH,
    HEAD,
    OPTIONS,
    TRACE,
    CONNECT,
} HTTPTYPE;

/*
Struct that encapsulates a HTTP request.
@param type -> HTTP request type.
@param target -> resource the request is acting upon.
@param version -> HTTP version number of the request.
@param options -> Labelled information about the request, in hashmap form.
@param payload -> Extra data attached to the end of the request.
*/
typedef struct HttpRequest {
    HTTPTYPE type;
    char* target;
    float version;
    stringMap* options;
    char* payload;
} HttpRequest;

/*
Debug display for a HTTP request.
@arg request -> request to display.
*/
void debugHttpRequest(HttpRequest request) {
    cottageCheck();
    fprintf(stderr,
        "REQUEST DEBUG\n"
        "TARGET:%s\n"
        "PAYLOAD:%s\n"
        "VERSION:%f\n"
        "TYPE:%i\n",
        request.target, request.payload, request.version, request.type
    );

}

/*
Function prototype for a route function, which is code that executes
depending on the request type acted upon a defined route.
@arg request -> the HTTP request to get information from.
@arg clientfd -> the client that sent the HTTP request.
@arg extraData -> external siteVars to be used.
@return status of route success.
*/
typedef bool (*RouteFunction)(HttpRequest request, int clientfd, siteVar* extraData);

/*
Macro for reducing boilerplate when writing a new RouteFunction definition.
*/
#define NewRouteFunction(functionName) bool functionName(HttpRequest request, int clientfd, siteVar* extraData)

/*
Struct that holds all of the possible route functions for a given route.
Should hold a RouteFunction for each individual HTTP request type.
(cottage v1 has only the main GET, POST, PUT and DELETE types. More to be added in the future.)
*/
typedef struct RouteEntry {
    RouteFunction routeGet;
    RouteFunction routePost;
    RouteFunction routePut;
    RouteFunction routeDelete;
} RouteEntry;

#endif