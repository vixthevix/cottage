#ifndef SERVERSOURCE
#define SERVERSOURCE

//basic utilities
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

//needed for sockets
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


//TCP stuff
int getMainFD(const char* address, const char* port, bool passive) {
    struct addrinfo settings, *results;
    int status, fd;
    
    memset(&settings, 0, sizeof(struct addrinfo));
    settings.ai_family = AF_UNSPEC;
    settings.ai_socktype = SOCK_STREAM; //TCP
    if (passive) settings.ai_flags = AI_PASSIVE;

    if (!passive) status = getaddrinfo(address, port, &settings, &results);
    else status = getaddrinfo(NULL, port, &settings, &results);

    if (status < 0) {
        printf("error getting addrinfo\n");
        freeaddrinfo(results);
        return -1;
    }


    fd = socket(results->ai_family, results->ai_socktype, results->ai_protocol);
    if (fd <= -1) {
        printf("error getting fd\n");
        freeaddrinfo(results);
        return -1;
    }

    //free the port for other programs so its safe to use for this one
    //the last two parameters are for setting the change to true (1) ie yeah make the change 
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &((int){1}), sizeof(int)) <= -1) {
        printf("error setting reuse address option for socket\n");
        freeaddrinfo(results);
        return -1;
    }

    //now bind
    if (bind(fd, results->ai_addr, results->ai_addrlen) <= -1) {
        printf("error binding\n");
        freeaddrinfo(results);
        return -1;
    }

    //now we are done

    freeaddrinfo(results);
    return fd;
}



//there are nine RESTful http requests that are commonly used:
//GET, PUT, POST, DELETE, PATCH, HEAD, OPTIONS, TRACE, CONNECT
//we will develop a meaningful response to each of these

#define ERROR -1
#define GET 0
#define PUT 1
#define POST 2
#define DELETE 3
#define PATCH 4
#define HEAD 5
#define OPTIONS 6
#define TRACE 7
#define CONNECT 8

//we will also encapsulate a request in a struct, allong with additional data if needed

typedef struct clientreq {
    char type;
    char* data;
    unsigned int dataSize;
} clientreq;

void clientreqfree(clientreq req) {
    if (req.data) free(req.data);
}

//function and structs for spliting GET data
//for now it will just return the file.
//stuff to add:
//  clean up filepath to remove all instances of . or ..
//  get querys (?) working

//lets do this properly with a struct

//make it a hashmap
typedef struct queryPair {
    char* key;
    char* value;
    int pd; //used in round robin hashing
} queryPair;

typedef struct queryMap {
    queryPair** items;
    unsigned int count;
    unsigned int capacity;
} queryMap;


queryPair* queryInit(char* key, char* value) {
    queryPair* newpair = (queryPair*) calloc(1, sizeof(queryPair));
    //queryPair consists of a copy of key and value
    newpair->key = (char*) calloc(strlen(key) + 1, sizeof(char));
    newpair->value = (char*) calloc(strlen(value) + 1, sizeof(char));
    strcpy(newpair->key, key);
    strcpy(newpair->value, value);
    newpair->pd = 0;
    return newpair;
}

void queryFree(queryPair* query) {
    if (query->key) free(query->key);
    if (query->value) free(query->value);
    query->pd = -1;
    free(query);
}


queryMap* qmapNewSize(const unsigned int oldSize) {
    const unsigned int newSize = oldSize << 1; //doubled
    queryMap* newmap = malloc(sizeof(queryMap));
    newmap->count = 0;
    newmap->capacity = newSize;
    newmap->items = (queryPair**) calloc(newSize, sizeof(queryPair*));

    return newmap;
}

queryMap* qmapInit() {
    const unsigned int initSize = 8 >> 1; //start with size of 16
    return qmapNewSize(initSize);
}

int qmapFree(queryMap* qmap) {
    //free every query
    for (unsigned int i = 0; i < qmap->capacity; i++) {
        if (qmap->items[i]) queryFree(qmap->items[i]);
    }
    free(qmap->items);
    free(qmap);
    
    return 1;
}

int qmapInsert(queryMap* qmap, char* key, char* value);


queryMap* qmapResize(queryMap* qmap) { //currently only resize upwards, since deleting items isnt in the current scope
    queryMap* newmap = qmapNewSize(qmap->capacity);

    newmap->count = qmap->count;

    for (unsigned int i = 0; i < qmap->capacity; i++) {
        if (qmap->items[i]) { 
            char* newkey = (char*) malloc(strlen(qmap->items[i]->key));
            strcpy(newkey, qmap->items[i]->key);
            
            char* newval = (char*) malloc(strlen(qmap->items[i]->value));
            strcpy(newval, qmap->items[i]->value);

            qmapInsert(newmap, newkey, newval);
    
        }
    }
    qmapFree(qmap);

    return newmap;

}

