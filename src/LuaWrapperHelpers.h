#ifndef GUARD_LUAWRAPPERHELPER
#define GUARD_LUAWRAPPERHELPER

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include "../stolenCode/getline.h"

#include <Lua/lua.h>
#include <Lua/lualib.h>
#include <Lua/lauxlib.h>

//===================================================================================

#define PUSHLUAWRAPPER(scriptFunctionName,luaFunctionName) lua_pushcfunction(L,L_##scriptFunctionName);\
	lua_setglobal(L,luaFunctionName);

#define POINTER_TOCHAR(x) (*((char*)(x)))
#define POINTER_TOBOOL(x) POINTER_TOCHAR(x)
#define POINTER_TOSTRING(x) ((char*)(x))
#define POINTER_TOINT(x) (*((int*)(x)))
#define POINTER_TOFLOAT(x) (*((float*)(x)))
#define POINTER_TOPOINTER(x) (*((void**)(x)))

#define LUAREGISTER(x,y) lua_pushcfunction(L,x);\
	lua_setglobal(L,y);

#define GENERATELUAWRAPPER(scriptFunctionName) \
int L_##scriptFunctionName(lua_State* passedState){ \
	nathanscriptVariable* _madeArgs = makeScriptArgumentsFromLua(passedState); \
	nathanscriptVariable* _gottenReturnArray=NULL; \
	int _gottenReturnArrayLength; \
	scriptFunctionName(_madeArgs,lua_gettop(passedState),&_gottenReturnArray,&_gottenReturnArrayLength); \
	freeNathanscriptVariableArray(_madeArgs,lua_gettop(passedState)); \
	if (_gottenReturnArray!=NULL){ \
		pushReturnArrayToLua(passedState, _gottenReturnArray, _gottenReturnArrayLength); \
		return freeNathanscriptVariableArray(_gottenReturnArray,_gottenReturnArrayLength); \
	} \
	return 0; \
}

#define NATHAN_TYPE_NULL LUA_TNIL
#define NATHAN_TYPE_BOOL LUA_TBOOLEAN
#define NATHAN_TYPE_STRING LUA_TSTRING
#define NATHAN_TYPE_FLOAT LUA_TNUMBER
#define NATHAN_TYPE_POINTER LUA_TLIGHTUSERDATA
#define NATHAN_TYPE_ARRAYLENGTH 80 // Just hope this isn't used by the Lua constants

//===================================================================================
int nathanscriptTotalGamevar;
typedef struct fgurhrh{
	char variableType;
	void* value; // string or float made with malloc
}nathanscriptVariable;
// These are controlled with setvar
typedef struct fjiwejfe{
	nathanscriptVariable variable;
	char* name; // string made with malloc
}nathanscriptGameVariable;

nathanscriptGameVariable* nathanscriptGamevarList;

typedef void(*nathanscriptFunction)(nathanscriptVariable* _madeArgs, int _totalArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize);

int nathanCurrentMaxFunctions=0;
int nathanCurrentRegisteredFunctions=0;

// Realloc as needed
nathanscriptFunction* nathanFunctionList=NULL;
char** nathanFunctionNameList=NULL;
char* nathanFunctionPropertyList=NULL;

FILE* nathanscriptCurrentOpenFile;
int nathanscriptFoundFiIndex; // Array index of the fi command from the line parser
int nathanscriptFoundLabelIndex;

