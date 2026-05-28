#ifndef SERVERSOURCE
#define SERVERSOURCE

//basic utilities
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

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

/*
To summarise what they do
GET -> returns a resource specified by the link
PUT -> updates a resource specified by the link
POST -> creates a new resource with the link name
DELETE -> deletes a resource specified by the link
PATCH ->
HEAD ->
OPTIONS ->
TRACE ->
CONNECT ->
*/

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
    printf("key is %s\n", key);

    printf("begin hash\n");
    while ((c = *key++)) //for each character in the string
        hash = ((hash << 5) + hash) + c; //hash * 33 + c

    printf("hash got: %u\n", hash);
    return hash;
}





int qmapInsert(queryMap* qmap, char* key, char* value) {
    if (!key || !value || !qmap) return 1;
    
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
    //printf("qmap get starting\n");
    //if (key) printf("key valid\n");

    queryPair temp = {.key = key, .value = NULL};
    unsigned int hashed = qhash(&temp);
    unsigned int initpos = hashed % qmap->capacity;
    
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

//combining two queryMaps
queryMap* qmapCombine(queryMap* intruder, queryMap* home) {

    //we create the new map first, then populate with the right data if only intruder or home is valid. this is to ensure unique pointers
    queryMap* new = qmapInit();

    if (!home && !intruder) {
        qmapFree(new);
        return NULL;
    }
    
    if (!intruder && home) {
        for (int i = 0; i < home->capacity; i++) {
            queryPair* cur = home->items[i];
            if (!cur) continue;
            //will create new strings, so two new pointers. no worry of deletion
            qmapInsert(new, cur->key, cur->value);
        }
        return new;
    }
    if (!home && intruder) {
        for (int i = 0; i < intruder->capacity; i++) {
            queryPair* cur = intruder->items[i];
            if (!cur) continue;

            qmapInsert(new, cur->key, cur->value);
        }
        return new;
    }

    //qmapInsert works off existing pointers, so we have to make new ones here.
    //we insert the intruder into home
    //using a new qmap

    //queryMap* new = qmapInit();

    //first we populate new with values from home
    for (int i = 0; i < home->capacity; i++) {
        queryPair* cur = home->items[i];
        if (!cur) continue;
        //will create new strings, so two new pointers. no worry of deletion
        qmapInsert(new, cur->key, cur->value);
    }

    //then we insert the intruder into home
    for (int i = 0; i < intruder->capacity; i++) {
        queryPair* cur = intruder->items[i];
        if (!cur) continue;

        qmapInsert(new, cur->key, cur->value);
    }

    //finally we free both original maps
    //or do we? for now no lets give the programmer some control here

    return new;
}

//post consists of a link, maybe some variables
//and most importantly, the input variables
typedef struct postSplit {
    char* link;
    unsigned int linkSize;
    //no normal variables for now

    queryMap* input;
} postSplit;

postSplit splitPOST(clientreq request) {
    postSplit target = {.link = NULL, .linkSize = 0, .input = NULL};
    if (request.type != POST || request.data == NULL) return target;

    //our data starts at the link, so we will copy this first
    char* buffer = (char*) calloc(request.dataSize, sizeof(char));
    int i = 0;
    for (; i < request.dataSize && request.data[i] != ' '; i++) {
        buffer[i] = request.data[i];
    }
    target.linkSize = strlen(buffer) + 1;
    buffer = (char*) realloc(buffer, (target.linkSize * sizeof(char)));
    target.link = buffer;

    //now we have to find our carriage return, and put each variable into input.

    char* mainData = strstr(request.data, "\r\n\r\n");
    if (mainData) { //if exists
        mainData += 4; //we move past carriage returns
        //we can now safely start our input reading
        //variable name ends at =, variable value ends at &
        //we stop all reading once we reached \0
        target.input = qmapInit();
        const int initSize = 256;
        char* curVal = (char*) calloc(initSize, sizeof(char));
        char* curKey = (char*) calloc(initSize, sizeof(char));
        int curIndex = 0;
        bool isCurVal = false;
        for (i = 0; i < strlen(mainData); i++) {
            if (mainData[i] == '=') {
                if (!isCurVal) {
                    curIndex = 0;
                    isCurVal = true;
                }
                else { //bad query
                    qmapFree(target.input);
                    target.input = NULL;
                    return target;
                }
            }
            else if (mainData[i] == '&') {
                if (curVal) {
                    //we have our data, so now we insert
                    qmapInsert(target.input, curKey, curVal);
                    curIndex = 0;
                    isCurVal = false;
                    memset(curVal, 0, strlen(curVal));
                    memset(curKey, 0, strlen(curKey));
                }
                else { //bad query
                    qmapFree(target.input);
                    target.input = NULL;
                    return target;
                }
            }
            else {
                if (isCurVal) {
                    curVal[curIndex++] = mainData[i];
                }
                else {
                    curKey[curIndex++] = mainData[i];
                }
            }
        }

        //check if we were reading a value, and if we have a key.
        //if so, assign it
        if (isCurVal && strlen(curKey) != 0) {
            qmapInsert(target.input, curKey, curVal);
            curIndex = 0;
            isCurVal = false;
            memset(curVal, 0, strlen(curVal));
            memset(curKey, 0, strlen(curKey));
        }
        free(curVal);
        free(curKey);
        
    }

    return target;
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
    else if (strcmp(rqtype, "POST") == 0) {
        request.type = POST;

        unsigned int initSize = 1024;
        unsigned int getReadIndex = 0; //index to follow
        request.data = (char*) calloc(initSize, sizeof(char));
        
        //our bindex is currently on the space, so we bump it up once
        bindex++;

        while (buffer[bindex] != 0) {
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

//following functions are for string boolean calculations

void BC_putAt(char* exp, int i, char c) {
	if (i < 0) return;
	
	//shift everything up 1
	for (int j = strlen(exp); j != i; j--) {
		exp[j] = exp[j - 1];
	}
	exp[i] = c;

}

void BC_delAt(char* exp, int i) {
	if (i < 0 || i >= strlen(exp)) return;
	
	//shift everything up 1
	int n = strlen(exp);
	for (int j = i; j < strlen(exp); j++) {
		exp[j] = exp[j + 1];
	}
}

bool BC_isDoubleOperator(char c) {
	return (c == '=' || c == '&' || c == '|' || c == '^'); //special case for ! potentially
}

//removes all spaces, and turns double operators into single ones. also performs check.
char* BC_format(char* expression) {

	char* new = (char*) calloc(strlen(expression) + 1, sizeof(char));
	strcpy(new, expression);
		
	//first format spaces.
	int n = strlen(new);
	for (int i = 0; i < strlen(new);) {
		if (new[i] == ' ') BC_delAt(new, i);
		else i++;
	}

	//now format and check for operators
	n = strlen(new);
	bool opFound = false;
	char op = 0;
	for (int i = 0; i < strlen(new);) {
		char c = new[i];
		
		if (BC_isDoubleOperator(c)) {
			if (opFound) {
				if (c != op) {
					free(new);
					return NULL;
				}
				else BC_delAt(new, i);
				opFound = false;
			}
			else {
				opFound = true;
				op = c;
				i++;
			}
		}
		else {
			if (opFound) { //no corresponding double
				free(new);
				return NULL;
			}
			i++;
			opFound = false;
		}
	}
	
	return new;
}

bool BC_isOperator(char c) {
	return (c == '<' || c == '>' || c == '=' || c == '&' || c == '|' || c == '^'); //special case for ! potentially
}


char* BC_transform(const char* expression) {
	int size = strlen(expression);
	char* result = (char*) calloc(size * 2, sizeof(char));
	int resultIndex = 0;
	char stack[100];
	int top = -1;

	bool numFound = false;

	for (int i = size - 1; i >= 0; i--) {
		char c = expression[i];

		if (isalnum(c)) {
			if (!numFound) result[resultIndex++] = '"';
			
			numFound = true;
			result[resultIndex++] = c;
		}
		
		else {
			if (numFound) result[resultIndex++] = '"';
			numFound = false;
			
			if (c == ')') stack[++top] = c;
			
			else if (c == '(') {
				//until the start of the stack or a closing bracket, empty the stack into result.
				while (top != -1 && stack[top] != ')') {
					result[resultIndex++] = stack[top--];
				}
				if (top != -1) top--; //pop ')'

			}

			else if (BC_isOperator(c)) {
				//until the start of stack or a non-operator, empty the stack into result.
				while (top != -1 && BC_isOperator(stack[top])) {
					result[resultIndex++] = stack[top--];
				}
				stack[++top] = c; //add the operator in
			}
			else if (c == '!') {
				result[resultIndex++] = c;
			}
		}

	}

	//empty remaining stack into result
	while (top != -1) result[resultIndex++] = stack[top--];

	result[resultIndex] = 0;

	//reverse to get the right prefix.
	
	return result;
}

//for now just checks for unsigned integer. later make checks for floats and negatives
bool BC_isNum(char* exp) {
    for (int i = 0; i < strlen(exp); i++) {
        int digit = exp[i] - '0';
        if (0 <= digit && digit <= 9) continue;
        else return false;
    }
    return true;

}

int BC_StrToNum(char* exp) {
    int number = 0;
    int mult = 1;
    for (int i = 0; i < strlen(exp); i++) {
        char c = exp[i];
        number += ((c - '0') * mult);
        number *= 10;
        printf("strToNum i is %i\n", i);
    }
    number /= 10;
    return number;

}

/*
things to add.
negative number checking
floating point number checking
string comparison (includes single character strings)
*/

bool BC_evaluate(const char* expression, queryMap* variables) {
	int stack[100] = {0};
	int sp = -1;

	int n = strlen(expression);
	for (int i = 0; i < n; i++) {
		char c = expression[i];

		if (c == '"') {
            // first, check if its a number. if so, direct translate into int.
            // otherwise, check the variables. if match, try to translate.
            // if this fails, must mean that variable is not an number, so we return false;
			int number = 0;
			int mult = 1;
            char value[100] = {0};
			
            while ((c = expression[++i]) != '"'){
                BC_putAt(value, 0, c);
                printf("c is %c\n", c);
            } 
            printf("expression is '%s'\n", value);
            if (BC_isNum(value)) {
                number = BC_StrToNum(value);
                printf("expression -> number becomes %i\n", number);
            }	
            else { //must be a variable
                char* varVal = qmapGet(variables, value);
                if (BC_isNum(varVal)) number = BC_StrToNum(varVal);
                else return 0; //the variable is invalid for analysing, so we simply make the statement null.
            }

            stack[++sp] = number;
		}
		else if (c == '!') {
			stack[sp] = !stack[sp];
		}
		else if (BC_isOperator(c)) {
			//must have at least two numbers in here
			if (sp < 1) return false;
			int a = stack[sp--];
			int b = stack[sp--];
            printf("a is %i, b is %i\n", a, b);
			
			switch (c) {
				case '<': stack[++sp] = (a < b); break;
				case '>': stack[++sp] = (a > b); break;
				case '&': stack[++sp] = (a && b); break;
				case '|': stack[++sp] = (a || b); break;
				case '=': stack[++sp] = (a == b); break;
				case '^': stack[++sp] = (a != b); break;
			}
			
		}
	}

	if (sp > 0) return false;
	
	return stack[0];
}

bool sendNormal(char* filepath, char* type, int client) {

    char header[128] = {0};
    char* headerptr = header;
    sprintf(headerptr, 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: %s\r\n"
    "Connection: close\r\n\r\n", type);

    send(client, header, strlen(header), 0);

    //read the binary first
    FILE* file = fopen(filepath, "rb");
    if (!file) return false;

    //get the size
    fseek(file, 0, SEEK_END);
    const unsigned long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    //read into buffer, then send
    char* buffer = (char*) calloc(size, sizeof(char));
    if (!buffer) {
        fclose(file);
        return false;
    }
    fread(buffer, sizeof(char), size, file);

    send(client, buffer, size, 0);

    free(buffer);
    fclose(file);

    return true;
}

//recursive function for opening a file
//need it to open a file within a file
//just copy paste stuff over
char* openHTML(const char* filepath, queryMap* variables) {
    //first, prepare the html
    FILE* file = fopen(filepath, "r");
    if (!file) return NULL;

    //get the size of the file and fread it into a buffer

    fseek(file, 0, SEEK_END);
    unsigned int size = ftell(file);
    fseek(file, 0, SEEK_SET);

    //we will try write to data using a for loop, to keep track of our frontend shenanigans
    char* data = (char*) calloc(size + 1, sizeof(char));
    int c = 0, i = 0;

    //use a stack to keep track of brackets
    //char bracketStack[50] = {0};
    int bsi = 0;
    bool inCheck = false;
    bool reachedColon = false;
    int curlyCount = 0;
    bool curlyCounting = false;

    bool finishedEmbedRead = false;

    //use for inserting a file
    bool insertVars = false;
    //with the input parameters, you have to define name and value
    //this is your key and value.
    //if value is not a number, will check the current variables
    //if not found there, do not put in the new variables
    //otherwise, yes.

    queryMap* newVariables = qmapInit();

    //use a diamond bracket count to know when to read curlies
    int diamondCount = 0;

    //we need to use booleans to keep track of {}, mainly due to the IF statements, and nested IF statements.
    //use an if count to know which nest we are in
    //we also have to ensure that ELSE only appears once, after all ELSEIFs and IF
    //we make a boolean to show this happened.
    //If an ELSE appears before the other two, throw an error.
    int ifCount = 0;
    bool ifValid = true;
    bool elseValid = true;
    bool elseIfValid = true;
    int ifInvalidState = 0;
    int ifValidState = 0;
    bool elseAppeared = false;
    bool ifAppeared = false;

    int mode = -1;
    const int
    commandSize = 50,
    offloadSize = 100;
    char* command = (char*) calloc(commandSize, sizeof(char));
    char* offload = (char*) calloc(offloadSize, sizeof(char));
    int ci = 0, oi = 0;


    while ((c = fgetc(file)) != EOF) {
        if (inCheck) {
            if (c != ':' && c != '}') {
                command[ci++] = c;
            }
            else {
                ci = 0;
                reachedColon = true;
                inCheck = false;
            }
        }
        else if (reachedColon) {
            reachedColon = false;
            //check the command
            printf("command is %s\n", command);

            mode = -1;
            
            //lets consider else here
            if (!strcmp(command, "ELSE")) {
                printf("reached else, ifCount is %i, ifInvalidState is %i, ifValid is %i\n", ifCount, ifInvalidState, ifValid);
                //we have to be in the same ifInvalidState, and ifValid must be false
                if (ifCount == ifInvalidState && !ifValid) {
                    // if (!ifValid) {
                    //     printf("else happening\n");
                    //     ifValid = true;
                    // }
                    printf("else happening\n");
                    ifValid = true;
                }
                //if we are in a different state, then we know that the if passed
                //so the else fails.
                else ifValid = false;
                elseAppeared = true;
                finishedEmbedRead = true;
                oi = 0;
                memset(command, 0, commandSize * sizeof(char));
                memset(offload, 0, offloadSize * sizeof(char));
            }
            else if (!strcmp(command, "ELSE-IF")) {
                //kinda works like else, first check if an else has appeared
                if (elseAppeared) goto failure;
                if (!ifAppeared) goto failure;
                
                if (ifCount == ifInvalidState && !ifValid) {
                    // if (!ifValid) {
                    //     printf("else if happening\n");
                    //     mode = 2;
                    //     goto mode2;
                    // }
                    printf("else if happening\n");
                    mode = 2;
                    goto mode2;
                }
                else ifValid = false;
                finishedEmbedRead = true;
                oi = 0;
                memset(command, 0, commandSize * sizeof(char));
                memset(offload, 0, offloadSize * sizeof(char));
            }
            else if (!strcmp(command, "ENDIF")) {                
                if (ifCount == ifInvalidState) {
                    ifValid = true;
                    elseValid = true;
                    elseIfValid = true;
                }
                ifAppeared = false;
                elseAppeared = false;
                ifCount--;
                mode = -1;
                finishedEmbedRead = true;
                oi = 0;
                memset(command, 0, commandSize * sizeof(char));
                memset(offload, 0, offloadSize * sizeof(char));
                //mode = 2;
            }
            else if (!strcmp(command, "IF")) {
                ifCount++;
                if (elseAppeared) goto failure;
                ifAppeared = true;
                //assuming that we are in a nested if
                if (ifValid) {
                    mode = 2;
                    goto mode2;
                }
                finishedEmbedRead = true;
                oi = 0;
                memset(command, 0, commandSize * sizeof(char));
                memset(offload, 0, offloadSize * sizeof(char));
            }
            
            else if (!ifValid) mode = -1;
            
            else if (!strcmp(command, "VAR")) {
                mode = 0;
                goto mode0;
            }
            else if (!strcmp(command, "INSERT")) {
                mode = 1;
                goto mode1;
            }
            //add more cases here
            else {
                mode = -1;
                goto failure;
            }
        }
        else if (mode == 0) {
            mode0:
            //read the variable
            if (c != '}') {
                offload[oi++] = c;
            }
            else {
                finishedEmbedRead = true;
                printf("offload is %s\n", offload);
                oi = 0;
                //find in variables
                //first check variables is initialised
                if (!variables) goto failure;
                //printf("offload is %s\n", offload);
                char* value = qmapGet(variables, offload);
                if (value) {
                    //write into data
                    for (int j = 0; j < strlen(value); j++, i++) {
                        data[i] = value[j];
                    }
                }
                else {
                    //printf("no value found\n");
                    goto failure;
                }
                mode = -1;
                memset(command, 0, commandSize * sizeof(char));
                memset(offload, 0, offloadSize * sizeof(char));
            }
        }
        else if (mode == 1) {
            mode1:
            //printf("yeah\n");
            //we need to read the filepath, and the variables.
            if (c != '}') {
                offload[oi++] = c;
            }
            else {
                finishedEmbedRead = true;
                printf("offload is %s\n", offload);
                oi = 0;
                //first get the link
                const int linkSize = offloadSize;
                char* link = (char*) calloc(linkSize, sizeof(char));
                int j = 0;
                for (j = 0; offload[j] != 0 && offload[j] != ';'; j++) {
                    link[j] = offload[j];
                    //printf("current link is %s\n", link);
                }
                printf("link is %s\n", link);
                if (offload[j] == 0) { //no input variables
                    printf("im going\n");
                    //just read the data
                    goto readINPUT;
                }
                //here we put stuff into newVariables
                if (!newVariables) goto readINPUT;

                //we now have to put things inside the newVariables.
                //loop through the remainder of offload and find variables
                char 
                curVar[100] = {0},
                curValue[100] = {0};
                int curVarIndex = 0, curValueIndex = 0;
                bool isVar = true;

                //keep track of quotes
                bool inQuotes = false;
                
                j++;
                for (; offload[j] != 0; j++) {
                    if (offload[j] == '"') {
                        inQuotes = !inQuotes;
                    }
                    else if (offload[j] == '=' && isVar) {
                        isVar = false;
                        continue;
                    }
                    else if (offload[j] == ',' && !isVar && !inQuotes) {
                        isVar = true;

                        //we have a value and pair
                        if (curVar && curValue) {
                            //first, check if its a number
                            //a string, or a variable
                            if (BC_isNum(curValue)) {
                                qmapInsert(newVariables, curVar, curValue);
                            }
                            else if (curValue[0] == '"' && curValue[strlen(curValue) - 1] == '"') {
                                //remove the quote marks
                                BC_delAt(curValue, 0);
                                BC_delAt(curValue, strlen(curValue) - 1);
                                qmapInsert(newVariables, curVar, curValue);
                            }
                            else { //mut be variable
                                char* x = qmapGet(variables, curValue);
                                if (x) {
                                    qmapInsert(newVariables, curVar, x);
                                }
                                else {
                                    //do nothing, because nothing can be done
                                }
                            }
                        }

                        //reset both
                        memset(curVar, 0, 100);
                        curVarIndex = 0;
                        memset(curValue, 0, 100);
                        curValueIndex = 0;
                        continue;
                    }

                    if (isVar) {
                        curVar[curVarIndex++] = offload[j];
                    }
                    else {
                        curValue[curValueIndex++] = offload[j];
                    }
                }
                //put in anything left
                if (!isVar) {
                    if (curVar && curValue) {
                        //first, check if its a number
                        //a string, or a variable
                        if (BC_isNum(curValue)) {
                            qmapInsert(newVariables, curVar, curValue);
                        }
                        else if (curValue[0] == '"' && curValue[strlen(curValue) - 1] == '"') {
                            //remove the quote marks
                            BC_delAt(curValue, 0);
                            BC_delAt(curValue, strlen(curValue) - 1);
                            qmapInsert(newVariables, curVar, curValue);
                        }
                        else { //mut be variable
                            char* x = qmapGet(variables, curValue);
                            if (x) {
                                qmapInsert(newVariables, curVar, x);
                            }
                            else {
                                //do nothing, because nothing can be done
                            }
                        }
                    }
                }

                readINPUT:
                char* dataINPUT = openHTML(link, newVariables);
                free(link);
                //copy over the new data
                if (dataINPUT) {
                    printf("input data got\n");
                    printf("%s\n", dataINPUT);
                    //we need to reallocate our data to take into account
                    //increases in size
                    data = (char*) realloc(data, size + (strlen(dataINPUT) << 1));
                    for (j = 0; j < strlen(dataINPUT); j++, i++) {
                        data[i] = dataINPUT[j];
                    }
                    printf("dataINPUT read done\n");
                }
                else {
                    printf("input data not got\n");
                    goto failure;
                } 
                free(dataINPUT);
                mode = -1;
                memset(command, 0, commandSize * sizeof(char));
                memset(offload, 0, offloadSize * sizeof(char));

            }

        }
        else if (mode == 2) {
            mode2:
            if (c != '}') offload[oi++] = c;
            else {
                finishedEmbedRead = true;
                oi = 0;
                //we now have a boolean expression. lets evaluate it.
                if (offload) {
                    char* formatted = BC_format(offload);
                    char* transformed = BC_transform(formatted);
                    int result = BC_evaluate(transformed, variables);
                    printf("formatted is %s, transformed is %s, result is %i\n", formatted, transformed, result);
                    
                    if (result) {
                        ifValid = true;
                        ifValidState = ifCount;
                    }
                    else {
                        ifValid = false;
                        //we have to skip until we have reached the next 
                        ifInvalidState = ifCount;
                    }
                    
                    free(formatted);
                    free(transformed);
                }
                mode = -1;
                memset(command, 0, commandSize * sizeof(char));
                memset(offload, 0, offloadSize * sizeof(char));
            }
        }
        
        else if (c == '}' && finishedEmbedRead && !diamondCount) { //to ensure the final bracket is skipped
            finishedEmbedRead = false;
        }

        else if (c == '{' && !diamondCount) {
            bsi++;
            //the simplest way to do this is using a COMMAND:VARIABLE(S) system.
            //some example commands can be VAR (get a variable value) IF (conditional html) and INSERT (putting in other HTML files)
            //INSERT could potentially take in parameters to transfer variables over.
            //on that note, having a STORE (creating a new variable and putting it in the current variable map) could be nice.
            
            //we must check ahead here to see if there is another curly in front
            //we must use fgetc to see the future.
            //if its a curly, we set inCheck and continue on.
            //if not, we must revert c and goto data write

            char future = fgetc(file);
            // if (future == EOF) goto failure;
            if (future == '{') {
                inCheck = true;
            }
            else {
                fseek(file, -1, SEEK_CUR);
                goto dataWrite;
            }
        }
        // else if (c == '}') {
        //     bsi--;
        //     inCheck = false;
        // }
        else {
            dataWrite:
            if (ifValid) {
                if (c == '<') diamondCount++;
                else if (c == '>') diamondCount--;
                data[i] = (char)c;
                i++;
            }
        }
    }
    goto success;
    
    failure:
    printf("failed\n");
    free(data);
    data = NULL;

    success:
    fclose(file);
    free(offload);
    free(command);
    qmapFree(newVariables);
    if (data) {
        //set safety null terminator
        data[i] = 0;
        return data;
    }
    else {
        return NULL;
    }
    
}


//lets make a function for sending over an html file
int sendHTML(const char* filepath, int client, queryMap* variables) {
    //first, prepare the html

    char* data = openHTML(filepath, variables);

    if (data) goto success;
    
    failure:
    printf("failed\n");
    data = NULL;

    success:
 
    if (data) {
        //now send the HTTP response, it must be in a specific format
        //first, the header
        const char* header = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n";
        send(client, header, strlen(header), 0);


        //then the data
        send(client, data, strlen(data), 0);


        free(data);
        return 1;
    }
    else return 0;
    
}

//we need a function to get a requested resource
//this can be html, an image, js, whatever
//to do this, we check the file endings
bool sendFile(char* filepath, int client, queryMap* vars) {
    //first, check if this file actually exists
    if (access(filepath, F_OK) == 0) {
        //second, read the file ending
        if (strstr(filepath, ".png")) return sendNormal(filepath, "image/png", client);
        if (strstr(filepath, ".gif")) return sendNormal(filepath, "image/gif", client);
        if (strstr(filepath, ".jpg") || strstr(filepath, ".jpeg")) return sendNormal(filepath, "image/jpeg", client);
        if (strstr(filepath, ".css")) return sendNormal(filepath, "text/css", client);
        if (strstr(filepath, ".js")) return sendNormal(filepath, "text/javascript", client);
        if (strstr(filepath, ".html")) return sendHTML(filepath, client, vars);
        return sendNormal(filepath, "text/plain", client);
    }
    else {
        return false;
    }
}

int sendRediret(const char* path, int client) {
    char response[512];
    int len = sprintf(response,
        "HTTP/1.1 303 See Other\r\n"
        "Location: %s\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "\r\n",
        path);
    send(client, response, len, 0);
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
