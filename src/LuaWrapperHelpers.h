#ifndef GUARD_LUAWRAPPERHELPER
#define GUARD_LUAWRAPPERHELPER
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <Lua/lua.h>
#include <Lua/lualib.h>
#include <Lua/lauxlib.h>

#define PUSHLUAWRAPPER(scriptFunctionName,luaFunctionName) lua_pushcfunction(L,L_##scriptFunctionName);\
	lua_setglobal(L,luaFunctionName);

#define SCRIPT_TOCHAR(x,y) (*((char*)(x[y])))
#define SCRIPT_TOBOOL(x,y) SCRIPT_TOCHAR(x,y)
#define SCRIPT_TOSTRING(x,y) ((char*)(x[y]))
#define SCRIPT_TOINT(x,y) ((int)SCRIPT_TOFLOAT(x,y))
#define SCRIPT_TOFLOAT(x,y) (*((float*)(x[y])))
#define SCRIPT_TONULL(x,y) SCRIPT_TOINT(x,y)
#define SCRIPT_TOPOINTER(x,y) (x[y])

#define SCRIPT_ISMAYBENULL(x,y) (SCRIPT_TOCHAR(x,y) == 0 ? 1 : 0)

#define LUAREGISTER(x,y) lua_pushcfunction(L,x);\
	lua_setglobal(L,y);

#define GENERATELUAWRAPPER(scriptFunctionName) \
int L_##scriptFunctionName(lua_State* passedState){ \
	void** _madeArgs = makeScriptArguments(passedState); \
	void** _gottenReturnArray = scriptFunctionName(_madeArgs,lua_gettop(passedState)); \
	freeScriptArguments(_madeArgs,lua_gettop(passedState)); \
	if (_gottenReturnArray!=NULL){ \
		pushReturnArrayToLua(passedState, _gottenReturnArray); \
		return freeReturnArray(_gottenReturnArray); \
	} \
	return 0; \
}

#define NATHAN_RETURNTYPE_NULL 0
#define NATHAN_RETURNTYPE_INT 1
#define NATHAN_RETURNTYPE_BOOL 2
#define NATHAN_RETURNTYPE_STRING 3
#define NATHAN_RETURNTYPE_FLOAT 4
#define NATHAN_RETURNTYPE_POINTER 5

/*============================================================================*/
void** makeScriptArguments(lua_State* passedState){
	int i;
	//lua_type
	int _numberOfArguments = lua_gettop(passedState);
	void** _returnedArguments = malloc(sizeof(void*)*lua_gettop(passedState));
	for (i=0;i<lua_gettop(passedState);++i){ // bool, string, int
		switch (lua_type(passedState,i+1)){
			case LUA_TNIL:
				_returnedArguments[i] = calloc(1,1);
				break;
			case LUA_TNUMBER:
				_returnedArguments[i] = malloc(sizeof(float));
				*((float*)_returnedArguments[i])=lua_tonumber(passedState,i+1);
				break;
			case LUA_TBOOLEAN:
				_returnedArguments[i] = malloc(sizeof(char));
				*((char*)_returnedArguments[i])=lua_toboolean(passedState,i+1);
				break;
			case LUA_TSTRING:
				;
				const char* _passedString = lua_tostring(passedState,i+1);
				_returnedArguments[i] = malloc(strlen(_passedString)+1);
				strcpy((char*)(_returnedArguments[i]),_passedString);
				break;
			case LUA_TLIGHTUSERDATA:
				_returnedArguments[i] = malloc(sizeof(void*));
				_returnedArguments[i]=lua_touserdata(passedState,i+1);
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
				_returnedArguments[i] = calloc(1,1);
				break;
		}
	}
	return _returnedArguments;
}
void freeScriptArguments(void** _passedArguments, int _numArguments){
	int i;
	for (i=0;i<_numArguments;i++){
		free(_passedArguments[i]);
	}
	free(_passedArguments);
}
void** nathanscriptMakeNewReturnArray(int _numberOfArguments){
	void** _tempReturnArray = malloc(sizeof(void*)*_numberOfArguments*2+1);
	int i;
	for (i=0;i<_numberOfArguments*2+1;i++){
		_tempReturnArray[i]=NULL;
	}
	_tempReturnArray[0] = malloc(sizeof(int));
	SCRIPT_TOFLOAT(_tempReturnArray,0)=_numberOfArguments;
	return _tempReturnArray;
}