/*============================================================================*/
char nathanvariableArrayChangeIndexIfReturnArray(nathanscriptVariable*  _variableArray, char _index){
	if (_variableArray[0].variableType==NATHAN_TYPE_ARRAYLENGTH){
		return _index+1;
	}
	return _index;
}
void nathanvariableArraySetFloat(nathanscriptVariable*  _variableArray, unsigned char _index, float _value){
	_index = nathanvariableArrayChangeIndexIfReturnArray(_variableArray,_index);
	_variableArray[_index].variableType = NATHAN_TYPE_FLOAT;
	_variableArray[_index].value = malloc(sizeof(float));
	POINTER_TOFLOAT(_variableArray[_index].value)=_value;
}
void nathanvariableArraySetBool(nathanscriptVariable*  _variableArray, unsigned char _index, char _value){
	_index = nathanvariableArrayChangeIndexIfReturnArray(_variableArray,_index);
	_variableArray[_index].variableType = NATHAN_TYPE_BOOL;
	_variableArray[_index].value = malloc(sizeof(char));
	POINTER_TOCHAR(_variableArray[_index].value)=_value;
}
void nathanvariableArraySetString(nathanscriptVariable*  _variableArray, unsigned char _index, const char* _value){
	_index = nathanvariableArrayChangeIndexIfReturnArray(_variableArray,_index);
	_variableArray[_index].variableType = NATHAN_TYPE_STRING;
	_variableArray[_index].value = malloc(strlen(_value)+1);
	strcpy(_variableArray[_index].value,_value);
}
void nathanvariableArraySetPointer(nathanscriptVariable*  _variableArray, unsigned char _index, void* _value){
	_index = nathanvariableArrayChangeIndexIfReturnArray(_variableArray,_index);
	_variableArray[_index].variableType = NATHAN_TYPE_POINTER;
	_variableArray[_index].value = malloc(sizeof(void*));
	POINTER_TOPOINTER(_variableArray[_index].value)=_value;
}
nathanscriptVariable* makeScriptArgumentsFromLua(lua_State* passedState){
	int i;
	//lua_type
	int _numberOfArguments = lua_gettop(passedState);
	nathanscriptVariable* _returnedArguments = malloc(sizeof(nathanscriptVariable)*lua_gettop(passedState));
	for (i=0;i<_numberOfArguments;++i){
		_returnedArguments[i].variableType = lua_type(passedState,i+1);
		switch (lua_type(passedState,i+1)){
			case LUA_TNUMBER:
				nathanvariableArraySetFloat(_returnedArguments,i,(float)lua_tonumber(passedState,i+1));
				break;
			case LUA_TBOOLEAN:
				nathanvariableArraySetBool(_returnedArguments,i,lua_toboolean(passedState,i+1));
				break;
			case LUA_TSTRING:
				nathanvariableArraySetString(_returnedArguments,i,lua_tostring(passedState,i+1));
				break;
			case LUA_TLIGHTUSERDATA:
				nathanvariableArraySetPointer(_returnedArguments,i,lua_touserdata(passedState,i+1));
				break;
			case LUA_TNIL:
				_returnedArguments[i].variableType = NATHAN_TYPE_NULL;
				_returnedArguments[i].value = calloc(1,1);
				break;
			/*
			case LUA_TTABLE:
				break;
			case LUA_TFUNCTION:
				break;
			case LUA_TUSERDATA:
				break;
			case LUA_TTHREAD:
				break;
			case LUA_TNONE:
				break;*/
			default:
				printf("Unsupported argument type.\n");
				_returnedArguments[i].value = calloc(1,1);
				break;
		}
	}
	return _returnedArguments;
}
int freeNathanscriptVariableArray(nathanscriptVariable* _passedVariableArray, int _passedArraySize){
	if (_passedVariableArray==NULL){
		return -1;
	}
	int i;
	for (i=0;i<_passedArraySize;i++){
		free(_passedVariableArray[i].value);
	}
	free(_passedVariableArray);
	return _passedArraySize;
}

void pushSingleReturnArgument(lua_State* passedState, char _returnValueType, void* _returnValue){
	switch(_returnValueType){
		case NATHAN_TYPE_STRING:
			lua_pushstring(passedState,POINTER_TOSTRING(_returnValue));
			break;
		case NATHAN_TYPE_POINTER:
			lua_pushlightuserdata(passedState,POINTER_TOPOINTER(_returnValue));
			break;
		case NATHAN_TYPE_FLOAT:
			lua_pushnumber(passedState,POINTER_TOFLOAT(_returnValue));
			break;
		case NATHAN_TYPE_BOOL:
			lua_pushboolean(passedState,POINTER_TOCHAR(_returnValue));
			break;
		case NATHAN_TYPE_NULL:
			lua_pushnil(passedState);
			break;
		default:
			printf("Bad value type %d\n",_returnValueType);
			break;
	}
}
void pushReturnArrayToLua(lua_State* passedState, nathanscriptVariable* _passedReturnArray, int _passedArraySize){
	int i;
	for (i=0;i<_passedArraySize;i++){
		pushSingleReturnArgument(passedState,_passedReturnArray[i].variableType,_passedReturnArray[i].value);
	}
}


