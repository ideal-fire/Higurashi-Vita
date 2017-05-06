// TODO - GOOD NEWLINE THINGIE - DONT DO WHAT NOVELS DO AND JUST WRITE HALF OF THE WORD ON ONE LINE AND THE OTHER HALF ON THE OTHER, THAT LOOKS SHIT
// It should be easy, but....
// The text is displayed as the memory is copied.
// If I reach the end of the line before the word ends, I'll have to move the word.
// If I move the word, it interrupts the user's reading.
// I HATE THAT!
// What I have to do is either predict if the next word will reach the end of the line, determine newline spots beforehand, don't draw as memory is written; write all then display.
#define PLAT_WINDOWS 1
#define PLAT_VITA 2
#define PLAT_3DS 3

#define REND_SDL 0
#define REND_VITA2D 1
#define REND_SF2D 2

#define SND_NONE 0
#define SND_SDL 1

#define RENDERER REND_SDL

#define PLATFORM PLAT_WINDOWS

#define SOUNDPLAYER SND_SDL

#define SILENTMODE 1

void Draw();

// Libraries all need
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
//

#include <Lua/lua.h>
#include <Lua/lualib.h>
#include <Lua/lauxlib.h>

//

#define LUAREGISTER(x,y) lua_pushcfunction(L,x);\
	lua_setglobal(L,y);

// Platform specific libraries
#if PLATFORM == PLAT_WINDOWS
	#define u64 unsigned long
	#include <time.h>
	#include <conio.h>
#endif

#if PLATFORM == PLAT_VITA
	#include <psp2/ctrl.h>
	#include <psp2/kernel/processmgr.h>
	#include <psp2/rtc.h>
	#include <psp2/types.h>
	#include <psp2/touch.h>
	#include <psp2/display.h>

	#define getch(); nothing();

	typedef uint8_t 	u8;
	typedef uint16_t 	u16;
	typedef uint32_t	u32;
	typedef uint64_t	u64;
	typedef int8_t		s8;
	typedef int16_t		s16;
	typedef int32_t		s32;
	typedef int64_t		s64;
#endif

#if RENDERER == REND_VITA2D
	#include <vita2d.h>
	// CROSS TYPES
	#define CrossTexture vita2d_texture
#endif

#if RENDERER == REND_SDL
	#define CrossTexture SDL_Texture
	#include <SDL2/SDL.h>
	#include <SDL2/SDL_image.h>
#endif

#if SOUNDPLAYER == SND_SDL
	#include <SDL2/SDL.h>
	#include <SDL2/SDL_mixer.h>

	#define CROSSSFX Mix_Chunk
	#define CROSSMUSIC Mix_Music
#endif

////////////////////////////////////////
// PLatform specific variables
///////////////////////////////////////
#if RENDERER == REND_SDL
	//The window we'll be rendering to
	SDL_Window* mainWindow;
	
	//The window renderer
	SDL_Renderer* mainWindowRenderer;

	CrossTexture* fontImage;

	float fontSize = 1.7;
#endif

#if RENDERER == REND_VITA2D
	int fontSize=32;
#endif

#if PLATFORM == PLAT_VITA
	// Controls at start of frame.
	SceCtrlData pad;
	// Controls from start of last frame.
	SceCtrlData lastPad;

	vita2d_font* fontImage;
#endif
#if PLATFORM == PLAT_WINDOWS
	enum SceCtrlPadButtons {
		SCE_CTRL_SELECT      = 0,	//!< Select button.
		SCE_CTRL_L3          = 1,	//!< L3 button.
		SCE_CTRL_R3          = 2,	//!< R3 button.
		SCE_CTRL_START       = 3,	//!< Start button.
		SCE_CTRL_UP          = 4,	//!< Up D-Pad button.
		SCE_CTRL_RIGHT       = 5,	//!< Right D-Pad button.
		SCE_CTRL_DOWN        = 6,	//!< Down D-Pad button.
		SCE_CTRL_LEFT        = 7,	//!< Left D-Pad button.
		SCE_CTRL_LTRIGGER    = 8,	//!< Left trigger.
		SCE_CTRL_RTRIGGER    = 9,	//!< Right trigger.
		SCE_CTRL_L1          = 10,	//!< L1 button.
		SCE_CTRL_R1          = 11,	//!< R1 button.
		SCE_CTRL_TRIANGLE    = 12,	//!< Triangle button.
		SCE_CTRL_CIRCLE      = 13,	//!< Circle button.
		SCE_CTRL_CROSS       = 14,	//!< Cross button.
		SCE_CTRL_SQUARE      = 15,	//!< Square button.
		SCE_CTRL_INTERCEPTED = 16,  //!< Input not available because intercepted by another application
		SCE_CTRL_VOLUP       = 17,	//!< Volume up button.
		SCE_CTRL_VOLDOWN     = 18	//!< Volume down button.
	};

	char pad[19]={0};
	char lastPad[19]={0};
