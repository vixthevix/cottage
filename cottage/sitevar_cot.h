#ifndef SITEVAR_COT
#define SITEVAR_COT

/*
This file will be used to test out the new flexible type system.

from notes:

Right now, everything is stored as a string, including numbers.
    Lets just make a generic struct (void* data and TYPE enum).
    What types do we want?
        INT8
        INT16
        INT32
        INT64
        Unsigned versions of these
        FLOAT
        DOUBLE
        BOOL
        STRING
        COMPOSITE
    
    Also, incorporate the isArray boolean, to state the obvious.


*/
#include "dependencies_cot.h"
#include "hashfunc_cot.h"
#include "init_cot.h"

// typedef enum VARTYPE {
//     INT8,
//     INT16,
//     INT32,
//     INT64,
//     UINT8,
//     UINT16,
//     UINT32,
//     UINT64,
//     FLOAT,
//     DOUBLE,
//     BOOL,
//     STRING,
//     COMPOSITE,
// } VARTYPE;

/**
 * Enum for denoting the type of a siteVar.
 * INT and UINT get translated into their 64 bit equivalents,
 * FLOAT gets translated into a double.
 * This is to ensure there is enough space for a single one of these variables, so that less
 * overflows may occur.
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

/**
 * siteVar struct, denoting a website variable to be used in HTML and data transfer.
 * @param name => name of var
 * @param data => data inside var
 * @param arrayLen => current length of array of elements in var
 * @param arrayItemCount => number of elements in var (used in hashmap)
 * @param type => type of var
 * @param pd => used in hashmap
 * @param isArray => bool denoting if var is an array
 */
typedef struct siteVar {
    char* name;
    void* data;
    size_t arrayLen;
    size_t arrayItemCount; //used for hashmap
    VARTYPE type;
    int16_t pd; //used for hashmap
    bool isArray;
} siteVar;

//some type definitions for better practice
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

bool siteVarFree(siteVar* target);
siteVar* INTERNAL_siteVarCompositeResize(siteVar* target, bool increase);
siteVar* siteVarInit(char* name, VARTYPE type, size_t size, void* data);
void* siteVarAccess(siteVar* target);
siteVar* siteVarClone(siteVar* target);
size_t INTERNAL_siteVarTypeSize(VARTYPE type);

siteVar* INTERNAL_siteVarCompositeNewSize(const size_t oldSize) {
    cottageCheck(NULL);
    const size_t newSize = oldSize << 1;
    siteVar* newVar = malloc(sizeof(siteVar));
    newVar->arrayLen = newSize;
    newVar->arrayItemCount = 0;
    newVar->data = calloc(newSize, sizeof(siteVar*));

    return newVar;
}


//#define stringHash stringHash

//#define siteVarInsertVar siteVarCompositeInsert

bool siteVarCompositeInsert(siteVar** target, siteVar* var) {
    cottageCheck(false);
    if (!(*target) || !var) return 0;
    if ((*target)->type != COMPOSITE) return 0;
    
    const unsigned int load = (*target)->arrayItemCount * 100 / (*target)->arrayLen;
    if (load > 60) {
        printf("resizing...\n");
        (*target) = INTERNAL_siteVarCompositeResize((*target), true);
    }

    printf("composite insert\n");

    //copy over var to insert
    siteVar* new = siteVarInit(var->name, var->type, var->arrayItemCount, var->data);
    size_t initpos = stringHash(new->name) % (*target)->arrayLen;
    size_t index;
    siteVar* curVar;

    printf("new made\n");

    siteVar** targetData = (siteVar**)((*target)->data);
    printf("not invalid pointer\n");

    for (size_t i = 0; i < (*target)->arrayLen; i++) {
        index = (initpos + i) % (*target)->arrayLen;
        curVar = targetData[index];

        if (curVar == NULL) {
            //new->pd++;
            //printf("(%s, %s) stored at index %lu inside of %s\n", new->name, *(char**)siteVarAccess(new), index, (*target)->name);
            targetData[index] = new;
            (*target)->arrayItemCount++;
            printf("uhhh\n");
            return true;
        }
        //we replace variables with the same name
        if (!strcmp(curVar->name, new->name)) {
            //new->pd++;
            printf("%s stored at index %lu inside of %s\n", new->name, index, (*target)->name);
            siteVarFree(curVar);
            targetData[index] = new;
            return true;
        }

        if (new->pd > curVar->pd) {
            targetData[index] = new;
            new = curVar;
        }

        new->pd++;
        printf("carried over\n");
    }

    //in case things go wrong
    siteVarFree(new);
    return false;
}