//will use djb2 hashing algorithm
unsigned int qhash(queryPair* pair) {
    unsigned int hash = 5381; //magic number
    int c;
    
    char* key = pair->key;

    while (c = *key++) //for each character in the string
        hash = ((hash << 5) + hash) + c; //hash * 33 + c

    return hash;
}





int qmapInsert(queryMap* qmap, char* key, char* value) {
    const unsigned int load = qmap->count * 100 / qmap->capacity;
    if (load > 60) qmap = qmapResize(qmap);
    
    queryPair* newpair = queryInit(key, value);
    unsigned int initpos = qhash(newpair) % qmap->capacity;
    unsigned int index;
    queryPair* curpair;

    for (unsigned int i = 0; i < qmap->capacity; i++) {
        index = (initpos + i) % qmap->capacity;
        curpair = qmap->items[index];

        if (curpair == NULL) { //empty
            qmap->items[index] = newpair;
            qmap->count++;
            return 1;
        }

        if (strcmp(curpair->key, key) == 0) {//value with same key so replace
            queryFree(qmap->items[index]);
            qmap->items[index] = newpair;
            return 1;
        }

        if (newpair->pd > curpair->pd) { //round robin
            qmap->items[index] = newpair;
            newpair = curpair;
        }

        newpair->pd++;


    }
    
    //in the case things do go wrong
    queryFree(newpair);
    return 0;

}






char* qmapGet(queryMap* qmap, char* key) {
    //queryPair* newpair = queryInit(key, value);
    unsigned int initpos = qhash(&(queryPair){.key = key, .value = NULL}) % qmap->capacity;
    //printf("initpos get\n");
    unsigned int index;
    queryPair* curpair;
    int curpd = 0;

    for (unsigned int i = 0; i < qmap->capacity; i++) {
        //printf("loop start\n");
        index = (initpos + i) % qmap->capacity;
        curpair = qmap->items[index];

        if (curpair == NULL || curpair->pd < curpd) return NULL;
        
        if (strcmp(curpair->key, key) == 0) return curpair->value;

        curpd++;
        //printf("loop end\n");
    }

    return NULL;
}

typedef struct getSplit {
    char* link;
    unsigned int linkSize;
    
    queryMap* variables;

} getSplit;

getSplit splitGET(clientreq request) {
    getSplit target = {.link = NULL, .linkSize = 0, .variables = NULL};
    if (request.type != GET || request.data == NULL) {
        return target;
    }


    //we copy into a buffer until ? appears
    char* buffer = (char*) calloc(request.dataSize, sizeof(char));
    int i = 0;
    for (; i < request.dataSize && request.data[i] != '?'; i++) {
        buffer[i] = request.data[i];
    }

    target.linkSize = strlen(buffer) + 1;
    buffer = (char*) realloc(buffer, (target.linkSize) * sizeof(char));
    target.link = buffer;


    //now we can work on queries.
    //we are going to treat this as a key value pair, both strings.
    //this makes it flexible for the server developer to use them
    //we should bump up i to be after the ?
    
    //first check if there is any more data to read
    i++;
    if (i >= request.dataSize) return target;
    
    target.variables = qmapInit();

    //now we have to look through the queries and follow two rules:
    //start with a key until an = sign, then its the corresponding value
    //stop reading when & sign encountered, then new key.
    //end at the end of data
    
    //probably best to make the query list into a query hashmap instead.
    
    //bump up i to go out of '?'
    int initSize = 32;
    char* current; //the current key/value
    int ci = 0;
    queryPair* curq;
    char state = 0; //0 is key, 1 is value
    
    current = (char*) calloc(initSize, sizeof(char));
    
    char* keybuffer = (char*) calloc(initSize, sizeof(char));

    for (; i < request.dataSize; i++) {
        if (request.data[i] == '=') {
            if (state == 0) {
                //we switch to value
                strcpy(keybuffer, current);
                memset(current, 0, initSize);
                state = 1;
                ci = 0;
                printf("state 0\n");
            }
            else { //encountered bad query
                qmapFree(target.variables);
                target.variables = NULL;
                return target;
            }
        }
        else if (request.data[i] == '&') {
            if (state == 1) {
                //we switch back to key and make new key-value
                char* newkey = (char*) calloc(strlen(keybuffer), sizeof(char));
                char* newvalue = (char*) calloc(strlen(current), sizeof(char));

                strcpy(newkey, keybuffer);
                memset(keybuffer, 0, initSize);
                strcpy(newvalue, current);
                memset(current, 0, initSize);
                ci = 0;

                qmapInsert(target.variables, newkey, newvalue);
                printf("state 1\n");
                state = 0;
            }
            else { //encountered bad query
                qmapFree(target.variables);
                target.variables = NULL;
                return target;
            }
        }
        else {
            current[ci] = request.data[i];
            ci++;
            if (ci >= initSize) {
                initSize <<= 1;
                current = (char*) realloc(current, initSize);
                keybuffer = (char*) realloc(current, initSize);
            }
        }
    }

    //after for loop is done, we check if we are in state 1 to add on data, otherwise bad query

    if (state == 1) {
        char* newkey = (char*) calloc(strlen(keybuffer), sizeof(char));
        char* newvalue = (char*) calloc(strlen(current), sizeof(char));
        strcpy(newkey, keybuffer);
        memset(keybuffer, 0, initSize);
        strcpy(newvalue, current);
        memset(current, 0, initSize);
        ci = 0;

        qmapInsert(target.variables, newkey, newvalue);
    }
    else {
        qmapFree(target.variables);
        target.variables = NULL;
    }

    return target;
}

