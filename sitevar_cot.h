/*
Code associated with variables used in HTTP communication.
Code is part of the cottage framework (https://github.com/vixthevix/cottage)
*/

#ifndef SITEVAR_COT
#define SITEVAR_COT
#include "dependencies_cot.h"
#include "hashfunc_cot.h"
#include "init_cot.h"

/*
A siteVar is an encapsulation of a piece of data, with a given type, data container,
and other data to make it a flexible struct.

They are used to:
    Insert custom data into a HTML file.
    Easily obtain payload data from a HTTP request.

siteVar types are your standard data types, except for COMPOSITE.
A COMPOSITE siteVar acts like a hashmap of other siteVars,
using the siteVar name as the key and the siteVar itself as the data returned.
*/

/*
Enum used to denote the type of a siteVar.
*/
typedef enum VARTYPE {
    ERROR = -1,
    INT,
    UINT,
    FLOAT,
    BOOL,
    STRING,
    COMPOSITE,
} VARTYPE;

/*
siteVar struct definition.
@param name => name of var
@param data => data inside var
@param arrayLen => current length of array of elements in var
@param arrayItemCount => number of elements in var (used in hashmap)
@param type => type of var
@param pd => used in hashmap for round-robin
@param isArray => bool denoting if var is an array
*/
typedef struct siteVar {
    char* name;
    void* data;
    size_t arrayLen;
    size_t arrayItemCount;
    VARTYPE type;
    int16_t pd;
    bool isArray;
} siteVar;

/*
Default type definitions for siteVar data storage.
*/
typedef int64_t int_cot;
typedef uint64_t uint_cot;
typedef double float_cot;
typedef bool bool_cot;
typedef char* string_cot;
typedef siteVar* composite_cot;

#define INTCOT_MAX INT64_MAX
#define UINTCOT_MAX UINT64_MAX
// #define FLOATCOT_MAX DOUBLE_MAX
// #define INTCOT_MAX INT64_MAX
// #define INTCOT_MAX INT64_MAX

// Function Prototypes
siteVar* INTERNAL_siteVarCompositeNewSize(const size_t oldSize);
bool siteVarCompositeInsert(siteVar** target, siteVar* var);
bool siteVarCompositeInsertNew(siteVar** target, char* name, VARTYPE type, size_t size, void* data);
bool siteVarCompositeInsertReference(siteVar** target, siteVar* var);
siteVar* siteVarCompositeAccess(siteVar* target, char* name);
siteVar* siteVarCompositeAccessReference(siteVar* target, char* name);
bool siteVarCompositeDelete(siteVar** target, char* name);
siteVar* siteVarCompositeCombine(siteVar* home, siteVar* intruder);
siteVar* INTERNAL_siteVarCompositeResize(siteVar* target, bool increase);
void INTERNAL_siteVarInitComposite(siteVar* target, void* data, size_t dataSize);
bool INTERNAL_siteVarNameLegal(char* name);
siteVar* siteVarInit(char* name, VARTYPE type, size_t elementCount, void* data);
siteVar* siteVarClone(siteVar* target);
void siteVarFree(siteVar* target);
size_t INTERNAL_siteVarTypeSize(VARTYPE type);
void* siteVarAccessRange(siteVar* target, size_t pointer, size_t stride);
void* siteVarAccessAt(siteVar* target, size_t index);
void* siteVarAccess(siteVar* target);
bool siteVarUpdateRange(siteVar* target, uint16_t index, uint16_t range, void* data);
bool siteVarUpdateAt(siteVar* target, uint16_t index, void* data);
bool siteVarUpdate(siteVar* target, void* data);
bool siteVarInsert(siteVar** target, void* data);

#if defined(COTTAGE_START)

