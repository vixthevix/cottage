#define COTTAGE_START
#include "cottage/cottage.h"

/*
right now, cottage has a lot of issues.
its a different approach to how serversource (the previous framework)
did things.

I think we really should implement global hashmaps for
components, stylesheets, and stuff like this.
For testing purposes, dont make it global right now.
For release, yeah make global stuff.



*/


NewRouteFunction(homeGet) {
    return defaultGet(request, clientfd, extraData, "./templates/main.html");
}


int main(void) {

    const char* ADDRESS = "0.0.0.0";
    const char* PORT = "8080";

    int socketfd = serverInit(ADDRESS, PORT, true);
    if (socketfd <= -1) return 1;

    if (!serverListen(socketfd, 1)) {
        printf("error listening\n");
        serverClose(socketfd);
        return 1;
    }

    //THE ROUTES
    RouteEntry home = {
        .routeGet = homeGet,
        .routePost = NULL,
        .routePut = NULL,
        .routeDelete = NULL
    };


    //the routemap
    RouteMap* routes = RouteMapInit();
    RouteMapInsert(routes, "/", home);

    while (true) {
        int clientfd = serverAcceptClient(socketfd, NULL, NULL);
        if (clientfd < 0) continue;
        char* clientOffload = serverGetRequest(clientfd);
        HttpRequest request = splitHttpRequest(clientOffload);

        //we have no extra data
        //we have a request
        //now we just wire up the routeMap

        if (!handleRequest(request, clientfd, NULL, routes)) {
            printf("could not handle request\n");
            sendError(clientfd, ERROR_404);
        }


        if (clientOffload) free(clientOffload);
        serverCloseClient(clientfd);
        HttpRequestFree(request);
    }

    return 0;
}