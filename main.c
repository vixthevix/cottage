#include "cottage/sitevar_cot.h"
#define COTTAGE_START
#include "cottage/cottage.h"

#include "routes/home_routes.h"
#include "routes/cool_routes.h"
#include "routes/chainsawman_routes.h"

/*
right now, cottage has a lot of issues.
its a different approach to how serversource (the previous framework)
did things.

I think we really should implement global hashmaps for
components, stylesheets, and stuff like this.
For testing purposes, dont make it global right now.
For release, yeah make global stuff.

1/8/2026 update
need to analyse whats wrong with cottage and fix that

also, make shell scripts / C executables for making routes
place these helper scripts into a cottage/help folder
make a makefile to compile the helper scripts.

4/8/2026 update
mkroutes is a cool helper script so far
shame that we need to include each route in our main file individually
idk if we can do that without introducing circular dependencies

*/


int main(void) {

    // const char* testString = "/hello";
    // char* cleanedString = cleanupPath(testString);
    // printf("huh\n");
    // printf("cleanedString: %s\n", cleanedString);
    // free(cleanedString);
    printf("hello\n");
    cottageInit();
    printf("oh no\n");

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
    .routePost = homePost,
    .routePut = homePut,
    .routeDelete = homeDelete
    };
    RouteEntry cool = {
    .routeGet = coolGet,
    .routePost = coolPost,
    .routePut = coolPut,
    .routeDelete = coolDelete
    };
    RouteEntry chainsawman = {
    .routeGet = chainsawmanGet,
    .routePost = chainsawmanPost,
    .routePut = chainsawmanPut,
    .routeDelete = chainsawmanDelete
    };


    newRoute("/cool", cool);
    newRoute("/", home);
    newRoute("/chainsawman", chainsawman);

    while (true) {
        int clientfd = serverAcceptClient(socketfd, NULL, NULL);
        if (clientfd < 0) continue;
        char* clientOffload = serverGetRequest(clientfd);
        //printf("client offload is \n%s\n", clientOffload);
        HttpRequest request = splitHttpRequest(clientOffload);
        debugHttpRequest(request);
        //we have no extra data
        //we have a request
        //now we just wire up the routeMap
        //printf("NEW CLIENT\n");

        //quick extraData
        siteVar* extraData = siteVarInit("global", COMPOSITE, 0, NULL);
        if (siteVarCompositeInsertNew(&extraData, "peak", UINT, 1, &((uint_cot){67}))) {
            printf("yippie\n");
        }
        else printf("not yippie\n");

        siteVar* peak = siteVarCompositeAccess(extraData, "peak");
        printf("peak: %u\n", *(uint_cot**)siteVarAccess(peak));
    

        if (!handleRequest(request, clientfd, extraData, GLOBALROUTES)) {
            printf("could not handle request\n");
            sendError(clientfd, ERROR_404);
        }


        if (clientOffload) free(clientOffload);
        serverCloseClient(clientfd);
        HttpRequestFree(request);
    }
    RouteMapFree(GLOBALROUTES);

    return 0;
}