/*
Creates a COMPOSITE siteVar given an initial size.
@arg oldSize -> initial size seed.
@return dynamically created COMPOSITE siteVar. 
*/
siteVar* INTERNAL_siteVarCompositeNewSize(const size_t oldSize) {
    cottageCheck(NULL);
    const size_t newSize = oldSize << 1; //doubled
    siteVar* newVar = malloc(sizeof(siteVar));
    newVar->arrayLen = newSize;
    newVar->arrayItemCount = 0;
    newVar->data = calloc(newSize, sizeof(siteVar*));
    newVar->type = COMPOSITE;

    return newVar;
}

/*
Inserts a siteVar into a COMPOSITE siteVar.
@arg target -> COMPOSITE to insert into.
@arg var -> siteVar to be inserted. A clone is actually inserted here.
@return status of insert.
*/
bool siteVarCompositeInsert(siteVar** target, siteVar* var) {
    cottageCheck(false);
    if (!(*target) || !var) return 0;
    if ((*target)->type != COMPOSITE) return 0;
    
    const unsigned int load = (*target)->arrayItemCount * 100 / (*target)->arrayLen;
    if (load > 60) {
        (*target) = INTERNAL_siteVarCompositeResize((*target), true);
    }
    
    //We insert a clone into the COMPOSITE, not the var itself.
    siteVar* new = siteVarClone(var);
    size_t initpos = stringHash(new->name) % (*target)->arrayLen;
    size_t index = 0;
    siteVar* curVar = NULL;
    siteVar** targetData = (siteVar**)((*target)->data);

    for (size_t i = 0; i < (*target)->arrayLen; i++) {
        index = (initpos + i) % (*target)->arrayLen;
        curVar = targetData[index];

        if (curVar == NULL) {
            //new->pd++;
            targetData[index] = new;
            (*target)->arrayItemCount++;
            return true;
        }
        //we replace variables with the same name
        if (!strcmp(curVar->name, new->name)) {
            //new->pd++;
            siteVarFree(curVar);
            targetData[index] = new;
            return true;
        }

        //Round-robin hashing
        if (new->pd > curVar->pd) {
            targetData[index] = new;
            new = curVar;
        }

        new->pd++;
    }

    //in case things go wrong
    siteVarFree(new);
    return false;
}

/*
Wrapper for CompositeInsert with specified siteVar data.
*/
bool siteVarCompositeInsertNew(siteVar** target, char* name, VARTYPE type, size_t size, void* data){
    cottageCheck(false);
    siteVar* new = siteVarInit(name, type, size, data);
    bool state = siteVarCompositeInsert(target, new);
    siteVarFree(new);
    return state;
}

/*
Alteration of CompositeInsert that directly puts in var, 
rather than a memory clone.
*/
bool siteVarCompositeInsertReference(siteVar** target, siteVar* var) {
    cottageCheck(false);
    if (!(*target) || !var) return 0;
    if ((*target)->type != COMPOSITE) return 0;
    
    const unsigned int load = (*target)->arrayItemCount * 100 / (*target)->arrayLen;
    if (load > 60) {
        (*target) = INTERNAL_siteVarCompositeResize((*target), true);
    }
    
    size_t initpos = stringHash(var->name) % (*target)->arrayLen;
    size_t index;
    siteVar* curVar;


    siteVar** targetData = (siteVar**)((*target)->data);

    for (size_t i = 0; i < (*target)->arrayLen; i++) {
        index = (initpos + i) % (*target)->arrayLen;
        curVar = targetData[index];

        if (curVar == NULL) {
            targetData[index] = var; //put in directly
            (*target)->arrayItemCount++;
            return true;
        }
        //we replace variables with the same name
        if (!strcmp(curVar->name, var->name)) {
            siteVarFree(curVar);
            targetData[index] = var; //put in directly.
            return true;
        }

        //Round-robin hashing
        if (var->pd > curVar->pd) {
            targetData[index] = var;
            var = curVar;
        }

        var->pd++;
    }

    //in case things go wrong
    return false;
}

