/*
Functions for splitting a HTTP request, and routing.

Code is part of the cottage framework (https://github.com/vixthevix/cottage)
*/

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
#include "error_cot.h"
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

// Function prototypes
HTTPTYPE StrToHTTPTYPE(char* data);
float StrToHttpVersion(char* data);
void HttpRequestFree(HttpRequest request);
bool HttpRequestValid(HttpRequest request);
cotResult splitHttpRequest(HttpRequest* input, char* data);
int hexToInt(char hex);
char* urlDecode(char* offload);
bool handleRequest(HttpRequest request, int clientfd, siteVar* extraData, RouteMap* routes);
void* INTERNAL_StrToData(string_cot value, VARTYPE type);
siteVar* offloadToVariables(char* offload);
bool defaultGet(HttpRequest request, int clientfd, siteVar* extraVariables, char* filePath);

#if defined(COTTAGE_START)

/*
Converts HTTP request type string into enum value.
@arg data -> HTTP request type in string form.
@return HTTP request type in enum form.
*/
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

/*
Converts HTTP version string into enum value.
@arg data -> HTTP version in string form.
@return HTTP version in enum form.
*/
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

/*
Frees HttpRequest from memory.
@arg request -> target to free.
*/
void HttpRequestFree(HttpRequest request) {
    cottageCheck();
    if (request.target) free(request.target);
    if (request.options) strMapFree(request.options);
    if (request.payload) free(request.payload);
}

/*
Checks if a HttpRequest is valid for interpreting.
@arg request -> target to analyze.
@return if valid.
*/
bool HttpRequestValid(HttpRequest request) {
    cottageCheck(false);
    return (request.target && request.options && (request.type > UNKNOWN) && (request.version > -1));
}

/*
Reads a HTTP request in string form, and stores in HttpRequest.
@arg input -> stores new HttpRequest.
@arg data -> HTTP request in string form.
@return error status of split/
*/
cotResult splitHttpRequest(HttpRequest* input, char* data) {
    cottageCheck(newResultError("splitHttpRequest: cottage not initialised."));
    if (!input) return newResultError("splitHttpRequest: input is empty");
    HttpRequest error = {
        .target = NULL,
        .options = NULL,
        .payload = NULL,
        .version = -1,
        .type = UNKNOWN
    };

    HttpRequest request = {
        .target = NULL,
        .options = NULL,
        .payload = NULL,
        .version = -1,
        .type = UNKNOWN
    };

    //check for valid data
    if (!data || strlen(data) == 0) {
        *input = error;
        return newResultError("splitHttpRequest: data is invalid");
    }

    //First line has type, target and version, separated by spaces.
    //Request lines also end with \r\n
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

            item += 1;
            if (item == 3) break; //got all 3 pieces of data from the line
        }
    }

    if ((data[dataIndex] == '\r' && data[dataIndex + 1] == '\n') || dataIndex >= dataLen) dataIndex += 2; //carriage return
    else {
        HttpRequestFree(request);
        *input = error;
        return newResultError("splitHttpRequest: data header is formatted incorrectly");
    }

    //For the following, we have multiple lines.
    //option:value separated by colon, lines separted by \r\n
    //with a final \r\n
    //We should treat this final \r\n as its own line, so we should check the beginning

    request.options = strMapInit();

    while (dataIndex < (dataLen - 1)) {
        if (data[dataIndex] != '\r' && data[dataIndex + 1] != '\n') {
            buffer[bufferIndex++] = data[dataIndex++];
        }
        else {
            if (!buffer[0]) { //is buffer empty?
                break;
            }
            dataIndex += 2; //skip \r\n

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
            strMapInsert(&request.options, option, value);
            memset(buffer, 0, bufferIndex);
            memset(value, 0, 512);
            memset(option, 0, 512);
            bufferIndex = 0;
        }
    }
    //stringmap is now filled up
    if ((data[dataIndex] == '\r' && data[dataIndex + 1] == '\n') || dataIndex >= dataLen) dataIndex += 2;
    else {
        HttpRequestFree(request);
        *input = error;
        return newResultError("splitHttpRequest: data options are formatted incorrectly");
    }

    //Finally we have our offload.
    //Copy it over as it can be interpreted in different ways depending on request type.
    //It can also be empty.
    if (dataLen > dataIndex) { //Do we have enough space in the buffer for an offload?
        request.payload = (char*) malloc(dataLen - dataIndex + 1);
        memcpy(request.payload, data + dataIndex, dataLen - dataIndex);
        request.payload[strlen(request.payload)] = 0;
    }
    else request.payload = NULL;

    *input = request;
    return newResultOK();
}