bool siteVarCompositeInsertNew(siteVar** target, char* name, VARTYPE type, size_t size, void* data){
    cottageCheck(false);
    printf("1");
    siteVar* new = siteVarInit(name, type, size, data);
    printf("2");
    bool state = siteVarCompositeInsert(target, new);
    if (state) printf("siteVarCompositeInsertNew success\n");
    //printf("3");
    siteVarFree(new);
    //printf("4\n");
    return state;
}


//this function adds var directly inside of target,
//rather than make a clone.
bool siteVarCompositeInsertReference(siteVar** target, siteVar* var) {
    cottageCheck(false);
    if (!(*target) || !var) return 0;
    if ((*target)->type != COMPOSITE) return 0;
    
    const unsigned int load = (*target)->arrayItemCount * 100 / (*target)->arrayLen;
    if (load > 60) {
        printf("resizing...\n");
        (*target) = INTERNAL_siteVarCompositeResize((*target), true);
    }

    //printf("composite insert\n");

    //copy over var to insert
    
    size_t initpos = stringHash(var->name) % (*target)->arrayLen;
    size_t index;
    siteVar* curVar;

    printf("new made\n");

    siteVar** targetData = (siteVar**)((*target)->data);
    printf("not invalid pointer\n");

    for (size_t i = 0; i < (*target)->arrayLen; i++) {
        index = (initpos + i) % (*target)->arrayLen;
        curVar = targetData[index];

        if (curVar == NULL) {
            //new->pd++;
            //printf("(%s, %s) stored at index %lu inside of %s\n", new->name, *(char**)siteVarAccess(new), index, (*target)->name);
            targetData[index] = var;
            (*target)->arrayItemCount++;
            //printf("uhhh\n");
            return true;
        }
        //we replace variables with the same name
        if (!strcmp(curVar->name, var->name)) {
            //new->pd++;
            //printf("%s stored at index %lu inside of %s\n", new->name, index, (*target)->name);
            siteVarFree(curVar);
            targetData[index] = var;
            return true;
        }

        if (var->pd > curVar->pd) {
            targetData[index] = var;
            var = curVar;
        }

        var->pd++;
        //printf("carried over\n");
    }

    //in case things go wrong
    //siteVarFree(new);
    return false;
}

//#define siteVarCompositeAccess siteVarCompositeGet
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
        printf("hashmap looking at index %lu of %s for %s\n", index, target->name, name);

        if (curVar == NULL || curpd > curVar->pd) {
            //printf("is curVar pd (%i) < curpd (%i)? %s\n", curVar->pd, curpd, (curVar->pd < curpd) ? "true":"false");
            return NULL;
        }

        //returning a reference here
        //nope now returning a clone
        if (!strcmp(curVar->name, name)) {
            if (curVar->type == STRING) printf("%s is a string\n", curVar->name);
            printf("curVar name is %s, curVar value is %u\n", curVar->name, *((uint_cot*)curVar->data));
            return siteVarClone(curVar);
        }

        // if (curpd > curVar->pd) return NULL;
        curpd++;
    }

    return NULL;
}