// Returns number of arguments
int freeReturnArray(void** _passedReturnArray){
	int _totalReturnArguments = *((int*)(_passedReturnArray[0]));
	int i;
	for (i=0;i<_totalReturnArguments;i++){
		free(_passedReturnArray[i*2+1]);
		free(_passedReturnArray[i*2+2]);
	}
	free(_passedReturnArray[0]);
	free(_passedReturnArray);
	return _totalReturnArguments;
}

#ifndef PLATFORM
	void removeNewline(char* _toRemove){
		int _cachedStrlen = strlen(_toRemove);
		int i;
		for (i=0;i!=2;i++){
			if (!(((_toRemove)[_cachedStrlen-(i+1)]==0x0A) || ((_toRemove)[_cachedStrlen-(i+1)]==0x0D))){
				break;
			}
		}
		(_toRemove)[_cachedStrlen-i] = '\0';
	}
#endif

// _bufferStartIndex becomes -1 if we reached end of buffer.
char* readSpaceTerminated(char* _bufferToReadFrom, int* _bufferStartIndex, char* _tempBuffer){
	// Value we will malloc later that will contain the read string with the correct malloc size
	char* _returnReadString;
	int i=0;
	while (1){
		if (_bufferToReadFrom[*_bufferStartIndex]==32 || _bufferToReadFrom[*_bufferStartIndex]==0){ // 32 is space character
			if (_bufferToReadFrom[*_bufferStartIndex]==0){
				*_bufferStartIndex=-1;
			}else{
				*_bufferStartIndex+=1; // Advance past the space character for next time.
			}
			break;
		}
		_tempBuffer[i]=_bufferToReadFrom[*_bufferStartIndex];
		*_bufferStartIndex+=1;
		i+=1;
	}
	_tempBuffer[i]=0; // Null character to finish off the string
	_returnReadString = malloc(strlen(_tempBuffer)+1);
	strcpy(_returnReadString,_tempBuffer);
	return _returnReadString;
}
char stringIsNumber(char* _stringToCheck){
	int i;
	char _usedDecimalPoint=0;
	int _cachedStrlen = strlen(_stringToCheck);
	for (i=_stringToCheck[0]=='-' ? 1 : 0;i<_cachedStrlen;i++){
		if (!(_stringToCheck[i]>=48 && _stringToCheck[i]<=57)){
			if (_stringToCheck[i]==0x2E && _usedDecimalPoint==0){
				_usedDecimalPoint=1;
			}else{
				return 0;
			}
		}
	}
	return 1;
}

// Searches the list for the command you pass it
// Returns -1 if not found
int searchStringArray(char** _passedArray, int _passedArraySize, char* _passedSearchTerm){
	int i;
	for (i=0;i<_passedArraySize;i++){
		if (strcmp(_passedSearchTerm,_passedArray[i])==0){
			return i;
		}
	}
	return -1;
}
/*
// Searches the list for the command you pass it
// Returns -1 if not found
int searchStringArray(char** _passedArray, int _passedArraySize,char* _passedCommandName){
	int i;
	for (i=0;i<nathanCurrentRegisteredFunctions;i++){
		if (strcmp(_passedCommandName,nathanFunctionNameList[i])==0){
			return i;
		}
	}
	return -1;
}
*/

int nathanscriptAddNewGameVariable(){
	nathanscriptTotalGamevar++;
	nathanscriptGamevarList = realloc(nathanscriptGamevarList,nathanscriptTotalGamevar*sizeof(nathanscriptGameVariable));
	nathanscriptGamevarList[nathanscriptTotalGamevar-1].name=NULL;
	nathanscriptGamevarList[nathanscriptTotalGamevar-1].variable.variableType=NATHAN_TYPE_NULL;
	nathanscriptGamevarList[nathanscriptTotalGamevar-1].variable.value=NULL;
	return nathanscriptTotalGamevar-1; // Index of new variable
}

