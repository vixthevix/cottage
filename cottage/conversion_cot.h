#ifndef CONVERSION_COT
#define CONVERSION_COT

#include "dependencies_cot.h"
#include "error_cot.h"
#include "sitevar_cot.h"
#include "init_cot.h"


void BC_putAt(char* exp, int i, char c) {
	cottageCheck();
	if (i < 0) return;
	
	//shift everything up 1
	for (int j = strlen(exp); j > i; j--) {
		exp[j] = exp[j - 1];
	}
	exp[i] = c;

}

void BC_delAt(char* exp, int i) {
	cottageCheck();
	if (i < 0 || i >= strlen(exp)) return;
	
	//shift everything up 1
	int n = strlen(exp);
	for (int j = i; j < strlen(exp); j++) {
		exp[j] = exp[j + 1];
	}
}

bool BC_isDoubleOperator(char c) {
	cottageCheck(false);
	return (c == '=' || c == '&' || c == '|' || c == '^'); //special case for ! potentially
}

bool BC_isUInt(char* exp) {
	cottageCheck(false);
	uint64_t number = 0, digitCount = 0;

	const uint64_t 
	uintMax = UINT64_MAX,
	digitMax = ((uint64_t)log10(uintMax)) + 1;

    for (int i = 0; i < strlen(exp); i++) {
        int digit = exp[i] - '0';
        if (0 <= digit && digit <= 9) continue;
        else return false;
		//number = (number * 10) + (uint64_t)digit;
		//digitCount++;

		//if (digitCount > digitMax || number > uintMax) return false;
    }
    return true;
}

uint_cot BC_StrToUInt(char* exp) {
	cottageCheck(0);
    uint_cot number = 0;
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
	cottageCheck(false);
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
	cottageCheck(0);
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
	cottageCheck(false);
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
	cottageCheck(0);
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
	cottageCheck(false);
	//must check apostrophe bounds, and a character length of 3
	if (strlen(exp) == 3 && exp[0] == '\'' && exp[2] == '\'') return true;
	else return false;
}

char BC_StrToChar(char* exp) {
	cottageCheck(0);
	return exp[1];
}

bool BC_isString(char* exp) {
	cottageCheck(false);
	if (!exp) return false;
	//must check quotation bounds and thats it
	size_t len = strlen(exp);
	if (exp[0] == '"' && exp[len - 1] == '"') return true;
	else return false;
}

char* BC_StrToStr(char* exp) {
	cottageCheck(NULL);
	if (!exp) return NULL;
	//just strip the border quotes
	char* new = (char*) malloc(strlen(exp) + 1);
	strcpy(new, exp);
	BC_delAt(new, 0);
	BC_delAt(new, strlen(new) - 1);
	return new;
}

bool BC_isBool(char* exp) {
	cottageCheck(false);
	return (!strcmp(exp, "true") || !strcmp(exp, "false"));
}