//gets the actual pointer to the value at name
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
        printf("hashmap looking at index %lu of %s for %s\n", index, target->name, name);

        if (curVar == NULL || curpd > curVar->pd) {
            //printf("is curVar pd (%i) < curpd (%i)? %s\n", curVar->pd, curpd, (curVar->pd < curpd) ? "true":"false");
            return NULL;
        }

        //returning a reference here
        //nope now returning a clone
        if (!strcmp(curVar->name, name)) {
            if (curVar->type == STRING) printf("%s is a string\n", curVar->name);
            printf("curVar name is %s, curVar value is %u\n", curVar->name, *((uint_cot*)curVar->data));
            return curVar;
        }

        // if (curpd > curVar->pd) return NULL;
        curpd++;
    }

    return NULL;
}


bool siteVarCompositeDelete(siteVar** target, char* name) {
    cottageCheck(false);
    if (!(*target) || !name) return 0;
    if ((*target)->type != COMPOSITE) return 0;
    
    // const unsigned int load = (*target)->arrayItemCount * 100 / (*target)->arrayLen;
    // if (load < 30) {
    //     printf("resizing...\n");
    //     (*target) = INTERNAL_siteVarCompositeResize((*target), false);
    // }

    printf("composite delete\n");

    size_t initpos = stringHash(name) % (*target)->arrayLen;
    size_t index;
    siteVar* curVar;
    int16_t curpd = 0;

    printf("new made\n");

    siteVar** targetData = (siteVar**)((*target)->data);
    printf("not invalid pointer\n");

    for (size_t i = 0; i < (*target)->arrayLen; i++) {
        index = (initpos + i) % (*target)->arrayLen;
        curVar = targetData[index];
        printf("hashmap looking at index %lu of %s for %s\n", index, (*target)->name, name);

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
We have a couple situations here
IN reality, this function should only apply to two composites
because insertion is there for a composite and a non-composite.
NULL, NULL -> NULL
siteVar, NULL -> NULL

you get the idea


another question, do we create a new siteVar?
new siteVar keeps data integrity, 
inserting directly into home makes some sense.
I think the fact that there are two ways to insert into a composite now
should make it clear that we should have only one way to insert.
so ill go with creation for now :)
*/
//#define siteVarCombine siteVarCompositeCombine
siteVar* siteVarCompositeCombine(siteVar* home, siteVar* intruder) {
    cottageCheck(NULL);
    if (!intruder || !home) return NULL;
    if (intruder->type != COMPOSITE || intruder->type != COMPOSITE) return NULL;

    //base new off of home
    siteVar* new = siteVarInit(home->name, home->type, home->arrayLen, home->data);

    //insert the intruder
    siteVar** intruderData = (siteVar**)intruder->data;
    for (size_t i = 0; i < intruder->arrayLen; i++) {
        if (intruderData[i]) {
            siteVarCompositeInsert(&home, intruderData[i]);
        }
    }

    //done
    return new;
}

siteVar* INTERNAL_siteVarCompositeResize(siteVar* target, bool increase) {
    cottageCheck(NULL);
    if (!target || target->type != COMPOSITE) return 0;

    siteVar* new = NULL;
    if (increase) new = INTERNAL_siteVarCompositeNewSize(target->arrayLen);
    else new = INTERNAL_siteVarCompositeNewSize(target->arrayLen >> 2);

    new->arrayItemCount = target->arrayItemCount;
    new->isArray = target->isArray;
    new->type = target->type;

    //name must be copied over differently
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
right now, an illegal name contains
quotation marks
apostrophes
hyphens
pretty much it, its more lenient than a normal languages,
but if problems arise, we can always add to this
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
        '.'
    }; 

    if (!name) return false;
    for (size_t i = 0; i < strlen(name); i++) {
        for (size_t j = 0; j < sizeof(illegalCharacters); j++) {
            if (name[i] == illegalCharacters[j]) return false;
        }
    }
    return true;
}

/**
 * siteVarInit must perform name checking as well
 * 
 */
