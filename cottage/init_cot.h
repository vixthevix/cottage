#ifndef INIT_COT
#define INIT_COT

#include "dependencies_cot.h"

//have some setup booleans and such
bool cottageInitialised = false;

#define cottageCheck(returnVal) if (!cottageInitialised) return returnVal

#endif