/*
Converts a hexadecimal character in char form into its decimal integer counterpart.
@arg hex -> hex character to convert.
@return decimal integer counterpart.
*/
int hexToInt(char hex) {
    if ('0' <= hex && hex <= '9') return hex - '0';
    if ('a' <= hex && hex <= 'f') return hex - 'a' + 10; //a=10
    if ('A' <= hex && hex <= 'F') return hex - 'A' + 10; //A=10
    return -1; //invalid 
}

/*
Decodes the URL for a file into a cottage readable format.
@arg offload -> raw URL string.
@return decoded URL string.
*/
char* urlDecode(char* offload) {
    cottageCheck(NULL);
    //not sure if I should do a strlen check here.
    if (!offload) return NULL;

    char* decoded = (char*) calloc(strlen(offload) + 1, sizeof(char));
    size_t j = 0;

    size_t i = 0;
    while (offload[i]) {
        if (offload[i] == '%' && offload[i + 1] && offload[i + 2]) { //Special % format.
            //convert into hex
            int high = hexToInt(offload[i + 1]);
            int low = hexToInt(offload[i + 2]);
            if (high >= 0 && low >= 0) {
                char byte = (char)((high << 4) | low);

                decoded[j++] = byte;
                i += 3;
                continue;
            }
        }
        else if (offload[i] == '+') { //Special space format.
            decoded[j++] = ' ';
            i++;
            continue;
        }

        //normal character
        decoded[j++] = offload[i++];
    }
    
    decoded = (char*)realloc(decoded, strlen(decoded) + 1);
    return decoded;
}

/*
Reads a HttpRequest and performs routing appropriately.
@arg request -> encapsulated HTTP request.
@arg clientfd -> client to send data to.
@arg extraData -> external site variables for use.
@arg routes -> route table to be checked.
*/
bool handleRequest(HttpRequest request, int clientfd, siteVar* extraData, RouteMap* routes) {
    cottageCheck(false);
    if (!HttpRequestValid(request) || !routes) return false;

    //HTTP target is split into link and offload, separated by '?'
    char link[512] = {0};
    int index = 0;
    while (index < strlen(request.target) && request.target[index] != '?') {
        link[index] = request.target[index];
        index++;
    }

    //Handle routing
    RouteEntry route = RouteMapGet(routes, link);
    switch (request.type) {
        case GET: {
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
            break;
        }
    }

    //If no routing occurred, treat the link as an asset request.
    char* linkDecode = urlDecode(link);
    char* linkClean = cleanupPath(linkDecode);
    char* linkAsset = prependAssetFolder(linkClean);

    bool fileSent = sendFile(linkAsset, clientfd, extraData);
    free(linkClean);
    free(linkAsset);
    free(linkDecode);

    if (!fileSent) {
        newResultError("handleRequest: GET for asset failed");
        return false;
    }

    success:
    return true;
}

/*
Converts data in string form into raw form.
@arg value -> data in string form.
@arg type -> type of data.
@return raw data.
*/
void* INTERNAL_StrToData(string_cot value, VARTYPE type) {
    cottageCheck(NULL);
    if (!value || strlen(value) == 0) return NULL;
    if (type == ERROR) return NULL;

    void* data = NULL;
    char* strData = NULL; //for strings
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
            data = malloc(sizeof(bool_cot));
            memcpy(data, &((bool_cot){BC_StrToBool(value)}), sizeof(bool_cot));
            break;
        }
        default: {
            newResultError("INTERNAL_StrToData: variable is invalid");
            return NULL;
        }
    }
    return data;
}