siteVar* siteVarInit(char* name, VARTYPE type, size_t elementCount, void* data) {
    cottageCheck(NULL);
    if (!INTERNAL_siteVarNameLegal(name)) return NULL;
    printf("hi\n");
    //if (!data) return NULL; //must put in some data
    
    siteVar* target = (siteVar*) calloc(1, sizeof(siteVar));

    target->name = (char*) calloc(strlen(name) + 1, sizeof(char));
    strcpy(target->name, name);

    target->type = type;
    target->isArray = (elementCount > 1);
    target->arrayLen = 8; //default arrayLen
    target->arrayItemCount = elementCount;
    target->pd = 0;
    
    //with the data, we have to use our enum here to allocate
    //the correct amount of memory

    //what if data is NULL?
    //we need to initialise the data, but keep it empty

    const unsigned itemCount = elementCount ? elementCount:target->arrayLen;

    if (type == STRING) {
        target->data = calloc(itemCount, sizeof(string_cot));
        if (!data) goto end;
        //for each string in the array, we must also allocate data for them.
        //we store an array of these addresses, then memcpy it over
        string_cot* stringArray = (string_cot*) data;
        string_cot* targetData = (string_cot*) target->data;
        printf("yes\n");
        printf("elementcount is %u\n", elementCount);
        for (size_t i = 0; i < elementCount; i++) {
            printf("sitevarinit: string was %s\n", stringArray[i]);
            char* string = calloc(strlen(stringArray[i]) + 1, sizeof(char));
            printf("woah\n");
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

siteVar* siteVarClone(siteVar* target) {
    cottageCheck(NULL);
    return siteVarInit(target->name, target->type, target->arrayItemCount, target->data);
}


/*
recursive function
base case is when not composite
recursive case is when composite
special case for strings
*/
bool siteVarFree(siteVar* target) {
    cottageCheck(false);
    if (!target) return false;

    printf("freeing %s...\n", target->name);
    if (target->type != COMPOSITE && target->type != STRING) { //base case
        printf("%s is normal\n", target->name);
        if (target->data) free(target->data);
    }
    else if (target->type == STRING) {
        printf("%s is string\n", target->name);
        char** data = (char**) target->data;
        if (!data) return false;
        for (size_t i = 0; i < target->arrayItemCount; i++) {
            printf("freeing %u\n", i);
            if (data[i]) free(data[i]);
        }
        free(data);
        printf("%s fully freed\n", target->name);
    }
    else { //recursive case
        printf("%s is composite\n", target->name);
        for (size_t i = 0; i < target->arrayLen; i++) {
            siteVar* cur = ((siteVar**)target->data)[i];
            if (cur) {
                printf("composite cur received %u: %s\n", i, cur->name);
                siteVarFree(cur);
            }
        }
    }
    
    //free everything else
    free(target->name);
    target->name = NULL;
    free(target);
    target = NULL;
    printf("fully freed\n");
    return true;
}


/*
Some things about access.
right now, its returning the memory address of the thing.
do we want to keep this, or do we want to put in calloc for data duplication?
i think duplicate, because references eliminate the need for update

another thing with access and update.
do we restrict this to the non-composites, for safety?
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

void* siteVarAccessAt(siteVar* target, size_t index) {
    cottageCheck(NULL);
    if (!target || target->type == COMPOSITE) return NULL;
    if (index >= target->arrayLen) return NULL;

    //void* data;
    size_t size = INTERNAL_siteVarTypeSize(target->type);

    //returns a pointer to that item, since dynamically allocated can be accessed normally.
    //uses pointer arithmetic

    void* data = malloc(size);
    void* address = (target->data + (size * index));

    // if (target->type == STRING) {


    // }

    memcpy(data, (target->data + (size * index)), size);

    return data;
}

void* siteVarAccess(siteVar* target) {
    cottageCheck(NULL);
    return (!target->isArray) ? siteVarAccessAt(target, 0) : NULL;
}


bool siteVarUpdateRange(siteVar* target, uint16_t index, uint16_t range, void* data) {
    cottageCheck(false);
    if (!target || target->type == COMPOSITE) return false;
    if (index >= target->arrayLen || (index + range) > target->arrayLen) return false;
    //special cases for these two, due to heap memory
    bool isString = false;
    bool isComposite = false;
    
    size_t size = INTERNAL_siteVarTypeSize(target->type);
    if (target->type == COMPOSITE) isComposite = true;
    else if (target->type == STRING) isString = true;

    if (isString) {
        string_cot* storage = (string_cot*) target->data;
        string_cot* stringData = (string_cot*) data;
        for (uint16_t i = 0, j = index; i < range; i++, j++) {
            //need to reallocate, clear (just in case, maybe not needed), then copy
            storage[j] = (char*) realloc(storage[j], sizeof(char) * (strlen(stringData[i]) + 1));
            memset(storage[j], 0, sizeof(char) * (strlen(stringData[i]) + 1));
            strcpy(storage[j], stringData[i]);
        }
    }
    else if (isComposite) {
        //we should make our siteVarFree function first
        composite_cot* storage = (composite_cot*) target->data;
        composite_cot* siteVarData = (composite_cot*) data;
        for (uint16_t i = 0, j = index; i < range; i++, j++) {
            //free, then init
            if (siteVarFree(storage[j])) {
                storage[j] = siteVarInit(siteVarData[i]->name, siteVarData[i]->type, siteVarData[i]->arrayItemCount, siteVarData[i]->data);
            }
        }
    }
    else {
        //get our location, then copy over
        void* storage = (target->data + (size * (index)));
        memcpy(storage, data, size * range);
    }

    return true;
}

bool siteVarUpdateAt(siteVar* target, uint16_t index, void* data) {
    cottageCheck(false);
    return siteVarUpdateRange(target, index, 1, data);
}

bool siteVarUpdate(siteVar* target, void* data) {
    cottageCheck(false);
    return (!target->isArray) ? siteVarUpdateAt(target, 0, data) : false;
}


//array insertion
//just implement pushBack for now

bool siteVarInsert(siteVar** target, void* data) {
    cottageCheck(false);
    if (!(*target) || (*target)->type == COMPOSITE) return false;
    printf("sitevar insert hi\n");
    size_t size = INTERNAL_siteVarTypeSize((*target)->type);
    bool isString = false;
    //we need to use a load balancer to allocate enough space.
    unsigned int load = 0;
    if ((*target)->arrayLen > 0) load = (*target)->arrayItemCount * 100 / (*target)->arrayLen;
    if (load > 60) {
        printf("resizing...\n");
        //target = INTERNAL_siteVarCompositeResize(target);
        (*target)->arrayLen <<= 1; //double the size
        (*target)->data = realloc((*target)->data, size * (*target)->arrayLen);
    }

    isString = (*target)->type == STRING;

    if (isString) {
        string_cot string = *((string_cot*)data);
        printf("sitevarinsert: string is %s\n", string);
        string_cot* stringData = (string_cot*)(*target)->data;

        string_cot insertion = (string_cot)calloc(strlen(string) + 1, sizeof(char));
        strcpy(insertion, string);
        printf("sitevarinsert: insertion is %s\n", insertion);

        stringData[(*target)->arrayItemCount] = insertion;
        printf("sitevarinsert: stringData is %s\n", stringData[(*target)->arrayItemCount]);
        (*target)->arrayItemCount++;
    }
    else {
        //assume data consists of only one value
        memcpy((*target)->data + (size * (*target)->arrayItemCount), data, size);
        (*target)->arrayItemCount++;
    }

    return true;

}

#define siteVarTransfer(type, name, function) type name = *(type*)function

#define siteVarTransfer(type, name, function) type name = *(type*)function


#endif