void nathanscriptConvertVariable(nathanscriptVariable* _variableToConvert, char _newType){
	if (_variableToConvert->variableType==_newType || _variableToConvert->variableType==NATHAN_TYPE_POINTER){
		return;
	}
	if (_variableToConvert->value==NATHAN_TYPE_NULL){
		if (_newType==NATHAN_TYPE_FLOAT){
			_variableToConvert->value = calloc(1,sizeof(float));
		}else if (_newType == NATHAN_TYPE_STRING){
			_variableToConvert->value = calloc(1,1);
		}
		_variableToConvert->variableType = _newType;
		return; // Do not remove. 
	}else if (_newType==NATHAN_TYPE_BOOL){
		char _boolValue;
		if (_variableToConvert->variableType==NATHAN_TYPE_FLOAT){
			_boolValue = (int)POINTER_TOFLOAT(_variableToConvert->value);
		}else if (_variableToConvert->variableType==NATHAN_TYPE_STRING){
			if (POINTER_TOSTRING(_variableToConvert->value)[0]=='1'){
				_boolValue=1;
			}else{
				_boolValue=0;
			}
		}
		free(_variableToConvert->value);
		_variableToConvert->value = malloc(sizeof(char));
		POINTER_TOCHAR(_variableToConvert->value)=_boolValue;
	}else if (_newType==NATHAN_TYPE_STRING){
		char _resultingStringBuffer[256];
		if (_variableToConvert->variableType==NATHAN_TYPE_FLOAT){
			sprintf(_resultingStringBuffer, "%.0f", floor(POINTER_TOFLOAT(_variableToConvert->value))); // is okay because original vnds only supported int
		}else if (_variableToConvert->variableType==NATHAN_TYPE_BOOL){
			if (POINTER_TOCHAR(_variableToConvert->value)==1){
				_resultingStringBuffer[0]='1';
			}else{
				_resultingStringBuffer[0]='0';
			}
			_resultingStringBuffer[1]=0;
		}
		free(_variableToConvert->value);
		_variableToConvert->value = malloc(strlen(_resultingStringBuffer)+1);
		strcpy(_variableToConvert->value,_resultingStringBuffer);
	}else if (_newType==NATHAN_TYPE_FLOAT){
		float* _newConverterValue = malloc(sizeof(float));
		if (_variableToConvert->variableType==NATHAN_TYPE_STRING){
			*(_newConverterValue) = atof(_variableToConvert->value);
		}else if (_variableToConvert->variableType==NATHAN_TYPE_BOOL){
			*(_newConverterValue) = (float)(POINTER_TOCHAR(_variableToConvert->value)==1 ? 1 : 0);
		}
		free(_variableToConvert->value);
		_variableToConvert->value=_newConverterValue;
	}

	_variableToConvert->variableType = _newType;
}

int searchVariableArray(nathanscriptGameVariable* _listToSearch, int _passedArraySize, char* _passedSearchTerm){
	int i;
	for (i=0;i<_passedArraySize;i++){
		if (strcmp(_passedSearchTerm,_listToSearch[i].name)==0){
			return i;
		}
	}
	return -1;
}

nathanscriptGameVariable* nathanscriptGetGameVariable(char* _passedSearchTerm){
	int _foundIndex = searchVariableArray(nathanscriptGamevarList,nathanscriptTotalGamevar,_passedSearchTerm);
	if (_foundIndex==-1){
		return NULL;
	}
	return &(nathanscriptGamevarList[_foundIndex]);
}

