// TODO - GOOD NEWLINE THINGIE - DONT DO WHAT NOVELS DO AND JUST WRITE HALF OF THE WORD ON ONE LINE AND THE OTHER HALF ON THE OTHER, THAT LOOKS SHIT
// TODO - It did not like "-", did not display correctly! >:(
// TODO - THE DASHES IN THE SCRIPT ACTUALLY ARN'T DASHES! — opposed to -

// Libraries all need
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>

#include <Lua/lua.h>
#include <Lua/lualib.h>
#include <Lua/lauxlib.h>

#define LUAREGISTER(x,y) lua_pushcfunction(L,x);\
	lua_setglobal(L,y);

lua_State* L;


/*
	Line_ContinueAfterTyping=0; (No wait after text display, go right to next script line)
	Line_WaitForInput=1; (Wait for the player to click. Displays an arrow.)
	Line_Normal=2; (Wait for the player to click. DIsplays a page icon and almost 100% of the time has the screen cleared with the next command)
*/
int endType;

// ~60 chars per line???
char currentMessages[15][61];

int currentLine=0;

int GetNextCharOnLine(int _linenum){
	return strlen(currentMessages[_linenum]);
}

void ClearMessageArray(){
	int i,j;
	for (i = 0; i < 15; i++){
		for (j = 0; j < 61; j++){
			currentMessages[i][j]='\0';
		}
	}
}

void PrintScreen(){
	system("cls");
	for (int i = 0; i < 15; i++){
		printf("%s\n",currentMessages[i]);
	}
}

int L_ClearMessage(lua_State* passedState){
	system("cls");
	currentLine=0;
	ClearMessageArray();
}




int L_OutputLine(lua_State* passedState){
	int i;
	int currentChar = GetNextCharOnLine(currentLine);
	const char* message = lua_tostring(passedState,4);
	//printf("Output. Line: %d; Char: %d\n",currentLine,currentChar);
	//getch();
	endType = lua_tonumber(passedState,5);

	for (i = 0; i < strlen(message); i++){
		//printf("%d;%d;%d\n",i,currentLine,currentChar);

		if (currentChar==60){
			currentLine++;
			currentChar = GetNextCharOnLine(currentLine);
		}

		if (message[i]=='\n'){
			//printf("es un new line!\n");
			//getch();
			currentLine++;
			currentChar = GetNextCharOnLine(currentLine);
		}else{
			memcpy(&(currentMessages[currentLine][currentChar]),&(message[i]),1);
			currentChar++;

			//PrintScreen();
			//getch();

		}
	}
	//memcpy(currentMessages[currentLine],message,strlen(message));
	
	//printf("Ended output. Line: %d; Char: %d\n", currentLine, currentChar);
	//getch();
	//printf("%s",message);
	return 0;
}

// THIS IS AN (almost) DIRECT COPY OF L_OutputLine 
int L_OutputLineAll(lua_State* passedState){
	int i;
	int currentChar = GetNextCharOnLine(currentLine);
	const char* message = lua_tostring(passedState,2);
	endType = lua_tonumber(passedState,3);
	for (i = 0; i < strlen(message); i++){
		if (currentChar==60){
			currentLine++;
			currentChar = GetNextCharOnLine(currentLine);
		}
		if (message[i]=='\n'){
			currentLine++;
			currentChar = GetNextCharOnLine(currentLine);
		}else{
			memcpy(&(currentMessages[currentLine][currentChar]),&(message[i]),1);
			currentChar++;
		}
	}
	return 0;
}


void trace(lua_State *L, lua_Debug *ar) {
	// display debug info
	int blocked=1; 
	if (endType==1 || endType==2){
		PrintScreen();
		getch();
		endType=0;
	}
}



int main(int argc, char const *argv[]){
	/* code */
	// Fill with null char
	ClearMessageArray();
	


	// Initiate Lua
	L = luaL_newstate();
	luaL_openlibs(L);

	LUAREGISTER(L_OutputLine,"OutputLine")
	LUAREGISTER(L_ClearMessage,"ClearMessage")
	LUAREGISTER(L_OutputLineAll,"OutputLineAll")


	luaL_dofile(L,"./happy.lua");
	lua_sethook(L, trace, LUA_MASKLINE, 5);

	luaL_dofile(L,"./test.lua");
	return 0;
}