void _nathanscriptSetArgumentType(void** _returnArray, int _friendlyArrayIndex, int _argumentType){
	_returnArray[_friendlyArrayIndex*2+1] = malloc(sizeof(char));
	SCRIPT_TOCHAR(_returnArray,_friendlyArrayIndex*2+1)=_argumentType;
}

void nathanscriptReturnArraySetInt(void** _returnArray, int _friendlyArrayIndex, int _valueToSet){
	_nathanscriptSetArgumentType(_returnArray,_friendlyArrayIndex,NATHAN_RETURNTYPE_INT);
	_returnArray[_friendlyArrayIndex*2+2] = malloc(sizeof(int));
	SCRIPT_TOFLOAT(_returnArray,_friendlyArrayIndex*2+2)=_valueToSet;
}

void nathanscriptReturnArraySetFloat(void** _returnArray, int _friendlyArrayIndex, float _valueToSet){
	_nathanscriptSetArgumentType(_returnArray,_friendlyArrayIndex,NATHAN_RETURNTYPE_FLOAT);
	_returnArray[_friendlyArrayIndex*2+2] = malloc(sizeof(float));
	SCRIPT_TOFLOAT(_returnArray,_friendlyArrayIndex*2+2)=_valueToSet;
}

void nathanscriptReturnArraySetPointer(void** _returnArray, int _friendlyArrayIndex, void* _valueToSet){
	_nathanscriptSetArgumentType(_returnArray,_friendlyArrayIndex,NATHAN_RETURNTYPE_POINTER);
	_returnArray[_friendlyArrayIndex*2+2] = malloc(sizeof(void*));
	SCRIPT_TOPOINTER(_returnArray,_friendlyArrayIndex*2+2)=_valueToSet;
}

void nathanscriptReturnArraySetBool(void** _returnArray, int _friendlyArrayIndex, char _valueToSet){
	_nathanscriptSetArgumentType(_returnArray,_friendlyArrayIndex,NATHAN_RETURNTYPE_BOOL);
	_returnArray[_friendlyArrayIndex*2+2] = malloc(sizeof(char));
	SCRIPT_TOFLOAT(_returnArray,_friendlyArrayIndex*2+2)=_valueToSet;
}

void nathanscriptReturnArraySetString(void** _returnArray, int _friendlyArrayIndex, char* _valueToSet){
	_nathanscriptSetArgumentType(_returnArray,_friendlyArrayIndex,NATHAN_RETURNTYPE_STRING);
	_returnArray[_friendlyArrayIndex*2+2] = malloc(strlen(_valueToSet)+1);
	strcpy(_returnArray[_friendlyArrayIndex*2+2],_valueToSet);
}

void pushSingleReturnArgument(lua_State* passedState, char _returnValueType, void* _returnValue){
	printf("Trying to push...\n");
	switch(_returnValueType){
		case NATHAN_RETURNTYPE_INT:
			lua_pushinteger(passedState,*((int*)_returnValue));
			break;
		case NATHAN_RETURNTYPE_STRING:
			lua_pushstring(passedState,(char*)_returnValue);
			break;
		case NATHAN_RETURNTYPE_POINTER:
			lua_pushlightuserdata(passedState,_returnValue);
			break;
		case NATHAN_RETURNTYPE_FLOAT:
			lua_pushnumber(passedState,*((float*)_returnValue));
			break;
		case NATHAN_RETURNTYPE_BOOL:
			lua_pushboolean(passedState,*(char*)_returnValue);
			break;
		case NATHAN_RETURNTYPE_NULL:
			lua_pushnil(passedState);
			break;
		default:
			printf("Bad value type %d\n",_returnValueType);
			break;
	}
	printf("The dark deed is done.\n");
}

void pushReturnArrayToLua(lua_State* passedState, void** _passedReturnArray){
	int _totalReturnArguments = *((int*)(_passedReturnArray[0]));
	int i;
	for (i=0;i<_totalReturnArguments;i++){
		pushSingleReturnArgument(passedState,*((char*)(_passedReturnArray[i*2+1])),_passedReturnArray[i*2+2]);
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

#endif