/*
// Value we will malloc later that will contain the read string with the correct malloc size
char* _returnReadString;
int i=0;
while (1){
	if (_bufferToReadFrom[*_bufferStartIndex]==32 || _bufferToReadFrom[*_bufferStartIndex]==0){ // 32 is space character
		if (_bufferToReadFrom[*_bufferStartIndex]==0){
			*_bufferStartIndex=-1;
		}else{
			*_bufferStartIndex+=1; // Advance past the space character for next time.
		}
		break;
	}
	_tempBuffer[i]=_bufferToReadFrom[*_bufferStartIndex];
	*_bufferStartIndex+=1;
	i+=1;
}
_tempBuffer[i]=0; // Null character to finish off the string
_returnReadString = malloc(strlen(_tempBuffer)+1);
strcpy(_returnReadString,_tempBuffer);
return _returnReadString;
*/
void replaceIfIsVariable(char** _possibleVariableString){
	int i;
	int _cachedStrlen = strlen(*_possibleVariableString);
	for (i=0;i<_cachedStrlen;i++){
		if ((*_possibleVariableString)[i]=='$' && (i==0 || (*_possibleVariableString)[i-1]==' ')){
			int j;
			for (j=1;j<=_cachedStrlen-i;j++){ // Less than or equal to because we need to check for the null character
				if ((*_possibleVariableString)[i+j]==' ' || (*_possibleVariableString)[i+j]==0){ // Find end of the string
					char _endCharacterCache = (*_possibleVariableString)[i+j];
					(*_possibleVariableString)[i+j]=0; // trim string for searching
					int _foundVariableIndex = searchVariableArray(nathanscriptGamevarList,nathanscriptTotalGamevar,&((*_possibleVariableString)[i+1]));
					if (_foundVariableIndex!=-1){
						char* _newStringBuffer;

						char* _varaibleStringToReplace =  NULL;
						if (nathanscriptGamevarList[_foundVariableIndex].variable.variableType==NATHAN_TYPE_STRING){
							_varaibleStringToReplace = nathanscriptGamevarList[_foundVariableIndex].variable.value;
						}else if (nathanscriptGamevarList[_foundVariableIndex].variable.variableType==NATHAN_TYPE_FLOAT){
							_varaibleStringToReplace = malloc(256);
							sprintf(_varaibleStringToReplace, "%.0f", *((float*)nathanscriptGamevarList[_foundVariableIndex].variable.value));
						}else{
							printf("Invalid variable type when doing replace stupidity!\n");
						}

						int _singlePhraseStrlen = strlen(&((*_possibleVariableString)[i]));
						_newStringBuffer = malloc(_cachedStrlen-_singlePhraseStrlen+strlen(_varaibleStringToReplace)+1);
						if (i==0){
							if ((_singlePhraseStrlen)==_cachedStrlen){
								strcpy(_newStringBuffer,_varaibleStringToReplace);
							}else{
								sprintf(_newStringBuffer, "%s %s",_varaibleStringToReplace,&((*_possibleVariableString)[i+j+1]));
							}
						}else{
							(*_possibleVariableString)[i-1]=0;
							if (_endCharacterCache!=0){
								sprintf(_newStringBuffer,"%s %s %s",(*_possibleVariableString),_varaibleStringToReplace,&((*_possibleVariableString)[i+j+1]));
							}else{
								sprintf(_newStringBuffer, "%s %s",(*_possibleVariableString),_varaibleStringToReplace);
							}
						}
						free((*_possibleVariableString));
						(*_possibleVariableString)=_newStringBuffer;
						_cachedStrlen = strlen(*_possibleVariableString);

						if (nathanscriptGamevarList[_foundVariableIndex].variable.variableType==NATHAN_TYPE_FLOAT){
							free(_varaibleStringToReplace);
						}
					}else{
						printf("Variable not found, %s\n",&((*_possibleVariableString)[i+1]));
						(*_possibleVariableString)[i+j]=_endCharacterCache;
					}
					break;
				}
			}
		}
	}
}

