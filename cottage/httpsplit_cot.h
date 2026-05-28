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
#include "routemap_cot.h"
#include "sitevar_cot.h"
#include "fopen_cot.h"

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

bool HttpRequestValid(HttpRequest request) {
    return (request.target && request.options && request.payload && (request.type > ERROR) && (request.version > -1));
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
//for now, try not to use any options, just work with the target and payload


/*
a general handleRequest function
will split the target into the link and its offload
it will then look into the routeMap to get the specific route to take,
based on the request.

it is up to the programmer to decide what to do with the routes.
though there are default handles you can use to help.

a route function that points to null will do nothing.
no "default" behaviour with null to prevent unintended behaviour,
everything must be explicitly defined.
*/

bool handleRequest(HttpRequest request, int clientfd, siteVar* extraData, RouteMap* routes) {
    if (!HttpRequestValid(request) || !routes) return false;


    char link[512] = {0};
    int index = 0;

    while (index < strlen(request.target) && request.target[index] != '?') {
        link[index] = request.target[index];
        index++;
    }

    //with this link, we can get our routes

    RouteEntry route = RouteMapGet(routes, link);

    //for now, nothing happens if NULL
    //but maybe perform an error 405 method not allowed block

    switch (request.type) {
        case GET: {
            if (route.routeGet) route.routeGet(request, clientfd, extraData);
            break;
        }
        case POST: {
            if (route.routePost) route.routePost(request, clientfd, extraData);
            break;
        }
        case PUT: {
            if (route.routePut) route.routePut(request, clientfd, extraData);
            break;
        }
        case DELETE: {
            if (route.routeDelete) route.routeDelete(request, clientfd, extraData);
            break;
        }
        default: {
            //do nothing
            break;
        }
    }

    return true;

}



//maybe include extraVariables here who knows
//but this will involve some encoding
siteVar* offloadToVariables(char* offload) {
    if (!offload || strlen(offload) <= 0) return NULL;
    
    //look for equals and question marks
    
    char key[512] = {0};
    char value[512] = {0};

    size_t index = 0;

    siteVar* variables = siteVarInit("variables", COMPOSITE, 0, NULL);

    bool state = false;

    for (size_t i = 0; i < strlen(offload); i++) {
        if (offload[i] == '=') {
            if (state == false) {
                strcpy(key, value);
                memset(value, 0, 512);
                index = 0;
                state = true;
            }
            else { //bad query
                siteVarFree(variables);
                return NULL;
            }
        }
        else if (offload[i] == '&') {
            if (state == true) {
                //we have to get the type of our data, then insert it
                //key remains the same
                VARTYPE type = BC_StrToType(value);
                if (type != ERROR) {

                    void* data;
                    switch (type) {
                        case UINT: {
                            data = malloc(sizeof(uint_cot));
                            memcpy(data, &((uint_cot){BC_StrToUInt(value)}), sizeof(uint_cot));
                            break;
                        }
                        case INT: {
                            data = malloc(sizeof(int_cot));
                            memcpy(data, &((int_cot){BC_StrToInt(value)}), sizeof(int_cot));
                            break;
                        }
                        case FLOAT: {
                            data = malloc(sizeof(float_cot));
                            memcpy(data, &((float_cot){BC_StrToFloat(value)}), sizeof(float_cot));
                            break;
                        }
                        case STRING: {
                            data = BC_StrToStr(value);
                            break;
                        }
                        case BOOL: {
                            data = malloc(sizeof(bool_cot));
                            memcpy(data, &((bool_cot){BC_StrToBool(value)}), sizeof(bool_cot));
                            break;
                        }
                        default: {
                            siteVarFree(variables);
                            return NULL;
                        }
                    }

                    siteVarCompositeInsertNew(variables, key, type, 1, data);
                    free(data);
                }
                else {

                    //what else could it be?
                    //it could be an array, so we'll take that into account.
                    if (BC_isArray(value)) {
                        siteVar* array = BC_ArrayToSiteVar(key, value, NULL);
                        if (array) {
                            siteVarCompositeInsert(variables, array);
                        }
                    }
                    else {
                        //otherwise, its a variable name.
                        //since these are the base variables, we must return an error here
                        //or ignore the variable
                        siteVarFree(variables);
                        return NULL;
                    }

                    //alternatively, we can interprete this as a string,
                    //but feels kind of weird
                }

                //after inserting, we must clear our key and value
                memset(key, 0, strlen(key));
                memset(value, 0, strlen(value));
                index = 0;
                state = false;
                
            }
            else { //bad query
                siteVarFree(variables);
                return NULL;
            }
        }
        else {
            value[index++] = offload[i];
        }
    }
    
    if (state == true) {
        //we have to get the type of our data, then insert it
        //key remains the same
        VARTYPE type = BC_StrToType(value);
        if (type != ERROR) {

            void* data;
            switch (type) {
                case UINT: {
                    data = malloc(sizeof(uint_cot));
                    memcpy(data, &((uint_cot){BC_StrToUInt(value)}), sizeof(uint_cot));
                    break;
                }
                case INT: {
                    data = malloc(sizeof(int_cot));
                    memcpy(data, &((int_cot){BC_StrToInt(value)}), sizeof(int_cot));
                    break;
                }
                case FLOAT: {
                    data = malloc(sizeof(float_cot));
                    memcpy(data, &((float_cot){BC_StrToFloat(value)}), sizeof(float_cot));
                    break;
                }
                case STRING: {
                    data = BC_StrToStr(value);
                    break;
                }
                case BOOL: {
                    data = malloc(sizeof(bool_cot));
                    memcpy(data, &((bool_cot){BC_StrToBool(value)}), sizeof(bool_cot));
                    break;
                }
                default: {
                    siteVarFree(variables);
                    return NULL;
                }
            }

            siteVarCompositeInsertNew(variables, key, type, 1, data);
            free(data);
        }
        else {

            //what else could it be?
            //it could be an array, so we'll take that into account.
            if (BC_isArray(value)) {
                siteVar* array = BC_ArrayToSiteVar(key, value, NULL);
                if (array) {
                    siteVarCompositeInsert(variables, array);
                }
            }
            else {
                //otherwise, its a variable name.
                //since these are the base variables, we must return an error here
                //or ignore the variable
                siteVarFree(variables);
                return NULL;
            }

            //alternatively, we can interprete this as a string,
            //but feels kind of weird
        }

        //after inserting, we must clear our key and value
        memset(key, 0, strlen(key));
        memset(value, 0, strlen(value));
        index = 0;
        state = false;
        
    }
    else { //bad query
        siteVarFree(variables);
        return NULL;
    }

    return variables;
}

//handle request is good, but we can provide default functions for each type of request as well.
//not all of them probably, but GET is a good start


bool defaultGet(HttpRequest request, int clientfd, siteVar* extraVariables, char* filePath) {
    if (!HttpRequestValid(request) || !extraVariables || request.type != GET) return false;

    //filePath has our direct link.
    //we need our offload though, we can use strstr for this conveniently

    char* offloadPosition = strstr(request.target, "?");
    char* offload = NULL;
    if (offloadPosition) {
        size_t offloadPositionLen = strlen(offloadPosition);
        offload = (char*) malloc(offloadPositionLen + 1);
        strcpy(offload, offloadPosition);
        offload[offloadPositionLen] = 0;
        BC_delAt(offload, 0); //to remove question mark
    }

    siteVar* offloadVars = offloadToVariables(offload);
    siteVarCompositeInsert(offloadVars, extraVariables);
    
    sendFile(filePath, clientfd, offloadVars);

    siteVarFree(offloadVars);
    
    return true;
}



#endif