//function for processing messages sent from the server
clientreq getClientRequest(int client) {
    //by default, assume an error has occured
    clientreq request = {.type = ERROR, .data = NULL, .dataSize = 0};
    
    const int bufferSize = 2048;

    char buffer[bufferSize];
    int bytesrecv = recv(client, buffer, bufferSize, 0);
    if (bytesrecv > 0) buffer[bufferSize - 1] = 0; //to allow reading
    else return request; //end it there if nothing has been recieved

    //now we have to parse the buffer
    //the first word of the request will always be what type it is (GET, POST etc.)
    //so we just have to check what this actually is.
    //the longest request is CONNECT/OPTIONS with 7 characters, not a lot. so we can while loop until a space appears

    char rqtype[8] = {0}; //8 for terminator
    int bindex = 0; //buffer index

    while (buffer[bindex] != ' ') {
        rqtype[bindex] = buffer[bindex];
        bindex++;
    }
    

    //check it!!
    //cant use switch since its not a single value which slows stuff down
    if (strcmp(rqtype, "GET") == 0) {
        //typically with GET, after the space, we have a string until another space.
        //this is our data
        request.type = GET;

        unsigned int initSize = 1024;
        unsigned int getReadIndex = 0; //index to follow
        request.data = (char*) calloc(initSize, sizeof(char));
        
        //our bindex is currently on the space, so we bump it up once
        bindex++;

        while (buffer[bindex] != ' ') {
            request.data[getReadIndex] = buffer[bindex];
            bindex++;
            getReadIndex++;

            //check the index to update buffer if needed.
            if (getReadIndex == initSize - 1) {
                initSize <<= 1; //double it
                request.data = (char*) realloc(request.data, initSize * sizeof(char));
            }
        }

        //finally, realloc the data back to a smaller size
        request.data = (char*) realloc(request.data, (getReadIndex + 1) * sizeof(char));
        request.dataSize = getReadIndex + 1;
    }
    
    return request;
}



//lets make a function for sending over an html file
int sendHTML(const char* filepath, int client, queryMap* variables) {
    //first, prepare the html
    FILE* file = fopen(filepath, "r");

    //get the size of the file and fread it into a buffer

    fseek(file, 0, SEEK_END);
    unsigned int size = ftell(file);
    fseek(file, 0, SEEK_SET);


    //now send the HTTP response, it must be in a specific format
    //first, the header
    const char* header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";
    
    send(client, header, strlen(header), 0);

    //we will try write to data using a for loop, to keep track of our frontend shenanigans
    char* data = (char*) calloc(size + 1, sizeof(char));
    int c = 0, i = 0;

    //use a stack to keep track of brackets
    char bracketStack[50] = {0};
    int bsi = 0;

    while ((c = fgetc(file)) != EOF) {
        if (c == '{') {
            bracketStack[bsi] = c;
            bsi++;
            //the simplest way to do this is using a COMMAND:VARIABLE(S) system.
            //some example commands can be VAR (get a variable value) IF (conditional html) and INSERT (putting in other HTML files)
            //INSERT could potentially take in parameters to transfer variables over.
            //on that note, having a STORE (creating a new variable and putting it in the current variable map) could be nice.
        }
        else if (c == '}') {
            bracketStack[bsi] = 0;
            bsi--;
        }
        else {
            data[i] = (char)c;
            i++;
        }
    }
    data[i] = 0;

    send(client, data, strlen(data), 0);
    

    free(data);
    fclose(file);
    return 1;
}

//typedef int error;
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