#define MAXNATHANARGUMENTS 20
void nathanscriptParseSingleLine(FILE* fp, int* _storeCommandIndex, nathanscriptVariable** _storeArguments, int* _storeNumArguments){
	// Contains the entire line. Will be resized by getline function
	char* _tempReadLine = calloc(1,512);
	// Will be changed if the buffer isn't big enough
	size_t _readLineBufferSize = 512;
	getline(&_tempReadLine,&_readLineBufferSize,fp);
	char* _tempSingleElementBuffer;
	// getline function includes the new line character in the string
	removeNewline(_tempReadLine);
	if (strlen(_tempReadLine)==0){
		*_storeCommandIndex=-2;
		//*_storeArguments=NULL; // Don't uncomment because we don't know if these are valid pointers
		//*_storeNumArguments=0;
		free(_tempReadLine);
		return;
	}
	_tempSingleElementBuffer = malloc(strlen(_tempReadLine)+1);

	int i;
	// The space in the string we're at after reading the last argument
	int _lineBufferIndex=0;
	// Total number of parsed arguments
	int _totalArguments=0;
	// Does not include the main command
	nathanscriptVariable _parsedArguments[MAXNATHANARGUMENTS]; // This is so much easier than allowing unlimited arguments. Fix this on a rainy day. Or don't. It won't matter.
	char* _parsedMainCommand = readSpaceTerminated(_tempReadLine,&_lineBufferIndex,_tempSingleElementBuffer);
	*_storeCommandIndex = searchStringArray(nathanFunctionNameList,nathanCurrentRegisteredFunctions,_parsedMainCommand);

	// Search for arguments only if we haven't already reached the end of the line buffer and it's a valid command
	if (_lineBufferIndex!=-1 && *_storeCommandIndex!=-1 && _storeArguments!=NULL){
		// the text command doesn't have quotation marks around the text, so we treat everything as one argument.
		if (nathanFunctionPropertyList[*_storeCommandIndex]==1){
			_totalArguments=1;
			_parsedArguments[0].value = malloc(strlen(_tempReadLine)-strlen(_parsedMainCommand)); // No need to remove the space character because we need that extra byte for the null character
			strcpy(_parsedArguments[0].value,&(_tempReadLine[_lineBufferIndex]));
			replaceIfIsVariable((char**)&(_parsedArguments[0].value));
		}else{
			// Read up to MAXNATHANARGUMENTS arguments
			for (i=0;i<MAXNATHANARGUMENTS;i++){
				_parsedArguments[i].value = readSpaceTerminated(_tempReadLine,&_lineBufferIndex,_tempSingleElementBuffer);
				replaceIfIsVariable((char**)&(_parsedArguments[i].value));
				_totalArguments++;
				if (_lineBufferIndex==-1){
					break;
				}
			}
		}
	}


	if (_totalArguments!=0){
		*_storeArguments = malloc(sizeof(nathanscriptVariable)*_totalArguments);
		for (i=0;i<_totalArguments;i++){
			(*_storeArguments)[i] = _parsedArguments[i];
			(*_storeArguments)[i].variableType=NATHAN_TYPE_STRING;
		}
	}else{
		*_storeArguments=NULL;
	}
	*_storeNumArguments = _totalArguments;

	free(_parsedMainCommand);
	free(_tempReadLine);
	free(_tempSingleElementBuffer);
}

// TODO - Reaplce with the variable converter function
char nathanscriptConvertArgumentToFloat(void** _argumentArray, int _argumentIndex){
	if (stringIsNumber(_argumentArray[_argumentIndex])){
		float* _newConverterValue = malloc(sizeof(float));
		*(_newConverterValue) = atof(_argumentArray[_argumentIndex]);
		free(_argumentArray[_argumentIndex]);
		_argumentArray[_argumentIndex]=_newConverterValue;
		return 1;
	}
	return 0;
}

void nathanReallocFunctionLists(int _newSize){
	nathanFunctionPropertyList = realloc(nathanFunctionPropertyList,_newSize*sizeof(char));
	nathanFunctionList = realloc(nathanFunctionList,_newSize*sizeof(nathanscriptFunction));
	nathanFunctionNameList = realloc(nathanFunctionNameList,_newSize*sizeof(char*));
}

void nathanscriptAddFunction(nathanscriptFunction _passedFunction, char _passedFunctionProperties, char* _passedScriptFunctionName){
	if (nathanCurrentMaxFunctions==nathanCurrentRegisteredFunctions){
		nathanCurrentMaxFunctions++;
		nathanReallocFunctionLists(nathanCurrentMaxFunctions);
	}
	nathanFunctionList[nathanCurrentRegisteredFunctions] = _passedFunction;
	nathanFunctionNameList[nathanCurrentRegisteredFunctions] = malloc(strlen(_passedScriptFunctionName)+1);
	nathanFunctionPropertyList[nathanCurrentRegisteredFunctions]=_passedFunctionProperties;
	strcpy(nathanFunctionNameList[nathanCurrentRegisteredFunctions],_passedScriptFunctionName);
	nathanCurrentRegisteredFunctions++;
}

/*
	char* makeNathanArgumentArray(int _numberOfArguments){
	int i;
	char* _tempReturnArray = malloc(sizeof(char)*_numberOfArguments);
	for (i=0;i<_numberOfArguments;i++){
		_tempReturnArray[i] = NATHAN_TYPE_STRING;
	}
	return _tempReturnArray;
}*/

void nathanscriptInit(int _startMaxFunctions){
	nathanReallocFunctionLists(_startMaxFunctions);
	nathanCurrentMaxFunctions=_startMaxFunctions;
}

