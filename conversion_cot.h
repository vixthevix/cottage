/*
Functions for converting between different variable types.

Code is part of the cottage framework (https://github.com/vixthevix/cottage)
*/

#ifndef CONVERSION_COT
#define CONVERSION_COT

#include "dependencies_cot.h"
#include "error_cot.h"
#include "sitevar_cot.h"
#include "init_cot.h"

//Defines a number that all of cottage's numerical types
//can be type casted into.
typedef double generalNumber;

void BC_putAt(char* exp, int i, char c);
void BC_delAt(char* exp, int i);
bool BC_isUInt(char* exp);
uint_cot BC_StrToUInt(char* exp);
bool BC_isInt(char* exp);
int_cot BC_StrToInt(char* exp);
bool BC_isFloat(char* exp);
float_cot BC_StrToFloat(char* exp);
bool BC_isString(char* exp);
string_cot BC_StrToStr(char* exp);
bool BC_isBool(char* exp);
bool_cot BC_StrToBool(char* exp);
cotResult BC_siteVarToNumber(generalNumber* input, VARTYPE type, void* val);
VARTYPE BC_StrToType(char* exp);
cotResult BC_StrToVariable(siteVar** input, char* exp, siteVar* variables, siteVar* originalVariables);
char* BC_IntToStr(int_cot target);
char* BC_UIntToStr(uint_cot target);
char* BC_FloatToStr(float_cot target);
char* BC_BoolToStr(bool_cot target);
char* BC_VariableToStringAt(siteVar* variable, size_t index);
char* BC_VariableToString(siteVar* variable);
bool BC_isArray(char* exp);
cotResult BC_ArrayToSiteVar(siteVar** input, char* name, char* exp, siteVar* variables);

#if defined(COTTAGE_START)

/*
Inserts a character at an index of a string.
WARNING: THIS IS AN UNSAFE FUNCTION. 
Ensure exp has enough allocated space to safely use this function.
@arg exp -> string to add character to.
@arg i -> index to put character at.
@arg c -> character to insert.
*/
void BC_putAt(char* exp, int i, char c) {
	cottageCheck();
	if (i < 0 || !exp || i >= strlen(exp)) return;
	
	//shift everything up 1
	for (int j = strlen(exp); j > i; j--) {
		exp[j] = exp[j - 1];
	}
	exp[i] = c;

}

/*
Deletes a character at an index of a string.
@arg exp -> string to delete character from.
@arg i -> index to delete character at.
*/
void BC_delAt(char* exp, int i) {
	cottageCheck();
	if (i < 0 || !exp || i >= strlen(exp)) return;
	
	//shift everything up 1
	int n = strlen(exp);
	for (int j = i; j < strlen(exp); j++) {
		exp[j] = exp[j + 1];
	}
	//In this way, the character at i is overwritten and thus deleted
}

/*
Checks if contents of string matches form of a uint_cot.
@arg exp -> string to check.
@return status of check.
*/
bool BC_isUInt(char* exp) {
	cottageCheck(false);
	if (!exp || strlen(exp) == 0) return false;

	// const uint64_t 
	// uintMax = UINT64_MAX,
	// digitMax = ((uint64_t)log10(uintMax)) + 1;

    for (int i = 0; i < strlen(exp); i++) {
        int digit = exp[i] - '0';
        if (0 <= digit && digit <= 9) continue;
        else return false;
    }
    return true;
}


/*
Extracts a uint_cot from a string.
@arg exp -> string to extract from.
@return data in string.
*/
uint_cot BC_StrToUInt(char* exp) {
	cottageCheck(0);
	if (!exp || strlen(exp) == 0) return 0;
    uint_cot number = 0;
    int mult = 1;
    for (int i = 0; i < strlen(exp); i++) {
        char c = exp[i];
        number += ((c - '0') * mult);
        number *= 10;
    }
    number /= 10;
    return number;

}

