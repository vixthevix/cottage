#ifndef ERROR_COT
#define ERROR_COT

#include "dependencies_cot.h"
#include "init_cot.h"
/*
lets implement a new error result struct.
just like rust!

the only difference i will make is,
functions that use the error struct will return it.
and the original return data will be provided by
insertion into a pointer provided into the first parameter.

like this:

int myfunction(int foo, char bar);
            |
            v
cotResult myFunction(int* result, int foo, char bar);

this eliminates type ambiguity and makes the cotResult easy to work with.


*/

typedef bool ErrorStatus;
#define COT_OK true
#define COT_ERROR false

typedef struct cotResult {
    ErrorStatus status; 
    char message[256];
} cotResult;

cotResult newResult(ErrorStatus status, const char message[256]) {
    cotResult x = {
        .status = (status) ? COT_OK:COT_ERROR,
        .message = 0,
    };

    if (message && status == COT_ERROR) {
        //what we can do is append the current time the error occured as well.
        time_t rawtime;
        struct tm* timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        char* timeString = asctime(timeinfo); timeString[strlen(timeString) - 1] = 0;
        sprintf(x.message, "[%s] %s\n", timeString, message);
        //for now, also print directly to stderr. maybe move this somewhere else?
        fprintf(stderr, x.message);
    }

    return x;
}

cotResult newResultOK() {
    return newResult(COT_OK, NULL);
}
cotResult newResultError(const char message[256]) {
    return newResult(COT_ERROR, message);
}


typedef enum ErrorType {
    ERROR_400,
    ERROR_401,
    ERROR_402,
    ERROR_404,
    ERROR_405,
    ERROR_406,
    ERROR_408,
    ERROR_409,
    ERROR_410,
    ERROR_411,
    ERROR_412,
    ERROR_413,
    ERROR_414,
    ERROR_415,
    ERROR_416,
    ERROR_417,
    ERROR_418,
    ERROR_421,
    ERROR_422,
    ERROR_423,
    ERROR_424,
    ERROR_425,
    ERROR_426,
    ERROR_428,
    ERROR_429,
    ERROR_431,
    ERROR_451
} ErrorType;

int sendError(int client, ErrorType error) {
    cottageCheck(0);
    switch (error) {
        case ERROR_404: {
            const char* msg = 
            "HTTP/1.1 404 Not found\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n"
            "\r\n";
            send(client, msg, strlen(msg), 0);
        }
        default: {
            return 0;
        }
    }

    return 1;
}

#endif