char* nathanvariableToString(nathanscriptVariable* _passedVariable){
	nathanscriptConvertVariable(_passedVariable,NATHAN_TYPE_STRING);
	return POINTER_TOSTRING(_passedVariable->value);
}

char nathanvariableToBool(nathanscriptVariable* _passedVariable){
	nathanscriptConvertVariable(_passedVariable,NATHAN_TYPE_BOOL);
	return POINTER_TOBOOL(_passedVariable->value);
}

float nathanvariableToFloat(nathanscriptVariable* _passedVariable){
	nathanscriptConvertVariable(_passedVariable,NATHAN_TYPE_FLOAT);
	return POINTER_TOFLOAT(_passedVariable->value);
}

int nathanvariableToInt(nathanscriptVariable* _passedVariable){
	return (int)nathanvariableToFloat(_passedVariable);
}

void makeNewReturnArray(nathanscriptVariable** _returnedReturnArray, int* _returnArraySize, int _newArraySize){
	*_returnedReturnArray = calloc(1,sizeof(nathanscriptVariable)*_newArraySize);
	*_returnArraySize = _newArraySize;
}

/*
================================================================
SCRIPT
================================================================
*/
// setvar varname modifier value
// modifier: =, +. -
// setvar v_d0 = 1
void scriptSetVar(nathanscriptVariable* _argumentList, int _totalArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	char* _passedVariableName = nathanvariableToString(&_argumentList[0]);
	char* _passedModifier = nathanvariableToString(&_argumentList[1]);
	char* _passedNewValue = nathanvariableToString(&_argumentList[2]);
	if (strlen(_passedModifier)!=1){
		printf("modifier string too long.");
		return;
	}
	int _foundVariableIndex = searchVariableArray(nathanscriptGamevarList,nathanscriptTotalGamevar,_passedVariableName);
	// Make a new variable
	if (_foundVariableIndex==-1){
		_foundVariableIndex = nathanscriptAddNewGameVariable();
		nathanscriptGamevarList[_foundVariableIndex].name = malloc(strlen(_passedVariableName)+1);
		strcpy(nathanscriptGamevarList[_foundVariableIndex].name,_passedVariableName);
	}
	// There's no sure way to tell if the user passed a string or number
	char _guessedVariableType = NATHAN_TYPE_FLOAT;
	if (!stringIsNumber(_passedNewValue)){
		_guessedVariableType = NATHAN_TYPE_STRING;
	}else if (nathanscriptGamevarList[_foundVariableIndex].variable.variableType==NATHAN_TYPE_STRING){
		_guessedVariableType = NATHAN_TYPE_STRING;
	}
	nathanscriptConvertVariable(&(nathanscriptGamevarList[_foundVariableIndex].variable),_guessedVariableType);
	if (_guessedVariableType==NATHAN_TYPE_FLOAT){
		float _convertedNewValue = nathanvariableToFloat(&_argumentList[2]);
		switch (_passedModifier[0]){
			case '+':
				*((float*)nathanscriptGamevarList[_foundVariableIndex].variable.value)+=_convertedNewValue;
				break;
			case '-':
				*((float*)nathanscriptGamevarList[_foundVariableIndex].variable.value)-=_convertedNewValue;
				break;
			case '=':
				*((float*)nathanscriptGamevarList[_foundVariableIndex].variable.value)=_convertedNewValue;
				break;
			default:
				printf("Bad operator %c on int value.\n",_passedModifier[0]);
				break;
		}
	}else if (_guessedVariableType==NATHAN_TYPE_STRING){
		switch (_passedModifier[0]){
			case '+':
				;
				char* _newStringBuffer = malloc(strlen(nathanscriptGamevarList[_foundVariableIndex].variable.value)+strlen(_passedNewValue)+1);
				strcpy(_newStringBuffer,nathanscriptGamevarList[_foundVariableIndex].variable.value);
				strcat(_newStringBuffer,_passedNewValue);
				free(nathanscriptGamevarList[_foundVariableIndex].variable.value);
				nathanscriptGamevarList[_foundVariableIndex].variable.value = _newStringBuffer;
				break;
			case '=':
				free(nathanscriptGamevarList[_foundVariableIndex].variable.value);
				nathanscriptGamevarList[_foundVariableIndex].variable.value = malloc(strlen(_passedNewValue)+1);
				strcpy(nathanscriptGamevarList[_foundVariableIndex].variable.value,_passedNewValue);
				break;
			default:
				printf("Bad operator %c on string value.\n",_passedModifier[0]);
				break;
		}
	}
	return;
}