#endif

//////////////
#if PLATFORM == PLAT_VITA
	#define STREAMINGASSETS "ux0:data/HIGURASHI/StreamingAssets"
#endif
#if PLATFORM == PLAT_WINDOWS
	#define STREAMINGASSETS "./StreamingAssets"
#endif

/*
CUSTOM INCLUDES
*/
#include "GeneralGood.h"
#include "FpsCapper.h"

lua_State* L;
/*
	Line_ContinueAfterTyping=0; (No wait after text display, go right to next script line)
	Line_WaitForInput=1; (Wait for the player to click. Displays an arrow.)
	Line_Normal=2; (Wait for the player to click. DIsplays a page icon and almost 100% of the time has the screen cleared with the next command)
*/
#define Line_ContinueAfterTyping 0
#define Line_WaitForInput 1
#define Line_Normal 2
int endType;
char useVsync=0;
// ~60 chars per line???
unsigned char currentMessages[15][61];
int currentLine=0;
int place=0;
int screenHeight = 544;
int screenWidth = 960;

#define STATUS_NONE 0
#define STATUS_WAITINGFORINPUT 1
#define STATUS_DISPLAYINGTEXT 2
#define STATUS_WAITING 3
char LuaThreadStatus = STATUS_NONE;
pthread_t tid;
CrossTexture* currentBackground = NULL;
CROSSMUSIC* currentMusic;

int TextSpeed=10;
#define MESSAGE_NONE 0
#define MESSAGE_FINISHLINE 1
#define MESSAGE_IM_WAITING 2
#define MESSAGE_WAIT 3
unsigned char MessageToLua=0;

unsigned char MessageFromLua=0;
// Alpha of black rectangle over screen
int MessageBoxAlpha = 100;

/*
====================================================
*/

/*
*/
#if RENDERER == REND_SDL
	void DrawLetterUnscaled(int letterId, int _x, int _y, float size){
		DrawTexturePartScale(fontImage,_x,_y,(letterId-32)*(8),0,8,8,size,size);
	}
#endif
void DrawText(int x, int y, const char* text, float size){
	#if RENDERER == REND_VITA2D
		vita2d_font_draw_text(fontImage,x,y, RGBA8(255,255,255,255),floor(size),text);
	#endif
	#if RENDERER == REND_SDL
		int i=0;
		int notICounter=0;
		for (i = 0; i < strlen(text); i++){
			DrawLetterUnscaled(text[i],(x+(notICounter*(8*size))+notICounter),(y),size);
			notICounter++;
		}
	#endif
}

char WasJustPressed(int value){
	#if PLATFORM == PLAT_VITA
		if (pad.buttons & value && !(lastPad.buttons & value)){
			return 1;
		}
	#elif PLATFORM == PLAT_WINDOWS
		if (pad[value]==1 && lastPad[value]==0){
			return 1;
		}
	#elif PLATFORM==PLAT_3DS
		if (wasJustPad & value){
			return 1;
		}
	#endif
	return 0;
}

char IsDown(int value){
	#if PLATFORM == PLAT_VITA
		if (pad.buttons & value){
			return 1;
		}
	#elif PLATFORM == PLAT_WINDOWS

		if (pad[value]==1){
			return 1;
		}
	#elif PLATFORM == PLAT_3DS
		if (pad & value){
			return 1;
		}
	#endif
	return 0;
}

/*
*/
void WriteSDLError(){
	FILE *fp;
	fp = fopen("ux0:data/HIGURASHI/a.txt", "a");
	fprintf(fp,"%s\n",SDL_GetError());
	fclose(fp);
}

size_t u_strlen(const unsigned char * array){
	return (const size_t)strlen((const char*)array);
}