/*
Converts HTTP target offload into a COMPOSITE siteVar.
@arg offload -> HTTP target offload.
@return COMPOSITE siteVar.
*/
siteVar* offloadToVariables(char* offload) {
    cottageCheck(NULL);
    if (!offload || strlen(offload) <= 0) {
        newResultError("offloadToVariables: offload is empty");
        return NULL;
    }
    
    char key[512] = {0};
    char value[512] = {0};

    size_t index = 0;

    siteVar* variables = siteVarInit("variables", COMPOSITE, 0, NULL);
    if (!variables) {
        newResultError("offloadToVariables: could not initialise variables");
        return NULL;
    }
    
    bool state = false;
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
                newResultError("offloadToVariables: offload formatted incorrectly");
                return NULL;
            }
        }
        else if (offload[i] == '&') {
            if (state == true) {
                //we have to get the type of our data, then insert it
                //key remains the same
                VARTYPE type = BC_StrToType(value);
                void* data = INTERNAL_StrToData(value, type);
                if (data) {
                    bool status = siteVarCompositeInsertNew(&variables, key, type, 1, data);
                    if (!status) newResultError("offloadToVariables: could not insert into variables");
                    if (type == STRING) {
                        string_cot* strData = (string_cot*)data;
                        free(*strData);
                    }
                    free(data);
                }
                else {
                    //what else could it be?
                    //it could be an array, so we'll take that into account.
                    if (BC_isArray(value)) {
                        siteVar* array = NULL;
                        if (BC_ArrayToSiteVar(&array, key, value, NULL).status == COT_OK) {
                            siteVarCompositeInsert(&variables, array);
                            siteVarFree(array);
                        }
                    }
                    else {
                        //otherwise, its a variable name.
                        //since these are the base variables, we must return an error here
                        //or ignore the variable
                        siteVarFree(variables);
                        newResultError("offloadToVariables: variable data is invalid");
                        return NULL;
                    }

                    //alternatively, we can interprete this as a string,
                    //but feels kind of weird, let kutaj decide
                }
                memset(key, 0, strlen(key));
                memset(value, 0, strlen(value));
                index = 0;
                state = false;
            }
            else { //bad query
                siteVarFree(variables);
                newResultError("offloadToVariables: offload formatted incorrectly");
                return NULL;
            }
        }
        else {
            if (index < 511) value[index++] = offload[i];
        }
    }
    
    //Do we have any data left to insert?
    if (state == true && strlen(key) > 0 && strlen(value) > 0) {
        //we have to get the type of our data, then insert it
        //key remains the same
        VARTYPE type = BC_StrToType(value);
        void* data = INTERNAL_StrToData(value, type);
        if (data) {
            bool status = siteVarCompositeInsertNew(&variables, key, type, 1, data);
            if (!status) newResultError("offloadToVariables: could not insert into variables");
            if (type == STRING) {
                string_cot* strData = (string_cot*)data;
                free(*strData);
            }
            free(data);
        }
        else {
            //what else could it be?
            //it could be an array, so we'll take that into account.
            if (BC_isArray(value)) {
                siteVar* array = NULL;
                if (BC_ArrayToSiteVar(&array, key, value, NULL).status == COT_OK) {
                    siteVarCompositeInsert(&variables, array);
                    siteVarFree(array);
                }
            }
            else {
                //otherwise, its a variable name.
                //since these are the base variables, we must return an error here
                //or ignore the variable
                siteVarFree(variables);
                newResultError("offloadToVariables: variable data is invalid");
                return NULL;
            }

            //alternatively, we can interprete this as a string,
            //but feels kind of weird, let kutaj decide
        }
        memset(key, 0, strlen(key));
        memset(value, 0, strlen(value));
        index = 0;
        state = false;
    }
    else { //bad query
        siteVarFree(variables);
        newResultError("offloadToVariables: offload formatted incorrectly");
        return NULL;
    }

    return variables;
}

/*
Standard response function to GET request.
@arg request -> encapsulated HTTP request.
@arg clientfd -> client to send data to.
@arg extraVariables -> external site variables for use.
@arg filePath -> file to send in response to request.
*/
bool defaultGet(HttpRequest request, int clientfd, siteVar* extraVariables, char* filePath) {
    cottageCheck(false);
    if (!HttpRequestValid(request) || request.type != GET) return false;

    //filePath has our direct link.
    //we need our offload though, we can use strstr for this conveniently

    char* offloadPosition = strstr(request.target, "?");

    char* offload = NULL;
    if (offloadPosition && offloadPosition[1]) offload = offloadPosition + 1;

    //decode the url, and then send it.
    char* offloadDecode = urlDecode(offload);
    char* offloadClean = cleanupPath(offloadDecode);

    siteVar* offloadVars = offloadToVariables(offloadClean);
    //In the case we have no offload
    if (!offloadVars) offloadVars = siteVarInit("variables", COMPOSITE, 0, NULL);
    siteVarCompositeInsert(&offloadVars, extraVariables);
    
    bool state = sendFile(filePath, clientfd, offloadVars);

    siteVarFree(offloadVars);
    if (offloadDecode) free(offloadDecode);
    if (offloadClean) free(offloadClean);

    if (!state) newResultError("defaultGet: could not send file");
    
    return state;
}

#endif
#endif