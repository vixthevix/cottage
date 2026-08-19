#define COTTAGE_START
#include "../cottage/cottage.h"

NewRouteFunction(notesGet) {
      return defaultGet(request, clientfd, extraData, "./notes.txt");
}
NewRouteFunction(notesPost) {
   return false;
}
NewRouteFunction(notesPut) {
   return false;
}
NewRouteFunction(notesDelete) {
   return false;
}

/*
Enter the following into your main code, under where you setup your routes:
RouteEntry notes = {
    .routeGet = notesGet,
    .routePost = notesPost,
    .routePut = notesPut,
    .routeDelete = notesDelete
};


newRoute(YOUR SITE PATH HERE, notes);
*/
