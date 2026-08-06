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
#include "init_cot.h"
#include "manager_cot.h"
#include "routefunction_cot.h"
#include "stringmap_cot.h"
#include "routemap_cot.h"
#include "sitevar_cot.h"
#include "fopen_cot.h"
#include <stddef.h>
#include <stdlib.h>
#include <threads.h>
#include <time.h>

siteVar* offloadToVariables(char* offload);


//rename ERROR if conflicts with other enum types

HTTPTYPE StrToHTTPTYPE(char* data) {
    cottageCheck(UNKNOWN);
    if (!data) return UNKNOWN;

    if (!strcmp(data, "GET")) return GET;
    if (!strcmp(data, "PUT")) return PUT;
    if (!strcmp(data, "POST")) return POST;
    if (!strcmp(data, "DELETE")) return DELETE;
    if (!strcmp(data, "PATCH")) return PATCH;
    if (!strcmp(data, "HEAD")) return HEAD;
    if (!strcmp(data, "OPTIONS")) return OPTIONS;
    if (!strcmp(data, "TRACE")) return TRACE;
    if (!strcmp(data, "CONNECT")) return CONNECT;

    return UNKNOWN;
}

float StrToHttpVersion(char* data) {
    cottageCheck(0);
    if (!data) return UNKNOWN;

    if (!strcmp(data, "HTTP/0.9")) return 0.9;
    if (!strcmp(data, "HTTP/1.0")) return 1.0;
    if (!strcmp(data, "HTTP/1.1")) return 1.1;
    if (!strcmp(data, "HTTP/2")) return 2;
    if (!strcmp(data, "HTTP/3")) return 3;

    return -1;

}

bool HttpRequestFree(HttpRequest request) {
    cottageCheck(false);
    if (request.target) free(request.target);
    if (request.options) strMapFree(request.options);
    if (request.payload) free(request.payload);

    return true;
}

bool HttpRequestValid(HttpRequest request) {
    cottageCheck(false);
    return (request.target && request.options && (request.type > UNKNOWN) && (request.version > -1));
}