/*
Checks if contents of string matches form of an int_cot.
@arg exp -> string to check.
@return status of check.
*/
bool BC_isInt(char* exp) {
	cottageCheck(false);
	if (!exp || strlen(exp) == 0) return false;
	if (strcmp(exp, "-") == 0) return false;
	
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

/*
Extracts an int_cot from a string.
@arg exp -> string to extract from.
@return data in string.
*/
int_cot BC_StrToInt(char* exp) {
	cottageCheck(0);
	if (!exp || strlen(exp) == 0) return 0;
    
	int_cot number = 0;
    int mult = 1;
	bool isNegative = false;
    for (int i = 0; i < strlen(exp); i++) {
        char c = exp[i];
		
		if (i == 0 && c == '-') { //denoting a negative
			isNegative = true;
			continue;
		}
        
		number *= 10;
		number += ((c - '0') * mult);
    }
	
	if (isNegative) number *= -1;
    return number;
}


/*
Checks if contents of string matches form of a float_cot.
@arg exp -> string to check.
@return status of check.
*/
bool BC_isFloat(char* exp) {
	cottageCheck(false);
	if (!exp || strlen(exp) == 0) return 0;
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

/*
Extracts a float_cot from a string.
@arg exp -> string to extract from.
@return data in string.
*/
float_cot BC_StrToFloat(char* exp) {
	cottageCheck(0);
	if (!exp || strlen(exp) == 0) return 0;

    float_cot number = 0;
	float_cot pointTrail = 0;
    int mult = 1;
	float_cot div = 10;
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
        
		if (!pointFound) {
			number *= 10;
			number += ((c - '0') * mult);
		}
		else {
			double digit = (c - '0') / div;
			pointTrail += digit;
			div *= 10;
		}
    }
	number += pointTrail;
	if (isNegative) number *= -1;
    return number;
}

/*
Checks if contents of string matches form of a string_cot.
@arg exp -> string to check.
@return status of check.
*/
bool BC_isString(char* exp) {
	cottageCheck(false);
	if (!exp || strlen(exp) < 2) return false;
	size_t len = strlen(exp);
	if (exp[0] == '"' && exp[len - 1] == '"') return true;
	else return false;
}

/*
Extracts a string_cot from a string.
@arg exp -> string to extract from.
@return data in string.
*/
string_cot BC_StrToStr(char* exp) {
	cottageCheck(NULL);
	if (!exp) return NULL;
	//just strip the border quotes
	string_cot new = (string_cot) malloc(strlen(exp) + 1);
	strcpy(new, exp);
	BC_delAt(new, 0);
	BC_delAt(new, strlen(new) - 1);
	return new;
}

/*
Checks if contents of string matches form of a bool_cot.
@arg exp -> string to check.
@return status of check.
*/
bool BC_isBool(char* exp) {
	cottageCheck(false);
	if (!exp || strlen(exp) == 0) return false;
	return ((strcmp(exp, "true") == 0) || (strcmp(exp, "false") == 0));
}

/*
Extracts a bool_cot from a string.
@arg exp -> string to extract from.
@return data in string.
*/
bool_cot BC_StrToBool(char* exp) {
	cottageCheck(false);
	if (!exp || strlen(exp) == 0) return false;
	if (!strcmp(exp, "true")) return true;
	else if (!strcmp(exp, "false")) return false;

	return false;
}

/*
Converts a siteVar's contents into a generalNumber.
@arg input -> stores result of conversion.
@arg type -> data type of siteVar to convert.
@arg val -> data container of siteVar to convert.
*/
cotResult BC_siteVarToNumber(generalNumber* input, VARTYPE type, void* val) {
	cottageCheck(newResultError("BC_siteVarToNumber: cottage not initialised."));
	switch (type) {
		case INT:  {
            int_cot raw_val = *((int_cot*)val);
			*input = (generalNumber)raw_val;	
		    break;
        }
        case UINT: {
            uint_cot raw_val = *((uint_cot*)val);
			*input = (generalNumber)raw_val;	
		    break;
		}
        case FLOAT: {
            double raw_val = *((double*)val);
			*input = (generalNumber)raw_val;	
		    break;
		}
        default:     {
			return newResultError("BC_siteVarToNumber: invalid VARTYPE for conversion");
		}
	}

	return newResultOK();
}

/*
Checks for the type of a string.
@arg exp -> string to check.
@return type of string.
*/
VARTYPE BC_StrToType(char* exp) {
	cottageCheck(ERROR);
	if (BC_isUInt(exp)) return UINT;
	if (BC_isInt(exp)) return INT;
	if (BC_isFloat(exp)) return FLOAT;
	if (BC_isString(exp)) return STRING;
	if (BC_isBool(exp)) return BOOL;

	return ERROR; //no type found for this
}

/*
Retrieves siteVar data from variables, given a string to read the variable from.
@arg input -> stores the target siteVar.
@arg exp -> the string to read from.
@arg variables -> where to look for target siteVar.
@arg originalVariables -> used to allow for proper recursion.
@return error status of retrieval.
*/
cotResult BC_StrToVariable(siteVar** input, char* exp, siteVar* variables, siteVar* originalVariables) {
	cottageCheck(newResultError("BC_StrToVariable: cottage not initialised."));
	
	//current part of variable we are checking.
	char* var = (char*)calloc(strlen(exp) + 1, sizeof(char));
	bool inBrackets = false;
	
	size_t i = 0;
	for (; i < strlen(exp); i++) {
		char c = exp[i];


		if (c == '.') {
			//'.' means 
			//"treat the variable before the dot as a COMPOSITE,
			//and the variable after as a subvariable of this COMPOSITE"

			char* remainder = (char*)calloc(strlen(exp) + 1, sizeof(char));
			for (size_t j = i + 1, k = 0; j < strlen(exp); j++, k++) {
				remainder[k] = exp[j];
			}

			siteVar* compositeVars = siteVarCompositeAccess(variables, var);
			siteVar* returnVal = NULL;
			
			//recursion
			cotResult returnValResult = BC_StrToVariable(&returnVal, remainder, compositeVars, originalVariables);
			if (returnValResult.status == COT_ERROR) {
				siteVarFree(compositeVars);
				if (remainder) free(remainder);
				if (var) free(var);
				return newResultError("BC_StrToVariable: could not process '.'");
			}

			siteVarFree(compositeVars);
			free(remainder);
			free(var);

			*input = returnVal;

			return newResultOK();
		}
		else if (c == '[') {
			//Square brackets indicate accessing an item in an array.

			char* bracketVar = (char*)calloc(strlen(exp) + 1, sizeof(char));
			int16_t bracketCount = 0;
			i++;
			size_t j = 0;
			while (bracketCount >= 0 && i < strlen(exp)) {
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

			siteVar* x = siteVarCompositeAccess(variables, var);
			if (!x || (x && x->type == COMPOSITE)) {
				siteVarFree(x);
				free(var);
				free(bracketVar);
				if (x) return newResultError("BC_StrToVariable: trying to access an index of a COMPOSITE siteVar");
				else return newResultError("BC_StrToVariable: could not find variable within known variables");
			}

			
			if (BC_isUInt(bracketVar)) {
				//Treat bracketVar as a numerical index.

				uint64_t index = BC_StrToUInt(bracketVar);

				void* data = siteVarAccessAt(x, index);
				if (!data) {
					siteVarFree(x);
					free(var);
					free(bracketVar);
					return newResultError("BC_StrToVariable: could not access data in siteVar with UINT index");
				}
				siteVar* new = siteVarInit("arrayitem", x->type, 1, data);
				free(data);
				siteVarFree(x);
				free(var);
				free(bracketVar);
				*input = new;
				return newResultOK();
			}
			else {
				//Treat bracketVar as a variable.
				
				siteVar* indexVar = NULL;
				cotResult indexVarResult = BC_StrToVariable(&indexVar, bracketVar, originalVariables, originalVariables);
				if (indexVarResult.status == COT_ERROR) {
					siteVarFree(x);
					free(var);
					free(bracketVar);
					return newResultError("BC_StrToVariable: could not convert indexVar");
				}
				
				if ((indexVar->type == STRING || indexVar->type == BOOL)) {
					siteVarFree(indexVar);
					siteVarFree(x);
					free(var);
					free(bracketVar);
					return newResultError("BC_StrToVariable: indexVar found, but of invalid type (either STRING or BOOL)");
				}

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

				void* data = siteVarAccessAt(x, index);
				if (!data) {
					siteVarFree(indexVar);
					siteVarFree(x);
					free(indexData);
					free(var);
					free(bracketVar);
					return newResultError("BC_StrToVariable: could not access data at index");
				}

				siteVar* new = siteVarInit("arrayitem", x->type, 1, data);
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

	//We have a complete var, so treat it as a siteVar name.
	siteVar* returnVal =  siteVarCompositeAccess(variables, var);
	free(var);
	if (!returnVal) return newResultError("BC_StrToVariable: could not access simple var in variables");
	*input = returnVal;
	return newResultOK();
}


/*
Puts an int_cot into string form.
@arg target -> target to transform.
@return target in string form.
*/
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
    if (buffer) sprintf(buffer, "%s%li", isNegative ? "-":"", target);
    return buffer;
}

/*
Puts a uint_cot into string form.
@arg target -> target to transform.
@return target in string form.
*/
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
    if (buffer) sprintf(buffer, "%lu", target);

    return buffer;
}

/*
Puts a float_cot into string form.
@arg target -> target to transform.
@return target in string form.
*/
char* BC_FloatToStr(float_cot target) {
	cottageCheck(NULL);
    //to get the number of digits, use log base 10, truncate it, then add 1
    size_t digitCount = snprintf(NULL, 0, "%lf", target);
    char* buffer = (char*) calloc(digitCount + 1, sizeof(char));
    if (buffer) snprintf(buffer, digitCount + 1, "%lf", target);

    return buffer;
}

/*
Puts an bool_cot into string form.
@arg target -> target to transform.
@return target in string form.
*/
char* BC_BoolToStr(bool_cot target) {
	cottageCheck(NULL);
    char* buffer = (char*) calloc(5 + 1, sizeof(char)); //false has 5 characters
	if (buffer) sprintf(buffer, "%s", target ? "true" : "false");

    return buffer;
}

/*
Puts data at an index in a siteVar into string form.
@arg variable -> siteVar to get data from.
@arg index -> index of data.
@return variable data in string form.
*/
char* BC_VariableToStringAt(siteVar* variable, size_t index) {
	cottageCheck(NULL);
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
		case STRING: {
			string_cot* value = (string_cot*)siteVarAccessAt(variable, index);
			return *value;

		}
        default: {
			newResultError("BC_VariableToString: variable is of an invalid type");
            return NULL;
        }
    }
}

//Wrapper around VariableToStringAt for 0 index.
char* BC_VariableToString(siteVar* variable) {
	return BC_VariableToStringAt(variable, 0);
}

/*
Checks if contents of string matches form of array.
@arg exp -> string to check.
@return status of check.
*/
bool BC_isArray(char* exp) {
	cottageCheck(false);
    if (!exp || strlen(exp) < 2) return false;
    return (exp[0] =='[' && exp[strlen(exp) - 1] == ']');
}


/*
in the future, add a heirarchy of numbers.
in order of lowest to highest priority:
UINT, INT, FLOAT
look out for the other types. if they appear, we throw an error.
otherwise, we change the type of the array to fit what we have.
*/

/*
Converts an array in string form into a siteVar.
@arg input -> stores created siteVar.
@arg name -> name to give to created siteVar.
@arg exp -> string to read array data from.
@arg variables -> where to look for array data in (used if array data has variable names).
@return error status of conversion.
*/
cotResult BC_ArrayToSiteVar(siteVar** input, char* name, char* exp, siteVar* variables) {
	cottageCheck(newResultError("BC_ArrayToSiteVar: cottage not initialised."));
	//elements are divided by commas
	//they cannot be arrays themselves.
	//they must all be of the same type, with the first element as a reference point
	//existing variables can exist here as well
	//values separated by commas
	
	char arrayVar[512] = {0};

	VARTYPE arrayType = ERROR;
	bool typeFound = false;
	siteVar* storage = NULL;

	size_t i = 1; //starting from not the bracket
	size_t explen = strlen(exp);
	
	while (i < explen) {
		size_t k = 0;
		while (exp[i] != ',' && exp[i] != ']' && i < explen) {
			if (k < 511) { //buffer overflow stop
				arrayVar[k++] = exp[i];
			}
			i++;
		}
		i++; //skip bracket or comma

		if (k == 0) continue; //skips empty elements

		VARTYPE type = BC_StrToType(arrayVar);
		void* data = NULL;
		siteVar* foundVar = NULL; //in case element is a variable

		switch (type) {
			case INT: {
				int_cot arrayVal = BC_StrToInt(arrayVar);
				data = malloc(sizeof(int_cot));
				memcpy(data, &arrayVal, sizeof(int_cot));
				break;
			}
			case UINT: {
				uint_cot arrayVal = BC_StrToUInt(arrayVar);
				data = malloc(sizeof(uint_cot));
				memcpy(data, &arrayVal, sizeof(uint_cot));
				break;
			}
			case FLOAT: {
				float_cot arrayVal = BC_StrToFloat(arrayVar);
				data = malloc(sizeof(float_cot));
				memcpy(data, &arrayVal, sizeof(float_cot));
				break;
			}
			case BOOL: {
				bool_cot arrayVal = BC_StrToBool(arrayVar);
				data = malloc(sizeof(bool_cot));
				memcpy(data, &arrayVal, sizeof(bool_cot));
				break;
			}
			case STRING: {
				string_cot arrayVal = BC_StrToStr(arrayVar);
				data = malloc(sizeof(string_cot));
				memcpy(data, &arrayVal, sizeof(string_cot));
				break;
			}
			default: {
				if (BC_StrToVariable(&foundVar, arrayVar, variables, variables).status == COT_ERROR) {
					siteVarFree(storage);
					return newResultError("BC_ArrayToSiteVar: variable in array could not be accessed");
				}
				if (foundVar->type == COMPOSITE) {
					siteVarFree(storage);
					siteVarFree(foundVar);
					return newResultError("BC_ArrayToSiteVar: variable in array is a COMPOSITE, not allowed");
				}
				type = foundVar->type;
				data = siteVarAccess(foundVar);
				if (!data) {
					siteVarFree(foundVar);
					siteVarFree(storage);
					return newResultError("BC_ArrayToSiteVar: could not access data in variable in array");
				}
				break;
			}
		}

		if (!typeFound) {
			arrayType = type;
			typeFound = true;
			storage = siteVarInit(name, arrayType, 0, NULL);
			storage->isArray = true;
		}
		else if (type != arrayType) {
			if (type == STRING && data) {
				string_cot* strData = (string_cot*)data;
				if (*strData) free(*strData);
			}
			if (data) free(data);
			siteVarFree(foundVar);
			siteVarFree(storage);
			return newResultError("BC_ArrayToSiteVar: values in array are not of a consistent type");
		}

		siteVarInsert(&storage, data);
		if (type == STRING && data) {
			string_cot* strData = (string_cot*)data;
			if (*strData) free(*strData);
		}
		if (data) free(data);
		siteVarFree(foundVar);
		memset(arrayVar, 0, 512);
	}

	if (!storage) return newResultError("BC_ArrayToSiteVar: array was empty");

	*input = storage;
	return newResultOK();
}

#endif
#endif