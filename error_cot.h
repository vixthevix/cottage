/*
Standardised error logging functionality.

Code is part of the cottage framework (https://github.com/vixthevix/cottage)
*/

#ifndef ERROR_COT
#define ERROR_COT

#include "dependencies_cot.h"
#include "init_cot.h"

typedef bool ErrorStatus;
#define COT_OK true
#define COT_ERROR false

/*
Struct that reports details of error.
*/
typedef struct cotResult {
    ErrorStatus status; 
    char message[256];
} cotResult;

/*
The following enum and sendError function are used for HTTP error handling.
This must be put into its own file, or into fopen_cot.h
*/
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

// Function prototypes
cotResult newResult(ErrorStatus status, const char message[256]);
cotResult newResultOK(void);
cotResult newResultError(const char message[256]);
int sendError(int client, ErrorType error);

#if defined(COTTAGE_START)

/*
Creates and logs a new error.
@arg status -> error status.
@arg message -> error message.
@return new error log.
*/
cotResult newResult(ErrorStatus status, const char message[256]) {
    cotResult x = {
        .status = (status) ? COT_OK:COT_ERROR,
        .message = 0,
    };

    if (message && status == COT_ERROR) {
        //The designated message will also include the time and date 
        //at which the error occured.
        time_t rawtime;
        struct tm* timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        char* timeString = asctime(timeinfo); 
        timeString[strlen(timeString) - 1] = 0; //Replaces \n with terminator at the end
        
        snprintf(x.message, 256, "[%s] %s\n", timeString, message);
        //Additionally, print the message to standard error to be displayed.
        fprintf(stderr, x.message);
    }

    return x;
}

//Wrappers for newResult
cotResult newResultOK() {
    return newResult(COT_OK, NULL);
}
cotResult newResultError(const char message[256]) {
    return newResult(COT_ERROR, message);
}

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
#endif