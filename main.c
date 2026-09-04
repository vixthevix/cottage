#include "cottage/tcpsetup_cot.h"
#define COTTAGE_START
#include "cottage/cottage.h"

#include "routes/home_routes.h"
#include "routes/cool_routes.h"
#include "routes/chainsawman_routes.h"
#include "routes/notes_routes.h"

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
    const int MAXCLIENTCOUNT = 64;

    ServerConfig* server = serverInit(ADDRESS, PORT, MAXCLIENTCOUNT);
    if (!server) return 1;

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
    RouteEntry notes = {
    .routeGet = notesGet,
    .routePost = notesPost,
    .routePut = notesPut,
    .routeDelete = notesDelete
    };

    newRoute("/", home);
    newRoute("/cool", cool);
    newRoute("/chainsawman", chainsawman);
    newRoute("/notes", notes);

    while (true) {
        int ready_count = CotPollPoll(server->poll);
        for (int i = 0; i < ready_count; i++) {
            int active_fd = CotPollAccess(server->poll, i);
            if (active_fd == server->server_fd) {
                //new client
                int clientfd = serverAcceptClient(server);
                if (clientfd < 0) continue;
                
                CotPollPush(server->poll, clientfd);
            }
            else {
                //existing client
                
                char* clientOffload = serverRecvClient(active_fd);
                HttpRequest request = {0};
                if (splitHttpRequest(&request, clientOffload) .status == COT_ERROR) {
                    return 1;
                }
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

                if (!handleRequest(request, active_fd, extraData, GLOBALROUTES)) {
                    printf("could not handle request\n");
                    sendError(active_fd, ERROR_404);
                }

                HttpRequestFree(request);
                siteVarFree(extraData);

                CotPollPop(server->poll, active_fd);
                if (clientOffload) free(clientOffload);
                serverCloseClient(active_fd);

            }
        }
    }

    RouteMapFree(GLOBALROUTES);

    serverClose(server);

    return 0;
}