void scriptIfStatement(nathanscriptVariable* _argumentList, int _totalArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	nathanscriptGameVariable* _firstVariable = nathanscriptGetGameVariable(nathanvariableToString(&_argumentList[0]));
	if (_firstVariable==NULL){
		printf("Invalid variable.\n");
		return;
	}
	char* _passedOperator = nathanvariableToString(&_argumentList[1]);
	char _ifStatementResult=0;
	if (_firstVariable->variable.variableType==NATHAN_TYPE_FLOAT){
		float v1 = *((float*)_firstVariable->variable.value);
		float v2 = nathanvariableToFloat(&_argumentList[2]);
		if (_passedOperator[0]=='=' && _passedOperator[1]=='=') {
			_ifStatementResult = (v1==v2);
		}else if (_passedOperator[0]=='!' && _passedOperator[1]=='='){
			_ifStatementResult = (v1!=v2);
		}else if (_passedOperator[0]=='>' && _passedOperator[1]=='='){
			_ifStatementResult = (v1>=v2);
		}else if (_passedOperator[0]=='<' && _passedOperator[1]=='='){
			_ifStatementResult = (v1<=v2);
		}else if (_passedOperator[0]=='>'){
			_ifStatementResult = (v1 > v2);
		}else if (_passedOperator[0]=='<'){
			_ifStatementResult = (v1 < v2);
		}else{
			printf("bad operator. %s\n",_passedOperator);
		}
	}else{
		_ifStatementResult = !(strcmp((char*)_firstVariable->variable.value,nathanvariableToString(&_argumentList[2])));
		if (_passedOperator[0]=='!'){
			_ifStatementResult = !_ifStatementResult;
		}
	}
	if (_ifStatementResult==0){
		while (!feof(nathanscriptCurrentOpenFile)){
			int _foundCommandIndex;
			nathanscriptVariable* _parsedArguments;
			int _parsedArgumentsLength;
			nathanscriptParseSingleLine(nathanscriptCurrentOpenFile,&_foundCommandIndex,&_parsedArguments,&_parsedArgumentsLength);
			freeNathanscriptVariableArray(_parsedArguments,_parsedArgumentsLength);
			if (_foundCommandIndex==nathanscriptFoundFiIndex){
				break;
			}
		}
	}
	return;
}

void scriptGoto(nathanscriptVariable* _argumentList, int _totalArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	long int _startSearchSpot = ftell(nathanscriptCurrentOpenFile);
	char _didFoundLabel=0;
	if (fseek(nathanscriptCurrentOpenFile,0,SEEK_SET)!=0){
		printf("Seek error 1.\n");
	}
	while (!feof(nathanscriptCurrentOpenFile)){
		int _foundCommandIndex;
		nathanscriptVariable* _parsedArguments;
		int _parsedArgumentsLength;
		nathanscriptParseSingleLine(nathanscriptCurrentOpenFile,&_foundCommandIndex,&_parsedArguments,&_parsedArgumentsLength);
		if (_foundCommandIndex==nathanscriptFoundLabelIndex){
			if (strcmp(nathanvariableToString(&_parsedArguments[0]),nathanvariableToString(&_argumentList[0]))==0){
				freeNathanscriptVariableArray(_parsedArguments,_parsedArgumentsLength);
				_didFoundLabel=1;
				break;
			}
		}
		freeNathanscriptVariableArray(_parsedArguments,_parsedArgumentsLength);
	}
	if (_didFoundLabel==0){
		printf("Label not found.\n");
		if (fseek(nathanscriptCurrentOpenFile,_startSearchSpot,SEEK_SET)!=0){
			printf("Seek error 2.\n");
		}
	}
	return;
}

void scriptLuaDostring(nathanscriptVariable* _madeArgs, int _totalArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	luaL_dostring(L,nathanvariableToString(&_madeArgs[0]));
	return;
}

#endif