/*
Gets a siteVar inside a COMPOSITE siteVar.
@arg target -> COMPOSITE siteVar to search in.
@arg name -> name of siteVar to retrieve.
@return clone of siteVar with searched name.
*/
siteVar* siteVarCompositeAccess(siteVar* target, char* name) {
    cottageCheck(NULL);
    if (!target || target->type != COMPOSITE) return 0;
    
    size_t hashed = stringHash(name);
    size_t initpos = hashed % target->arrayLen;

    size_t index;
    siteVar* curVar;
    int16_t curpd = 0;

    siteVar** data = (siteVar**)target->data;

    for (size_t i = 0; i < target->arrayLen; i++) {
        index = (initpos + i) % target->arrayLen;
        curVar = data[index];

        //Round-robin check
        if (curVar == NULL || curpd > curVar->pd) {
            return NULL;
        }
        //Normal check
        if (strcmp(curVar->name, name) == 0) {
            return siteVarClone(curVar);
        }

        curpd++;
    }

    return NULL;
}

/*
Alteration of CompositeAccess that returns the actual reference of the searched
siteVar, rather than a memory clone.
*/
siteVar* siteVarCompositeAccessReference(siteVar* target, char* name) {
    cottageCheck(NULL);
    if (!target || target->type != COMPOSITE) return 0;
    
    size_t hashed = stringHash(name);
    size_t initpos = hashed % target->arrayLen;

    size_t index;
    siteVar* curVar;
    int16_t curpd = 0;

    siteVar** data = (siteVar**)target->data;

    for (size_t i = 0; i < target->arrayLen; i++) {
        index = (initpos + i) % target->arrayLen;
        curVar = data[index];

        //Round-robin check
        if (curVar == NULL || curpd > curVar->pd) {
            return NULL;
        }
        //normal check
        if (strcmp(curVar->name, name) == 0) {
            return curVar;
        }

        curpd++;
    }

    return NULL;
}

/*
Frees a siteVar in a COMPOSITE siteVar from memory.
@arg target -> COMPOSITE siteVar to delete from.
@arg name -> name of siteVar to delete in target.
@return status of delete. 
*/
bool siteVarCompositeDelete(siteVar** target, char* name) {
    cottageCheck(false);
    if (!(*target) || !name) return 0;
    if ((*target)->type != COMPOSITE) return 0;

    //NEEDS A RESIZE CHECK TO LOWER THE SIZE.

    size_t initpos = stringHash(name) % (*target)->arrayLen;
    size_t index;
    siteVar* curVar;
    int16_t curpd = 0;


    siteVar** targetData = (siteVar**)((*target)->data);

    for (size_t i = 0; i < (*target)->arrayLen; i++) {
        index = (initpos + i) % (*target)->arrayLen;
        curVar = targetData[index];

        if (curVar == NULL || curpd > curVar->pd) {
            //value does not exist
            return false;
        }

        //returning a reference here
        //nope now returning a clone
        if (!strcmp(curVar->name, name)) {
            //free the value, set to null, then decrease size
            //siteVarFree(curVar);
            curVar = NULL;
            siteVarFree(targetData[index]);
            targetData[index] = NULL;
            (*target)->arrayItemCount--;
            return true;
        }

        // if (curpd > curVar->pd) return NULL;
        curpd++;
    }

    return false;
}

//no siteVarCompositeUpdate because insert kind of handles that.

/*
Copies siteVars from one COMPOSITE siteVar into another.
@arg home -> COMPOSITE to be base for insertion.
@arg intruder -> COMPOSITE with data to be inserted into home.
@return new COMPOSITE siteVar with data from both home and intruder.
*/
siteVar* siteVarCompositeCombine(siteVar* home, siteVar* intruder) {
    cottageCheck(NULL);
    if (!intruder || !home) return NULL;
    if (intruder->type != COMPOSITE || intruder->type != COMPOSITE) return NULL;

    //base new off of home
    siteVar* new = siteVarClone(home);

    //insert the intruder
    siteVar** intruderData = (siteVar**)intruder->data;
    for (size_t i = 0; i < intruder->arrayLen; i++) {
        if (intruderData[i]) {
            siteVarCompositeInsert(&new, intruderData[i]);
        }
    }

    //done
    return new;
}

