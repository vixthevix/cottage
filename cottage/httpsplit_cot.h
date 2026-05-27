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



bool handleGet(HttpRequest request, int clientfd, stringMap* links, siteVar* extraVariables) {
    if (!HttpRequestValid(request) || !extraVariables || request.type != GET) return false;
    
    //our request has the link in the target section.
    //we will use it to access the links
    //because this is kind of meant to be a basic way of handling a GET request,
    //not a lot of thought here lowkey.

    //we actually need to first split the request target into 
    //the actual link, and the variables

    char link[512] = {0};
    char offload[512] = {0};
    bool markFound = false;
    int index = 0;

    for (size_t i = 0; i < strlen(request.target); i++) {
        if (request.target[i] == '?') {
            markFound = true;
            index = 0;
            continue;
        }

        if (!markFound) {
            link[index++] = request.target[i];
        }
        else {
            offload[index++] = request.target[i];
        }
    }

    //now the link is okay, but we need to convert our offload into vars

    siteVar* offloadVars = offloadToVariables(offload);

    //which has priority, extraVariables or offloadVars?
    //we could differentiate it in HTML by having an extraVar container
    //for example, called global
    //referenced in the html as global.variable
    
    //siteVar* vars = siteVarCompositeCombine()
    siteVarCompositeInsert(offloadVars, extraVariables);


    char* filepath = strMapGet(links, request.target);

    sendFile(filepath, clientfd, offloadVars);

    siteVarFree(offloadVars);
    

    return true;
} 


/*
For a default POST request, technically they differ by Content type.
this being URL encoded, or a multi part.
We will initially use just URL encoded.

the target will be interpreted as a folder,
also using a stringMap for links (probably separate than GET links?)
if the target has any parameters, ignore them.
use only the offload for input data.

because its a POST request, target as a folder means we can create a unique file
with a unique name, like just numbering them.
idk, a "default POST" feels like it wouldnt work, but this is the best i can come up with

*/
bool handlePost(HttpRequest request, int clientfd, stringMap* links, siteVar* extraVariables) {


    return true;
}



#endif