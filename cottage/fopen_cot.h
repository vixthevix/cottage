#ifndef FOPEN_COT
#define FOPEN_COT

#include "conversion_cot.h"
#include "dependencies_cot.h"
#include "boolcalc_cot.h"
#include "error_cot.h"
#include "sitevar_cot.h"
#include "init_cot.h"

bool sendNormal(char* filepath, char* type, int client) {
    cottageCheck(false);
    char header[128] = {0};
    char* headerptr = header;
    sprintf(headerptr, 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: %s\r\n"
    "Connection: close\r\n\r\n", type);

    send(client, header, strlen(header), 0);

    //read the binary first
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        newResultError("sendNormal: filepath not found");
        return false;
    }

    //get the size
    fseek(file, 0, SEEK_END);
    const unsigned long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    //read into buffer, then send
    char* buffer = (char*) calloc(size, sizeof(char));
    if (!buffer) {
        newResultError("sendNormal: buffer for file unable to be made");
        fclose(file);
        return false;
    }
    fread(buffer, sizeof(char), size, file);

    send(client, buffer, size, 0);

    free(buffer);
    fclose(file);

    return true;
}

typedef struct dataVector {
    char* data;
    uint32_t capacity;
    uint32_t index;
} dataVector;

dataVector dataVectorInit(uint32_t capacity) {
    if (capacity == 0) {
        return (dataVector){0, 0, 0};
    }
    
    dataVector target = {
        .data = (char*) calloc(capacity, sizeof(char)),
        .capacity = capacity,
        .index = 0,
    };

    return target;
}

bool dataVectorPush(dataVector* target, char c) {
    uint32_t load = target->index / target->capacity * 100;
    if (load > 60) {
        target->capacity <<= 1;
        target->data = (char*) realloc(target->data, target->capacity * sizeof(char));
        if (!target->data) return false;
    }

    target->data[target->index++] = c;
    return true;
}


typedef struct conditionalState {
    bool valid;
    bool ifAppeared;
    bool elseAppeared;
    bool chainSuccess;
} conditionalState;

typedef struct loopState {
    siteVar* iterator;
    void* list;
    VARTYPE listType;
    uint_cot count;
    uint_cot cur;
    bool listComposite;
    uint32_t returnIndex;
} loopState;

//recursive function for opening a file
//need it to open a file within a file
//just copy paste stuff over