/*
Creates a COMPOSITE siteVar of a new size, from a base COMPOSITE.
(BECAUSE OF INSERT REFERENCE, THIS FUNCTION MAY BREAK A LOT OF THINGS.)
@arg target -> base COMPOSITE for new one.
@arg bool -> are we increasing or decreasing in size?
@return new COMPOSITE.
*/
siteVar* INTERNAL_siteVarCompositeResize(siteVar* target, bool increase) {
    cottageCheck(NULL);
    if (!target || target->type != COMPOSITE) return 0;

    siteVar* new = NULL;
    if (increase) new = INTERNAL_siteVarCompositeNewSize(target->arrayLen);
    else new = INTERNAL_siteVarCompositeNewSize(target->arrayLen >> 2); //NewSize doubles, so we divide by 4. Fails if results in 0.

    new->arrayItemCount = target->arrayItemCount;
    new->isArray = target->isArray;
    new->type = target->type;

    new->name = (char*) calloc(strlen(target->name) + 1, sizeof(char));
    strcpy(new->name, target->name);

    siteVar** newData = (siteVar**)new->data;
    siteVar** targetData = (siteVar**)target->data;

    for (size_t i = 0; i < target->arrayLen; i++) {
        if (targetData[i]) {
            siteVarCompositeInsert(&new, targetData[i]);
        }
    }
    siteVarFree(target);

    return new;

}

/*
Initialises a base COMPOSITE siteVar. Used in siteVarInit only.
@arg target -> already created siteVar to update with data.
@arg data -> collection of siteVar data to insert.
@arg dataSize -> number of siteVars in data.
*/
void INTERNAL_siteVarInitComposite(siteVar* target, void* data, size_t dataSize) {
    cottageCheck();
    if (!target || target->type != COMPOSITE) return;
    //we fix the arrayLen and arrayItemCount
    siteVar* newVar = INTERNAL_siteVarCompositeNewSize(8 >> 1); //start with size of 8
    target->isArray = true; //default to array because it kind of is
    target->arrayLen = newVar->arrayLen;
    target->arrayItemCount = newVar->arrayItemCount;
    target->data = newVar->data;
    free(newVar);

    if (!data) return;

    //we copy over the data now, using hashing.
    siteVar** newData = (siteVar**) data;
    for (size_t i = 0; i < dataSize; i++) {
        if (newData[i]) {
            siteVarCompositeInsert(&target, newData[i]);
        }
    }
}

/*
Checks if a siteVar name is legal. Used in siteVarInit.
@arg name -> target to check.
@return status of check.
*/
bool INTERNAL_siteVarNameLegal(char* name) {
    cottageCheck(false);

    const char illegalCharacters[] = {
        '\'',
        '\"',
        '-',
        '[',
        ']',
        ',',
        '.',
        ';',
        ':',
    }; 

    if (!name) return false;
    for (size_t i = 0; i < strlen(name); i++) {
        for (size_t j = 0; j < sizeof(illegalCharacters); j++) {
            if (name[i] == illegalCharacters[j]) return false;
        }
    }
    return true;
}

