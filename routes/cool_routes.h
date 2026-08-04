#define COTTAGE_START
#include "../cottage/cottage.h"

NewRouteFunction(coolGet) {
   return defaultGet(request, clientfd, extraData, "./templates/cool.html");
}
NewRouteFunction(coolPost) {
   return false;
}
NewRouteFunction(coolPut) {
   return false;
}
NewRouteFunction(coolDelete) {
   return false;
}

/*
Enter the following into your main code, under where you setup your routes:
RouteEntry cool = {
    .routeGet = coolGet,
    .routePost = coolPost,
    .routePut = coolPut,
    .routeDelete = coolDelete
};


newRoute(YOUR SITE PATH HERE, cool);
*/