HttpRequest splitHttpRequest(char* data) {
    HttpRequest error = {
        .target = NULL,
        .options = NULL,
        .payload = NULL,
        .version = -1,
        .type = UNKNOWN
    };
    cottageCheck(error);


    HttpRequest request = {
        .target = NULL,
        .options = NULL,
        .payload = NULL,
        .version = -1,
        .type = UNKNOWN
    };

    //check for valid data
    if (!data || strlen(data) == 0) return error;
    //printf("data valid\n");
    //first line has type, target and version, separated by spaces

    char buffer[512] = {0};
    int bufferIndex = 0;
    int item = 0;
    bool stopReading = false;

    int dataIndex = 0;
    size_t dataLen = strlen(data);

    for (; dataIndex < (dataLen - 1); dataIndex++) {
        if (data[dataIndex] != ' ' && (data[dataIndex] != '\r' && data[dataIndex + 1] != '\n')) 
            buffer[bufferIndex++] = data[dataIndex];
        else {
            ////printf("buffer: %s\n", buffer);
            if (item == 0) { //type
                request.type = StrToHTTPTYPE(buffer);
            }
            else if (item == 1) { //target
                request.target = (char*) malloc(strlen(buffer) + 1);
                strcpy(request.target, buffer);
                request.target[strlen(buffer)] = 0;
            }
            else { //version
                request.version = StrToHttpVersion(buffer);
            }
            memset(buffer, 0, bufferIndex);
            bufferIndex = 0;

            if (item > 1) break;
            item += 1;
        }
    }

    //debugHttpRequest(request);

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

    //printf("enabling stringmap\n");
    //problem here

    request.options = strMapInit();

    while (dataIndex < (dataLen - 1)) {
        if (data[dataIndex] != '\r' && data[dataIndex + 1] != '\n') {
            ////printf("writing...\n");
            buffer[bufferIndex++] = data[dataIndex++];
        }
        else {
            ////printf("buffer is %s\n", buffer);
            if (!buffer[0]) { //is buffer empty?
                //printf("buffer empty\n");
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
                if (buffer[i] == ':' && !colonFound) {
                    colonFound = true;
                    optvalIndex = 0;
                    //there could be spaces in front of colon, remove them
                    while (buffer[++i] == ' ');
                    i--;
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
            //printf("%s:%s\n", option, value);
            strMapInsert(&request.options, option, value);
            //printf("string map inserted\n");
            memset(buffer, 0, bufferIndex);
            bufferIndex = 0;
            //printf("buffer reset\n");
        }
    }
    //printf("stringmap filled\n");
    //stringmap is now filled up
    if ((data[dataIndex] == '\r' && data[dataIndex + 1] == '\n') || dataIndex >= dataLen) dataIndex += 2;
    else {
        HttpRequestFree(request);
        return error;
    }

    //finally we have our offload
    //just copy it over
    //we may not have an offload, so keep that in mind
    if (dataLen > dataIndex) {
        request.payload = (char*) malloc(dataLen - dataIndex + 1);
        memcpy(request.payload, data + dataIndex, dataLen - dataIndex);
        request.payload[strlen(request.payload)] = 0;
    }
    else request.payload = NULL;


    return request;
}

int hexToInt(char hex) {
    if ('0' <= hex && hex <= '9') return hex - '0';
    if ('a' <= hex && hex <= 'f') return hex - 'a' + 10; //a=10
    if ('A' <= hex && hex <= 'F') return hex - 'A' + 10; //A=10
    return -1; //invalid 
}

char* urlDecode(char* offload) {
    if (!offload) return NULL;

    char* decoded = (char*) calloc(strlen(offload) + 1, sizeof(char));
    size_t j = 0;
    // char* readptr = offload;
    // char* writeptr = offload;

    size_t i = 0;
    while (offload[i]) {
        if (offload[i] == '%' && offload[i + 1] && offload[i + 2]) {
            //convert into hex
            int high = hexToInt(offload[i + 1]);
            int low = hexToInt(offload[i + 2]);
            if (high >= 0 && low >= 0) {
                char byte = (char)((high << 4) | low);

                decoded[j++] = byte;
                i += 3;

                // *writeptr = byte;
                // readptr += 3;
                // writeptr++;
                continue;
            }
        }
        else if (offload[i] == '+') {
            decoded[j++] = ' ';
            i++;
            continue;
        }

        //normal character
        decoded[j++] = offload[i++];
        // *writeptr = *readptr;
        // readptr++;
        // writeptr++;
    }
    
    decoded = (char*)realloc(decoded, strlen(decoded) + 1);
    return decoded;
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
    cottageCheck(false);
    if (!HttpRequestValid(request) || !routes) return false;

    //printf("handling requests\n");

    char link[512] = {0};
    int index = 0;

    while (index < strlen(request.target) && request.target[index] != '?') {
        link[index] = request.target[index];
        index++;
    }

    RouteEntry route = RouteMapGet(routes, link);
    
    //for now, nothing happens if NULL
    //but maybe perform an error 405 method not allowed block

    //decoding of '?' payload and request.payload is done in respective http functions


    switch (request.type) {
        case GET: {
            //printf("route is GET\n");
            if (route.routeGet) {
                route.routeGet(request, clientfd, extraData);
                goto success;
            }
            break;
        }
        case POST: {
            if (route.routePost) {
                route.routePost(request, clientfd, extraData);
                goto success;
            }
            break;
        }
        case PUT: {
            if (route.routePut) {
                route.routePut(request, clientfd, extraData);
                goto success;
            }
            break;
        }
        case DELETE: {
            if (route.routeDelete) {
                route.routeDelete(request, clientfd, extraData);
                goto success;
            }
            break;
        }
        default: {
            //do nothing
            //printf("route is INVALID\n");
            break;
        }
    }

    //if the request type is not valid, we look at the assets folder
    //we clean up link, prepend the asset folder, and then check there
    char* linkDecode = urlDecode(link);
    char* linkClean = cleanupPath(linkDecode);
    char* linkAsset = prependAssetFolder(linkClean);

    bool fileSent = sendFile(linkAsset, clientfd, extraData);
    free(linkClean);
    free(linkAsset);
    free(linkDecode);

    if (!fileSent) {
        printf("stylesheet not sent\n");
        return false;
    }


    success:
    return true;

}

//maybe include extraVariables here who knows
//but this will involve some encoding
siteVar* offloadToVariables(char* offload) {
    cottageCheck(NULL);
    if (!offload || strlen(offload) <= 0) return NULL;

    //before starting, we need to format our offload.
    //it has weird symbols, particularily with strings
    //so lets change this
    
    // offload = urlDecode(offload);
    // printf("url decoded offload is %s\n", offload);
    
    //look for equals and question marks
    
    char key[512] = {0};
    char value[512] = {0};

    size_t index = 0;

    siteVar* variables = siteVarInit("variables", COMPOSITE, 0, NULL);

    bool state = false;
    printf("offload to variables start\n");
    for (size_t i = 0; i < strlen(offload); i++) {
        if (offload[i] == '=') {
            if (state == false) {
                strcpy(key, value);
                memset(value, 0, index);
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
                printf("key is %s, value is %s\n", key, value);
                VARTYPE type = BC_StrToType(value);
                if (type != ERROR) {

                    void* data = NULL;
                    char* strData = NULL; //for strings
                    switch (type) {
                        case UINT: {
                            printf("offload to variables uint\n");
                            data = malloc(sizeof(uint_cot));
                            memcpy(data, &((uint_cot){BC_StrToUInt(value)}), sizeof(uint_cot));
                            break;
                        }
                        case INT: {
                            printf("offload to variables int\n");
                            data = malloc(sizeof(int_cot));
                            memcpy(data, &((int_cot){BC_StrToInt(value)}), sizeof(int_cot));
                            break;
                        }
                        case FLOAT: {
                            printf("offload to variables float\n");
                            data = malloc(sizeof(float_cot));
                            memcpy(data, &((float_cot){BC_StrToFloat(value)}), sizeof(float_cot));
                            break;
                        }
                        case STRING: {
                            printf("offload to variables string\n");
                            char* strData = BC_StrToStr(value);
                            if (strData)
                            {
                                data = malloc(sizeof(string_cot));
                                memcpy(data, &strData, sizeof(string_cot));
                                //free(strData);
                            }
                            break;
                        }
                        case BOOL: {
                            printf("offload to variables bool\n");
                            data = malloc(sizeof(bool_cot));
                            memcpy(data, &((bool_cot){BC_StrToBool(value)}), sizeof(bool_cot));
                            break;
                        }
                        default: {
                            printf("offload to variables error\n");
                            siteVarFree(variables);
                            return NULL;
                        }
                    }

                    siteVarCompositeInsertNew(&variables, key, type, 1, data);
                    if (strData) free(strData);
                    free(data);
                }
                else {

                    //what else could it be?
                    //it could be an array, so we'll take that into account.
                    if (BC_isArray(value)) {
                        siteVar* array = BC_ArrayToSiteVar(key, value, NULL);
                        if (array) {
                            siteVarCompositeInsert(&variables, array);
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
        printf("key is %s, value is %s\n", key, value);
        if (type != ERROR) {

            void* data = NULL;
            char* strData = NULL;

            //first, check if its an array
            

            switch (type) {
                case UINT: {
                    printf("offload to variables uint\n");
                    data = malloc(sizeof(uint_cot));
                    memcpy(data, &((uint_cot){BC_StrToUInt(value)}), sizeof(uint_cot));
                    break;
                }
                case INT: {
                    printf("offload to variables int\n");
                    data = malloc(sizeof(int_cot));
                    memcpy(data, &((int_cot){BC_StrToInt(value)}), sizeof(int_cot));
                    printf("int data was %i\n", *(int_cot**)data);
                    break;
                }
                case FLOAT: {
                    printf("offload to variables float\n");
                    data = malloc(sizeof(float_cot));
                    memcpy(data, &((float_cot){BC_StrToFloat(value)}), sizeof(float_cot));
                    break;
                }
                case STRING: {
                    printf("its a string yo\n");
                    strData = BC_StrToStr(value);
                    if (strData)
                    {
                        data = malloc(sizeof(string_cot));
                        memcpy(data, &strData, sizeof(string_cot));
                        //free(strData);
                    }
                    break;
                }
                case BOOL: {
                    printf("offload to variables bool\n");
                    data = malloc(sizeof(bool_cot));
                    memcpy(data, &((bool_cot){BC_StrToBool(value)}), sizeof(bool_cot));
                    break;
                }
                default: {
                    printf("offload to variables error\n");
                    siteVarFree(variables);
                    return NULL;
                }
            }
            printf("hi\n");
            //printf("data was %s\n", *(char**)data);
            bool status = siteVarCompositeInsertNew(&variables, key, type, 1, data);
            printf("offload to variables no\n");
            if (strData) free(strData);
            free(data);
        }
        else {

            //what else could it be?
            //it could be an array, so we'll take that into account.
            if (BC_isArray(value)) {
                siteVar* array = BC_ArrayToSiteVar(key, value, NULL);
                if (array) {
                    printf("offload to variables, its an array\n");
                    siteVarCompositeInsert(&variables, array);
                }
                else printf("offload to variables, not a valid array");
            }
            else {
                //otherwise, its a variable name.
                //since these are the base variables, we must return an error here
                //or ignore the variable
                printf("yikes\n");
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

    printf("offload to variables end\n");
    return variables;
}

//handle request is good, but we can provide default functions for each type of request as well.
//not all of them probably, but GET is a good start


bool defaultGet(HttpRequest request, int clientfd, siteVar* extraVariables, char* filePath) {
    cottageCheck(false);
    if (!HttpRequestValid(request) || request.type != GET) return false;

    //printf("default get\n");
    //filePath has our direct link.
    //we need our offload though, we can use strstr for this conveniently

    char* offloadPosition = strstr(request.target, "?");

    char* offload = NULL;
    if (offloadPosition && offloadPosition[1]) offload = offloadPosition + 1;

    //decode the url, and then send it.
    char* offloadDecode = urlDecode(offload);
    char* offloadClean = cleanupPath(offloadDecode);

    siteVar* offloadVars = offloadToVariables(offloadClean);
    siteVarCompositeInsert(&offloadVars, extraVariables);
    
    bool state = sendFile(filePath, clientfd, offloadVars);

    siteVarFree(offloadVars);
    if (offloadDecode) free(offloadDecode);
    if (offloadClean) free(offloadClean);
    
    return state;
}



#endif