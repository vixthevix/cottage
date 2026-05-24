#ifndef FOPEN_COT
#define FOPEN_COT

#include "dependencies_cot.h"
#include "boolcalc_cot.h"
#include "sitevar_cot.h"

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
char* openHTML(const char* filepath, siteVar* variables) {
    //first, prepare the html
    FILE* file = fopen(filepath, "r");
    if (!file) return NULL;

    //get the size of the file and fread it into a buffer

    fseek(file, 0, SEEK_END);
    unsigned int size = ftell(file);
    fseek(file, 0, SEEK_SET);

    //we will try write to data using a for loop, to keep track of our frontend shenanigans
    char* data = (char*) calloc(size + 1, sizeof(char));
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
                //char* value = qmapGet(variables, offload);
                siteVar* var = BC_StrToVariable(offload, variables, variables);
                if (!var) goto failure;
                //we need to convert this value into a string
                char* value = BC_VariableToString(var, 0);
                if (value) {
                    //write into data
                    for (int j = 0; j < strlen(value); j++, di++) {
                        data[di] = value[j];
                    }
                    free(value);
                    free(var);
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
                                siteVarCompositeInsertNew(newVariables, curVar, UINT, 1, &((uint_cot){BC_StrToUInt(curValue)}));
                            }
                            else if (BC_isInt(curValue)) {
                                siteVarCompositeInsertNew(newVariables, curVar, INT, 1, &((int_cot){BC_StrToInt(curValue)}));
                            }
                            else if (BC_isFloat(curValue)) {
                                siteVarCompositeInsertNew(newVariables, curVar, FLOAT, 1, &((float_cot){BC_StrToFloat(curValue)}));
                            }
                            else if (BC_isBool(curValue)) {
                                siteVarCompositeInsertNew(newVariables, curVar, BOOL, 1, &((bool){BC_StrToBool(curValue)}));
                            }
                            else if (BC_isString(curValue)) {
                                //remove the quote marks
                                //BC_delAt(curValue, 0);
                                //BC_delAt(curValue, strlen(curValue) - 1);
                                //qmapInsert(newVariables, curVar, curValue);
                                siteVarCompositeInsertNew(newVariables, curVar, STRING, 1, &((string_cot){BC_StrToStr(curValue)}));
                            }

                            else if (BC_isArray(curValue)) { //NEXT TASK

                                siteVar* storage = BC_ArrayToSiteVar(curVar, curValue, variables);
                                //now we have storage, first check if its null
                                //then put it into our thing
                                if (storage) {
                                    siteVarCompositeInsert(newVariables, storage);
                                    siteVarFree(storage);
                                }
                            }

                            else { //must be variable
                                //char* x = qmapGet(variables, curValue);
                                //siteVar* x = siteVarCompositeAccess(variables, curValue);
                                siteVar* x = BC_StrToVariable(curValue, variables, variables);
                                if (x) {
                                    //qmapInsert(newVariables, curVar, x);
                                    siteVarCompositeInsert(newVariables, x);
                                    siteVarFree(x);
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
                //copy over from above
                //TO DO, first clean up your code and put it into separate functions
                //THEN copy it over.
                if (!isVar) {
                    if (curVar && curValue) {
                        if (BC_isUInt(curValue)) {
                            siteVarCompositeInsertNew(newVariables, curVar, UINT, 1, &((uint_cot){BC_StrToUInt(curValue)}));
                        }
                        else if (BC_isInt(curValue)) {
                            siteVarCompositeInsertNew(newVariables, curVar, INT, 1, &((int_cot){BC_StrToInt(curValue)}));
                        }
                        else if (BC_isFloat(curValue)) {
                            //qmapInsert(newVariables, curVar, curValue);
                            siteVarCompositeInsertNew(newVariables, curVar, FLOAT, 1, &((float_cot){BC_StrToFloat(curValue)}));
                        }
                        else if (BC_isBool(curValue)) {
                            siteVarCompositeInsertNew(newVariables, curVar, BOOL, 1, &((bool){BC_StrToBool(curValue)}));
                        }
                        else if (BC_isString(curValue)) {
                            //remove the quote marks
                            //BC_delAt(curValue, 0);
                            //BC_delAt(curValue, strlen(curValue) - 1);
                            //qmapInsert(newVariables, curVar, curValue);
                            siteVarCompositeInsertNew(newVariables, curVar, STRING, 1, &((string_cot){BC_StrToStr(curValue)}));
                        }
                        else if (BC_isArray(curValue)) { //NEXT TASK

                            siteVar* storage = BC_ArrayToSiteVar(curVar, curValue, variables);
                            //now we have storage, first check if its null
                            //then put it into our thing
                            if (storage) {
                                siteVarCompositeInsert(newVariables, storage);
                                siteVarFree(storage);
                            }
                        }
                        else { //must be variable
                            //char* x = qmapGet(variables, curValue);
                            //siteVar* x = siteVarCompositeAccess(variables, curValue);
                            siteVar* x = BC_StrToVariable(curValue, variables, variables);
                            if (x) {
                                //qmapInsert(newVariables, curVar, x);
                                siteVarCompositeInsert(newVariables, x);
                                siteVarFree(x);
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

                //reset newVariables
                siteVarFree(newVariables);
                newVariables = NULL;
                //copy over the new data
                if (dataINPUT) {
                    printf("input data got\n");
                    printf("%s\n", dataINPUT);
                    //we need to reallocate our data to take into account
                    //increases in size
                    data = (char*) realloc(data, size + (strlen(dataINPUT) << 1));
                    for (j = 0; j < strlen(dataINPUT); j++, di++) {
                        data[di] = dataINPUT[j];
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
                    bool result = BC_evaluate(transformed, variables);
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
                data[di] = (char)c;
                di++;
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
    //qmapFree(newVariables);
    if (data) {
        //set safety null terminator
        data[di] = 0;
        return data;
    }
    else {
        return NULL;
    }
}


//lets make a function for sending over an html file
bool sendHTML(const char* filepath, int client, siteVar* variables) {
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
        return true;
    }
    else return false;
    
}

//we need a function to get a requested resource
//this can be html, an image, js, whatever
//to do this, we check the file endings
bool sendFile(char* filepath, int client, siteVar* vars) {
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

#endif