void nothing(){
	printf("nothing");
}

int GetNextCharOnLine(int _linenum){
	return u_strlen(currentMessages[_linenum]);
}

void ClearMessageArray(){
	int i,j;
	for (i = 0; i < 15; i++){
		for (j = 0; j < 61; j++){
			currentMessages[i][j]='\0';
		}
	}
}

int TextHeight(float scale){
	#if RENDERER == REND_SDL
		return (8*scale);
	#elif RENDERER == REND_VITA2D
		return vita2d_font_text_height(fontImage,scale,"a");
	#endif
}

void DrawMessageText(){
	//system("cls");
	int i;
	for (i = 0; i < 15; i++){
		//printf("%s\n",currentMessages[i]);
		DrawText(20,20+TextHeight(fontSize)+i*(TextHeight(fontSize)),currentMessages[i],fontSize);
	}
}

void PrintScreenValues(){
	system("cls");
	int i,j;
	for (i = 0; i < 15; i++){
		for (j = 0; j < 61; j++){
			printf("%d;",currentMessages[i][j]);
		}
		printf("\n");
	}
}

void ControlsStart(){
	#if PLATFORM == PLAT_VITA
		lastPad=pad;
		sceCtrlPeekBufferPositive(0, &pad, 1);
		//sceTouchPeek(SCE_TOUCH_PORT_FRONT, &currentTouch, 1);
	#endif
	#if RENDERER == REND_SDL
		SDL_Event e;
		while( SDL_PollEvent( &e ) != 0 ){
			if( e.type == SDL_QUIT ){
				place=2;
			}
			#if PLATFORM == PLAT_WINDOWS
				if( e.type == SDL_KEYDOWN ){
					if (e.key.keysym.sym==SDLK_z){ /* X */
						pad[SCE_CTRL_CROSS]=1;
					}else if (e.key.keysym.sym==SDLK_x){/* O */
						pad[SCE_CTRL_CIRCLE]=1;
					}else if (e.key.keysym.sym==SDLK_LEFT){/* Left */
						pad[SCE_CTRL_LEFT]=1;
					}else if (e.key.keysym.sym==SDLK_RIGHT){ /* Right */
						pad[SCE_CTRL_RIGHT]=1;
					}else if (e.key.keysym.sym==SDLK_DOWN){ /* Down */
						pad[SCE_CTRL_DOWN]=1;
					}else if (e.key.keysym.sym==SDLK_UP){ /* Up */
						pad[SCE_CTRL_UP]=1;
					}else if (e.key.keysym.sym==SDLK_a){ /* Square */
						pad[SCE_CTRL_SQUARE]=1;
					}else if (e.key.keysym.sym==SDLK_s){ /* Triangle */
						pad[SCE_CTRL_TRIANGLE]=1;
					}else if (e.key.keysym.sym==SDLK_ESCAPE){ /* Start */
						pad[SCE_CTRL_START]=1;
					}
				}else if (e.type == SDL_KEYUP){
					if (e.key.keysym.sym==SDLK_z){ /* X */
						pad[SCE_CTRL_CROSS]=0;
					}else if (e.key.keysym.sym==SDLK_x){/* O */
						pad[SCE_CTRL_CIRCLE]=0;
					}else if (e.key.keysym.sym==SDLK_LEFT){/* Left */
						pad[SCE_CTRL_LEFT]=0;
					}else if (e.key.keysym.sym==SDLK_RIGHT){ /* Right */
						pad[SCE_CTRL_RIGHT]=0;
					}else if (e.key.keysym.sym==SDLK_DOWN){ /* Down */
						pad[SCE_CTRL_DOWN]=0;
					}else if (e.key.keysym.sym==SDLK_UP){ /* Up */
						pad[SCE_CTRL_UP]=0;
					}else if (e.key.keysym.sym==SDLK_a){ /* Square */
						pad[SCE_CTRL_SQUARE]=0;
					}else if (e.key.keysym.sym==SDLK_s){ /* Triangle */
						pad[SCE_CTRL_TRIANGLE]=0;
					}else if (e.key.keysym.sym==SDLK_ESCAPE){ /* Start */
						pad[SCE_CTRL_START]=0;
					}
				}
			#endif
		}
	#endif
}

