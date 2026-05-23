#ifndef CONVERSION_COT
#define CONVERSION_COT

#include "dependencies_cot.h"
#include "sitevar_cot.h"


bool BC_isUInt(char* exp) {
    for (int i = 0; i < strlen(exp); i++) {
        int digit = exp[i] - '0';
        if (0 <= digit && digit <= 9) continue;
        else return false;
    }
    return true;
}

uint64_t BC_StrToUInt(char* exp) {
    uint64_t number = 0;
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

//with negatives, enforcing only one negative sign at the front
//if issues arise, change to trailing negatives at the front check
bool BC_isInt(char* exp) {
    //perform a check for a solitary negative sign
	//if (exp[0] == '-' && exp[1] == 0) return false;
	if (!strcmp(exp, "-")) return false;
	
	for (int i = 0; i < strlen(exp); i++) {
		char c = exp[i];
		if (i == 0 && c == '-') { //denoting a negative
			continue;
		}
        int digit = c - '0';
        if (0 <= digit && digit <= 9) continue;
        else return false;
    }
    return true;
}

int64_t BC_StrToInt(char* exp) {
    int64_t number = 0;
    int mult = 1;
	bool isNegative = false;
    for (int i = 0; i < strlen(exp); i++) {
        char c = exp[i];
		
		if (i == 0 && c == '-') { //denoting a negative
			isNegative = true;
			continue;
		}
		//if (i != 0 && c == '-') return 0; //error
        
		number += ((c - '0') * mult);
        number *= 10;
        printf("strToNum i is %i\n", i);
    }
    number /= 10;
	if (isNegative) number *= -1;
    return number;

}


// float x = -.;

bool BC_isFloat(char* exp) {
	if ((!strcmp(exp, "-")) || (!strcmp(exp, ".")) || (!strcmp(exp, "-.")) || (!strcmp(exp, ".-"))) return false;

    bool pointFound = false;
	for (int i = 0; i < strlen(exp); i++) {
		char c = exp[i];
		if (i == 0 && c == '-') { //denoting a negative
			continue;
		}
		if (c == '.' && !pointFound) {
			pointFound = true;
			continue;
		}
        int digit = c - '0';
        if (0 <= digit && digit <= 9) continue;
        else return false;
    }
    return true;
}

double BC_StrToFloat(char* exp) {
    double number = 0;
	double pointTrail = 0;
    int mult = 1;
	double div = 10;
	bool isNegative = false;
	bool pointFound = false;
    for (int i = 0; i < strlen(exp); i++) {
        char c = exp[i];
		
		if (i == 0 && c == '-') { //denoting a negative
			isNegative = true;
			continue;
		}
		if (c == '.' && !pointFound) {
			pointFound = true;
			continue;
		}
		//if (i != 0 && c == '-') return 0; //error
        
		if (!pointFound) {
			number += ((c - '0') * mult);
			number *= 10;
		}
		else {
			//we have to convert this digit over to a number
			//divide it by div
			//then add it on to number
			double digit = (c - '0') / div;
			pointTrail += digit;
			div *= 10;
		}
        //printf("strToNum i is %i\n", i);
    }
    number /= 10;
	printf("number is %f, trailing is %f\n", number, pointTrail);
	number += pointTrail;
	if (isNegative) number *= -1;
	printf("number is %f\n", number);
    return number;

}

bool BC_isChar(char* exp) {
	//must check apostrophe bounds, and a character length of 3
	if (strlen(exp) == 3 && exp[0] == '\'' && exp[2] == '\'') return true;
	else return false;
}

char BC_StrToChar(char* exp) {
	return exp[1];
}

bool BC_isString(char* exp) {
	//must check quotation bounds and thats it
	size_t len = strlen(exp);
	if (exp[0] == '"' && exp[len - 1] == '"') return true;
	else return false;
}

char* BC_StrToStr(char* exp) {
	//just strip the border quotes
	char* new = (char*) malloc(strlen(exp) + 1);
	strcpy(new, exp);
	BC_delAt(new, 0);
	BC_delAt(new, strlen(new) - 1);
	return new;
}

bool BC_isBool(char* exp) {
	return (!strcmp(exp, "true") || !strcmp(exp, "false"));
}

bool BC_StrToBool(char* exp) {
	if (!strcmp(exp, "true")) return true;
	else if (!strcmp(exp, "false")) return false;

	//uhhhh sure
	return false;
}


// int BC_StrToNum(char* exp) {
//     int number = 0;
//     int mult = 1;
//     for (int i = 0; i < strlen(exp); i++) {
//         char c = exp[i];
//         number += ((c - '0') * mult);
//         number *= 10;
//         printf("strToNum i is %i\n", i);
//     }
//     number /= 10;
//     return number;

// }

/*
If this works, it may help solve the issue of a billion types
just add comparison type checks here!
*/

#define generalNumber double

generalNumber BC_siteVarToNumber(VARTYPE type, void* val) {
	switch (type) {
		// case INT64:  {
		// 	printf("INT\n");
		// 	return (generalNumber)(*((int64_t*)val));
		// }
		case INT:  {
            int64_t raw_val = *((int64_t*)val);
            // Print the pointer address, the exact 64-bit integer, and the casted double
            printf("INT check | Address: %p | Raw Int: %lld | Casted Double: %f\n", 
                   val, (long long)raw_val, (double)raw_val);
            return (generalNumber)raw_val;
        }
        case UINT: {
			printf("UINT\n");
			return (generalNumber)(*((uint64_t*)val));
		}
        case FLOAT: { //used to be DOUBLE
            double raw_val = *((double*)val);
            // Print the pointer address, the exact 64-bit integer, and the casted double
            printf("INT check | Address: %p | Raw Int: %lld | Casted Double: %f\n", 
                   val, (long long)raw_val, (double)raw_val);
            return (generalNumber)raw_val;
		}
        case BOOL:   {
			printf("BOOL\n");
			return (generalNumber)(*((bool*)val));
		}
        default:     {
			printf("ERROR\n");
			return 0.0; //error
		}
	}
}

VARTYPE BC_StrToType(char* exp) {
	if (BC_isUInt(exp)) return UINT;
	if (BC_isInt(exp)) return INT;
	if (BC_isFloat(exp)) return FLOAT;
	if (BC_isString(exp)) return STRING;
	if (BC_isBool(exp)) return BOOL;

	return ERROR; //no type found for this
}

/*
	must now perform variable analysis.
	var => just the variable
	var[i] => item at index i. only works for non-composite arrays
	var.subvar => variable stored inside composite. 
	can be subvar.subsubvar or subvar[i] or just subvar, you get the idea
	
	BUT NOT subvar[i].subsubvar, because this assumes that
	subvar[i] is a composite, meaning that subvar must be a composite.
	composites can only ever come from composites.
	and since i personally blocked off composites from using accessAt,
	we should account for this.

	As a result, we must check for square brackets and get the value inside
	and (when not in square brackets) for a dot, to denote a subvar

	actually, an issue with square brackets is, while yes there can be a number inside
	there can also be another variable inside with an integer value.
	as a result, this function must be recursive
*/
siteVar* BC_StrToVariable(char* exp, siteVar* variables, siteVar* originalVariables) {
	char* var = (char*)calloc(strlen(exp) + 1, sizeof(char));
	bool inBrackets = false;
	size_t i = 0;
	for (; i < strlen(exp); i++) {
		char c = exp[i];


		if (c == '.') {
			//copy over the rest of the string
			//load it into strToVariable, with variables being
			//GetVar of var
			char* remainder = (char*)calloc(strlen(exp) + 1, sizeof(char));
			//perform bounds check here
			for (size_t j = i + 1, k = 0; j < strlen(exp); j++, k++) {
				remainder[k] = exp[j];
			}
			printf("var is %s, remainder is %s\n", var, remainder);
			printf("looking for %s inside of %s\n", var, variables->name);
			siteVar* compositeVars = siteVarCompositeAccess(variables, var);
			if (!compositeVars) printf("OH NO\n");
			printf("looking for %s inside of %s\n", remainder, compositeVars->name);
			siteVar* returnVal = BC_StrToVariable(remainder, compositeVars, originalVariables);
			
			siteVarFree(compositeVars);
			free(remainder);
			free(var);

			// if (returnVal->type == STRING) {
			// 	printf("string found\n");
			// }
			return returnVal;
		}
		else if (c == '[') {
			//keep count of number of brackets seen.
			//the thing in between the brackets must be
			//either whole positive number (base)
			//or another variable (recursive)
			char* bracketVar = (char*)calloc(strlen(exp) + 1, sizeof(char));
			int16_t bracketCount = 0;
			i++;
			size_t j = 0;
			while (bracketCount >= 0) {
				if (exp[i] == '[') {
					bracketCount++;
				}
				else if (exp[i] == ']') {
					bracketCount--;
				}
				
				if (bracketCount >= 0) {
					bracketVar[j++] = exp[i];
					i++;
				}
			}

			//now, we must perform a check
			//first check for invalid data types
			//strings, floats, booleans, characters
			//for ints, no need, can convert to 
			//technically speaking, bugged with floats,
			//as you can have a whole number float with a decimal spot in there,
			//but this is a fix for later

			printf("thing inside of brackets is a variable: %s\n", bracketVar);
			printf("looking for %s inside of %s\n", var, variables->name);
			siteVar* x = siteVarCompositeAccess(variables, var);
			//if x is a composite, we cannot perform accessAt with it.
			//so the statement is invalid.
			//return NULL and do some clean up
			if (!x || (x && x->type == COMPOSITE)) {
				printf("oh crap\n");
				if (x && x->type == COMPOSITE) printf("x is a composite\n");
				else if (!x) printf("x is null\n");
				siteVarFree(x);
				free(var);
				free(bracketVar);
				return NULL;
			}

			
			if (BC_isUInt(bracketVar)) {
				//we have an index, so convert it to a number
				//and pop it in
				uint64_t index = BC_StrToUInt(bracketVar);

				void* data = siteVarAccessAt(x, index);
				if (!data) {
					siteVarFree(x);
					free(var);
					free(bracketVar);
					return NULL;
				}
				siteVar* new = siteVarInit("", x->type, 1, data);
				free(data);
				siteVarFree(x);
				free(var);
				free(bracketVar);
				return new;
			}
			else { //the thing inside is a variable
				//we kinda have to hope this is a number
				//we will use originalVariables to get stuff
				
				siteVar* indexVar = BC_StrToVariable(bracketVar, originalVariables, originalVariables);
				if (!indexVar || (indexVar->type == STRING || indexVar->type == BOOL)) {
					siteVarFree(x);
					free(var);
					free(bracketVar);
					return NULL;
				}
				//make sure its a number aka not a string or boolean, but doubles ill allow?
				// if (indexVar->type == STRING || indexVar->type == BOOL) {

				// }
				printf("strToVariable bracket pass\n");

				void* indexData = siteVarAccess(indexVar);
				if (!indexData) {
					siteVarFree(indexVar);
					siteVarFree(x);
					free(var);
					free(bracketVar);
					return NULL;
				}

				generalNumber index = BC_siteVarToNumber(indexVar->type, indexData);
				printf("bracket index is %lf\n", index);
				void* data = siteVarAccessAt(x, index);
				if (!data) {
					siteVarFree(indexVar);
					siteVarFree(x);
					free(indexData);
					free(var);
					free(bracketVar);
					return NULL;
				}
				siteVar* new = siteVarInit("", x->type, 1, data);
				
				free(data);
				siteVarFree(indexVar);
				siteVarFree(x);
				free(indexData);
				free(var);
				free(bracketVar);
				
				return new;	
			}
		}

		else var[i] = c;
	}

	//if the var was completely untouched, no brackets no dots no anything,
	//just get the site var
	siteVar* returnVal =  siteVarCompositeAccess(variables, var);
	free(var);
	return returnVal;
}



//conversions from type to string, like itoa

char* BC_IntToStr(int_cot target) {
    //to get the number of digits, use log base 10, truncate it, then add 1
    size_t digitCount = ((size_t)log10(target)) + 1;
    char* buffer = (char*) calloc(digitCount + 1, sizeof(char));
    char* ptr = buffer;
    ptr += sprintf(ptr, "%i", target);

    return buffer;
}

char* BC_UIntToStr(uint_cot target) {
    //to get the number of digits, use log base 10, truncate it, then add 1
    size_t digitCount = ((size_t)log10(target)) + 1;
    char* buffer = (char*) calloc(digitCount + 1, sizeof(char));
    char* ptr = buffer;
    ptr += sprintf(ptr, "%u", target);

    return buffer;
}

//different number strategy
char* BC_FloatToStr(float_cot target) {
    //to get the number of digits, use log base 10, truncate it, then add 1
    size_t digitCount = snprintf(NULL, 0, "%d", target);
    char* buffer = (char*) calloc(digitCount + 1, sizeof(char));
    char* ptr = buffer;
    ptr += snprintf(ptr, digitCount + 1, "%d", target);

    return buffer;
}

char* BC_BoolToStr(bool_cot target) {
    char* buffer = (char*) calloc(5 + 1, sizeof(char)); //false has 5 characters
    char* ptr = buffer;
    ptr += sprintf(ptr, "%s", target ? "true" : "false");

    return buffer;
}

char* BC_VariableToString(siteVar* variable, size_t index) {
    if (!variable) return NULL;

    switch(variable->type) {
        case INT: {
            int_cot* value = (int_cot*)siteVarAccessAt(variable, index);
            char* buffer = BC_IntToStr(*value);
            free(value);
            return buffer;
        }
        case UINT: {
            uint_cot* value = (uint_cot*)siteVarAccessAt(variable, index);
            char* buffer = BC_UIntToStr(*value);
            free(value);
            return buffer;
        }
        case FLOAT: {
            float_cot* value = (float_cot*)siteVarAccessAt(variable, index);
            char* buffer = BC_FloatToStr(*value);
            free(value);
            return buffer;
        }
        case BOOL: {
            bool_cot* value = (bool_cot*)siteVarAccessAt(variable, index);
            char* buffer = BC_BoolToStr(*value);
            free(value);
            return buffer;
        }
        default: {
            //just return the name
            // char* buffer = (char*) calloc(strlen(variable->name) + 1, sizeof(char));
            // strcpy(buffer, variable->name);
            // return buffer;
            return NULL;
        }
    }
}


bool BC_isArray(char* exp) {
    if (!exp) return false;
    return (exp[0] =='[' && exp[strlen(exp) - 1] == ']');
}

siteVar* BC_ArrayToSiteVar(char* name, char* exp, siteVar* variables) {
	//elements are divided by commas
	//they cannot be arrays themselves.
	//they must all be of the same type, with the first element as a reference point
	//existing variables can exist here as well
	//values separated by commas
	char arrayVar[512] = {0};

	VARTYPE arrayType = ERROR;
	bool typeFound = false;
	siteVar* storage = NULL;

	//we need some storage for each variable.
	//make a void* container

	size_t i = 1; //starting from not the bracket

	while (i < strlen(exp)) {
		size_t k = 0;
		while (exp[i] != ',' && i < strlen(exp)) { //read until next variable
			arrayVar[k++] = exp[i++];
		}
		if (i >= strlen(exp)) { //reached the end
			continue; //end early
		}
		i++; //to skip the comma

		VARTYPE type = BC_StrToType(arrayVar);
		if (!typeFound) {
			arrayType = type;
			typeFound = true;
			if (arrayType == ERROR) { //must be a variable, we dont set the type yet

			}
			else storage = siteVarInit(name, arrayType, 2, NULL); //2 to make it an array
		}
		else {
			if (type != arrayType) goto failure;
		}
		//void* storage = NULL;
		switch (type) {
			case INT: {
				int_cot arrayVal = BC_StrToInt(arrayVar);
				siteVarInsert(storage, &arrayVal);
				break;
			}
			case UINT: {
				uint_cot arrayVal = BC_StrToUInt(arrayVar);
				siteVarInsert(storage, &arrayVal);
				break;
			}
			case FLOAT: {
				float_cot arrayVal = BC_StrToFloat(arrayVar);
				siteVarInsert(storage, &arrayVal);
				break;
			}
			case BOOL: {
				bool_cot arrayVal = BC_StrToBool(arrayVar);
				siteVarInsert(storage, &arrayVal);
				break;
			}
			case STRING: {
				string_cot arrayVal = BC_StrToStr(arrayVar);
				siteVarInsert(storage, &arrayVal);
				free(arrayVal);
				break;
			}
			default: { //its a variable
				siteVar* arrayVal = BC_StrToVariable(arrayVal, variables, variables);
				if (!arrayVal) {
					siteVarFree(arrayVal);
					goto failure;
				}
				//if this is a composite, we must throw failure
				//because it assumes we have an array of composites
				//which is not allowed
				if (arrayVal->type == COMPOSITE) goto failure;

				//initialise storage here if not done yet
				if (!typeFound) {
					typeFound = true;
					arrayType = arrayVal->type;
					storage = siteVarInit(name, arrayType, 2, NULL);
				}

				//can only be one element
				void* arrayVal_Value = siteVarAccess(arrayVal);
				siteVarInsert(storage, arrayVal);
				free(arrayVal_Value);
				siteVarFree(arrayVal);

			}
		}

		//reset arrayVar
		memset(arrayVar, 0, strlen(arrayVar));
	}

	return storage;

	failure:
	siteVarFree(storage);
	return NULL;
}


#endif