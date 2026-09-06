/*
Setup needed for cottage to run.

Code is part of the cottage framework (https://github.com/vixthevix/cottage)
*/

#ifndef INIT_COT
#define INIT_COT

#include "dependencies_cot.h"

//Global variable for allowing all cottage functions to run.
bool cottageInitialised = false;
/*
Macro for checking if cottage has been initialised.
@arg returnVal -> value that function we are in should return.
*/
#define cottageCheck(returnVal) if (!cottageInitialised) return returnVal

#endif