void StartDrawingA(){
	#if RENDERER == REND_VITA2D
		vita2d_start_drawing();
		vita2d_clear_screen();
	#elif RENDERER == REND_SDL
		SDL_RenderClear(mainWindowRenderer);
	#elif RENDERER == REND_SF2D
		sf2d_start_frame(GFX_TOP, GFX_LEFT);
	#endif
}

void EndDrawingA(){
	#if RENDERER == REND_VITA2D
		vita2d_end_drawing();
		vita2d_swap_buffers();
		vita2d_wait_rendering_done();
	#elif RENDERER == REND_SDL
		SDL_RenderPresent(mainWindowRenderer);
	#elif RENDERER == REND_SF2D
		sf2d_end_frame();
		sf2d_swapbuffers();
	#endif
}

void ControlsEnd(){
	#if PLATFORM == PLAT_VITA
		//previousTouchData=currentTouch;
	#elif PLATFORM == PLAT_WINDOWS
		memcpy(lastPad,pad,19);
	#endif
}

void trace(lua_State *L, lua_Debug *ar) {
	// display debug info
	//if (endType==1 || endType==2){
	//	//Draw();
	//	getch();
	//	endType=0;
	//}
	while (LuaThreadStatus==STATUS_WAITINGFORINPUT){
		Wait(50);
	}
}

void WriteDebugInfo(){
	FILE *fp;
	fp = fopen("ux0:data/HIGURASHI/a.txt", "a");
	fprintf(fp,"There are %d music deocoders available\n", Mix_GetNumMusicDecoders());
	fclose(fp);
}

void WaitMainThread(){
	MessageFromLua = MESSAGE_WAIT;
}

void ResumeMainThread(){
	MessageFromLua = MESSAGE_NONE;
}

void GoodFreeTexture(CrossTexture* passedTexture){
	WaitMainThread();
	while (MessageToLua!=MESSAGE_IM_WAITING){
		Wait(1);
	}
	FreeTexture(passedTexture);
	ResumeMainThread();
}

//===================

/*
=================================================
*/

int L_ClearMessage(lua_State* passedState){
	printf("clearing messages\n");
	//system("cls");
	currentLine=0;
	ClearMessageArray();
	return 0;
}

int L_OutputLine(lua_State* passedState){
	int i;
	int currentChar = GetNextCharOnLine(currentLine);
	unsigned const char* message = (unsigned const char*)lua_tostring(passedState,4);
	//printf("Output. Line: %d; Char: %d\n",currentLine,currentChar);
	//getch();
	endType = lua_tonumber(passedState,5);

	printf("output new line %s\n",message);

	for (i = 0; i < u_strlen(message); i++){
		LuaThreadStatus = STATUS_DISPLAYINGTEXT;
		//printf("%d;%d;%d\n",i,currentLine,currentChar);
		//printf("%d\n",i);
		//getch();
		if (currentChar==60){
			currentLine++;
			currentChar = GetNextCharOnLine(currentLine);
		}

		//if (message[i]=="—"){
		//	currentMessages[currentLine][currentChar]="-";
		//}

		if (message[i]==226 && message[i+1]==128 && message[i+2]==148){
			i=i+2;

			//currentMessages[currentLine][currentChar]=/*"-"*/45;
			memset(&(currentMessages[currentLine][currentChar]),45,1);
			//printf("is %c\n",(char)currentMessages[currentLine][currentChar]);
			//getch();
			currentChar++;
		}else if (message[i]=='\n'){
			//printf("es un new line!\n");
			//getch();
			currentLine++;
			currentChar = GetNextCharOnLine(currentLine);
		}else{
			memcpy(&(currentMessages[currentLine][currentChar]),&(message[i]),1);
			currentChar++;
			if (MessageToLua!=MESSAGE_FINISHLINE){
				Wait(TextSpeed);
			}
			//PrintScreen();
			//getch();

		}
	}
	//memcpy(currentMessages[currentLine],message,u_strlen(message));
	
	//printf("Ended output. Line: %d; Char: %d\n", currentLine, currentChar);
	//getch();
	//printf("%s",message);
	if (endType == Line_WaitForInput || endType == Line_Normal){
		LuaThreadStatus=STATUS_WAITINGFORINPUT;
	}else{
		LuaThreadStatus = STATUS_NONE;
	}

	if (MessageToLua == MESSAGE_FINISHLINE){
		MessageToLua=MESSAGE_NONE;
	}
	return 0;
}

