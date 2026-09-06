/*
Functionality for reading strings as boolean statements and evaluating.

Code is part of the cottage framework (https://github.com/vixthevix/cottage)
*/

#ifndef BOOLCALC_COT
#define BOOLCALC_COT


#include "dependencies_cot.h"
#include "sitevar_cot.h"
#include "conversion_cot.h"
#include "init_cot.h"
#include "error_cot.h"

char* BC_format(char* expression);
bool BC_isOperator(char c);
bool BC_isDoubleOperator(char c);
bool BC_isReadableValue(char c);
char* BC_transform(const char* expression);
cotResult BC_evaluate(bool* result, const char* expression, siteVar* variables);

#if defined(COTTAGE_START)

/*
Formats string expression to be read correctly.
@arg expression -> input expression.
@return new formatted expression.
*/
char* BC_format(char* expression) {
	cottageCheck(NULL);
	char* new = (char*) calloc(strlen(expression) + 1, sizeof(char));
	strcpy(new, expression);

	//are we in quotation marks?
	bool inString = false;
		
	//first format spaces.
	int n = strlen(new);
	for (int i = 0; i < strlen(new);) {
		//if (new[i] == '\"') i++;
		if (new[i] == '"') {
			inString = !inString;
			i++;
		}
		else if (new[i] == ' ') 
		{
			if (!inString) BC_delAt(new, i);
			else i++;
		}
		else i++;
	}

	if (inString) { //closed off on string, so invalid
		free(new);
		return NULL;
	}

	//now format and check for operators
	inString = false;
	n = strlen(new);
	bool opFound = false;
	char op = 0;
	for (int i = 0; i < strlen(new);) {
		char c = new[i];
		if (c == '"') {
			inString = !inString;
			i++;
		}
		else if (BC_isDoubleOperator(c)) {
			if (!inString) {
				if (opFound) {
					if (c != op) { //no corresponding double
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
			else i++;
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

/*
Checks if a character is a valid operator in a boolean expression.
@arg c -> character to be checked.
@return check for c.
*/
bool BC_isOperator(char c) {
	cottageCheck(false);
	return (c == '<' || c == '>' || c == '=' || c == '&' || c == '|' || c == '^'); //special case for ! potentially
}

/*
Checks if a character is a boolean operator in an expression.
@arg c -> character to check.
@return status of check.
*/
bool BC_isDoubleOperator(char c) {
	cottageCheck(false);
	return (c == '=' || c == '&' || c == '|' || c == '^'); //special case for ! potentially
}

//Stand in character to separate targets in a boolean expression
//Can't be a keyboard ASCII value, so must be less than ' ' (32)
#define BC_TERMINATOR 1

/*
Checks if a character is special for specific instances.
@arg c -> character to be checked.
@return check for c.
*/
bool BC_isReadableValue(char c) {
	cottageCheck(false);
	return (isalnum(c) || c == '"' || c == '-' || c == '.' || c == '[' || c == ']');
}

/*
Reads an infix expression and converts it into a postfix notation.
@arg expression -> input expression.
@return new transformed expression.
*/
char* BC_transform(const char* expression) {
	cottageCheck(NULL);
	if (!expression) return NULL;
	
	const int size = strlen(expression);
	char* result = (char*) calloc(size * 2, sizeof(char));
	int resultIndex = 0;
	char stack[256];
	int top = -1;

	bool quoteFound = false;
	bool aposFound = false;

	bool numFound = false;


	for (int i = size - 1; i >= 0; i--) {
		char c = expression[i];

		if (BC_isReadableValue(c)) {
			if (!numFound) result[resultIndex++] = BC_TERMINATOR;
			
			if (c == '"') quoteFound = !quoteFound;

			numFound = true;
			result[resultIndex++] = c;
		}
		else if (quoteFound) {
			result[resultIndex++] = c;
		}
		else {
			if (numFound) result[resultIndex++] = BC_TERMINATOR;
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

	//reallocate to save space
	result = (char*)realloc(result, strlen(result) + 1);
	
	return result;
}


/*
things to add.
negative number checking
floating point number checking
string comparison (includes single character strings)
*/

/*
Evaluates a postfix string expression.
@arg result -> stores the result of the evaluation.
@arg expression -> expression to be evaluated.
@arg variables -> contains site variables that may exist in expression.
@return error status of evaluation.
*/
cotResult BC_evaluate(bool* result, const char* expression, siteVar* variables) {
	cottageCheck(newResultError("BC_evaluate: cottage not initialised."));
	//check if the variables are initialised properly
	if (!variables || variables->type != COMPOSITE) return newResultError("BC_evaluate: invalid variables");
	

	//stack is of type siteVar to account for different data types.
	const uint64_t stackSize = 256;
	siteVar* stack[256] = {0};
	int sp = -1; //stack pointer

	char errorMsg[256] = {0};

	int n = strlen(expression);
	for (int i = 0; i < n; i++) {
		char c = expression[i];

		if (c == BC_TERMINATOR) { //start of new target value
			siteVar* varVal = NULL;
            char value[256] = {0};
			char varValName[100] = {0};
			snprintf(varValName, 100, "varVal_%i", i);
			
            while ((c = expression[++i]) != BC_TERMINATOR) {
                BC_putAt(value, 0, c);
            } 
            
			//type checking done here
			//everything must converge on varVal

			if (BC_isUInt(value)) {
				uint_cot number = BC_StrToUInt(value);
				varVal = siteVarInit(varValName, UINT, 1, &number);
			}
			else if (BC_isInt(value)) {
				int_cot number = BC_StrToInt(value);
				varVal = siteVarInit(varValName, INT, 1, &number);
			}
			else if (BC_isFloat(value)) {
				float_cot number = BC_StrToFloat(value);
				varVal = siteVarInit(varValName, FLOAT, 1, &number);
			}
			else if (BC_isString(value)) {
				string_cot string = BC_StrToStr(value);
				varVal = siteVarInit(varValName, STRING, 1, &string);
				free(string);
			}
			else if (BC_isBool(value)) {
				bool_cot boolean = BC_StrToBool(value);
				varVal = siteVarInit(varValName, BOOL, 1, &boolean);
			}
            else { //must be a variable
				if (BC_StrToVariable(&varVal, value, variables, variables).status == COT_ERROR) {
					strcpy(errorMsg, "BC_evaluate: invalid element in expression");
					goto failure;
				}
            }

            stack[++sp] = varVal;
		}
		else if (c == '!') { //boolean inversion

			void* stackVal = siteVarAccess(stack[sp]);

			switch (stack[sp]->type) {
				case UINT: {
					uint_cot* val = (uint_cot*)stackVal;
					*val = !(*val);
					break;
				}
				case INT: {
					int_cot* val = (int_cot*)stackVal;
					*val = !(*val);
					break;
				}
				case FLOAT: { //used to be DOUBLE
					float_cot* val = (float_cot*)stackVal;
					*val = !(*val);
					break;
				}
				case BOOL: {
					bool_cot* val = (bool_cot*)stackVal;
					*val = !(*val);
					break;
				}
				//no case for STRING
				default: {
					break;
				}
			}

			siteVarUpdate(stack[sp], stackVal);
			free(stackVal);
			
		}
		else if (BC_isOperator(c)) {
			//must have at least two numbers in here
			if (sp < 1) {
				strcpy(errorMsg, "BC_evaluate: stack pointer underflow");
				goto failure;
			}

			//Access the top of the stack.
			siteVar* a = stack[sp];
			stack[sp] = NULL;
			sp--;

			siteVar* b = stack[sp];
			stack[sp] = NULL;
			sp--;

			//Make strings incompatible to compare with the other data types.
			//This is to keep things simple.
			if ((a->type == STRING && b->type != STRING) || (b->type == STRING && a->type != STRING)) {
				stack[++sp] = siteVarInit("badcomp", BOOL, 1, &((bool){false}));
				siteVarFree(a);
				siteVarFree(b);
				a = NULL;
				b = NULL;
				continue;
			}

			void* aVal = siteVarAccess(a);
			void* bVal = siteVarAccess(b);

			if (a->type == STRING) {
				string_cot aString = *((string_cot*)aVal);
				string_cot bString = *((string_cot*)bVal);

				switch (c) {
					case '<': {
						break;
					}
					case '>': {
						break;
					}
					case '&': {
						break;
					}
					case '|': {
						break;
					}
					case '=': {
						if (!strcmp(aString, bString)) {
							stack[++sp] = siteVarInit("strcmp", BOOL, 1, &((bool){true}));
						}
						else {
							stack[++sp] = siteVarInit("strcmp", BOOL, 1, &((bool){false}));
						}
						break;
					}
					case '^': {
						break;
					}
					default: {
						strcpy(errorMsg, "BC_evaluate: invalid operator found");
						if (aVal) free(aVal); 
						if (bVal) free(bVal);
						siteVarFree(a); siteVarFree(b);
						goto failure;
					}
				}
			}
			else {
				//Transform our values into general numbers,
				//to allow them to be compared.
				generalNumber aNum = 0, bNum = 0;
				if (BC_siteVarToNumber(&aNum, a->type, aVal).status == COT_ERROR || BC_siteVarToNumber(&bNum, b->type, bVal).status == COT_ERROR) {
					strcpy(errorMsg, "BC_evaluate: a number was invalid");
					if (aVal) free(aVal); 
					if (bVal) free(bVal);
					siteVarFree(a); siteVarFree(b);
					goto failure;
				}

				bool curResult = false;
				switch (c) {
					case '<': curResult = (aNum < bNum); break;
					case '>': curResult = (aNum > bNum); break;
					case '&': curResult = (aNum && bNum); break;
					case '|': curResult = (aNum || bNum); break;
					case '=': curResult = (aNum == bNum); break; // Note: careful with strict float equality
					case '^': curResult = (aNum != bNum); break;
					default:  {
						strcpy(errorMsg, "BC_evaluate: invalid operator found");
						if (aVal) free(aVal); 
						if (bVal) free(bVal);
						siteVarFree(a); siteVarFree(b);
						goto failure;
					}
				}
				stack[++sp] = siteVarInit("numcmp", BOOL, 1, &((bool){curResult}));
			}
			if (aVal) free(aVal); 
			if (bVal) free(bVal);
			siteVarFree(a); siteVarFree(b);
			a = NULL;
			b = NULL;
		}
	}

	if (sp > 0) {
		strcpy(errorMsg, "BC_evaluate: stack pointer not 0 at end of evaluation");
		goto failure;
	}
	
	bool* resultptr = (bool*)siteVarAccess(stack[0]);
	*result = (*resultptr) && true;
	free(resultptr);

	failure:
	//free the whole stack
	for (uint64_t i = 0; i < stackSize; i++) {
		siteVarFree(stack[i]);
	}

	if (errorMsg[0] != 0) return newResultError(errorMsg);
	
	return newResultOK();
}



#endif
#endif