bool BC_StrToBool(char* exp) {
	cottageCheck(false);
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

cotResult BC_siteVarToNumber(generalNumber* input, VARTYPE type, void* val) {
	// cottageCheck((cotResult){0});
	switch (type) {
		// case INT64:  {
		// 	printf("INT\n");
		// 	return (generalNumber)(*((int64_t*)val));
		// }
		case INT:  {
            int_cot raw_val = *((int_cot*)val);
            // Print the pointer address, the exact 64-bit integer, and the casted double
            printf("INT check | Address: %p | Raw Int: %lld | Casted Double: %f\n", 
                   val, (long long)raw_val, (double)raw_val);
			*input = (generalNumber)raw_val;	
		    break;
        }
        case UINT: {
            uint_cot raw_val = *((uint_cot*)val);
            // Print the pointer address, the exact 64-bit integer, and the casted double
            printf("INT check | Address: %p | Raw Int: %lld | Casted Double: %f\n", 
                   val, (long long)raw_val, (double)raw_val);
			*input = (generalNumber)raw_val;	
		    break;
		}
        case FLOAT: { //used to be DOUBLE
            double raw_val = *((double*)val);
            // Print the pointer address, the exact 64-bit integer, and the casted double
            printf("INT check | Address: %p | Raw Int: %lld | Casted Double: %f\n", 
                   val, (long long)raw_val, (double)raw_val);
			*input = (generalNumber)raw_val;	
		    break;
		}
        default:     {
			printf("ERROR\n");
			return newResultError("BC_siteVarToNumber: invalid VARTYPE for conversion");
		}
	}

	return newResultOK();
}

VARTYPE BC_StrToType(char* exp) {
	cottageCheck(ERROR);
	if (BC_isUInt(exp)) return UINT;
	if (BC_isInt(exp)) return INT;
	if (BC_isFloat(exp)) return FLOAT;
	if (BC_isString(exp)) return STRING;
	if (BC_isBool(exp)) return BOOL;

	return ERROR; //no type found for this
}

// void* BC_StrToData(char* exp) {
// 	VARTYPE type = BC_StrToType(exp);



// 	switch (type) {
// 		case UINT: {

// 			break;
// 		}
// 		case UINT: {
// 			break;
// 		}
// 		case UINT: {
// 			break;
// 		}
// 		case UINT: {
// 			break;
// 		}
// 		case UINT: {
// 			break;
// 		}
// 		default: {
// 			return NULL;
// 			break;
// 		}
// 	}
// }

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
cotResult BC_StrToVariable(siteVar** input, char* exp, siteVar* variables, siteVar* originalVariables) {
	//cottageCheck(NULL);
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
			siteVar* returnVal = NULL;
			cotResult returnValResult = BC_StrToVariable(&returnVal, remainder, compositeVars, originalVariables);
			if (returnValResult.status == COT_ERROR) {
				return newResultError("BC_StrToVariable: could not process '.'");
			}

			if (returnVal) printf("returnVal found\n");
			else printf("returnVal Not found\n");

			siteVarFree(compositeVars);
			free(remainder);
			free(var);

			// if (returnVal->type == STRING) {
			// 	printf("string found\n");
			// }
			return newResultOK();
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
				//printf("oh crap\n");
				//if (x && x->type == COMPOSITE) printf("x is a composite\n");
				//else if (!x) printf("x is null\n");
				
				siteVarFree(x);
				free(var);
				free(bracketVar);
				if (x) return newResultError("BC_StrToVariable: trying to access an index of a COMPOSITE siteVar");
				else return newResultError("BC_StrToVariable: could not find variable within known variables");
				//return NULL;
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
					return newResultError("BC_StrToVariable: could not access data in siteVar with UINT index");
				}
				siteVar* new = siteVarInit("", x->type, 1, data);
				free(data);
				siteVarFree(x);
				free(var);
				free(bracketVar);
				*input = new;
				return newResultOK();
			}
			else { //the thing inside is a variable
				//we kinda have to hope this is a number
				//we will use originalVariables to get stuff
				siteVar* indexVar = NULL;
				cotResult indexVarResult = BC_StrToVariable(&indexVar, bracketVar, originalVariables, originalVariables);
				if (indexVarResult.status == COT_ERROR) {
					siteVarFree(x);
					free(var);
					free(bracketVar);
					return newResultError("BC_StrToVariable: could not convert indexVar");
				}
				
				if ((indexVar->type == STRING || indexVar->type == BOOL)) {
					siteVarFree(x);
					free(var);
					free(bracketVar);
					return newResultError("BC_StrToVariable: indexVar found, but of invalid type (either STRING or BOOL)");
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
					return newResultError("BC_StrToVariable: could not access value inside indexVar");
				}
				generalNumber index = 0;
				cotResult indexResult = BC_siteVarToNumber(&index, indexVar->type, indexData);
				if (indexResult.status == COT_ERROR) {
					siteVarFree(indexVar);
					siteVarFree(x);
					free(indexData);
					free(var);
					free(bracketVar);
					return newResultError("BC_StrToVariable: could not convert index into a number");
				}
				printf("bracket index is %lf\n", index);
				void* data = siteVarAccessAt(x, index);
				if (!data) {
					siteVarFree(indexVar);
					siteVarFree(x);
					free(indexData);
					free(var);
					free(bracketVar);
					return newResultError("BC_StrToVariable: could not access data at index");
				}
				siteVar* new = siteVarInit("", x->type, 1, data);
				
				free(data);
				siteVarFree(indexVar);
				siteVarFree(x);
				free(indexData);
				free(var);
				free(bracketVar);
				*input = new;
				return newResultOK();	
			}
		}

		else var[i] = c;
	}

	//if the var was completely untouched, no brackets no dots no anything,
	//just get the site var
	siteVar* returnVal =  siteVarCompositeAccess(variables, var);
	free(var);
	if (!returnVal) return newResultError("BC_StrToVariable: could not access simple var in variables");
	*input = returnVal;
	return newResultOK();
}



//conversions from type to string, like itoa

char* BC_IntToStr(int_cot target) {
	cottageCheck(NULL);
	
	bool isNegative = false;

	if (target == 0) {
		char* buffer = (char*) calloc(2, sizeof(char));
		sprintf(buffer, "0");
		return buffer;
	}
	else if (target < 0) {
		isNegative = true;
		target *= -1;
	}
    //to get the number of digits, use log base 10, truncate it, then add 1
    size_t digitCount = ((size_t)log10(target)) + 1;
    char* buffer = (char*) calloc(digitCount + 1 + 1 , sizeof(char));
    sprintf(buffer, "%s%li", isNegative ? "-":"", target);
    return buffer;
}