cotResult openHTML(char** input, const char* filepath, siteVar* variables) {
    //cottageCheck(NULL);
    //first, prepare the html
    FILE* file = fopen(filepath, "r");
    if (!file) return newResultError("openHTML: filepath not found");

    //get the size of the file and fread it into a buffer

    fseek(file, 0, SEEK_END);
    unsigned int size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* fullFile = (char*) malloc(size);
    fread(fullFile, sizeof(char), size, file);
    //fseek(file, 0, SEEK_SET);
    uint32_t fi = 0;

    unsigned int curDataSize = size + 1;
    dataVector data = dataVectorInit(curDataSize);

    //we will try write to data using a for loop, to keep track of our frontend shenanigans
    //char* data = (char*) calloc(curDataSize, sizeof(char));
    int c = 0, di = 0;

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

    // siteVar* newVariables = siteVarInit("newVariables", COMPOSITE, 0, NULL);

    //use a diamond bracket count to know when to read curlies
    int diamondCount = 0;

    //we need to use booleans to keep track of {}, mainly due to the IF statements, and nested IF statements.
    //use an if count to know which nest we are in
    //we also have to ensure that ELSE only appears once, after all ELSEIFs and IF
    //we make a boolean to show this happened.
    //If an ELSE appears before the other two, throw an error.
    // int ifCount = 0;
    // bool ifValid = true;
    // bool elseValid = true;
    // bool elseIfValid = true;
    // int ifInvalidState = 0;
    // int ifValidState = 0;
    // bool elseAppeared = false;
    // bool ifAppeared = false;

    //lets use an array to keep track of if statements
    conditionalState states[512] = {0};
    int curCondState = 0;
    //layer 0 is the base plane. here, all initial if statements
    //are safe to check.
    states[0].valid = true; states[0].ifAppeared = false; states[0].elseAppeared = false; states[0].chainSuccess = false;

    //array to keep track of loops
    loopState loopStates[512] = {0};
    int curLoopState = -1;
    //layer 0 is the base state. here, everything is 0
    //we dont need to perform any checks or send any signals, since there
    //is only one entrance and exit for a for loop
    const int 
    modeNONE = -1,
    modeVAR = 0,
    modeINSERT = 1,
    modeIF = 2,
    modeFOR = 3;

    int mode = -1;
    const int
    commandSize = 50,
    offloadSize = 100;
    char* command = (char*) calloc(commandSize, sizeof(char));
    char* offload = (char*) calloc(offloadSize, sizeof(char));
    int ci = 0, oi = 0;

    while (fi < size) {
        c = fullFile[fi++];
    //while ((c = fgetc(file)) != EOF) {
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
            mode = modeNONE;
            
            if (!strcmp(command, "IF")) {
                curCondState++;
                if (curCondState <= 0) {
                    newResultError("openHTML: current conditional state is too low.");
                    goto failure;
                }
                if (curCondState >= 512) {
                    newResultError("openHTML: current conditional state is too high.");
                    goto failure;
                }

                memset(&states[curCondState], 0, sizeof(conditionalState));
                states[curCondState].ifAppeared = true;

                if (states[curCondState - 1].valid) {
                    mode = modeIF;
                    goto jumpIF;   
                }
                else {
                    //if the prev state is false, we have to skip everything.
                    //to signal this to the other branches, we set chainSuccess to be true
                    //(it will act like it is)
                    states[curCondState].chainSuccess = true;
                }
                finishedEmbedRead = true;
                oi = 0;
                memset(command, 0, commandSize * sizeof(char));
                memset(offload, 0, offloadSize * sizeof(char));


                // ifCount++;
                // if (ifCount <= 0) goto failure;
                // //if (elseAppeared) goto failure;
                // ifAppeared = true;
                // //assuming that we are in a nested if
                // if (ifValid) {
                //     mode = 2;
                //     goto jumpIF;
                // }
                // finishedEmbedRead = true;
                // oi = 0;
                // memset(command, 0, commandSize * sizeof(char));
                // memset(offload, 0, offloadSize * sizeof(char));
            }
            else if (!strcmp(command, "ELSE-IF")) {                
                if (!states[curCondState].ifAppeared || states[curCondState].elseAppeared) {
                    newResultError("openHTML: ELSE-IF in invalid spot.");
                    goto failure;
                }
                
                //assume not valid
                states[curCondState].valid = false;

                //are we in a valid prev state, and has there been a success signal from earlier?
                if (states[curCondState - 1].valid && !states[curCondState].chainSuccess) {
                    mode = modeIF;
                    goto jumpIF;
                }
                //mode = -1;
                finishedEmbedRead = true;
                oi = 0;
                memset(command, 0, commandSize * sizeof(char));
                memset(offload, 0, offloadSize * sizeof(char));

                //kinda works like else, first check if an else has appeared
                // if (elseAppeared) goto failure;
                // if (!ifAppeared) goto failure;
                
                // if (ifStates[ifCount] == false) {
                //     // if (!ifValid) {
                //     //     mode = 2;
                //     //     goto jumpIF;
                //     // }
                //     mode = 2;
                //     goto jumpIF;
                // }
                // else ifValid = false;
                // finishedEmbedRead = true;
                // oi = 0;
                // memset(command, 0, commandSize * sizeof(char));
                // memset(offload, 0, offloadSize * sizeof(char));
            }
            else if (!strcmp(command, "ELSE")) {
                
                if (!states[curCondState].ifAppeared || states[curCondState].elseAppeared) {
                    newResultError("openHTML: ELSE in invalid spot.");
                    goto failure;
                }
                
                states[curCondState].elseAppeared = true;

                //has the chain signal happened, and are we in a valid prevstate?
                if (states[curCondState - 1].valid && !states[curCondState].chainSuccess) {
                    states[curCondState].valid = true;
                    states[curCondState].chainSuccess = true;
                }
                else states[curCondState].valid = false;
                
                finishedEmbedRead = true;
                oi = 0;
                memset(command, 0, commandSize * sizeof(char));
                memset(offload, 0, offloadSize * sizeof(char));
                
                //we have to be in the same ifInvalidState, and ifValid must be false
                
                //this code executes if ifStates[ifCount] is false
                // if (!ifAppeared) goto failure;
                // if (elseAppeared) goto failure;
                // ifStates[ifCount] = !ifStates[ifCount];

                // if (ifStates[ifCount] == false) {
                //     ifValid = true;
                //     ifStates[ifCount] = true;
                // }
                // else ifValid = false;
                
                // // if (ifCount == ifInvalidState && !ifValid) {
                // //     // if (!ifValid) {
                // //     //     ifValid = true;
                // //     // }
                // //     ifValid = true;
                // // }
                // // //if we are in a different state, then we know that the if passed
                // // //so the else fails.
                // // else ifValid = false;
                // elseAppeared = true;
                // finishedEmbedRead = true;
                // oi = 0;
                // memset(command, 0, commandSize * sizeof(char));
                // memset(offload, 0, offloadSize * sizeof(char));
            }
            else if (!strcmp(command, "ENDIF")) {  
                
                if (!states[curCondState].ifAppeared) {
                    newResultError("openHTML: ENDIF appeared before IF.");
                    goto failure;
                }

                curCondState--;
                if (curCondState < 0) {
                    newResultError("openHTML: current conditional state too low.");
                    goto failure;
                }
                
                mode = modeNONE;
                finishedEmbedRead = true;
                oi = 0;
                memset(command, 0, commandSize * sizeof(char));
                memset(offload, 0, offloadSize * sizeof(char));
                
                //if (!ifAppeared) goto failure;
                //if (ifCount == ifInvalidState || ifCount == ifValidState) {
                // ifValid = true;
                // elseValid = true;
                // elseIfValid = true;
                // //}
                // ifAppeared = false;
                // elseAppeared = false;
                // ifCount--;
                // if (ifCount < 0) goto failure;
                // mode = -1;
                // finishedEmbedRead = true;
                // oi = 0;
                // memset(command, 0, commandSize * sizeof(char));
                // memset(offload, 0, offloadSize * sizeof(char));
                //mode = 2;
            }
            
            // else if (!ifValid) mode = -1;
            else if (!states[curCondState].valid) mode = modeNONE;

            //FOR LOOP STUFF
            else if (!strcmp(command, "FOR")) {
                curLoopState++;
                if (curLoopState < 0) {
                    newResultError("openHTML: current loop state is too low.");
                    goto failure;
                }
                if (curLoopState >= 512) {
                    newResultError("openHTML: current loop state is too high.");
                    goto failure;
                }
                mode = modeFOR;
                goto jumpFOR;
            }
            else if (!strcmp(command, "ENDFOR")) {
                if (curLoopState <= -1) {
                    newResultError("openHTML: ENDFOR appeared before FOR.");
                    goto failure;
                }
                loopStates[curLoopState].cur++;
                if (loopStates[curLoopState].cur == loopStates[curLoopState].count) {
                    //the end.
                    //we delete the iterator from our variables, and go down a loopstate
                    siteVarCompositeDelete(&variables, loopStates[curLoopState].iterator->name);
                    loopStates[curLoopState].iterator = NULL;
                    curLoopState--;
                }
                else {
                    //otherwise, update the iterator, and set di to where we need to be
                    size_t varSize = INTERNAL_siteVarTypeSize(loopStates[curLoopState].listType);
                    void* value = &loopStates[curLoopState].list[loopStates[curLoopState].cur * varSize];
                    siteVarUpdate(loopStates[curLoopState].iterator, value);
                    fi = loopStates[curLoopState].returnIndex;
                }
                
                mode = modeNONE;
                finishedEmbedRead = true;
                oi = 0;
                memset(command, 0, commandSize * sizeof(char));
                memset(offload, 0, offloadSize * sizeof(char));
            }
            
            else if (!strcmp(command, "VAR")) {
                mode = modeVAR;
                goto jumpVAR;
            }
            else if (!strcmp(command, "INSERT")) {
                mode = modeINSERT;
                goto jumpINSERT;
            }
            //add more cases here
            else {
                mode = modeNONE;
                newResultError("openHTML: invalid command found.");
                goto failure;
            }
        }
        else if (mode == modeVAR) {
            jumpVAR:
            //read the variable
            if (c != '}') {
                offload[oi++] = c;
            }
            else {
                finishedEmbedRead = true;
                oi = 0;
                //find in variables
                //first check variables is initialised
                // if (!variables) {
                //     newResultError("openHTML: VAR, variables not initialised.");
                //     goto failure;
                // }
                //char* value = qmapGet(variables, offload);

                /*
                FOR ERROR HANDLING:
                we have something like
                {{VAR:variable;"default error message"}}
                ensure this error message is a string.
                */
                
                char varName[512] = {0};
                char varError[512] = {0};
                int index = 0;
                bool semicolonFound = false;
                for (int i = 0; i < strlen(offload); i++) {
                    if (offload[i] == ';') {
                        semicolonFound = true;
                        index = 0;
                        continue;
                    }
                    if (!semicolonFound) {
                        varName[index++] = offload[i];
                    }
                    else {
                        varError[index++] = offload[i];
                    }
                }

                if (strlen(varError) > 0 && !BC_isString(varError)) {
                    newResultError("openHTML: VAR, error message is not valid string.");
                    goto failure;
                }
                //remove quotations
                BC_delAt(varError, 0);
                BC_delAt(varError, strlen(varError) - 1);

                siteVar* var = NULL;
                cotResult varResult = BC_StrToVariable(&var, varName, variables, variables);
                
                if (varResult.status == COT_ERROR){
                    //point of failure
                    if (strlen(varError) > 0) {
                        for (int j = 0; j < strlen(varError); j++, di++) {
                            //data[di] = value[j];
                            dataVectorPush(&data, varError[j]);
                        }
                        goto VARdone;
                    }
                    else {
                        newResultError("openHTML: VAR could not convert string into siteVar");
                        goto failure;
                    }
                } 
                //we need to convert this value into a string
                char* value = BC_VariableToString(var, 0);
                if (var) siteVarFree(var);
                if (value) {
                    //write into data
                    for (int j = 0; j < strlen(value); j++, di++) {
                        //data[di] = value[j];
                        dataVectorPush(&data, value[j]);
                    }
                    free(value);
                }
                else {
                    if (strlen(varError) > 0) {
                        for (int j = 0; j < strlen(varError); j++, di++) {
                            //data[di] = value[j];
                            dataVectorPush(&data, varError[j]);
                        }
                        goto VARdone;
                    }
                    else {
                        newResultError("openHTML: VAR, could not extract value from var");
                        goto failure;
                    }
                }
                VARdone:
                mode = -1;
                memset(command, 0, commandSize * sizeof(char));
                memset(offload, 0, offloadSize * sizeof(char));
            }
        }
        else if (mode == modeINSERT) {
            jumpINSERT:
            //we need to read the filepath, and the variables.
            if (c != '}') {
                offload[oi++] = c;
            }
            else {
                finishedEmbedRead = true;
                oi = 0;
                //first get the link
                const int linkSize = offloadSize;
                char* link = (char*) calloc(linkSize, sizeof(char));
                int j = 0;
                for (j = 0; offload[j] != 0 && offload[j] != ';'; j++) {
                    link[j] = offload[j];
                }
                if (offload[j] == 0) { //no input variables
                    //just read the data
                    goto readINPUT;
                }
                //here we put stuff into newVariables
                
                // if (!newVariables) goto readINPUT;
                siteVar* newVariables = siteVarInit("newVariables", COMPOSITE, 0, NULL);

                //we now have to put things inside the newVariables.
                //loop through the remainder of offload and find variables
                char 
                curVar[100] = {0},
                curValue[100] = {0};
                int curVarIndex = 0, curValueIndex = 0;
                bool isVar = true;

                //keep track of quotes
                bool inQuotes = false;

                //keep track of array
                bool inArray = false;
                
                j++;
                for (; offload[j] != 0; j++) {
                    if (offload[j] == '"') {
                        inQuotes = !inQuotes;
                    }
                    else if (offload[j] == '=' && isVar) {
                        isVar = false;
                        continue;
                    }
                    else if (offload[j] == '[') {
                        inArray = true;
                    }
                    else if (offload[j] == ']') {
                        inArray = false;
                    }
                    else if (offload[j] == ',' && !isVar && !inQuotes && !inArray) {
                        isVar = true;

                        //we have a value and pair
                        if (curVar && curValue) {

                            //curVar is the name, curValue is the value
                            //we check the basic types first (int -> string) and put in as siteVar
                            //we also check if its an array by checking for []
                            //  only single dimension arrays right now, so just check each value
                            //  you can put variables in this array
                            //  the type of the array is inferred from the first member. if a member with a different type is found,
                            //  failure.
                            //  for this, we assume numbers default to doubles, this is lowkey kind of bad design right now but oh well


                            //otherwise must be an existing variable, so we reinsert

                            //first, check if its a number
                            //a string, or a variable
                            if (BC_isUInt(curValue)) {
                                siteVarCompositeInsertNew(&newVariables, curVar, UINT, 1, &((uint_cot){BC_StrToUInt(curValue)}));
                            }
                            else if (BC_isInt(curValue)) {
                                siteVarCompositeInsertNew(&newVariables, curVar, INT, 1, &((int_cot){BC_StrToInt(curValue)}));
                            }
                            else if (BC_isFloat(curValue)) {
                                siteVarCompositeInsertNew(&newVariables, curVar, FLOAT, 1, &((float_cot){BC_StrToFloat(curValue)}));
                            }
                            else if (BC_isBool(curValue)) {
                                siteVarCompositeInsertNew(&newVariables, curVar, BOOL, 1, &((bool){BC_StrToBool(curValue)}));
                            }
                            else if (BC_isString(curValue)) {
                                //remove the quote marks
                                //BC_delAt(curValue, 0);
                                //BC_delAt(curValue, strlen(curValue) - 1);
                                //qmapInsert(newVariables, curVar, curValue);
                                siteVarCompositeInsertNew(&newVariables, curVar, STRING, 1, &((string_cot){BC_StrToStr(curValue)}));
                            }

                            else if (BC_isArray(curValue)) { //NEXT TASK
                                siteVar* storage = NULL;
                                cotResult storageResult = BC_ArrayToSiteVar(&storage, curVar, curValue, variables);
                                //now we have storage, first check if its null
                                //then put it into our thing
                                if (storageResult.status == COT_OK) {
                                    siteVarCompositeInsert(&newVariables, storage);
                                    siteVarFree(storage);
                                }
                                else {
                                    newResultError("openHTML: INSERT, conversion into array is invalid");
                                    goto failure;
                                }
                            }

                            else { //must be variable
                                //char* x = qmapGet(variables, curValue);
                                //siteVar* x = siteVarCompositeAccess(variables, curValue);
                                siteVar* x = NULL;
                                cotResult xResult = BC_StrToVariable(&x, curValue, variables, variables);
                                
                                if (xResult.status == COT_OK) {
                                    //qmapInsert(newVariables, curVar, x);
                                    siteVarCompositeInsert(&newVariables, x);
                                    siteVarFree(x);
                                }
                                else {
                                    //do nothing, because nothing can be done
                                    newResultError("openHTML: INSERT, a parameter value is invalid");
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
                //copy over from above
                //TO DO, first clean up your code and put it into separate functions
                //THEN copy it over.
                if (!isVar) {
                    if (curVar && curValue) {
                        if (BC_isUInt(curValue)) {
                            siteVarCompositeInsertNew(&newVariables, curVar, UINT, 1, &((uint_cot){BC_StrToUInt(curValue)}));
                        }
                        else if (BC_isInt(curValue)) {
                            siteVarCompositeInsertNew(&newVariables, curVar, INT, 1, &((int_cot){BC_StrToInt(curValue)}));
                        }
                        else if (BC_isFloat(curValue)) {
                            //qmapInsert(newVariables, curVar, curValue);
                            siteVarCompositeInsertNew(&newVariables, curVar, FLOAT, 1, &((float_cot){BC_StrToFloat(curValue)}));
                        }
                        else if (BC_isBool(curValue)) {
                            siteVarCompositeInsertNew(&newVariables, curVar, BOOL, 1, &((bool){BC_StrToBool(curValue)}));
                        }
                        else if (BC_isString(curValue)) {
                            //remove the quote marks
                            //BC_delAt(curValue, 0);
                            //BC_delAt(curValue, strlen(curValue) - 1);
                            //qmapInsert(newVariables, curVar, curValue);
                            char* curValueStripped = BC_StrToStr(curValue); 
                            siteVarCompositeInsertNew(&newVariables, curVar, STRING, 1, &curValueStripped);
                            
                            //try getting back the variable
                            siteVar* strAccess = siteVarCompositeAccess(newVariables, curVar);
                        }
                        else if (BC_isArray(curValue)) { //NEXT TASK
                            siteVar* storage = NULL;
                            cotResult storageResult = BC_ArrayToSiteVar(&storage, curVar, curValue, variables);
                            //now we have storage, first check if its null
                            //then put it into our thing
                            if (storageResult.status == COT_OK) {
                                //iterate over storage strings just in case
                                siteVarCompositeInsert(&newVariables, storage);
                                siteVarFree(storage);
                            }
                            else {
                                newResultError("openHTML: INSERT, conversion into array is invalid");
                                goto failure;
                            }
                        }
                        else { //must be variable
                            //char* x = qmapGet(variables, curValue);
                            //siteVar* x = siteVarCompositeAccess(variables, curValue);
                            siteVar* x = NULL;
                            cotResult xResult = BC_StrToVariable(&x, curValue, variables, variables);
                            
                            if (xResult.status == COT_OK) {
                                //qmapInsert(newVariables, curVar, x);
                                siteVarCompositeInsert(&newVariables, x);
                                siteVarFree(x);
                            }
                            else {
                                //do nothing, because nothing can be done
                                newResultError("openHTML: INSERT, a parameter value is invalid");
                            }
                        }
                    }
                }

                readINPUT:
                char* dataINPUT = NULL;
                cotResult dataINPUTResult = openHTML(&dataINPUT, link, newVariables);
                free(link);
                if (dataINPUTResult.status == COT_ERROR) {
                    newResultError("openHTML: INSERT, failed to read input file");
                    goto failure;
                }

                //reset newVariables
                if (newVariables) {
                    siteVarFree(newVariables);
                    newVariables = NULL;
                }
                //copy over the new data
                if (dataINPUT) {
                    //we need to reallocate our data to take into account
                    //increases in size
                    //curDataSize += strlen(dataINPUT);
                    //data = (char*) realloc(data, curDataSize);
                    for (j = 0; j < strlen(dataINPUT); j++, di++) {
                        //data[di] = dataINPUT[j];
                        dataVectorPush(&data, dataINPUT[j]);
                    }
                }
                else {
                    newResultError("openHTML: INSERT, dataINPUT empty for some reason");
                    goto failure;
                } 
                free(dataINPUT);
                mode = -1;
                memset(command, 0, commandSize * sizeof(char));
                memset(offload, 0, offloadSize * sizeof(char));

            }

        }
        else if (mode == modeIF) {
            jumpIF:
            if (c != '}') offload[oi++] = c;
            else {
                finishedEmbedRead = true;
                oi = 0;
                //we now have a boolean expression. lets evaluate it.
                if (offload) {
                    char* formatted = BC_format(offload);
                    char* transformed = BC_transform(formatted);
                    bool result = 0;
                    cotResult resultResult = BC_evaluate(&result, transformed, variables);
                    if (resultResult.status == COT_ERROR) {
                        newResultError("openHTML: IF, error with expression evaluation");
                        goto failure;
                    }
                    
                    
                    //has there been a chainsuccess signal?
                    if (result && !states[curCondState].chainSuccess) {
                        states[curCondState].valid = true;
                        states[curCondState].chainSuccess = true; //set the signal
                    }
                    else states[curCondState].valid = false;
                    
                    // if (result) {
                    //     ifValid = true;
                    //     ifValidState = ifCount;
                    // }
                    // else {
                    //     ifValid = false;
                    //     //we have to skip until we have reached the next 
                    //     ifInvalidState = ifCount;
                    // }
                    // ifStates[ifCount] = ifValid;

                    if (formatted) free(formatted);
                    if (transformed) free(transformed);
                }
                mode = -1;
                memset(command, 0, commandSize * sizeof(char));
                memset(offload, 0, offloadSize * sizeof(char));
            }
        }
        else if (mode == modeFOR) {
            jumpFOR:
            if (c != '}') offload[oi++] = c;
            else {
                finishedEmbedRead = true;
                oi = 0;
                if (offload) {
                    /*
                        offload is split into two sections, the iterator,
                        and the target.
                        the target can be either a set range, an array, or a siteVar.
                        if siteVar, only valid if its an array, or a composite.
                        range is in the format (start, count)
                        start is inclusive, count is exclusive.
                        maybe eventually make it (start, count, jump), where jump is how much to increment start by.
                    */
                    char iteratorName[256] = {0};
                    uint32_t i = 0;
                    while (offload[i] != ',') {
                        iteratorName[i] = offload[i];
                        i++;
                    }
                    i++;
                    //now get the target
                    char iteratorTarget[256] = {0};
                    uint32_t j = 0;
                    while (i < strlen(offload)) {
                        iteratorTarget[j++] = offload[i++];
                    }
                    
                    //now we check what target can be
                    //is it a specified range?
                    if (iteratorTarget[0] == '(' && iteratorTarget[strlen(iteratorTarget) - 1] == ')') {
                        BC_delAt(iteratorTarget, 0);
                        BC_delAt(iteratorTarget, strlen(iteratorTarget) - 1);

                        //extract start and end
                        char startString[256] = {0};
                        char countString[256] = {0};
                        int rangei = 0;
                        bool isStart = true;
                        for (int k = 0; k < strlen(iteratorTarget); k++) {
                            if (iteratorTarget[k] == ',') {
                                rangei = 0;
                                isStart = false;
                                continue;
                            }
                            if (isStart) startString[rangei++] = iteratorTarget[k];
                            else countString[rangei++] = iteratorTarget[k];
                        }

                        //we have to ensure that startString and countString are both
                        //UINTS. right now, this is for simplicity
                        if (!BC_isUInt(startString) || !BC_isUInt(countString)) goto failure;

                        uint_cot start = BC_StrToUInt(startString);
                        uint_cot count = BC_StrToUInt(countString);

                        //we have to now essentially create a new array
                        uint_cot* forArray = (uint_cot*) malloc(count * sizeof(uint_cot));
                        for (uint_cot k = 0; k < count; k++) {
                            forArray[k] = start++;
                        }

                        loopStates[curLoopState].count = count;
                        loopStates[curLoopState].cur = 0;
                        loopStates[curLoopState].list = forArray;
                        loopStates[curLoopState].listComposite = false;
                        loopStates[curLoopState].listType = UINT;
                        //right now, we are on the first inner curly bracket
                        //that means we jump to 2 ahead of where we are right now;
                        loopStates[curLoopState].returnIndex = fi + 2;

                        //we create a new iterator siteVar, initialise it to 
                        //the first value of our array, and add it to our composites

                        siteVar* iterator = siteVarInit(iteratorName, UINT, 1, &forArray[0]);
                        loopStates[curLoopState].iterator = iterator;
                        //insert this reference into our variables
                        siteVarCompositeInsertReference(&variables, iterator);



                    }
                    /*
                    if its not a range, it can be either an array, or a variable.
                    */
                    else if (BC_isArray(iteratorTarget)) {
                        //we transform the array into a sitevar
                        //maybe we dont need to? its a lot of unneccesray work
                        //to use only some of the code already written in
                        //array to site var. in the future, split it into separate
                        //extraction functions.
                        siteVar* forArrayContainer = NULL;
                        cotResult forArrayContainerResult = BC_ArrayToSiteVar(&forArrayContainer, "a", iteratorTarget, variables);
                        
                        if (forArrayContainerResult.status == COT_ERROR) {
                            newResultError("openHTML: FOR, could not convert array to siteVar");
                            goto failure;
                        } 
                        VARTYPE type = forArrayContainer->type;
                        size_t size = INTERNAL_siteVarTypeSize(type);
                        void* forArray = siteVarAccessRange(forArrayContainer, 0, forArrayContainer->arrayItemCount);

                        loopStates[curLoopState].count = forArrayContainer->arrayItemCount;
                        loopStates[curLoopState].cur = 0;
                        loopStates[curLoopState].list = forArray;
                        loopStates[curLoopState].listComposite = false;
                        loopStates[curLoopState].listType = type;
                        loopStates[curLoopState].returnIndex = fi + 2;

                        siteVar* iterator = siteVarInit(iteratorName, type, 1, &forArray[0]);
                        loopStates[curLoopState].iterator = iterator;
                        siteVarCompositeInsertReference(&variables, iterator);
                        siteVarFree(forArrayContainer);
                    }
                    else {
                        //it is a variable.
                        //with variables, we must ensure that they are either
                        //arrays or composites.
                        siteVar* var = NULL;
                        cotResult varResult = BC_StrToVariable(&var, iteratorTarget, variables, variables);
                        if (varResult.status == COT_ERROR) {
                            newResultError("openHTML: FOR, could not convert string into siteVar");
                            goto failure;
                        }
                        if (var->type == COMPOSITE) {
                            //with a composite, it does work differently.
                            //better to treat it as an array of sitevars,
                            //and have the iterator extract the first value there.
                            //but this will be something extra, not right now.
                            newResultError("openHTML: FOR, cannot access COMPOSITE siteVar (for now)");
                            goto failure;
                        }
                        else if (var->isArray) {
                            VARTYPE type = var->type;
                            size_t size = INTERNAL_siteVarTypeSize(type);
                            void* forArray = siteVarAccessRange(var, 0, var->arrayItemCount);

                            loopStates[curLoopState].count = var->arrayItemCount;
                            loopStates[curLoopState].cur = 0;
                            loopStates[curLoopState].list = forArray;
                            loopStates[curLoopState].listComposite = false;
                            loopStates[curLoopState].listType = type;
                            loopStates[curLoopState].returnIndex = fi + 2;

                            siteVar* iterator = siteVarInit(iteratorName, type, 1, &forArray[0]);
                            loopStates[curLoopState].iterator = iterator;
                            siteVarCompositeInsertReference(&variables, iterator);
                        }
                        else {
                            newResultError("openHTML: FOR, target of loop is not an array");
                            goto failure;
                        }
                    }
                    //else goto failure;

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

            // char future = fgetc(file);
            char future = fullFile[fi++];
            // if (future == EOF) goto failure;
            if (future == '{') {
                inCheck = true;
            }
            else {
                //fseek(file, -1, SEEK_CUR);
                fi--;
                goto dataWrite;
            }
        }
        // else if (c == '}') {
        //     bsi--;
        //     inCheck = false;
        // }
        else {
            dataWrite:
            if (states[curCondState].valid) {
                if (c == '<') diamondCount++;
                else if (c == '>') diamondCount--;
                dataVectorPush(&data, (char)c);
                di++;
            }
        }
    }
    goto success;
    
    failure:
    free(data.data);
    data.data = NULL;

    success:
    fclose(file);
    free(offload);
    free(command);
    free(fullFile);
    //qmapFree(newVariables);

    //ensure that the state indexes are where they should be
    if (curCondState != 0 || curLoopState != -1) {
        //failure
        newResultError("openHTML: conditional/loop state invalid");
        free(data.data);
        data.data = NULL;
    }

    if (data.data) {
        //set safety null terminator
        data.data[data.index] = 0;
        *input = data.data;
        return newResultOK();
    }
    else {
        return newResultError("openHTML: failed to openHTML");
    }
}


//lets make a function for sending over an html file
bool sendHTML(const char* filepath, int client, siteVar* variables) {
    cottageCheck(false);
    //first, prepare the html
    char* data = NULL;
    if (openHTML(&data, filepath, variables).status == COT_ERROR) {
        return false;
    }

    if (data) goto success;
    
    failure:
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
        return true;
    }
    else return false;
    
}

//we need a function to get a requested resource
//this can be html, an image, js, whatever
//to do this, we check the file endings
bool sendFile(char* filepath, int client, siteVar* vars) {
    cottageCheck(false);
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

bool sendRediret(const char* path, int client) {
    cottageCheck(false);
    char response[512];
    int len = sprintf(response,
        "HTTP/1.1 303 See Other\r\n"
        "Location: %s\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "\r\n",
        path);
    send(client, response, len, 0);
    return true;
}

#endif