/*
Initialises a siteVar.
@arg name -> name of siteVar.
@arg type -> data type of contents of siteVar.
@arg elementCount -> number of elements in data.
@arg data -> raw data to be inputted into siteVar.
@return newly created siteVar.
*/
siteVar* siteVarInit(char* name, VARTYPE type, size_t elementCount, void* data) {
    cottageCheck(NULL);
    if (!INTERNAL_siteVarNameLegal(name)) return NULL;

    siteVar* target = (siteVar*) calloc(1, sizeof(siteVar));

    target->name = (char*) calloc(strlen(name) + 1, sizeof(char));
    strcpy(target->name, name);

    target->type = type;
    target->isArray = (elementCount > 1);
    target->arrayLen = 8; //default arrayLen
    target->arrayItemCount = elementCount;
    target->pd = 0;

    //Fallback value to ensure memory is created normally.
    const unsigned itemCount = elementCount ? elementCount : target->arrayLen;

    if (type == STRING) {
        target->data = calloc(itemCount, sizeof(string_cot));
        if (!data) goto end;
        //for each string in the array, we must also allocate data for them.
        //we store an array of these addresses, then memcpy it over
        string_cot* stringArray = (string_cot*) data;
        string_cot* targetData = (string_cot*) target->data;
        for (size_t i = 0; i < elementCount; i++) {
            char* string = calloc(strlen(stringArray[i]) + 1, sizeof(char));
            strcpy(string, stringArray[i]);
            targetData[i] = string;
        }
    }
    else if (type == COMPOSITE) {
        INTERNAL_siteVarInitComposite(target, data, itemCount);
    }
    else {
        size_t size = INTERNAL_siteVarTypeSize(type);
        target->data = calloc(itemCount, size);
        if (!data) goto end;
        memcpy(target->data, data, (itemCount) * size);
    }
    
    end:
    return target;
}

/*
Creates a memory clone of a siteVar.
@arg target -> siteVar to clone.
@return clone of target.
*/
siteVar* siteVarClone(siteVar* target) {
    cottageCheck(NULL);
    return siteVarInit(target->name, target->type, target->arrayItemCount, target->data);
}


/*
Recursive memory free for a siteVar.
@arg target -> siteVar to free.
*/
void siteVarFree(siteVar* target) {
    cottageCheck();
    if (!target) return;

    if (target->type != COMPOSITE && target->type != STRING) { //base case
        if (target->data) free(target->data);
    }
    else if (target->type == STRING) { //string case
        char** data = (char**) target->data;
        if (!data) return;
        for (size_t i = 0; i < target->arrayItemCount; i++) {
            if (data[i]) free(data[i]);
        }
        free(data);
    }
    else { //composite case. recursion happens here.
        for (size_t i = 0; i < target->arrayLen; i++) {
            siteVar* cur = ((siteVar**)target->data)[i];
            if (cur) {
                siteVarFree(cur);
            }
        }
    }
    
    //free everything else
    free(target->name);
    target->name = NULL;
    free(target);
    target = NULL;
    return;
}

/*
Returns the size that an element of data a siteVar of a given type should have in bytes.
@arg type -> siteVar type to analyze.
@return size in bytes of type.
*/
size_t INTERNAL_siteVarTypeSize(VARTYPE type) {
    cottageCheck(0);
    size_t size = 0;
    switch (type) {
        case INT:
            size = sizeof(int_cot);
            break;
        case UINT:
            size = sizeof(uint_cot);
            break;
        case FLOAT: //used to be DOUBLE
            size = sizeof(float_cot);
            break;
        case BOOL:
            size = sizeof(bool_cot);
            break;
        case STRING:
            size = sizeof(string_cot);
            break;
        case COMPOSITE:
            size = sizeof(composite_cot);
            break;
    }

    return size;
}

/*
Gets a clone of a slice of a siteVar's stored data.
@arg target -> siteVar to retrieve data from.
@arg pointer -> starting index of data.
@arg stride -> number of elements to retrieve.
@return clone of raw data slice.
*/
void* siteVarAccessRange(siteVar* target, size_t pointer, size_t stride) {
    cottageCheck(NULL);
    if (!target || target->type == COMPOSITE) return NULL;
    //Will we be reading outside of the bounds of the target data?
    if (pointer + stride > target->arrayItemCount) return NULL;

    size_t size = INTERNAL_siteVarTypeSize(target->type);

    void* data = malloc(size * stride);
    void* address = (target->data + (size * pointer));

    if (target->type == STRING) { //special case due to pointers.
        string_cot* stringData = (string_cot*)data;
        string_cot* targetData = (string_cot*)target->data;
        for (size_t i = 0, j = pointer; i < stride; i++, j++) {
            string_cot string = targetData[j];
            stringData[i] = calloc(strlen(string) + 1, sizeof(char));
            strcpy(stringData[i], string);
        }
    }
    else {
        memcpy(data, address, stride * size);
    }
    return data;
}

