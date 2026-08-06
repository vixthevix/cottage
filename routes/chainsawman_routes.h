#define COTTAGE_START
#include "../cottage/cottage.h"

NewRouteFunction(chainsawmanGet) {
   return defaultGet(request, clientfd, extraData, "./templates/chainsaw.html");
}
NewRouteFunction(chainsawmanPost) {
   return false;
}
NewRouteFunction(chainsawmanPut) {
   return false;
}
NewRouteFunction(chainsawmanDelete) {
   return false;
}

/*
Enter the following into your main code, under where you setup your routes:
RouteEntry chainsawman = {
    .routeGet = chainsawmanGet,
    .routePost = chainsawmanPost,
    .routePut = chainsawmanPut,
    .routeDelete = chainsawmanDelete
};


newRoute(YOUR SITE PATH HERE, chainsawman);
*/