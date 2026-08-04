#define COTTAGE_START
#include "../cottage/cottage.h"

NewRouteFunction(homeGet) {
   return defaultGet(request, clientfd, extraData, "./templates/main.html");
}
NewRouteFunction(homePost) {
   return false;
}
NewRouteFunction(homePut) {
   return false;
}
NewRouteFunction(homeDelete) {
   return false;
}

/*
Enter the following into your main code, under where you setup your routes:
RouteEntry home = {
    .routeGet = homeGet,
    .routePost = homePost,
    .routePut = homePut,
    .routeDelete = homeDelete
};


newRoute(YOUR SITE PATH HERE, home);
*/
