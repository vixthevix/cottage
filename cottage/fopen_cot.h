/*
Functions for opening and sending files over HTTP.

Code is part of the cottage framework (https://github.com/vixthevix/cottage)
*/

#ifndef FOPEN_COT
#define FOPEN_COT

#include "conversion_cot.h"
#include "dependencies_cot.h"
#include "boolcalc_cot.h"
#include "error_cot.h"
#include "sitevar_cot.h"
#include "init_cot.h"
#include <cstdio>

/*
Sends a non-HTML file to a client.
@arg filepath -> location of file to send.
@arg type -> HTTP content type.
@arg client -> fd to send data to.
@return status of send.
*/
bool sendNormal(char* filepath, char* type, int client) {
    cottageCheck(false);
    char header[128] = {0};
    sprintf(header, 
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

/*
Struct for dynamically storing and resizing a character array.
*/
typedef struct dataVector {
    char* data;
    uint32_t capacity;
    uint32_t index;
} dataVector;

/*
Initialises a dataVector.
@arg capacity -> initial capacity to give.
@return new dataVector.
*/
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

/*
Pushes a character onto a dataVector.
@arg target -> dataVector to push character onto.
@arg c -> character to push.
@return status of push.
*/
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

/*
Struct to give information on current HTML IF statement shell.
@part valid -> result of IF expression evaluation.
@part ifAppeared -> IF statement appeared already?
@part elseAppeared -> ELSE statement appeared already?
@part chainSuccess -> signal for future ELSE-IF and ELSE to perform correctly.
*/
typedef struct conditionalState {
    bool valid;
    bool ifAppeared;
    bool elseAppeared;
    bool chainSuccess;
} conditionalState;

/*
Struct to give information on current HTML FOR statement shell.
@part iterator -> current FOR variable.
@part list -> data for iterator to read from.
@part listType -> data type of list.
@part count -> number of elements in list.
@part cur -> current element to read in list.
@part listComposite -> is the list actually a COMPOSITE? (unused)
@part returnIndex -> return point in HTML file at end of FOR loop.
*/
typedef struct loopState {
    siteVar* iterator;
    void* list;
    VARTYPE listType;
    uint_cot count;
    uint_cot cur;
    bool listComposite;
    uint32_t returnIndex;
} loopState;


/*
Opens and creates a cottage valid HTML buffer.
@arg input -> stores HTML buffer.
@filepath -> path to HTML file to read.
@arg variables -> external variables to bring into scope.
@return error status of buffer creation.
*/
cotResult openHTML(char** input, const char* filepath, siteVar* variables) {
    cottageCheck(newResultError("openHTML: cottage not initialised."));
    
    FILE* file = fopen(filepath, "r");
    if (!file) return newResultError("openHTML: filepath not found");

    fseek(file, 0, SEEK_END);
    unsigned int size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* fullFile = (char*) malloc(size);
    if (!fullFile) {
        fclose(file);
        return newResultError("openHTML: could not create file read buffer.");
    }
    fread(fullFile, sizeof(char), size, file);
    fclose(file);
    uint32_t fi = 0; //file index

    unsigned int curDataSize = size + 1;
    dataVector data = dataVectorInit(curDataSize);

    int bsi = 0; //Curly bracket stack pointer
    bool inCheck = false; //Are we updating our command array?
    bool reachedColon = false; //Have we read the colon after the command in a cottage tag?
    bool finishedEmbedRead = false; //Have we finished reading a cottage tag?

    //use a diamond bracket count to know when to read curlies
    int diamondCount = 0;

    const int stateMax = 512;

    //stack of FOR states in file.
    conditionalState states[stateMax];
    memset(states, 0, stateMax * sizeof(conditionalState));
    int curCondState = 0;
    //layer 0 is the base plane. here, all initial if statements are safe to check.
    states[0].valid = true; states[0].ifAppeared = false; states[0].elseAppeared = false; states[0].chainSuccess = false;

    //stack for FOR states in file.
    loopState loopStates[stateMax];
    memset(loopStates, 0, stateMax * sizeof(loopState));
    int curLoopState = -1;

    //values mode variable can take to indicate which cottage tag mode we are in.
    const int 
    modeNONE = -1,
    modeVAR = 0,
    modeINSERT = 1,
    modeIF = 2,
    modeFOR = 3;
    int mode = modeNONE; //current mode we are in.

    //command is VAR, INSERT, IF, FOR etc.
    //offload is the data after.
    const int
    commandSize = 50,
    offloadSize = 100;
    char* command = (char*) calloc(commandSize, sizeof(char));
    char* offload = (char*) calloc(offloadSize, sizeof(char));
    int 
    ci = 0, //command index 
    oi = 0; //offload index

    while (fi < size) {
        int c = fullFile[fi++];
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
                    //set the signal for future ELSE-IF and ELSE
                    states[curCondState].chainSuccess = true;
                }
                finishedEmbedRead = true;
                oi = 0;
                memset(command, 0, commandSize * sizeof(char));
                memset(offload, 0, offloadSize * sizeof(char));
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
                finishedEmbedRead = true;
                oi = 0;
                memset(command, 0, commandSize * sizeof(char));
                memset(offload, 0, offloadSize * sizeof(char));
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
            }
            
            else if (!states[curCondState].valid) mode = modeNONE;

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
                    //End of loop,
                    //We delete the iterator from our variables, and go down a loopstate.
                    siteVarCompositeDelete(&variables, loopStates[curLoopState].iterator->name);
                    loopStates[curLoopState].iterator = NULL;
                    curLoopState--;
                }
                else {
                    //otherwise, update the iterator and file index.
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
            //ADD MORE CASES HERE
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
                
                if (varResult.status == COT_ERROR) { //write the error
                    if (strlen(varError) > 0) {
                        for (int j = 0; j < strlen(varError); j++) {
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
                char* value = BC_VariableToString(var);
                if (var) siteVarFree(var);
                if (value) {
                    for (int j = 0; j < strlen(value); j++) {
                        dataVectorPush(&data, value[j]);
                    }
                    free(value);
                }
                else { //write the error
                    if (strlen(varError) > 0) {
                        for (int j = 0; j < strlen(varError); j++) {
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
                mode = modeNONE;
                memset(command, 0, commandSize * sizeof(char));
                memset(offload, 0, offloadSize * sizeof(char));
            }
        }
        else if (mode == modeINSERT) {
            jumpINSERT:
            if (c != '}') {
                offload[oi++] = c;
            }
            else {
                finishedEmbedRead = true;
                oi = 0;

                const int linkSize = offloadSize;
                char* link = (char*) calloc(linkSize, sizeof(char));
                int j = 0;
                for (j = 0; offload[j] != 0 && offload[j] != ';'; j++) {
                    link[j] = offload[j];
                }
                if (offload[j] == 0) { //no input variables
                    goto readINPUT;
                }

                siteVar* newVariables = siteVarInit("newVariables", COMPOSITE, 0, NULL);

                //Read through the offload for names and values of variables.
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
                        if (curVar[0] && curValue[0]) {
                            //first, check if its a number, a string, or a variable
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
                                string_cot str = BC_StrToStr(curValue);
                                siteVarCompositeInsertNew(&newVariables, curVar, STRING, 1, &str);
                                if (str) free(str);
                            }

                            else if (BC_isArray(curValue)) { 
                                siteVar* storage = NULL;
                                cotResult storageResult = BC_ArrayToSiteVar(&storage, curVar, curValue, variables);
                                
                                if (storageResult.status == COT_OK) {
                                    siteVarCompositeInsert(&newVariables, storage);
                                    siteVarFree(storage);
                                }
                                else {
                                    newResultError("openHTML: INSERT, conversion into array is invalid.");
                                    goto failure;
                                }
                            }

                            else { //must be variable
                                siteVar* x = NULL;
                                cotResult xResult = BC_StrToVariable(&x, curValue, variables, variables);
                                
                                if (xResult.status == COT_OK) {
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

                //Copy over remaining data 
                if (!isVar) {
                    if (curVar[0] && curValue[0]) {
                        //first, check if its a number, a string, or a variable
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
                            string_cot str = BC_StrToStr(curValue);
                            siteVarCompositeInsertNew(&newVariables, curVar, STRING, 1, &str);
                            if (str) free(str);
                        }

                        else if (BC_isArray(curValue)) { 
                            siteVar* storage = NULL;
                            cotResult storageResult = BC_ArrayToSiteVar(&storage, curVar, curValue, variables);
                            
                            if (storageResult.status == COT_OK) {
                                siteVarCompositeInsert(&newVariables, storage);
                                siteVarFree(storage);
                            }
                            else {
                                newResultError("openHTML: INSERT, conversion into array is invalid.");
                                goto failure;
                            }
                        }

                        else { //must be variable
                            siteVar* x = NULL;
                            cotResult xResult = BC_StrToVariable(&x, curValue, variables, variables);
                            
                            if (xResult.status == COT_OK) {
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
                if (link) free(link);
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
                    for (j = 0; j < strlen(dataINPUT); j++) {
                        dataVectorPush(&data, dataINPUT[j]);
                    }
                }
                else {
                    newResultError("openHTML: INSERT, dataINPUT empty for some reason");
                    goto failure;
                } 
                free(dataINPUT);
                mode = modeNONE;
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

                    if (formatted) free(formatted);
                    if (transformed) free(transformed);
                }
                mode = modeNONE;
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
                    char iteratorName[256] = {0};
                    uint32_t i = 0;
                    uint32_t j = 0;
                    while (offload[i] != ',') {
                        if (j < 256) {
                            iteratorName[j] = offload[i];
                            j++;
                        }
                        i++;
                    }
                    i++;
                    
                    char iteratorTarget[256] = {0};
                    j = 0;
                    while (i < strlen(offload)) {
                        if (j < 256) {
                            iteratorTarget[j] = offload[i];
                            j++;
                        }
                        i++;
                    }
                    
                    //What kind of data is the target?
                    //Range?
                    if (iteratorTarget[0] == '(' && iteratorTarget[strlen(iteratorTarget) - 1] == ')') {
                        //remove brackets
                        BC_delAt(iteratorTarget, 0);
                        BC_delAt(iteratorTarget, strlen(iteratorTarget) - 1);

                        //extract start and count
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
                        if (!BC_isUInt(startString) || !BC_isUInt(countString)) {
                            newResultError("openHTML: FOR, range does not have UINT values.");
                            goto failure;
                        }

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
                        if ((fi + 2) < size) loopStates[curLoopState].returnIndex = fi + 2;
                        else {
                            newResultError("openHTML: FOR, could not set up loop return index.");
                            goto failure;
                        }

                        siteVar* iterator = siteVarInit(iteratorName, UINT, 1, &forArray[0]);
                        loopStates[curLoopState].iterator = iterator;
                        //insert a reference, to be able to change iterator easily.
                        siteVarCompositeInsertReference(&variables, iterator);



                    }
                    //Array?
                    else if (BC_isArray(iteratorTarget)) {
                        siteVar* forArrayContainer = NULL;
                        cotResult forArrayContainerResult = BC_ArrayToSiteVar(&forArrayContainer, "x", iteratorTarget, variables);
                        
                        if (forArrayContainerResult.status == COT_ERROR) {
                            newResultError("openHTML: FOR, could not convert array to siteVar");
                            goto failure;
                        } 
                        VARTYPE type = forArrayContainer->type;
                        void* forArray = siteVarAccessRange(forArrayContainer, 0, forArrayContainer->arrayItemCount);

                        loopStates[curLoopState].count = forArrayContainer->arrayItemCount;
                        loopStates[curLoopState].cur = 0;
                        loopStates[curLoopState].list = forArray;
                        loopStates[curLoopState].listComposite = false;
                        loopStates[curLoopState].listType = type;
                        if ((fi + 2) < size) loopStates[curLoopState].returnIndex = fi + 2;
                        else {
                            siteVarFree(forArrayContainer);
                            newResultError("openHTML: FOR, could not set up loop return index.");
                            goto failure;
                        }

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
                            void* forArray = siteVarAccessRange(var, 0, var->arrayItemCount);

                            loopStates[curLoopState].count = var->arrayItemCount;
                            loopStates[curLoopState].cur = 0;
                            loopStates[curLoopState].list = forArray;
                            loopStates[curLoopState].listComposite = false;
                            loopStates[curLoopState].listType = type;
                            if ((fi + 2) < size) loopStates[curLoopState].returnIndex = fi + 2;
                            else {
                                newResultError("openHTML: FOR, could not set up loop return index.");
                                goto failure;
                            }

                            siteVar* iterator = siteVarInit(iteratorName, type, 1, &forArray[0]);
                            loopStates[curLoopState].iterator = iterator;
                            siteVarCompositeInsertReference(&variables, iterator);
                        }
                        else {
                            newResultError("openHTML: FOR, target of loop is not an array");
                            goto failure;
                        }
                    }
                }
                mode = modeNONE;
                memset(command, 0, commandSize * sizeof(char));
                memset(offload, 0, offloadSize * sizeof(char));
            }
        }

        //To ensure the final curly bracket in a cottage tag is skipped
        else if (c == '}' && finishedEmbedRead && !diamondCount) {
            finishedEmbedRead = false;
        }

        else if (c == '{' && !diamondCount) {
            bsi++;

            //Check if there is another curly in front.
            char future = fullFile[fi++];
            if (future == '{') {
                inCheck = true;
            }
            else {
                fi--;
                goto dataWrite;
            }
        }
        else {
            dataWrite:
            if (states[curCondState].valid) {
                if (c == '<') diamondCount++;
                else if (c == '>') diamondCount--;
                dataVectorPush(&data, (char)c);
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

    //Clean up states
    for (int i = 0; i < stateMax; i++) {
        //IF
        // Nothing to be done.
        
        //FOR
        siteVarFree(loopStates[i].iterator);
        //check for strings in list
        if (loopStates[i].listType == STRING) {
            char** strList = (char**)loopStates[i].list;
            for (int j = 0; j < loopStates[i].count; j++) {
                if (strList && strList[j]) free(strList[j]);
            }
            if (strList) free(strList);
        }
        else free(loopStates[i].list);
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


/*
Sends a HTML file to a client.
@arg filepath -> location of file to send.
@arg client -> fd to send data to.
@arg variables -> external variables to bring into scope of file.
@return status of send.
*/
bool sendHTML(const char* filepath, int client, siteVar* variables) {
    cottageCheck(false);

    char* data = NULL;
    if (openHTML(&data, filepath, variables).status == COT_ERROR) {
        newResultError("sendHTML: failed to open HTML file.");
        return false;
    }

    if (data) goto success;
    
    failure:
    data = NULL;

    success:
    if (data) {
        //Build HTTP header
        const char* header = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n";
        send(client, header, strlen(header), 0);

        //Then the data.
        send(client, data, strlen(data), 0);

        free(data);
        return true;
    }
    else return false;
}

/*
Wrapper for file send functions for different file types.
@arg filepath -> location of file to send.
@arg client -> fd to send data to.
@arg vars -> external variables to bring into scope of HTML file.
@return status of send.
*/
bool sendFile(char* filepath, int client, siteVar* vars) {
    cottageCheck(false);
    //First, check if this file actually exists
    if (access(filepath, F_OK) == 0) {
        //Second, read the file ending
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

/*
Temporary function for sending a redirect message to a client.
@arg path -> file to send for redirect.
@client -> fd to send data to.
@return status of send.
*/
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