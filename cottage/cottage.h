/*
Main cottage header to include in main code.

Code is part of the cottage framework (https://github.com/vixthevix/cottage)
*/

//debug statement, remove from final code
#define COTTAGE_START

#ifdef COTTAGE_START //start definition


#ifndef COTTAGE_COT //guard
#define COTTAGE_COT

#include "tcpsetup_cot.h"
#include "stringmap_cot.h"
#include "sitevar_cot.h"
#include "boolcalc_cot.h"
#include "fopen_cot.h"
#include "error_cot.h"
#include "hashfunc_cot.h"
#include "dependencies_cot.h"
#include "conversion_cot.h"
#include "routemap_cot.h"
#include "httpsplit_cot.h"
#include "manager_cot.h"
#include "init_cot.h"

#endif //guard
#endif //start definition