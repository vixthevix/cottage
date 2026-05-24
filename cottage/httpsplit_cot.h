#ifndef HTTPSPLIT_COT
#define HTTPSPLIT_COT

/*
A request from the client can be split into
request type (GET, POST etc.)
request target (a url usually, like with a GET request)
HTTP version (usually HTTP/1.1)

then a \r\n

header options (represented by option: details, followed by a \r\n for each option)

then a \r\n

payload (just a string)
with a POST request, can be a bit weird because of something called a Multipart Form Submission
https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Methods/POST

but thats pretty much it

*/




#include "dependencies_cot.h"
#include "stringmap_cot.h"

//rename ERROR if conflicts with other enum types
typedef enum HTTPTYPE {
    ERROR = -1,
    GET,
    PUT,
    POST,
    DELETE,
    PATCH,
    HEAD,
    OPTIONS,
    TRACE,
    CONNECT,
} HTTPTYPE;

HTTPTYPE StrToHTTPTYPE(char* data) {
    if (!data) return ERROR;

    if (!strcmp(data, "GET")) return GET;
    if (!strcmp(data, "PUT")) return PUT;
    if (!strcmp(data, "POST")) return POST;
    if (!strcmp(data, "DELETE")) return DELETE;
    if (!strcmp(data, "PATCH")) return PATCH;
    if (!strcmp(data, "HEAD")) return HEAD;
    if (!strcmp(data, "OPTIONS")) return OPTIONS;
    if (!strcmp(data, "TRACE")) return TRACE;
    if (!strcmp(data, "CONNECT")) return CONNECT;

    return ERROR;
}

float StrToHttpVersion(char* data) {
    if (!data) return ERROR;

    if (!strcmp(data, "HTTP/0.9")) return 0.9;
    if (!strcmp(data, "HTTP/1.0")) return 1.0;
    if (!strcmp(data, "HTTP/1.1")) return 1.1;
    if (!strcmp(data, "HTTP/2")) return 2;
    if (!strcmp(data, "HTTP/3")) return 3;

    return -1;

}

typedef struct HttpRequest {
    char* target; //8 bytes
    stringMap* options; //8 bytes
    char* payload; //8 bytes
    
    float version; //4 bytes, if double then 8 bytes
    HTTPTYPE type; //4 bytes
} HttpRequest;

bool HttpRequestFree(HttpRequest request) {
    if (request.target) free(request.target);
    if (request.options) strMapFree(request.options);
    if (request.payload) free(request.payload);

    return true;
}


HttpRequest splitHttpRequest(char* data) {
    HttpRequest error = {
        .target = NULL,
        .options = NULL,
        .payload = NULL,
        .version = -1,
        .type = ERROR
    };


    HttpRequest request = {
        .target = NULL,
        .options = NULL,
        .payload = NULL,
        .version = -1,
        .type = ERROR
    };

    //check for valid data
    if (!data || strlen(data) == 0) return error;

    //first line has type, target and version, separated by spaces

    char buffer[512] = {0};
    int bufferIndex = 0;
    int item = 0;
    bool stopReading = false;

    int dataIndex = 0;
    size_t dataLen = strlen(data);

    for (; dataIndex < (dataLen - 1); dataIndex++) {
        if (data[dataIndex] != ' ' || (data[dataIndex] != '\r' && data[dataIndex + 1] != '\n')) 
            buffer[bufferIndex++] = data[dataIndex];
        else {
            if (item == 0) { //type
                request.type = StrToHTTPTYPE(buffer);
            }
            else if (item == 1) { //target
                request.target = malloc(strlen(buffer) + 1);
                strcpy(request.target, buffer);
                request.target[strlen(buffer)] = 0;
            }
            else { //version
                request.version = StrToHttpVersion(buffer);
            }
            item += 1;
            memset(buffer, 0, bufferIndex);
            bufferIndex = 0;

            if (item > 1) break;
        }
    }

    if ((data[dataIndex] == '\r' && data[dataIndex + 1] == '\n') || dataIndex >= dataLen) dataIndex += 2;
    else {
        HttpRequestFree(request);
        return error;
    }

    //for the following, we have multiple lines.
    //option:value separated by colon, lines separted by \r\n
    //with a final \r\n
    //we should treat this final \r\n as its own line, so we should check the beginning

    //buffer is already zeroed

    //enable the stringmap

    request.options = strMapInit();

    while (dataIndex < (dataLen - 1)) {
        if (data[dataIndex] != '\r' && data[dataIndex + 1] != '\n') {
            buffer[bufferIndex++] = data[dataIndex++];
        }
        else {
            if (!buffer[0]) { //is buffer empty?
                break;
            }
            dataIndex += 2;
            //buffer now has option:value
            //we can use strtok to get a substring up until a certain character
            //...but honestly lets just use two different buffers here

            char option[512] = {0};
            char value[512] = {0};
            int optvalIndex = 0;
            bool colonFound = false;
            for (int i = 0; i < strlen(buffer); i++) {
                if (buffer[i] == ':') {
                    colonFound = true;
                    optvalIndex = 0;
                    //there could be spaces in front of colon, remove them
                    while (buffer[++i] == ' ');
                    continue;
                }

                if (!colonFound) {
                    option[optvalIndex++] = buffer[i];
                }
                else {
                    value[optvalIndex++] = buffer[i];
                }
            }

            //now just insert them
            strMapInsert(request.options, option, value);
            memset(buffer, 0, bufferIndex);
            bufferIndex = 0;
        }
    }
    
    //stringmap is now filled up
    if ((data[dataIndex] == '\r' && data[dataIndex + 1] == '\n') || dataIndex >= dataLen) dataIndex += 2;
    else {
        HttpRequestFree(request);
        return error;
    }

    //finally we have our offload
    //just copy it over
    request.payload = (char*) malloc(dataLen - dataIndex + 1);
    memcpy(request.payload, data + dataIndex, dataLen - dataIndex);
    request.payload[strlen(request.payload)] = 0;

    return request;
}

//now that we have a request split into necessary components, we can go in two ways.
//one way is to let the programmer handle everything themselves, in a way that fits them.
//another is to provide helper functions for each http request type, to make life easier.
//i think ill go with the second option as it doesnt eliminate the first one,
//and it makes the framework more approachable




#endif