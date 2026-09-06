/*
Localises C Standard Library inclusions into one file.

Code is part of the cottage framework (https://github.com/vixthevix/cottage)
*/

#ifndef DEPENDENCIES_COT
#define DEPENDENCIES_COT

//basic utilities
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <ctype.h>
#include <math.h>


//needed for error handling
#include <time.h>

//needed for sockets
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

//needed for multi-user
#include <sys/epoll.h>
#include <fcntl.h>

#endif