/*
Wrapper for AccessRange that accesses one value at an index.
*/
void* siteVarAccessAt(siteVar* target, size_t index) {
    cottageCheck(NULL);
    return siteVarAccessRange(target, index, 1);
}

/*
Wrapper for AccessAt that accesses the single value of a non-array siteVar.
*/
void* siteVarAccess(siteVar* target) {
    cottageCheck(NULL);
    return (!target->isArray) ? siteVarAccessAt(target, 0) : NULL;
}

/*
Updates a slice of a siteVar's stored data.
@arg target -> siteVar to update.
@arg index -> starting index of target data.
@arg range -> number of elements update.
@arg data -> new data to input.
@return status of update.
*/
bool siteVarUpdateRange(siteVar* target, uint16_t index, uint16_t range, void* data) {
    cottageCheck(false);
    if (!target || target->type == COMPOSITE) return false;
    //Will we be reading outside of the bounds of the target data?
    if (index >= target->arrayLen || (index + range) > target->arrayLen) return false;
    
    size_t size = INTERNAL_siteVarTypeSize(target->type);

    if (target->type == STRING) {
        string_cot* storage = (string_cot*) target->data;
        string_cot* stringData = (string_cot*) data;
        for (uint16_t i = 0, j = index; i < range; i++, j++) {
            //need to reallocate, clear (just in case, maybe not needed), then copy
            storage[j] = (char*) realloc(storage[j], sizeof(char) * (strlen(stringData[i]) + 1));
            memset(storage[j], 0, sizeof(char) * (strlen(stringData[i]) + 1));
            strcpy(storage[j], stringData[i]);
        }
    }
    else {
        //get our location, then copy over
        void* storage = (target->data + (size * (index)));
        memcpy(storage, data, size * range);
    }

    return true;
}

/*
Wrapper for UpdateRange that updates one value at an index.
*/
bool siteVarUpdateAt(siteVar* target, uint16_t index, void* data) {
    cottageCheck(false);
    return siteVarUpdateRange(target, index, 1, data);
}

/*
Wrapper for UpdateAt that updates the single value of a non-array siteVar.
*/
bool siteVarUpdate(siteVar* target, void* data) {
    cottageCheck(false);
    return (!target->isArray) ? siteVarUpdateAt(target, 0, data) : false;
}

/*
Pushes a value onto the data storage of a siteVar.
Updates if the siteVar is now an array or not.
@arg target -> siteVar to push data onto.
@arg data -> data to push.
@return status of insert.
*/
bool siteVarInsert(siteVar** target, void* data) {
    cottageCheck(false);
    if (!(*target) || (*target)->type == COMPOSITE) return false;
    size_t size = INTERNAL_siteVarTypeSize((*target)->type);
    bool isString = false;
    //we need to use a load balancer to allocate enough space.
    unsigned int load = 0;
    if ((*target)->arrayLen > 0) load = (*target)->arrayItemCount * 100 / (*target)->arrayLen;
    if (load > 60) {
        (*target)->arrayLen <<= 1; //double the size
        (*target)->data = realloc((*target)->data, size * (*target)->arrayLen);
    }

    if ((*target)->type == STRING) {
        string_cot string = *((string_cot*)data);
        string_cot* stringData = (string_cot*)(*target)->data;

        string_cot insertion = (string_cot)calloc(strlen(string) + 1, sizeof(char));
        strcpy(insertion, string);

        stringData[(*target)->arrayItemCount] = insertion;
        (*target)->arrayItemCount++;
    }
    else {
        //assume data consists of only one value
        memcpy((*target)->data + (size * (*target)->arrayItemCount), data, size);
        (*target)->arrayItemCount++;
    }

    //ensure it becomes an array.
    if ((*target)->arrayItemCount > 1) (*target)->isArray = true;

    return true;

}

#endif
#endif