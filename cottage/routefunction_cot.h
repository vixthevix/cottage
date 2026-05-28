#ifndef ROUTEFUNCTION_COT
#define ROUTEFUNCTION_COT

/*
what does a RouteFunction really need?
    the request data to work with it and respond to it properly
    the clientfd as a direct link to the client.
    we need to have a link to some external data if its needed
    e.g. data about a user from a database.
    a siteVar could be pretty good here honestly, since its flexible

return a boolean for success or failure
*/

#include "sitevar_cot.h"
#include "stringmap_cot.h"
#include "dependencies_cot.h"


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

typedef struct HttpRequest {
    char* target; //8 bytes
    stringMap* options; //8 bytes
    char* payload; //8 bytes
    
    float version; //4 bytes, if double then 8 bytes
    HTTPTYPE type; //4 bytes
} HttpRequest;


typedef bool (*RouteFunction)(HttpRequest request, int clientfd, siteVar* extraData);

//also make a macro for creating a default definition of a RouteFunction

#define NewRouteFunction(functionName) bool functionName(HttpRequest request, int clientfd, siteVar* extraData)


typedef struct RouteEntry {
    RouteFunction routeGet;
    RouteFunction routePost;
    RouteFunction routePut;
    RouteFunction routeDelete;
} RouteEntry;

#endif