char* BC_UIntToStr(uint_cot target) {
	cottageCheck(NULL);
	if (target == 0) {
		char* buffer = (char*) calloc(2, sizeof(char));
		sprintf(buffer, "0");
		return buffer;
	}
    //to get the number of digits, use log base 10, truncate it, then add 1
    size_t digitCount = ((size_t)log10(target)) + 1;
    char* buffer = (char*) calloc(digitCount + 1, sizeof(char));
    char* ptr = buffer;
    ptr += sprintf(ptr, "%lu", target);

    return buffer;
}

//different number strategy
char* BC_FloatToStr(float_cot target) {
	cottageCheck(NULL);
    //to get the number of digits, use log base 10, truncate it, then add 1
    size_t digitCount = snprintf(NULL, 0, "%lf", target);
    char* buffer = (char*) calloc(digitCount + 1, sizeof(char));
    char* ptr = buffer;
    ptr += snprintf(ptr, digitCount + 1, "%lf", target);

    return buffer;
}

char* BC_BoolToStr(bool_cot target) {
	cottageCheck(NULL);
    char* buffer = (char*) calloc(5 + 1, sizeof(char)); //false has 5 characters
    char* ptr = buffer;
    ptr += sprintf(ptr, "%s", target ? "true" : "false");

    return buffer;
}

