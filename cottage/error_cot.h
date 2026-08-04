#ifndef ERROR_COT
#define ERROR_COT

#include "dependencies_cot.h"
#include "init_cot.h"

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