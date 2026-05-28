#ifndef BOOLCALC_COT
#define BOOLCALC_COT

/*
This will be code for turning a boolean expression in string form, and evaluating it.
It will first convert into RPN (kinda), then evaluate the RPN easily.
When a TRUE or FALSE is created, it will be interpreted as a 1 or 0 respectively. this is because im lazy.

Also, no variables, only bNumers. 

When reimplented in C framework, will be able to take in variables using qmap.


now we will be using siteVars
We need to assume a couple things.
first, strings can be defined in expressions now, surrounded by quotation marks
second, characters can be defined in expressions now, surrounded by apostrophes (and expecting just one character)
third, floats and doubles can be used now.
fourth, booleans can be used now (expressed with true or false)

fifth, variables can now be shown in different ways

var => just the variable
var[i] => item at index i. only works for arrays
var.subvar => variable stored inside composite. can be subvar.subsubvar or subvar[i] or just subvar, you get the idea
*/

#include "dependencies_cot.h"
#include "sitevar_cot.h"
#include "conversion_cot.h"


//removes all spaces, and turns double operators into single ones. also performs check.
//do not delete spaces inside of strings
char* BC_format(char* expression) {

	char* new = (char*) calloc(strlen(expression) + 1, sizeof(char));
	strcpy(new, expression);

	//with quotation mark count, we should also take into account
	//quotation marks within strings,
	//denoted by \"
	//though I don't think it will matter too much
	bool inString = false;
	//we also need the case for apostrophes and characters
	//if this becomes too difficult, just straight up remove characters
		
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
	//take strings and characters into account (characters suck not doing that for now)
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

bool BC_isOperator(char c) {
	return (c == '<' || c == '>' || c == '=' || c == '&' || c == '|' || c == '^'); //special case for ! potentially
}

//since working with strings now, we need a new character to define
//beginning and end of a thing, it being either a string, character, number
//or variable name
//can't be a keyboard ASCII value, so must be less than ' ' (32)
#define BC_TERMINATOR 1

//takes into account negative sign
bool BC_isReadableValue(char c) {
	//return (isalnum(c) || c == '"' || c == '\'' || c == '-');
	return (isalnum(c) || c == '"' || c == '-' || c == '.' || c == '[' || c == ']');
}

char* BC_transform(const char* expression) {
	int size = strlen(expression);
	char* result = (char*) calloc(size * 2, sizeof(char));
	int resultIndex = 0;
	char stack[100];
	int top = -1;

	bool quoteFound = false;
	bool aposFound = false;

	bool numFound = false;


	for (int i = size - 1; i >= 0; i--) {
		char c = expression[i];

		if (BC_isReadableValue(c)) {
			if (!numFound) result[resultIndex++] = BC_TERMINATOR;
			
			if (c == '"') quoteFound = !quoteFound;
			//else if (c == '\'') aposFound = !aposFound;

			//we need specific cases for "'" and '"'
			//if (!quoteFound && aposFound) aposFound = false;

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

	//reverse to get the right prefix.
	
	return result;
}


/*
things to add.
negative number checking
floating point number checking
string comparison (includes single character strings)
*/

bool BC_evaluate(const char* expression, siteVar* variables) {
	//check if the variables are initialised properly
	if (!variables || variables->type != COMPOSITE) return false;
	

	//stack becomes of type siteVar, translations to primitives
	//are done during evaluation.
	const uint64_t stackSize = 256;
	siteVar* stack[256] = {0};
	//int stack[100] = {0};
	int sp = -1;

	int n = strlen(expression);
	for (int i = 0; i < n; i++) {
		char c = expression[i];

		if (c == BC_TERMINATOR) {
            // first, check if its a number. if so, direct translate into int.
            // otherwise, check the variables. if match, try to translate.
            // if this fails, must mean that variable is not an number, so we return false;
			
			//dealing with strings now, so keep quote mark
			//int number = 0;
			siteVar* varVal = NULL;
			//int mult = 1;
            char value[256] = {0};
			
            while ((c = expression[++i]) != BC_TERMINATOR) {
			//while () {
                BC_putAt(value, 0, c);
                printf("c is %c\n", c);
            } 
            printf("expression is '%s'\n", value);
            
			//type checking done here
			//everything must converge on varVal

			if (BC_isUInt(value)) {
				uint_cot number = BC_StrToUInt(value);
				printf("its a uint: %lu\n", number);
				printf("number is %lu\n", number);
				varVal = siteVarInit("", UINT, 1, &number);
			}
			else if (BC_isInt(value)) {
				int_cot number = BC_StrToInt(value);
				printf("its a int: %li\n", number);
				varVal = siteVarInit("", INT, 1, &number);
			}
			else if (BC_isFloat(value)) {
				float_cot number = BC_StrToFloat(value);
				printf("its a double: %lf\n", number);
				varVal = siteVarInit("", FLOAT, 1, &number);
			}
			// else if (BC_isChar(value)) {
			// 	char character = BC_StrToChar(value);
			// 	varVal = siteVarInit("", UINT8, false, 1, &character);
			// }
			else if (BC_isString(value)) {
				string_cot string = BC_StrToStr(value);
				printf("string is %s\n", string);
				varVal = siteVarInit("", STRING, 1, &string);

				printf("string varVal with value %s\n", *((char**)siteVarAccess(varVal)));
				free(string);
			}
			else if (BC_isBool(value)) {
				bool_cot boolean = BC_StrToBool(value);
				varVal = siteVarInit("", BOOL, 1, &boolean);
			}
            else { //must be a variable

				varVal = BC_StrToVariable(value, variables, variables);
				if (!varVal) return 0; //if variable is not defined, terminate

				if (varVal->type == STRING) {
					printf("%s is a string, with value %s\n", varVal->name, *((char**)siteVarAccess(varVal)));
				}
            }

            stack[++sp] = varVal;
		}
		else if (c == '!') {
			//we have to invert the siteVar at the current position
			//inversion should work on everything.
			//just to be safe, work it normally on numbers, bools and chars
			//and I guess ignore for strings? either that or ignore them
			//or negate them directly idk
			//stack[sp] = !stack[sp];

			void* stackVal = siteVarAccess(stack[sp]);

			//tried something, idk if it would work
			// long double stackNum = BC_siteVarToNumber(stack[sp]->type, stackVal);
			// stackNum = !stackNum;
			// siteVarUpdate(stack[sp], stackVal);


			switch (stack[sp]->type) {
				case UINT: {
					uint_cot* val = (uint_cot*)stackVal;
					printf("val was %lu, ", *val);
					*val = !(*val);
					printf("val is now %lu\n", *val);
					// siteVarUpdate(stack[sp], val);
					// free(val);
					break;
				}
				case INT: {
					int_cot* val = (int_cot*)stackVal;
					*val = !(*val);
					// siteVarUpdate(stack[sp], val);
					// free(val);
					break;
				}
				case FLOAT: { //used to be DOUBLE
					float_cot* val = (float_cot*)stackVal;
					*val = !(*val);
					// siteVarUpdate(stack[sp], val);
					// free(val);
					break;
				}
				case BOOL: {
					bool_cot* val = (bool_cot*)stackVal;
					*val = !(*val);
					// siteVarUpdate(stack[sp], val);
					// free(val);
					break;
				}
				default: {
					break;
				}
			}

			siteVarUpdate(stack[sp], stackVal);
			free(stackVal);
			
		}
		else if (BC_isOperator(c)) {
			//must have at least two numbers in here
			if (sp < 1) return false;


			//now comes the tricky part.
			//we have to use siteVar comparisons to see which ones are valid.
			//in truth, right now, comparisons are done primarily
			//between numbers and numbers, and strings and strings
			//composites are another story, that goes earlier


			siteVar* a = stack[sp--];
			siteVar* b = stack[sp--];

			if ((a->type == STRING && b->type != STRING) || (b->type == STRING && a->type != STRING)) {
				//uhhhh idk return 0
				//return false;
				
				/*
				actually, dont return 0.
				instead, just store a boolean false here

				*/
				stack[++sp] = siteVarInit("", BOOL, 1, &((bool){false}));
				siteVarFree(a);
				siteVarFree(b);
				a = NULL;
				b = NULL;
				continue;
			}

			/*
			How should string comparisons work?
			Strings are compared by value, everything is compared by value
			For now just use equals for strings, everything else 
			for now just evaluate to false lowkey

			the numbers are just numbers lmao
			*/

			void* aVal = siteVarAccess(a);
			void* bVal = siteVarAccess(b);

			

			printf("a.name = %s\nb.name = %s\n", a->name, b->name);

			if (a->type == STRING) {

				string_cot aString = *((string_cot*)aVal);
				string_cot bString = *((string_cot*)bVal);
				//siteVarFree(a);
				//siteVarFree(b);
				// a = NULL;
				// b = NULL;

				printf("comparing %s to %s\n", aString, bString);

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
						printf("equalling\n");
						//make a siteVar and put it in
						if (!strcmp(aString, bString)) {
							printf("bombaclatt\n");
							stack[++sp] = siteVarInit("", BOOL, 1, &((bool){true}));
							//siteVarFree(a);
							//siteVarFree(b);
							//a = NULL;
							//b = NULL;
						}
						else {
							printf("what\n");
							stack[++sp] = siteVarInit("", BOOL, 1, &((bool){false}));
							//siteVarFree(a);
							//siteVarFree(b);
							//a = NULL;
							//b = NULL;
						}
						break;
					}
					case '^': {
						break;
					}
				}
			}
			else {
				//this is a pain for a bunch of different variable combinations
				//but we gotta do it
				//im probably definitely going to limit the number of types actually
				//sure less memory efficient but its better for me sanity wise,
				//and will probably lead to straight up safer checks
				/*
				INT->INT
				UINT->UINT
				DOUBLE->DOUBLE
				UINT8->UINT8
				BOOL->BOOL

				INT->UINT
				INT->DOUBLE
				INT->UINT8
				INT->BOOL
				
				UINT->DOUBLE
				UINT->UINT8
				UINT->BOOL

				DOUBLE->UINT8
				DOUBLE->BOOL

				UINT8->BOOL

				*/
				
				generalNumber aNum = BC_siteVarToNumber(a->type, aVal);
				generalNumber bNum = BC_siteVarToNumber(b->type, bVal);

				printf("aNum is %lf, bNum is %lf\n", aNum, bNum);


				bool result = false;
				switch (c) {
					case '<': result = (aNum < bNum); break;
					case '>': result = (aNum > bNum); break;
					case '&': result = (aNum && bNum); break;
					case '|': result = (aNum || bNum); break;
					case '=': result = (aNum == bNum); break; // Note: careful with strict float equality
					case '^': result = (aNum != bNum); break;
					default:  result = false; break;
				}
				stack[++sp] = siteVarInit("", BOOL, 1, &((bool){result}));
				siteVarFree(a);
				siteVarFree(b);
				a = NULL;
				b = NULL;

			}
			free(aVal);
			free(bVal);
		}
	}

	if (sp > 0) { //error
		//free the whole stack
		for (uint64_t i = 0; i < stackSize; i++) {
			siteVarFree(stack[i]);
		}
		return false;
	}
	
	bool* resultptr = (bool*)siteVarAccess(stack[0]);
	bool result = (*resultptr) && true;
	free(resultptr);

	//free the whole stack
	for (uint64_t i = 0; i < stackSize; i++) {
		siteVarFree(stack[i]);
	}

	if (result) return true;
	return false;
}

/*
We are going to put in the siteVar now,
I believe we only have to do this in evaluate thankfully
*/



#endif