char* BC_VariableToString(siteVar* variable, size_t index) {
	cottageCheck(NULL);
    if (!variable) return NULL;
	printf("hi\n");

    switch(variable->type) {
        case INT: {
			printf("yep its an int\n");
            int_cot* value = (int_cot*)siteVarAccessAt(variable, index);
			printf("did we get it\n");
			if (!value) printf("value not got\n");
            char* buffer = BC_IntToStr(*value);
			printf("buffer made\n");
            free(value);
            return buffer;
        }
        case UINT: {
            uint_cot* value = (uint_cot*)siteVarAccessAt(variable, index);
			printf("uint value is %u\n", *value);
            char* buffer = BC_UIntToStr(*value);
			printf("buffer made\n");
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
		case STRING: {
			printf("variable to string, its a string\n");
			string_cot* value = (string_cot*)siteVarAccessAt(variable, index);
            if (!value) printf("variable to string, string vale invalid\n");
			else printf("variable to string, string value valid: %u\n", value);
			return *value;

		}
        default: {
			//we can maybe print to stderror anyway
			newResultError("BC_VariableToString: variable is of an invalid type");
            return NULL;
        }
    }
}


bool BC_isArray(char* exp) {
	cottageCheck(false);
    if (!exp) return false;
    return (exp[0] =='[' && exp[strlen(exp) - 1] == ']');
}


/*
in the future, add a heirarchy of numbers.
in order of lowest to highest priority:
UINT, INT, FLOAT
look out for the other types. if they appear, we throw an error.
otherwise, we change the type of the array to fit what we have.
*/
cotResult BC_ArrayToSiteVar(siteVar** input, char* name, char* exp, siteVar* variables) {
	//cottageCheck(NULL);
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

	while (i < strlen(exp) - 1) {
		size_t k = 0;
		while (exp[i] != ',' && i < strlen(exp) - 1) { //read until next variable
			arrayVar[k++] = exp[i++];
		}
		if (i >= strlen(exp) - 1) { //reached the end
			continue; //end early
		}
		i++; //to skip the comma

		printf("arraytositevar: arrayVar is %s\n", arrayVar);

		VARTYPE type = BC_StrToType(arrayVar);
		printf("arraytositevar: type is %i\n", type);
		if (!typeFound) {
			arrayType = type;
			typeFound = true;
			if (arrayType == ERROR) { //must be a variable, we dont set the type yet

			}
			else {
				printf("doing this now\n");
				storage = siteVarInit(name, arrayType, 0, NULL); //2 to make it an array
				storage->isArray = true;
				printf("this has been done\n");
			}
		}
		else {
			if (type != arrayType) {
				siteVarFree(storage);
				return newResultError("BC_ArrayToSiteVar: values in array are not of a consistent type");
			}
		}
		//void* storage = NULL;
		switch (type) {
			case INT: {
				int_cot arrayVal = BC_StrToInt(arrayVar);
				siteVarInsert(&storage, &arrayVal);
				break;
			}
			case UINT: {
				uint_cot arrayVal = BC_StrToUInt(arrayVar);
				siteVarInsert(&storage, &arrayVal);
				break;
			}
			case FLOAT: {
				float_cot arrayVal = BC_StrToFloat(arrayVar);
				siteVarInsert(&storage, &arrayVal);
				break;
			}
			case BOOL: {
				bool_cot arrayVal = BC_StrToBool(arrayVar);
				siteVarInsert(&storage, &arrayVal);
				break;
			}
			case STRING: {
				printf("arraytositevar: its a string\n");
				string_cot arrayVal = BC_StrToStr(arrayVar);
				printf("arrayVal is %s\n", arrayVal);
				siteVarInsert(&storage, &arrayVal);
				free(arrayVal);
				break;
			}
			default: { //its a variable
				siteVar* arrayVal = NULL;
				cotResult arrayValResult = BC_StrToVariable(&arrayVal, arrayVar, variables, variables);
				if (arrayValResult.status == COT_ERROR) {
					//siteVarFree(arrayVal);
					siteVarFree(storage);
					return newResultError("BC_ArrayToSiteVar: variable in array could not be accessed");
				}
				//if this is a composite, we must throw failure
				//because it assumes we have an array of composites
				//which is not allowed
				if (arrayVal->type == COMPOSITE) {
					siteVarFree(storage);
					return newResultError("BC_ArrayToSiteVar: variable in array is a COMPOSITE, not allowed");
				}

				//initialise storage here if not done yet
				if (!typeFound) {
					typeFound = true;
					arrayType = arrayVal->type;
					storage = siteVarInit(name, arrayType, 2, NULL);
				}

				//can only be one element
				void* arrayVal_Value = siteVarAccess(arrayVal);
				if (!arrayVal_Value) {
					siteVarFree(arrayVal);
					siteVarFree(storage);
					return newResultError("BC_ArrayToSiteVar: could not access data in variable in array");
				}
				siteVarInsert(&storage, arrayVal_Value);
				free(arrayVal_Value);
				siteVarFree(arrayVal);

			}
		}

		//reset arrayVar
		memset(arrayVar, 0, strlen(arrayVar));
	}

	printf("arraytositevar: arrayVar is %s\n", arrayVar);

	VARTYPE type = BC_StrToType(arrayVar);
	printf("arraytositevar: type is %i\n", type);
	if (!typeFound) {
		arrayType = type;
		typeFound = true;
		if (arrayType == ERROR) { //must be a variable, we dont set the type yet

		}
		else {
			printf("doing this now\n");
			storage = siteVarInit(name, arrayType, 0, NULL); //2 to make it an array
			storage->isArray = true;
			printf("this has been done\n");
		}
	}
	else {
		if (type != arrayType) {
			siteVarFree(storage);
			return newResultError("BC_ArrayToSiteVar: values in array are not of a consistent type");
		}
	}
	//void* storage = NULL;
	switch (type) {
		case INT: {
			int_cot arrayVal = BC_StrToInt(arrayVar);
			siteVarInsert(&storage, &arrayVal);
			break;
		}
		case UINT: {
			uint_cot arrayVal = BC_StrToUInt(arrayVar);
			siteVarInsert(&storage, &arrayVal);
			break;
		}
		case FLOAT: {
			float_cot arrayVal = BC_StrToFloat(arrayVar);
			siteVarInsert(&storage, &arrayVal);
			break;
		}
		case BOOL: {
			bool_cot arrayVal = BC_StrToBool(arrayVar);
			siteVarInsert(&storage, &arrayVal);
			break;
		}
		case STRING: {
			printf("arraytositevar: its a string\n");
			string_cot arrayVal = BC_StrToStr(arrayVar);
			printf("arrayVal is %s\n", arrayVal);
			siteVarInsert(&storage, &arrayVal);
			free(arrayVal);
			break;
		}
		default: { //its a variable
			siteVar* arrayVal = NULL;
			cotResult arrayValResult = BC_StrToVariable(&arrayVal, arrayVar, variables, variables);
			if (arrayValResult.status == COT_ERROR) {
				//siteVarFree(arrayVal);
				siteVarFree(storage);
				return newResultError("BC_ArrayToSiteVar: variable in array could not be accessed");
			}
			//if this is a composite, we must throw failure
			//because it assumes we have an array of composites
			//which is not allowed
			if (arrayVal->type == COMPOSITE) {
				siteVarFree(storage);
				return newResultError("BC_ArrayToSiteVar: variable in array is a COMPOSITE, not allowed");
			}

			//initialise storage here if not done yet
			if (!typeFound) {
				typeFound = true;
				arrayType = arrayVal->type;
				storage = siteVarInit(name, arrayType, 2, NULL);
			}

			//can only be one element
			void* arrayVal_Value = siteVarAccess(arrayVal);
			if (!arrayVal_Value) {
				siteVarFree(arrayVal);
				siteVarFree(storage);
				return newResultError("BC_ArrayToSiteVar: could not access data in variable in array");
			}
			siteVarInsert(&storage, arrayVal_Value);
			free(arrayVal_Value);
			siteVarFree(arrayVal);

		}
	}	

	*input = storage;
	return newResultOK();
}


#endif