// THIS IS AN (almost) DIRECT COPY OF L_OutputLine 
int L_OutputLineAll(lua_State* passedState){
	int i;
	int currentChar = GetNextCharOnLine(currentLine);
	const unsigned char* message = (const unsigned char*)lua_tostring(passedState,2);
	endType = lua_tonumber(passedState,3);
	for (i = 0; i < u_strlen(message); i++){
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

//
int L_Wait(lua_State* passedState){
	Wait(lua_tonumber(passedState,1));
	return 0;
}

int L_DrawScene(lua_State* passedState){
	printf("Warning: Make new command for drawscenewithmask, right now it just ignroes all args.\n");
	if (currentBackground!=NULL){
		printf("Dispose old background\n");
		GoodFreeTexture(currentBackground);
		currentBackground=NULL;
	}
	char* tempstringconcat = calloc(strlen(lua_tostring(passedState,1))+5+strlen(STREAMINGASSETS"/CG/"),1);
	strcat(tempstringconcat, STREAMINGASSETS"/CG/");
	strcat(tempstringconcat, lua_tostring(passedState,1));
	strcat(tempstringconcat, ".png");
	currentBackground = LoadPNG(tempstringconcat);
	free(tempstringconcat);
	return 0;
}

int L_NotYet(lua_State* passedState){
	printf("AN UNIMPLEMENTED LUA FUNCTION WAS JUST EXECUTED! PLEASE REMEMBER TO FIX THIS, NATHAN!\n");
	return 0;
}

// Fist arg seems to be a channel arg.
	// Usually 1 for msys
	// Usually 2 for lsys
// Second arg is path in BGM folder without extention
// Third arg is volume. 128 seems to be average. I can hardly hear 8 with computer volume on 10.
// Fourth arg is unknown
int L_PlayBGM(lua_State* passedState){
	StopMusic();
	if (currentMusic!=NULL){
		FreeMusic(currentMusic);
		currentMusic=NULL;
	}
	char* tempstringconcat = calloc(strlen(lua_tostring(passedState,2))+5+strlen("ux0:data/HIGURASHI/StreamingAssets/BGM/"),1);
	strcat(tempstringconcat, STREAMINGASSETS"/BGM/");
	strcat(tempstringconcat, lua_tostring(passedState,2));
	strcat(tempstringconcat, ".ogg");
	currentMusic = LoadMusic(tempstringconcat);
	free(tempstringconcat);

	Mix_VolumeMusic(lua_tonumber(passedState,3));

	if (lua_tonumber(passedState,4)!=0){
		printf("*************** VERY IMPORTANT *******************\nThe last PlayBGM call didn't have 0 for the fourth argument! This is a good place to investigate!\n");
	}

	#if SILENTMODE == 0
		PlayMusic(currentMusic);
	#endif
	return 0;
}

// Last arg, I was right. Bool for if wait for fadeoutr
int L_FadeoutBGM(lua_State* passedState){
	// I BET THE SECOND ARGUMENT IS IF IT WAITS OR NOT
	Mix_FadeOutMusic(lua_tonumber(passedState,2));
	if (lua_toboolean(passedState,3)==1){
		LuaThreadStatus = STATUS_WAITING;
		Wait(lua_tonumber(passedState,2));
	}
	return 0;
}

void MakeLuaUseful(){
	LUAREGISTER(L_OutputLine,"OutputLine")
	LUAREGISTER(L_ClearMessage,"ClearMessage")
	LUAREGISTER(L_OutputLineAll,"OutputLineAll")
	LUAREGISTER(L_Wait,"Wait")
	LUAREGISTER(L_DrawScene,"DrawScene")
	LUAREGISTER(L_DrawScene,"DrawSceneWithMask")
	LUAREGISTER(L_PlayBGM,"PlayBGM")
	LUAREGISTER(L_FadeoutBGM,"FadeOutBGM")

	//  TEMP
	LUAREGISTER(L_NotYet,"DisableWindow")
	LUAREGISTER(L_NotYet,"DrawBustshotWithFiltering")
	LUAREGISTER(L_NotYet,"SetValidityOfInput")
}
//======================================================
void Draw(){
	//DrawTexture(testtex,32,32);
	StartDrawingA();

	if (currentBackground!=NULL){
		DrawTexture(currentBackground,160,32);
	}

	// This is the message box
	DrawRectangle(160,32,640,480,0,0,0,MessageBoxAlpha);

	DrawMessageText();
	EndDrawingA();
}

void Controls(){
	if (LuaThreadStatus!=STATUS_NONE){
		if (LuaThreadStatus==STATUS_WAITINGFORINPUT){
			if (WasJustPressed(SCE_CTRL_CROSS)){
				LuaThreadStatus = STATUS_NONE;
			}
		}
		if (LuaThreadStatus==STATUS_DISPLAYINGTEXT){
			if (WasJustPressed(SCE_CTRL_CROSS)){
				MessageToLua = MESSAGE_FINISHLINE;
			}
		}
	}
}

void* LuaThread(){
	printf("thread start\n"); 
	#if PLATFORM == PLAT_VITA
		luaL_dofile(L,"app0:a/happy.lua");
		lua_sethook(L, trace, LUA_MASKLINE, 5);
		luaL_dofile(L,"app0:a/onik_001.lua");
	#elif PLATFORM == PLAT_WINDOWS
		luaL_dofile(L,"./happy.lua");
		lua_sethook(L, trace, LUA_MASKLINE, 5);
		luaL_dofile(L,"./onik_001.lua");
	#endif
	return NULL;
}

// =====================================================

void init(){
	#if RENDERER == REND_SDL
		mainWindow = SDL_CreateWindow( "HappyWindo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenWidth, screenHeight, SDL_WINDOW_SHOWN );
		ShowErrorIfNull(mainWindow);
		
		if (useVsync==0){
			mainWindowRenderer = SDL_CreateRenderer( mainWindow, -1, SDL_RENDERER_ACCELERATED);
		}else{
			mainWindowRenderer = SDL_CreateRenderer( mainWindow, -1, SDL_RENDERER_PRESENTVSYNC);
		}
		ShowErrorIfNull(mainWindowRenderer);
		
		// Check if this fails?
		IMG_Init( IMG_INIT_PNG );

		fontImage=LoadPNG("./Font.png");

		SDL_SetRenderDrawBlendMode(mainWindowRenderer,SDL_BLENDMODE_BLEND);
	#endif
	#if RENDERER == REND_VITA2D
		// Init vita2d and set its clear color.
		vita2d_init();
		
		// We love default fonts.
		//defaultPgf = vita2d_load_default_pgf();

		// Zero a variable that should already be zero.
		memset(&pad, 0, sizeof(pad));

		fontImage = vita2d_load_font_file("app0:a/LiberationSans-Regular.ttf");
	#endif
	#if RENDERER == REND_SF2D
		sf2d_init();
	#endif

	SetClearColor(0,0,0,255);

	#if SOUNDPLAYER == SND_SDL
		SDL_Init( SDL_INIT_AUDIO );
		Mix_Init(MIX_INIT_OGG);
		//WriteSDLError();
		//Initialize SDL_mixer
		Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 );
	#endif
}

int main(int argc, char *argv[]){
	/* code */
	init();
	// Fill with null char
	ClearMessageArray();

	// Initiate Lua
	L = luaL_newstate();
	luaL_openlibs(L);
	MakeLuaUseful();
    pthread_create(&tid, NULL, LuaThread, NULL);
    // NO UNCOMMENT \/
    //pthread_create(&tid2, NULL, DrawingThread, NULL);
    //pthread_join(tid2,NULL);
	
	//CROSSMUSIC* testsong = LoadMusic("app0:a/testogg.ogg");
	//WriteSDLError();
	//PlayMusic(testsong);
	//WriteSDLError();
	//
	//FILE *fp;
	//fp = fopen("ux0:data/HIGURASHI/a.txt", "a");
	//fprintf(fp,"There are %d music deocoders available\n", Mix_GetNumMusicDecoders());
	//fclose(fp);

    while (place!=2){
		
		FpsCapStart();
		ControlsStart();
		Draw();
		Controls();
		ControlsEnd();
		FpsCapWait();
		if (MessageFromLua==MESSAGE_WAIT){
			MessageToLua=MESSAGE_IM_WAITING;
			while (MessageFromLua==MESSAGE_WAIT){
				Wait(1);
			}
			MessageToLua=MESSAGE_NONE;
		}
	}

	return 0;
}