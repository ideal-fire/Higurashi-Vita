#ifndef GUARD_LUAWRAPPERHELPER
#define GUARD_LUAWRAPPERHELPER
//===============================================================
#define GENERATELUAWRAPPER(scriptFunctionName) \
int L_##scriptFunctionName(lua_State* passedState){ \
	void** _madeArgs = makeScriptArguments(passedState); \
	scriptFunctionName(_madeArgs,lua_gettop(passedState)); \
	freeScriptArguments(_madeArgs,lua_gettop(passedState)); \
	return 0; \
}
#define PUSHLUAWRAPPER(scriptFunctionName,luaFunctionName) lua_pushcfunction(L,L_##scriptFunctionName);\
	lua_setglobal(L,luaFunctionName);
//===============================================================
#define SCRIPT_TOBOOL(x,y) (*((char*)(x[y])))
#define SCRIPT_TOSTRING(x,y) ((char*)(x[y]))
#define SCRIPT_TOBIGNUMBER(x,y) (*(lua_Integer*)(x[y]))
#define SCRIPT_TONUMBER(x,y) (*(int*)(x[y]))
#define SCRIPT_TONULL(x,y) SCRIPT_TONUMBER(x,y)
#define SCRIPT_TOLIGHTUSERDATA(x,y) (x[y])
#define SCRIPT_ISMAYBENULL(x,y) (SCRIPT_TONUMBER(x,y) ? 0 : 1)
//===============================================================
void** makeScriptArguments(lua_State* passedState){
	int i;
	int _numberOfArguments = lua_gettop(passedState);
	void** _returnedArguments = malloc(sizeof(void*)*lua_gettop(passedState));
	for (i=0;i<lua_gettop(passedState);++i){ // bool, string, int
		switch (lua_type(passedState,i+1)){
			case LUA_TNIL:
				_returnedArguments[i] = calloc(1,1);
				break;
			case LUA_TNUMBER:
				_returnedArguments[i] = malloc(sizeof(lua_Integer));
				*((lua_Integer*)_returnedArguments[i])=lua_tonumber(passedState,i+1);
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
#endif