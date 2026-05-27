/*
A routeMap is a kind of mapping from a string link, to a special struct
containing function pointers for the different http requests.

in this way, the programmer can define what each link does for each request,
and if not defined, default behaviour can occur, using the handle functions


*/


#include "dependencies_cot.h"
#include "httpsplit_cot.h"

