/*

	TODO - Remove music note ♪

	TODO - Italics
		OutputLine(NULL, "　……知レバ、…巻キ込マレテシマウ…。",
		   NULL, "...<i>If she found out... she would become involved</i>...", Line_Normal);

		So, here's my idea, I take whatever char it is and add 100 if it's italics

	TODO - FIx hima_tips_09
		THis is important. This was very significant to me, as I didn't expect a choice.
		I want everyone to have that experience
		(SEE SCRIPT CONVERTER FIXES BELOW)
	TODO - Test Himaburashi and Tatoregashi ps3 scripts
		ACTUALLY RUN THE FUNCTION WHEN TESTING, PLEASE
	TODO - Complete preset files for question arcs
		All that's left is to fix the tip unlocks, which is the most tedious thing

	TODO - Big script converter fix for hima_tips_09
		Convert char Item[2];
			to Item = {};

		For GetGlobalFlag and SetGlobalFlag, make the first argument a string.
			These functions will them become Lua functions in happy.lua which will return the value that is in an array using a string index
				See hima_tips_09.txt for examples
		For a right squiggily bracket, don't turn it into "end" if the next line is "else"
			If the next line is "else", just remove the squiggily bracket
				This is for if then else statements
					With my current setup, it's if then end else 
			If it isn't change the bracked to "end", as usual.


	TODO - Make CallSection actually call a global function
	TODO - Garbage collector won't collect functions made in script files??? i.e: function hima_tips_09_b()

*/

#define PLAT_WINDOWS 1
#define PLAT_VITA 2
#define PLAT_3DS 3

#define REND_SDL 0
#define REND_VITA2D 1
#define REND_SF2D 2

#define SND_NONE 0
#define SND_SDL 1

#define REZ_UNDEFINED 0
#define REZ_OLD 1
#define REZ_UPDATED 2
#define REZ_PS3_BACKGROUND 3
#define REZ_PS3_BUST 4

#define LOCATION_UNDEFINED 0
#define LOCATION_CG 1
#define LOCATION_CGALT 2

// Change these depending on the target platform
#define RENDERER REND_SDL
#define PLATFORM PLAT_WINDOWS
#define SOUNDPLAYER SND_SDL
#define SILENTMODE 0

unsigned char currentBackgroundRez;

unsigned char graphicsLocation = LOCATION_CGALT;

#define MAXBUSTS 7

void Draw();
void YeOlMainLoop();
void RecalculateBustOrder();

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
	#include <dirent.h>
#endif

#if PLATFORM == PLAT_VITA
	#include <psp2/ctrl.h>
	#include <psp2/kernel/processmgr.h>
	#include <psp2/rtc.h>
	#include <psp2/types.h>
	#include <psp2/touch.h>
	#include <psp2/io/fcntl.h>
	#include <psp2/io/dirent.h>


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
	char STREAMINGASSETS[] = "ux0:data/HIGURASHI/StreamingAssets/";
	char PRESETFOLDER[] = "ux0:data/HIGURASHI/StreamingAssets/Presets/";
	char SCRIPTFOLDER[] = "ux0:data/HIGURASHI/StreamingAssets/Scripts/";
	char SAVEFOLDER[] = "ux0:data/HIGURASHI/Saves/";
#elif PLATFORM == PLAT_WINDOWS
	char STREAMINGASSETS[] = "./StreamingAssets/";
	char PRESETFOLDER[] = "./StreamingAssets/Presets/";
	char SCRIPTFOLDER[] = "./StreamingAssets/Scripts/";
	char SAVEFOLDER[] = "./Saves/";
#endif

/*
CUSTOM INCLUDES
*/
#include "GeneralGood.h"
#include "FpsCapper.h"

#define BUST_STATUS_NORMAL 0
#define BUST_STATUS_FADEIN 1 // var 1 is alpha per frame. var 2 is time where 0 alpha
#define BUST_STATUS_FADEOUT 2 // var 1 is alpha per frame
#define BUST_STATUS_FADEOUT_MOVE 3 // var 1 is alpha per frame. var 2 is x per frame. var 3 is y per frame
#define BUST_STATUS_SPRITE_MOVE 4 // var 1 is x per frame, var 2 is y per frame

typedef struct hauighrehrge{
	CrossTexture* image;
	signed int xOffset;
	signed int yOffset;
	char isActive;
	char isInvisible;
	int layer;
	signed short alpha;
	unsigned char bustStatus;
	int statusVariable;
	int statusVariable2;
	int statusVariable3;
	int statusVariable4;
	unsigned int lineCreatedOn;
	unsigned char rez;
}bust;

bust Busts[MAXBUSTS];

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

CrossTexture* currentBackground = NULL;
CROSSMUSIC* currentMusic = NULL;
CROSSSFX* soundEffects[10];

CROSSSFX* menuSound;
char menuSoundLoaded=0;

// Alpha of black rectangle over screen
int MessageBoxAlpha = 100;

char MessageBoxEnabled=1;

char InputValidity = 1;

unsigned int currentScriptLine=0;

// Order of busts drawn as organized by layer
// element in array is their bust slot
// first element in array is highest up
// so, when drawing, start from the end and go backwards to draw the first element last
unsigned char bustOrder[MAXBUSTS];

unsigned char bustOrderOverBox[MAXBUSTS];

char* locationStrings[3] = {"CG/","CG/","CGAlt/"};

typedef struct grhuighruei{
	char** theArray;
	unsigned char length;
}goodStringMallocArray;
typedef struct grejgrkew{
	unsigned char* theArray;
	unsigned char length;
}goodu8MallocArray;

goodStringMallocArray currentPresetFileList;
goodStringMallocArray currentPresentTipList;
goodStringMallocArray currentPresentTipNameList;
goodu8MallocArray currentPresetTipUnlockList;
int16_t currentPresetChapter=0;
// Made with malloc
char* currentPresetFilename=NULL;

/*
0 - title
1 - Load preset from currentPresetFilename
2 - Open file selector for preset file selection
3 - Start Lua thread
4 - Navigation menu after preset loading
*/
char currentGameStatus=0;

char nextScriptToLoad[256] = {0};
char globalTempConcat[256] = {0};

unsigned char isSkipping=0;

char tipNamesLoaded=0;

/*
====================================================
*/

/*
*/

int TextHeight(float scale){
	#if RENDERER == REND_SDL
		return (8*scale);
	#elif RENDERER == REND_VITA2D
		return vita2d_font_text_height(fontImage,scale,"a");
	#endif
}
int TextWidth(float scale, char* message){
	#if RENDERER == REND_SDL
		return floor((8*scale)*strlen(message));
	#elif RENDERER == REND_VITA2D
		return vita2d_font_text_width(fontImage,scale,message);
	#endif
}

#if RENDERER == REND_SDL
	void DrawLetter(int letterId, int _x, int _y, float size){
		DrawTexturePartScale(fontImage,_x,_y,(letterId-32)*(8),0,8,8,size,size);
	}
	void DrawLetterColor(int letterId, int _x, int _y, float size, unsigned char r, unsigned char g, unsigned char b){
		DrawTexturePartScaleTint(fontImage,_x,_y,(letterId-32)*(8),0,8,8,size,size,r,g,b);
	}
#endif
void DrawText(int x, int y, const char* text, float size){
	#if RENDERER == REND_VITA2D
		vita2d_font_draw_text(fontImage,x,y+TextHeight(size), RGBA8(255,255,255,255),floor(size),text);
	#endif
	#if RENDERER == REND_SDL
		int i=0;
		int notICounter=0;
		for (i = 0; i < strlen(text); i++){
			DrawLetter(text[i],(x+(notICounter*(8*size))+notICounter),(y),size);
			notICounter++;
		}
	#endif
}
void DrawTextColored(int x, int y, const char* text, float size, unsigned char r, unsigned char g, unsigned char b){
	#if RENDERER == REND_VITA2D
		vita2d_font_draw_text(fontImage,x,y+TextHeight(size), RGBA8(r,g,b,255),floor(size),text);
	#endif
	#if RENDERER == REND_SDL
		int i=0;
		int notICounter=0;
		for (i = 0; i < strlen(text); i++){
			DrawLetterColor(text[i],(x+(notICounter*(8*size))+notICounter),(y),size,r,g,b);
			notICounter++;
		}
	#endif
}

char WasJustPressed(int value){
	if (InputValidity==1 || isSkipping==1){
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
	}
	return 0;
}

char IsDown(int value){
	if (InputValidity==1 || isSkipping==1){
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
	}
	return 0;
}

u64 waitwithCodeTarget;
void WaitWithCodeStart(int amount){
	waitwithCodeTarget = GetTicks()+amount;
}

void WaitWithCodeEnd(int amount){
	if (GetTicks()<waitwithCodeTarget){
		Wait(waitwithCodeTarget-GetTicks());
	}
}

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


void DrawMessageText(){
	//system("cls");
	int i;
	for (i = 0; i < 15; i++){
		//printf("%s\n",currentMessages[i]);
		DrawText(20,TextHeight(fontSize)+i*(TextHeight(fontSize)),(char*)currentMessages[i],fontSize);
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
		sceCtrlPeekBufferPositive(0, &pad, 1);
		//sceTouchPeek(SCE_TOUCH_PORT_FRONT, &currentTouch, 1);
	#endif
	#if RENDERER == REND_SDL
		SDL_Event e;
		while( SDL_PollEvent( &e ) != 0 ){
			if( e.type == SDL_QUIT ){
				if (currentGameStatus==3){
					//luaL_error(L,"game stopped\n");
					// Adds function to stack
					lua_getglobal(L,"quitxfunction");
					// Call funciton. Removes function from stack.
					lua_call(L, 0, 0);
				}else{
					printf("normalquit\n");
					currentGameStatus=99;
				}

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
		lastPad=pad;
	#elif PLATFORM == PLAT_WINDOWS
		memcpy(lastPad,pad,19);
	#endif
}

void WriteIntToDebugFile(int a){
	#if PLATFORM == PLAT_VITA
		FILE *fp;
		fp = fopen("ux0:data/HIGURASHI/a.txt", "a");
		fprintf(fp,"%d\n", a);
		fclose(fp);
	#endif
}

// Does not clear the debug file at ux0:data/HIGURASHI/a.txt  , I promise.
void ClearDebugFile(){
	#if PLATFORM == PLAT_VITA
	FILE *fp;
	fp = fopen("ux0:data/HIGURASHI/a.txt", "w");
	fclose(fp);
	#endif
}

void WriteToDebugFile(char* stuff){
	#if PLATFORM == PLAT_VITA
	FILE *fp;
	fp = fopen("ux0:data/HIGURASHI/a.txt", "a");
	fprintf(fp,"%s\n",stuff);
	fclose(fp);
	#endif
}

void ResetBustStruct(bust* passedBust, int canfree){
	if (canfree==1 && passedBust->image!=NULL){
		FreeTexture(passedBust->image);
	}
	passedBust->image=NULL;
	passedBust->xOffset=0;
	passedBust->yOffset=0;
	passedBust->isActive=0;
	passedBust->isInvisible=0;
	passedBust->alpha=255;
	passedBust->bustStatus = BUST_STATUS_NORMAL;
	passedBust->lineCreatedOn=0;
}

void DisposeOldScript(){
	// Frees the script main
	lua_getglobal(L,"FreeTrash");
	lua_call(L, 0, 0);
}

void RunScript(char* _scriptfolderlocation,char* filename, char addTxt){
	ClearMessageArray();	
	currentScriptLine=0;
	char tempstringconcat[strlen(_scriptfolderlocation)+strlen(filename)+strlen(".txt")+1];
	memset(&tempstringconcat,'\0',strlen(_scriptfolderlocation)+strlen(filename)+strlen(".txt")+1);
	strcpy(tempstringconcat,_scriptfolderlocation);
	strcat(tempstringconcat,filename);
	if (addTxt==1){
		strcat(tempstringconcat,".txt");
	}
	// Free garbage and old main function if it existed
	DisposeOldScript();

	luaL_dofile(L,tempstringconcat);
	
	
	// Adds function to stack
	lua_getglobal(L,"main");
	// Call funciton. Removes function from stack.
	lua_call(L, 0, 0);
}

char* CombineStringsPLEASEFREE(const char* first, const char* firstpointfive, const char* second, const char* third){
	char* tempstringconcat = calloc(1,strlen(first)+strlen(firstpointfive)+strlen(second)+strlen(third)+1);
	strcat(tempstringconcat, first);
	strcat(tempstringconcat, firstpointfive);
	strcat(tempstringconcat, second);
	strcat(tempstringconcat, third);
	return tempstringconcat;
}

char WaitCanSkip(int amount){
	int i=0;
	ControlsStart();
	ControlsEnd();
	for (i = 0; i < floor(amount/50); ++i){
		Wait(50);
		ControlsStart();
		if (WasJustPressed(SCE_CTRL_CROSS)){
			ControlsEnd();
			printf("Skipped with %d left\n",amount-i);
			return 1;
		}
		ControlsEnd();
	}
	Wait(amount%50);
	return 0;
}

void Update(){
	int i=0;
	for (i = 0; i < MAXBUSTS; i++){
		if (Busts[i].bustStatus == BUST_STATUS_FADEIN){
			if (Busts[i].statusVariable2>0){
				Busts[i].statusVariable2-=17;
				if (Busts[i].statusVariable2>4000000){
					Busts[i].statusVariable2=0;
					//Busts[i].bustStatus = BUST_STATUS_NORMAL;
				}
			}else{
				Busts[i].alpha += Busts[i].statusVariable;
				if (Busts[i].alpha>=255){
					Busts[i].alpha=255;
					Busts[i].bustStatus = BUST_STATUS_NORMAL;
				}
			}
		}
		if (Busts[i].bustStatus == BUST_STATUS_FADEOUT){
			Busts[i].alpha -= Busts[i].statusVariable;
			if (Busts[i].alpha<=0){
				Busts[i].alpha=0;
				Busts[i].isActive=0;
				ResetBustStruct(&(Busts[i]),1);
				Busts[i].bustStatus = BUST_STATUS_NORMAL;
				RecalculateBustOrder();
			}	
		}
		if (Busts[i].bustStatus == BUST_STATUS_SPRITE_MOVE){
			if (abs(Busts[i].statusVariable3-(Busts[i].xOffset+Busts[i].statusVariable))<abs(Busts[i].statusVariable3-Busts[i].xOffset)){
				Busts[i].xOffset+=Busts[i].statusVariable;
			}else{
				Busts[i].xOffset=Busts[i].statusVariable3;
			}

			if (abs(Busts[i].statusVariable4-(Busts[i].yOffset+Busts[i].statusVariable2))<abs(Busts[i].statusVariable4-Busts[i].yOffset)){
				Busts[i].yOffset+=Busts[i].statusVariable2;
			}else{
				Busts[i].yOffset=Busts[i].statusVariable4;
			}


			if ((Busts[i].xOffset == Busts[i].statusVariable3) && (Busts[i].yOffset==Busts[i].statusVariable4)){
				Busts[i].bustStatus = BUST_STATUS_NORMAL;
				printf("DONE!\n");
			}

		}
	}
}

int GetRezFromImage(CrossTexture* passedImage){
	int _width = GetTextureWidth(passedImage);
	int _height = GetTextureHeight(passedImage);
	if (_width==640 && _height==480){ // Old background, old bust, or updated bust
		return REZ_OLD;
	}else if (_width==960 && _height==540){ // PS3 background
		return REZ_PS3_BACKGROUND;
	}else if (_width==725 && _height==544){ // PS3 bust
		return REZ_PS3_BUST;
	}
	// Default
	return REZ_OLD;
}

void InBetweenLines(lua_State *L, lua_Debug *ar) {
	if (currentGameStatus==3){
		currentScriptLine++;
		if (isSkipping==1){
			ControlsStart();
			if (!IsDown(SCE_CTRL_SQUARE)){
				isSkipping=0;
			}
			ControlsEnd();
			if (isSkipping==1){
				endType=Line_ContinueAfterTyping;
			}
		}
		do{
			FpsCapStart();
			ControlsStart();
			Update();
			Draw();
			if (WasJustPressed(SCE_CTRL_CROSS)){
				MessageBoxEnabled=1;
				endType = Line_ContinueAfterTyping;
			}
			if (WasJustPressed(SCE_CTRL_CIRCLE)){
				if (MessageBoxEnabled==0){
					MessageBoxEnabled=1;
				}else{
					MessageBoxEnabled=0;
				}
			}
			if (WasJustPressed(SCE_CTRL_SQUARE)){
				isSkipping=1;
				endType=Line_ContinueAfterTyping;
			}
			if (WasJustPressed(SCE_CTRL_TRIANGLE)){
				printf("MENU\n");
			}
			ControlsEnd();
			FpsCapWait();
		}while(endType==Line_Normal || endType == Line_WaitForInput);
	}
}

void DrawBackground(CrossTexture* passedBackground, unsigned char passedRez){
	if (passedRez == REZ_OLD){
		DrawTexture(passedBackground,160,32);
	}else if (passedRez == REZ_PS3_BACKGROUND){
		DrawTexture(passedBackground,0,2);
	}else if (passedRez == REZ_UPDATED){
		printf("There are no updated background sprites in the original version. Perhaps you meant ps3?");
	}
}

void DrawBackgroundAlpha(CrossTexture* passedBackground, unsigned char passedRez, unsigned char passedAlpha){
	if (passedRez == REZ_OLD){
		DrawTextureAlpha(passedBackground,160,32,passedAlpha);
	}else if (passedRez == REZ_PS3_BACKGROUND){
		DrawTextureAlpha(passedBackground,0,2,passedAlpha);
	}else if (passedRez == REZ_UPDATED){
		printf("There are no updated background sprites in the original version. Perhaps you meant ps3?");
	}
}

void DrawBust(bust* passedBust){
	if (passedBust->alpha==255){
		if (passedBust->rez == REZ_PS3_BUST){
			DrawTexture(passedBust->image,141+passedBust->xOffset*1.13,passedBust->yOffset);
		}else if (passedBust->rez == REZ_OLD && currentBackgroundRez == REZ_PS3_BACKGROUND){ // In this case, the Steam busts should be bigger, but I'm too lazy to do that.
			DrawTexture(passedBust->image,141+passedBust->xOffset*1.13,passedBust->yOffset+64);
		}else if (passedBust->rez==REZ_OLD || passedBust->rez == REZ_UPDATED){
			DrawTexture(passedBust->image,160+passedBust->xOffset,passedBust->yOffset+32);
		}else if (passedBust->rez == REZ_PS3_BACKGROUND){
			DrawBackground(passedBust->image,passedBust->rez);
		}
	}else{
		if (passedBust->rez == REZ_PS3_BUST){
			DrawTextureAlpha(passedBust->image,141+passedBust->xOffset*1.13,passedBust->yOffset,passedBust->alpha);
		}else if (passedBust->rez == REZ_OLD && currentBackgroundRez == REZ_PS3_BACKGROUND){
			DrawTextureAlpha(passedBust->image,141+passedBust->xOffset*1.13,passedBust->yOffset+64,passedBust->alpha);
		}else if (passedBust->rez==REZ_OLD || passedBust->rez == REZ_UPDATED){
			DrawTextureAlpha(passedBust->image,160+passedBust->xOffset,passedBust->yOffset+32,passedBust->alpha);
		}else if (passedBust->rez == REZ_PS3_BACKGROUND){
			DrawBackgroundAlpha(passedBust->image,passedBust->rez,passedBust->alpha);
		}
	}
}

char CheckFileExist(char* location){
	#if PLATFORM == PLAT_VITA
		SceUID fileHandle = sceIoOpen(location, SCE_O_RDONLY, 0777);
		if (fileHandle < 0){
			return 0;
		}else{
			sceIoClose(fileHandle);
			return 1;
		}
	#elif PLATFORM == PLAT_WINDOWS
		if( access( location, F_OK ) != -1 ) {
			return 1;
		} else {
		    return 0;
		}
	#endif
}

void RecalculateBustOrder(){
	int i, j, k;

	for (i=0;i<MAXBUSTS;i++){
		bustOrder[i]=255;
		bustOrderOverBox[i]=255;
	}

	// This generates the orderOfAction list
	// i is the the current orderOfAction slot.
	for (i=0;i<MAXBUSTS;i++){
		// j is the current fighter we're testig
		for (j=0;j<MAXBUSTS;j++){
			// If current entity speed greater than
			if ((bustOrder[i]==255 || Busts[j].layer>Busts[bustOrder[i]].layer) && (Busts[j].layer<=31)){
				// Loops through all of orderOfAction to make sure you're not already in orderOfAction
				for (k=0;k<MAXBUSTS;k++){
					if (bustOrder[k]==j){
						break;
					}else{
						if (k==MAXBUSTS-1){
							bustOrder[i]=j;
						}
					}
				}
			}
		}
	}

	// Do another calculation for busts that have a layer greater than 31 and therefor are over the message box
	for (i=0;i<MAXBUSTS;i++){
		// j is the current fighter we're testig
		for (j=0;j<MAXBUSTS;j++){
			// If current entity speed greater than
			if ((bustOrderOverBox[i]==255 || Busts[j].layer>Busts[bustOrderOverBox[i]].layer) && (Busts[j].layer>31)){
				// Loops through all of orderOfAction to make sure you're not already in orderOfAction
				for (k=0;k<MAXBUSTS;k++){
					if (bustOrderOverBox[k]==j){
						break;
					}else{
						if (k==MAXBUSTS-1){
							bustOrderOverBox[i]=j;
						}
					}
				}
			}
		}
	}
}

void MoveFilePointerPastNewline(FILE* fp){
	unsigned char _temp;
	fread(&_temp,1,1,fp);
	if (_temp==13){
		fseek(fp,1,SEEK_CUR);
	}
}

// Checks if the byte is the one for a newline
// If it's 0D, it seeks past 0A
char IsNewLine(FILE* fp, unsigned char _temp){
	if (_temp==13){
		fseek(fp,1,SEEK_CUR);
		return 1;
	}
	if (_temp=='\n' || _temp==10){
		// It seems like the other newline char is skipped for me?
		return 1;
	}
}


// LAST ARG IS WHERE THE LENGTH IS STORED
unsigned char* ReadNumberStringList(FILE *fp, unsigned char* arraysize){
	int numScripts;
	char currentReadNumber[4];
	// Add null for atoi
	currentReadNumber[3]=0;

	fread(&currentReadNumber,3,1,fp);
	numScripts = atoi(currentReadNumber);
	MoveFilePointerPastNewline(fp);

	//fseek(fp,2,SEEK_CUR);

	
	unsigned char* _thelist;
	
	_thelist = (unsigned char*)calloc(numScripts,sizeof(char));

	int i=0;
	for (i=0;i<numScripts;i++){
		fread(&currentReadNumber,3,1,fp);
		_thelist[i]=atoi(currentReadNumber);



		MoveFilePointerPastNewline(fp);
	}

	(*arraysize) = numScripts;
	return _thelist;
}

char** ReadFileStringList(FILE *fp, unsigned char* arraysize){
	char currentReadLine[200];
	char currentReadNumber[4];
	// Add null for atoi
	currentReadNumber[3]=0;
	int linePosition=0;
	int numScripts;

	currentReadNumber[3]='\0';
	fread(&currentReadNumber,3,1,fp);
	numScripts = atoi(currentReadNumber);
	MoveFilePointerPastNewline(fp);
	

	char** _thelist;
	
	_thelist = (char**)calloc(numScripts,sizeof(char*));
	unsigned char justreadbyte=00;

	int i=0;
	for (i=0;i<numScripts;i++){
		while (currentGameStatus!=99){
			if (fread(&justreadbyte,1,1,fp)!=1){
				break;
			}
			// Newline char
			// By some black magic, even though newline is two bytes, I can still detect it?
			
			if (IsNewLine(fp,justreadbyte)==1){
				break;
			}
			currentReadLine[linePosition]=justreadbyte;
			linePosition++;
		}
		// Add null character
		currentReadLine[linePosition]='\0';
		//_thelist[i] = 

		_thelist[i] = (char*)calloc(1,linePosition+1);
		memcpy((_thelist[i]),currentReadLine,linePosition+1);
		
		linePosition=0;
	}

	(*arraysize) = numScripts;
	return _thelist;
}


void LoadPreset(char* filename){
	//currentPresetFileList
	FILE *fp;
	fp = fopen(filename, "r");
	//fprintf(fp,"There are %d music deocoders available\n", Mix_GetNumMusicDecoders());
	int i;
	
	currentPresetFileList.theArray = ReadFileStringList(fp,&currentPresetFileList.length);
	//for (i=0;i<currentPresetFileList.length;i++){
	//	printf("%s\n",currentPresetFileList.theArray[i]);
	//}
	currentPresentTipList.theArray = ReadFileStringList(fp,&currentPresentTipList.length);
	//for (i=0;i<currentPresentTipList.length;i++){
	//	printf("%s\n",currentPresentTipList.theArray[i]);
	//}
	//printf("Is %s\n",currentReadNumber);

	currentPresetTipUnlockList.theArray = ReadNumberStringList(fp,&(currentPresetTipUnlockList.length));
	//for (i=0;i<currentPresetTipUnlockList.length;i++){
	//	printf("%d\n",currentPresetTipUnlockList.theArray[i]);
	//}
	char tempreadstring[9];
	tempreadstring[8]='\0';

	// Check for the
	// tipnames
	// string
	// If it exists, read the tip's names
	if (fread(tempreadstring,8,1,fp)==1){
		if (strcmp(tempreadstring,"tipnames")==0){
			MoveFilePointerPastNewline(fp);
			currentPresentTipNameList.theArray = ReadFileStringList(fp,&currentPresentTipNameList.length);
			tipNamesLoaded=1;
		}else{
			tipNamesLoaded=0;
		}
	}else{
		tipNamesLoaded=0;
	}


	fclose(fp);
}

void SetNextScriptName(){
	memset((nextScriptToLoad),'\0',sizeof(nextScriptToLoad));
	strcpy(nextScriptToLoad,currentPresetFileList.theArray[currentPresetChapter]);
}

void LazyMessage(char* stra, char* strb, char* strc, char* strd){
	int _textheight = TextHeight(fontSize);
	ControlsEnd();
	while (currentGameStatus!=99){
		FpsCapStart();
		ControlsStart();
		if (WasJustPressed(SCE_CTRL_CROSS)){
			ControlsEnd();
			break;
		}
		ControlsEnd();
		StartDrawingA();
		if (stra!=NULL){
			DrawText(32,5+_textheight*(0+2),stra,fontSize);
		}
		if (strb!=NULL){
			DrawText(32,5+_textheight*(1+2),strb,fontSize);
		}
		if (strc!=NULL){
			DrawText(32,5+_textheight*(2+2),strc,fontSize);
		}
		if (strd!=NULL){
			DrawText(32,5+_textheight*(3+2),strd,fontSize);
		}
		DrawText(32,544-32-_textheight,"X to continue.",fontSize);
		EndDrawingA();
		FpsCapWait();
	}
}

void LoadGame(){
	strcpy(globalTempConcat,SAVEFOLDER);
	strcat(globalTempConcat,currentPresetFilename);
	currentPresetChapter=-1;
	if (CheckFileExist(globalTempConcat)==1){
		FILE *fp;
		fp = fopen(globalTempConcat, "r");
		fread(&currentPresetChapter,2,1,fp);
		fclose(fp);
	}
}

void SaveGame(){
	strcpy(globalTempConcat,SAVEFOLDER);
	strcat(globalTempConcat,currentPresetFilename);
	FILE *fp;
	fp = fopen(globalTempConcat, "w");
	fwrite(&currentPresetChapter,2,1,fp);
	fclose(fp);
}

//===================

void FadeBustshot(int passedSlot,int _time,char _wait){
	if (isSkipping==1){
		_time=0;
	}

	//int passedSlot = lua_tonumber(passedState,1)-1;
	//Busts[passedSlot].bustStatus = BUST_STATUS_FADEOUT;
	//Busts[passedSlot].statusVariable = 
	Busts[passedSlot].alpha=0;
	Busts[passedSlot].bustStatus = BUST_STATUS_FADEOUT;
	if (_time!=0){
		Busts[passedSlot].alpha=255;
		//int _time = floor(lua_tonumber(passedState,7));
		int _totalFrames = floor(60*(_time/(double)1000));
		if (_totalFrames==0){
			_totalFrames=1;
		}
		int _alphaPerFrame=floor(255/_totalFrames);
		if (_alphaPerFrame==0){
			_alphaPerFrame=1;
		}
		Busts[passedSlot].statusVariable=_alphaPerFrame;
		Busts[passedSlot].bustStatus = BUST_STATUS_FADEOUT;

		if (_wait==1){
			while (Busts[passedSlot].isActive==1){
				FpsCapStart();
				ControlsStart();
				Update();
				Draw();
				if (WasJustPressed(SCE_CTRL_CROSS)){
					Busts[passedSlot].alpha = 1;
				}
				ControlsEnd();
				FpsCapWait();
			}
		}
	}
}

void FadeAllBustshots(int _time, char _wait){
	if (isSkipping==1){
		_time=0;
	}

	int i=0;
	for (i=0;i<MAXBUSTS;i++){
		if (Busts[i].isActive==1){
			FadeBustshot(i,_time,0);
		}
	}
	if (_wait==1){
		char _isDone=0;
		while (_isDone==0){
			_isDone=1;
			for (i=0;i<MAXBUSTS;i++){
				if (Busts[i].isActive==1){
					if (Busts[i].alpha>0){
						_isDone=0;
						break;
					}
				}
			}
			FpsCapStart();
			ControlsStart();
			Update();
			Draw();
			if (WasJustPressed(SCE_CTRL_CROSS)){
				for (i=0;i<MAXBUSTS;i++){
					if (Busts[i].isActive==1){
						Busts[i].alpha=1;
					}
				}
			}
			ControlsEnd();
			FpsCapWait();
		}
	}
}

void DrawScene(const char* filename, int time){
	if (isSkipping==1){
		time=0;
	}


	int _alphaPerFrame=255;
	//FadeAllBustshots(time,0);
	int i=0;
	for (i=0;i<MAXBUSTS;i++){
		if (Busts[i].isActive==1 && Busts[i].lineCreatedOn != currentScriptLine-1){
			FadeBustshot(i,time,0);
		}
	}

	signed short _backgroundAlpha=255;

	if (time!=0){
		int _time = time;
		int _totalFrames = floor(60*(_time/(double)1000));
		if (_totalFrames==0){
			_totalFrames=1;
		}
		_alphaPerFrame=floor(255/_totalFrames);
		if (_alphaPerFrame==0){
			_alphaPerFrame=1;
		}
	}

	char* tempstringconcat = CombineStringsPLEASEFREE(STREAMINGASSETS, locationStrings[graphicsLocation],filename,".png");
	
	unsigned char newBackgroundRez;

	if (CheckFileExist(tempstringconcat)==0){
		free(tempstringconcat);
		if (graphicsLocation == LOCATION_CGALT){
			printf("Switching to cg\n");
			tempstringconcat = CombineStringsPLEASEFREE(STREAMINGASSETS, locationStrings[LOCATION_CG],filename,".png");
		}else if (graphicsLocation == LOCATION_CG){
			printf("Falling back on cgalt.\n");
			tempstringconcat = CombineStringsPLEASEFREE(STREAMINGASSETS, locationStrings[LOCATION_CGALT],filename,".png");
		}
	}

	CrossTexture* newBackground = LoadPNG(tempstringconcat);
	free(tempstringconcat);

	newBackgroundRez = GetRezFromImage(newBackground);

	while (_backgroundAlpha>0){
		FpsCapStart();

		Update();
		_backgroundAlpha-=_alphaPerFrame;
		if (_backgroundAlpha<0){
			_backgroundAlpha=0;
		}
		//int i;
		StartDrawingA();
		
		if (currentBackground!=NULL){
			DrawBackground(newBackground,newBackgroundRez);

			DrawBackgroundAlpha(currentBackground,currentBackgroundRez,_backgroundAlpha);
		}else{
			DrawBackgroundAlpha(newBackground,newBackgroundRez,255-_backgroundAlpha);
		}
		
		for (i = MAXBUSTS-1; i != -1; i--){
			if (Busts[bustOrder[i]].isActive==1){
				DrawBust(&(Busts[bustOrder[i]]));
			}
		}
		if (MessageBoxEnabled==1){
			DrawRectangle(0,0,960,544,0,0,0,MessageBoxAlpha);
		}
		for (i = MAXBUSTS-1; i != -1; i--){
			if (Busts[bustOrderOverBox[i]].isActive==1){
				DrawBust(&(Busts[bustOrderOverBox[i]]));
			}
		}
		if (MessageBoxEnabled==1){
			DrawMessageText();
		}
		EndDrawingA();

		ControlsStart();
		if (WasJustPressed(SCE_CTRL_CROSS)){
			for (i=0;i<MAXBUSTS;i++){
				if (Busts[i].isActive==1 && Busts[i].lineCreatedOn != currentScriptLine-1){
					Busts[i].alpha=1;
				}
			}
			_backgroundAlpha=1;
		}
		ControlsEnd();

		FpsCapWait();
	}

	if (currentBackground!=NULL){
		FreeTexture(currentBackground);
		currentBackground=NULL;
	}
	currentBackgroundRez = newBackgroundRez;
	currentBackground=newBackground;
}

void DrawBustshot(unsigned char passedSlot, const char* _filename, int _xoffset, int _yoffset, int _layer, int _fadeintime, int _waitforfadein, int _isinvisible){
	if (isSkipping==1){
		_fadeintime=0;
		_waitforfadein=0;
	}
	Draw();
	int i;
	unsigned char skippedInitialWait=0;
	//WaitWithCodeStart(_fadeintime);

	ResetBustStruct(&(Busts[passedSlot]),1);

	char* tempstringconcat = CombineStringsPLEASEFREE(STREAMINGASSETS, locationStrings[graphicsLocation],_filename,".png");


	if (CheckFileExist(tempstringconcat)==0){
		free(tempstringconcat);
		if (graphicsLocation == LOCATION_CGALT){
			printf("Switching to cg\n");
			tempstringconcat = CombineStringsPLEASEFREE(STREAMINGASSETS, locationStrings[LOCATION_CG],_filename,".png");
		}else{
			printf("Falling back on cgalt\n");
			tempstringconcat = CombineStringsPLEASEFREE(STREAMINGASSETS, locationStrings[LOCATION_CGALT],_filename,".png");
		}
	}


	Busts[passedSlot].image = LoadPNG(tempstringconcat);

	if (Busts[passedSlot].image==NULL){
		WriteToDebugFile("Failed to load");
		WriteToDebugFile(tempstringconcat);
		LazyMessage("failed to load",tempstringconcat,"WIll now crash!","bye!");
	}

	free(tempstringconcat);
	
	
	Busts[passedSlot].rez = GetRezFromImage(Busts[passedSlot].image);

	Busts[passedSlot].xOffset = _xoffset;
	Busts[passedSlot].yOffset = _yoffset;

	if (_isinvisible!=0){
		Busts[passedSlot].isInvisible=1;
	}else{
		Busts[passedSlot].isInvisible=0;
	}
	Busts[passedSlot].layer = _layer;
	Busts[passedSlot].lineCreatedOn = currentScriptLine;

	Busts[passedSlot].isActive=1;
	RecalculateBustOrder();
	if ((int)_fadeintime!=0){
		Busts[passedSlot].alpha=0;
		int _timeTotal = _fadeintime;
		int _time = floor(_fadeintime/2);
		int _totalFrames = floor(60*(_time/(double)1000));
		if (_totalFrames==0){
			_totalFrames=0;
		}
		int _alphaPerFrame=floor(255/_totalFrames);
		if (_alphaPerFrame==0){
			_alphaPerFrame=1;
		}
		Busts[passedSlot].statusVariable=_alphaPerFrame;
		Busts[passedSlot].statusVariable2 = _timeTotal -_time;
		Busts[passedSlot].bustStatus = BUST_STATUS_FADEIN;
	}

	i=1;
	if (_waitforfadein==1){
		while (Busts[passedSlot].alpha<255){
			FpsCapStart();
			ControlsStart();
			Update();
			if (Busts[passedSlot].alpha>255){
				Busts[passedSlot].alpha=255;
			}
			Draw();
			if (WasJustPressed(SCE_CTRL_CROSS) || skippedInitialWait==1){
				Busts[passedSlot].alpha = 255;
				Busts[passedSlot].bustStatus = BUST_STATUS_NORMAL;
				ControlsEnd();
				FpsCapWait();
				break;
			}
			ControlsEnd();
			i++;
			FpsCapWait();
		}
	}
}

void PlayMenuSound(){
	if (menuSoundLoaded==1){
		PlaySound(menuSound,1);
	}
}

/*
=================================================
*/

int L_ClearMessage(lua_State* passedState){
	//system("cls");
	currentLine=0;
	ClearMessageArray();
	return 0;
}

int L_OutputLine(lua_State* passedState){
	char waitingIsForShmucks=0;
	if (isSkipping==1){
		waitingIsForShmucks=1;
	}

	MessageBoxEnabled=1;
	int i,j;
	int currentChar = GetNextCharOnLine(currentLine);
	unsigned const char* message = (unsigned const char*)lua_tostring(passedState,4);
	endType = lua_tonumber(passedState,5);

	

	for (i = 0; i < u_strlen(message); i++){
		FpsCapStart();
		ControlsStart();
		//

		if (currentChar==60){
			currentLine++;
			currentChar = GetNextCharOnLine(currentLine);
		}
		// If it's a new word, add a newline if the word will be cut off
		if (message[i]==' '){
			for (j=1;j<61;j++){
				if (i+j>u_strlen(message)){
					if (currentChar+j>=60){
						currentLine++;
						currentChar = GetNextCharOnLine(currentLine);
					}
					break;
				}
				if (message[i+j]==' '){
					// Greater OR equal to 60 because I don't want to start a new line on a space
					if (currentChar+j>=60){
						currentLine++;
						currentChar = GetNextCharOnLine(currentLine);
						i++;
					}
					break;
				}
			}

			if (message[i]==' ' && currentChar==0){
				continue;
			}

		}

		if (message[i]==226 && message[i+1]==128 && message[i+2]==148){ // Don't write a wierd hyphen.
			i=i+2;
			memset(&(currentMessages[currentLine][currentChar]),45,1);
			currentChar++;
		}else if (message[i]==226 && message[i+1]==152 && message[i+2]==134){ // DISABLE STAR CHARACTER
			i=i+2;
			memset(&(currentMessages[currentLine][currentChar]),32,1);
			currentChar++;
		}else if (message[i]=='\n'){ // Interpret new line
			currentLine++;
			currentChar = GetNextCharOnLine(currentLine);
		}else{ // Normal letter
			memcpy(&(currentMessages[currentLine][currentChar]),&(message[i]),1);
			currentChar++;
		}

		//
		if (WasJustPressed(SCE_CTRL_CROSS)){
			waitingIsForShmucks=1;
			ControlsEnd();
		}
		//
		if (waitingIsForShmucks!=1){
			Draw();
			Update();
			ControlsEnd();
			FpsCapWait();
		}
		
	}
	return 0;
}

// Null, text, line type
int L_OutputLineAll(lua_State* passedState){
	MessageBoxEnabled=1;
	int i,j;
	int currentChar = GetNextCharOnLine(currentLine);
	unsigned const char* message = (unsigned const char*)lua_tostring(passedState,2);
	endType = lua_tonumber(passedState,5);

	char waitingIsForShmucks=1;

	for (i = 0; i < u_strlen(message); i++){
		FpsCapStart();
		ControlsStart();
		//

		if (currentChar==60){
			currentLine++;
			currentChar = GetNextCharOnLine(currentLine);
		}
		// If it's a new word, add a newline if the word will be cut off
		if (message[i]==' '){
			for (j=1;j<61;j++){
				if (i+j>u_strlen(message)){
					if (currentChar+j>=60){
						currentLine++;
						currentChar = GetNextCharOnLine(currentLine);
					}
					break;
				}
				if (message[i+j]==' '){
					// Greater OR equal to 60 because I don't want to start a new line on a space
					if (currentChar+j>=60){
						currentLine++;
						currentChar = GetNextCharOnLine(currentLine);
						i++;
					}
					break;
				}
			}
		}
		if (message[i]==226 && message[i+1]==128 && message[i+2]==148){ // Don't write a wierd hyphen.
			i=i+2;
			memset(&(currentMessages[currentLine][currentChar]),45,1);
			currentChar++;
		}else if (message[i]=='\n'){ // Interpret new line
			currentLine++;
			currentChar = GetNextCharOnLine(currentLine);
		}else{ // Normal letter
			memcpy(&(currentMessages[currentLine][currentChar]),&(message[i]),1);
			currentChar++;
		}

		//
		if (WasJustPressed(SCE_CTRL_CROSS)){
			waitingIsForShmucks=1;
			ControlsEnd();
		}
		//
		if (waitingIsForShmucks!=1){
			Draw();
			Update();
			ControlsEnd();
			FpsCapWait();
		}
		
	}
	return 0;
}

//
int L_Wait(lua_State* passedState){
	if (isSkipping!=1){
		Wait(lua_tonumber(passedState,1));
	}
	return 0;
}

// filename, filter, unknown, unknown, time
int L_DrawSceneWithMask(lua_State* passedState){
	DrawScene(lua_tostring(passedState,1),lua_tonumber(passedState,5));
	return 0;
}

// filename
// fadein
int L_DrawScene(lua_State* passedState){
	DrawScene(lua_tostring(passedState,1),lua_tonumber(passedState,2));
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
	#if SILENTMODE == 1
		return 0;
	#endif
	StopMusic();
	if (currentMusic!=NULL){
		FreeMusic(currentMusic);
		currentMusic=NULL;
	}
	
	char* tempstringconcat = CombineStringsPLEASEFREE(STREAMINGASSETS, "/BGM/", lua_tostring(passedState,2), ".ogg");
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

// Some int argument
// Maybe music slot
int L_StopBGM(lua_State* passedState){
	#if SILENTMODE == 1
		return 0;
	#endif
	StopMusic();
	if (currentMusic!=NULL){
		FreeMusic(currentMusic);
		currentMusic=NULL;
	}
	return 0;
}

// Last arg, I was right. Bool for if wait for fadeoutr
int L_FadeoutBGM(lua_State* passedState){
	// I BET THE SECOND ARGUMENT IS IF IT WAITS OR NOT
	Mix_FadeOutMusic(lua_tonumber(passedState,2));
	if (lua_toboolean(passedState,3)==1){
		Wait(lua_tonumber(passedState,2));
	}
	return 0;
}

// Bustshot slot? (Normally 1-3 used, 5 for black, 6 for cinema 7 for title)
	// Filename
	// Filter filename
	// ???
	// x offset (Character starts in the middle, screen is 640x480)
	// y offset (Character starts in the middle, screen is 640x480)
	// If the art starts in the middle of the screen and moves to its x and y offset
	// ???
	// ???
	// ???
	// ???
	// Some transparency argument? Nonzero makes the bust invisible
	// Layer. If one bust's layer is greater than another's, it's drawn above it. If it's the same layer number, more recent ones are on top
	// SPECIAL LAYERS -
		// Textbox's layer is 31. >31 layer shows a not darkened sprite.
	// Fadein time
	// (bool) wait for fadein? (15)
int L_DrawBustshotWithFiltering(lua_State* passedState){
	int i;
	for (i=8;i!=12;i++){
		if (lua_tonumber(passedState,i)!=0){
			printf("***********************IMPORTANT INFORMATION***************************\nAn argument I know nothing about was just used in DrawBustshotWithFiltering!\n***********************************************\n");
		}
	}

	//void DrawBustshot(unsigned char passedSlot, char* _filename, int _xoffset, int _yoffset, int _layer, int _fadeintime, int _waitforfadein, int _isinvisible){
	DrawBustshot(lua_tonumber(passedState,1)-1, lua_tostring(passedState,2), lua_tonumber(passedState,5), lua_tonumber(passedState,6), lua_tonumber(passedState,13), lua_tonumber(passedState,14), lua_toboolean(passedState,15), lua_tonumber(passedState,12));

	////SDL_SetTextureAlphaMod(Busts[passedSlot].image, 255);
	return 0;
}

// Butshot slot
	// Filename
	// x offset (Character starts in the middle, screen is 640x480)
	// y offset (Character starts in the middle, screen is 640x480)
	// Some sort of scaling thing. 400 makes it invisible, 200 is half size. (scale applies to width and height) 1 is normal
	// If the art starts in the middle of the screen and moves to its x and y offset
	// ???
	// ???
	// ???
	// ???
	// ???
	// ???
	// Some transparency argument? Nonzero makes the bust invisible
	// Layer. If one bust's layer is greater than another's, it's drawn above it. If it's the same layer number, more recent ones are on top
	// 	// SPECIAL LAYERS -
	// 	// Textbox's layer is 31. >31 layer shows a not darkened sprite.
	// Fadein time
	// (bool) wait for fadein? (16)
int L_DrawBustshot(lua_State* passedState){
	Draw();

	int i;
	for (i=8;i!=12;i++){
		if (lua_tonumber(passedState,i)!=0){
			printf("***********************IMPORTANT INFORMATION***************************\nAn argument I know nothing about was just used in DrawBustshotWithFiltering!\n***********************************************\n");
		}
	}
	
	//void DrawBustshot(unsigned char passedSlot, char* _filename, int _xoffset, int _yoffset, int _layer, int _fadeintime, int _waitforfadein, int _isinvisible){
	DrawBustshot(lua_tonumber(passedState,1)-1, lua_tostring(passedState,2), lua_tonumber(passedState,3), lua_tonumber(passedState,4), lua_tonumber(passedState,14), lua_tonumber(passedState,15), lua_toboolean(passedState,16), lua_tonumber(passedState,13));
	return 0;
}

int L_SetValidityOfInput(lua_State* passedState){
	if (lua_toboolean(passedState,1)==1){
		InputValidity=1;
	}else{
		InputValidity=0;
	}
	return 0;
}

// Fadeout time
// Wait for completely fadeout
int L_FadeAllBustshots(lua_State* passedState){
	FadeAllBustshots(lua_tonumber(passedState,1),lua_toboolean(passedState,2));
	//int i;
	//for (i=0;i<MAXBUSTS;i++){
	//	if (Busts[i].isActive==1){
	//		FreeTexture(Busts[i].image);
	//		ResetBustStruct(&(Busts[i]));
	//	}
	//}
	return 0;
}

int L_DisableWindow(lua_State* passedState){
	MessageBoxEnabled=0;
	return 0;
}

int L_FadeBustshotWithFiltering(lua_State* passedState){
	FadeBustshot(lua_tonumber(passedState,1)-1,lua_tonumber(passedState,7),lua_toboolean(passedState,8));
	return 0;
}

//FadeBustshot( 2, FALSE, 0, 0, 0, 0, 0, TRUE );
//FadeBustshot( SLOT, MOVE, X, Y, UNKNOWNA, UNKNOWNB, FADETIME, WAIT );
int L_FadeBustshot(lua_State* passedState){
	FadeBustshot(lua_tonumber(passedState,1)-1,lua_tonumber(passedState,7),lua_toboolean(passedState,8));
	return 0;
}

// Slot, file, volume
int L_PlaySE(lua_State* passedState){
	int passedSlot = lua_tonumber(passedState,1);
	#if SILENTMODE != 1
		if (soundEffects[passedSlot]!=NULL){
			FreeSound(soundEffects[passedSlot]);
			soundEffects[passedSlot]=NULL;
		}
		char* tempstringconcat = CombineStringsPLEASEFREE(STREAMINGASSETS, "/SE/",lua_tostring(passedState,2),".ogg");
		soundEffects[passedSlot] = LoadSound(tempstringconcat);
		Mix_VolumeChunk(soundEffects[passedSlot],lua_tonumber(passedState,3));
		free(tempstringconcat);
		PlaySound(soundEffects[passedSlot],1);
	#endif
	return 0;
}

int L_CallScript(lua_State* passedState){
	const char* filename = lua_tostring(passedState,1);

	char* tempstringconcat = CombineStringsPLEASEFREE(SCRIPTFOLDER, "",filename,".txt");
	char tempstring2[strlen(tempstringconcat)+1];
	strcpy(tempstring2,tempstringconcat);
	free(tempstringconcat);

	if (CheckFileExist(tempstring2)==1){
		printf("Do script %s\n",tempstring2);
		RunScript("",tempstring2,0);
	}else{
		printf("Failed to find script\n");
	}
	return 0;
}

int L_GetGlobalFlag(lua_State* passedState){
	// GLanguage
	// This is some thingie for the script to check the current language
	if (lua_tonumber(passedState,1)==1){
		// English
		lua_pushnumber(passedState,1);
		// Japanese
		//lua_pushnumber(passedState,0);
		return 1;
	}
	printf("Unknown GetGlobalFlag.\n");
	return 0;
}

// "bg_166", 7, 200, 0
int L_ChangeScene(lua_State* passedState){
	DrawScene(lua_tostring(passedState,1),0);
	return 0;
}

// DrawSprite(slot, filename, ?, x, y, ?, ?, ?, ?, ?, ?, ?, ?, LAYER, FADEINTIME, WAITFORFADEIN)
// x is relative to 320
	// y is relative to 240???
	// DrawSprite(slot, filename, ?, x, y, ?, ?, ?, ?, ?, ?, ?, ?, LAYER, FADEINTIME, WAITFORFADEIN)
int L_DrawSprite(lua_State* passedState){
	//void DrawBustshot(unsigned char passedSlot, char* _filename, int _xoffset, int _yoffset, int _layer, int _fadeintime, int _waitforfadein, int _isinvisible){
	DrawBustshot(lua_tonumber(passedState,1)-1,lua_tostring(passedState,2),320+lua_tonumber(passedState,4),240+lua_tonumber(passedState,5),lua_tonumber(passedState,14), lua_tonumber(passedState,15),lua_toboolean(passedState,16),0);
	//DrawBustshot(lua_tonumber(passedState,1)-1, lua_tostring(passedState,2), lua_tonumber(passedState,3), lua_tonumber(passedState,4), lua_tonumber(passedState,14), lua_tonumber(passedState,15), lua_toboolean(passedState,16), lua_tonumber(passedState,13));
}

//MoveSprite(slot, destinationx, destinationy, ?, ?, ?, ?, ?, timeittakes, waitforcompletion)
	// MoveSprite(5,-320,-4500,0,0,0,0,0,101400, TRUE)
int L_MoveSprite(lua_State* passedState){
	int _totalTime = lua_tonumber(passedState,9);
	int _passedSlot = lua_tonumber(passedState,1);
	_passedSlot--;
	// Number of x pixles the sprite has to move by the end

	printf("arg2:%d\n",(int)lua_tonumber(passedState,2));
	printf("x:%d\n",Busts[_passedSlot].xOffset);

	int _xTengoQue = lua_tonumber(passedState,2)-(Busts[_passedSlot].xOffset-320);
	int _yTengoQue = lua_tonumber(passedState,3)-(Busts[_passedSlot].yOffset-240);
	char _waitforcompletion = lua_toboolean(passedState,10);

	Busts[_passedSlot].statusVariable3 = lua_tonumber(passedState,2)+320;
	Busts[_passedSlot].statusVariable4 = lua_tonumber(passedState,3)+240;

	if (_totalTime!=0){
		unsigned int _totalFrames = floor(60*(_totalTime/(double)1000));
		int _xPerFrame = floor(_xTengoQue/_totalFrames);
		if (_xPerFrame==0){
			_xPerFrame=1;
		}
		printf("xtq: %d\n",_xTengoQue);
		printf("xprf: %d\n",_xPerFrame);
		printf("ytq: %d\n",_yTengoQue);
		printf("tf: %d\n",_totalFrames);
		int _yPerFrame = floor(_yTengoQue/(double)_totalFrames);
		if (_yPerFrame==0){
			_yPerFrame=1;
		}

		Busts[_passedSlot].statusVariable = _xPerFrame;
		printf("yperfraeme: %d\n",_yPerFrame);
		Busts[_passedSlot].statusVariable2 = _yPerFrame;
		Busts[_passedSlot].bustStatus = BUST_STATUS_SPRITE_MOVE;

	}else{
		Busts[_passedSlot].xOffset=lua_tonumber(passedState,2)+320;
		Busts[_passedSlot].yOffset=lua_tonumber(passedState,3)+240;
	}

	if (_waitforcompletion==1){
		while(Busts[_passedSlot].bustStatus!=BUST_STATUS_NORMAL){
			FpsCapStart();
			ControlsStart();
			if (WasJustPressed(SCE_CTRL_CROSS)){
				Busts[_passedSlot].xOffset=lua_tonumber(passedState,2)+320;
				Busts[_passedSlot].yOffset=lua_tonumber(passedState,3)+240;
			}
			ControlsEnd();
			Update();
			Draw();
			FpsCapWait();
		}
	}
}

//FadeSprite(slot, time, waitfrocompletion)
		// FadeSprite(5,700,FALSE)
int L_FadeSprite(lua_State* passedState){
	FadeBustshot(lua_tonumber(passedState,1)-1,lua_tonumber(passedState,2),lua_toboolean(passedState,3));	
}

void MakeLuaUseful(){
	LUAREGISTER(L_OutputLine,"OutputLine")
	LUAREGISTER(L_ClearMessage,"ClearMessage")
	LUAREGISTER(L_OutputLineAll,"OutputLineAll")
	LUAREGISTER(L_Wait,"Wait")
	LUAREGISTER(L_DrawScene,"DrawScene")
	LUAREGISTER(L_DrawSceneWithMask,"DrawSceneWithMask")
	LUAREGISTER(L_PlayBGM,"PlayBGM")
	LUAREGISTER(L_FadeoutBGM,"FadeOutBGM")
	LUAREGISTER(L_DrawBustshotWithFiltering,"DrawBustshotWithFiltering");
	LUAREGISTER(L_SetValidityOfInput,"SetValidityOfInput")
	LUAREGISTER(L_FadeAllBustshots,"FadeAllBustshots")
	LUAREGISTER(L_DisableWindow,"DisableWindow")
	LUAREGISTER(L_DrawBustshot,"DrawBustshot")
	LUAREGISTER(L_FadeBustshotWithFiltering,"FadeBustshotWithFiltering")
	LUAREGISTER(L_FadeBustshot,"FadeBustshot")
	LUAREGISTER(L_DrawScene,"DrawBG")
	LUAREGISTER(L_PlaySE,"PlaySE")
	LUAREGISTER(L_StopBGM,"StopBGM")
	LUAREGISTER(L_CallScript,"CallScript")
	LUAREGISTER(L_GetGlobalFlag,"GetGlobalFlag")
	LUAREGISTER(L_ChangeScene,"ChangeScene")
	LUAREGISTER(L_DrawSprite,"DrawSprite")
	LUAREGISTER(L_MoveSprite,"MoveSprite")
	LUAREGISTER(L_FadeSprite,"FadeSprite")

	// Functions that do nothing
	LUAREGISTER(L_NotYet,"SetFontId")
	LUAREGISTER(L_NotYet,"SetCharSpacing")
	LUAREGISTER(L_NotYet,"SetLineSpacing")
	LUAREGISTER(L_NotYet,"SetFontSize")
	LUAREGISTER(L_NotYet,"SetNameFormat")
	LUAREGISTER(L_NotYet,"SetScreenAspect")
	LUAREGISTER(L_NotYet,"SetWindowPos")
	LUAREGISTER(L_NotYet,"SetWindowSize")
	LUAREGISTER(L_NotYet,"SetWindowMargins")
	LUAREGISTER(L_NotYet,"SetGUIPosition")
	
	LUAREGISTER(L_NotYet,"FadeFilm") // Used for fixing negative???
	LUAREGISTER(L_NotYet,"Negative") // Command for color inversion
									// Negative( 1000, TRUE ); is inveted
									// FadeFilm( 200, TRUE ); fixes it??!
									// Name provably means to negate the colors, or replace the colors with their complementary ones on the other side of the color wheel
									// First arg is maybe time when it fades to inverted and argument is proably if it's inverted


	//  TEMP
	LUAREGISTER(L_NotYet,"SetSpeedOfMessage")
	LUAREGISTER(L_NotYet,"ShakeScreen")
	LUAREGISTER(L_NotYet,"ShakeScreenSx")
	LUAREGISTER(L_NotYet,"SetDrawingPointOfMessage")
	LUAREGISTER(L_NotYet,"SetStyleOfMessageSwinging")
	LUAREGISTER(L_NotYet,"SetValidityOfWindowDisablingWhenGraphicsControl")
	LUAREGISTER(L_NotYet,"DrawFilm")
	LUAREGISTER(L_NotYet,"EnableJumpingOfReturnIcon")
	LUAREGISTER(L_NotYet,"StopSE")
	LUAREGISTER(L_NotYet,"FadeBG")
	LUAREGISTER(L_NotYet,"SetValidityOfTextFade")
	LUAREGISTER(L_NotYet,"SetValidityOfSaving")
	LUAREGISTER(L_NotYet,"SetValidityOfSkipping")
	LUAREGISTER(L_NotYet,"GetAchievement")
	LUAREGISTER(L_NotYet,"CallSection")
	LUAREGISTER(L_NotYet,"SetFontOfMessage")
	LUAREGISTER(L_NotYet,"SetValidityOfLoading")
	LUAREGISTER(L_NotYet,"ActivateScreenEffectForcedly")
	LUAREGISTER(L_NotYet,"SetValidityOfUserEffectSpeed")
}

//======================================================
void Draw(){
	int i;
	//DrawTexture(testtex,32,32);
	StartDrawingA();

	if (currentBackground!=NULL){
		DrawBackground(currentBackground,currentBackgroundRez);
	}

	for (i = MAXBUSTS-1; i != -1; i--){
		if (Busts[bustOrder[i]].isActive==1){
			DrawBust(&(Busts[bustOrder[i]]));
		}
	}

	if (MessageBoxEnabled==1){
		// This is the message box
		DrawRectangle(0,0,960,544,0,0,0,MessageBoxAlpha);
	}

	for (i = MAXBUSTS-1; i != -1; i--){
		if (Busts[bustOrderOverBox[i]].isActive==1){
			DrawBust(&(Busts[bustOrderOverBox[i]]));
		}
	}

	if (MessageBoxEnabled==1){
		DrawMessageText();
	}
	EndDrawingA();
}

void LuaThread(char* _torun){
	RunScript(SCRIPTFOLDER, _torun,1);
}

void FileSelector(char* directorylocation, char** _chosenfile, char* promptMessage){
	int i=0;
	int totalFiles=0;

	// Can hold 50 filenames
	// Each with no more than 50 characters (51 char block of memory. extra one for null char)
	char** filenameholder;
	filenameholder = calloc(50,sizeof(char*));
	for (i=0;i<50;i++){
		filenameholder[i]=calloc(1,51);
	}

	CROSSDIR dir;
	CROSSDIRSTORAGE lastStorage;
	dir = OpenDirectory (directorylocation);
	for (i=0;i<50;i++){
		if (DirectoryRead(&dir,&lastStorage) == 0){
			break;
		}
		memcpy((filenameholder[i]),GetDirectoryResultName(&lastStorage),strlen(GetDirectoryResultName(&lastStorage))+1);
	}
	DirectoryClose (dir);
	
	totalFiles = i;
	printf("%d files in total\n",totalFiles);


	if (totalFiles==0){
		StartDrawingA();
		//DrawText(20,20+TextHeight(fontSize)+i*(TextHeight(fontSize)),currentMessages[i],fontSize);
		DrawText(32,5,"No presets found.",fontSize);
		DrawText(32,TextHeight(fontSize)+5,"If you ran the converter, you should've gotten some.",fontSize);
		DrawText(32,TextHeight(fontSize)*2+10,"You can manually put presets in:",fontSize);
		DrawText(32,TextHeight(fontSize)*3+15,"ux0:data/HIGURASHI/StreamingAssets/Presets/",fontSize);
		DrawText(32,200,"Press X to return",fontSize);
		EndDrawingA();
		
		while (currentGameStatus!=99){
			FpsCapStart();
			ControlsStart();
			if (WasJustPressed(SCE_CTRL_CROSS)){
				ControlsEnd();
				break;
			}
			ControlsEnd();
			FpsCapWait();
		}

		*_chosenfile=NULL;
		
	}else{
		int _textheight = TextHeight(fontSize);
		int _choice=0;
		int _maxPerNoScroll=floor((544-5-_textheight*2)/(_textheight));
		if (totalFiles<_maxPerNoScroll){
			_maxPerNoScroll=totalFiles;
		}
		int _tmpoffset=0;
		while (currentGameStatus!=99){
			FpsCapStart();
			ControlsStart();
	
			if (WasJustPressed(SCE_CTRL_UP)){
				_choice--;
				if (_choice<0){
					_choice=totalFiles-1;
				}
			}
			if (WasJustPressed(SCE_CTRL_DOWN)){
				_choice++;
				if (_choice>=totalFiles){
					_choice=0;
				}
			}
			if (WasJustPressed(SCE_CTRL_RIGHT)){
				_choice+=5;
				if (_choice>=totalFiles){
					_choice=totalFiles-1;
				}
			}
			if (WasJustPressed(SCE_CTRL_LEFT)){
				_choice-=5;
				if (_choice<0){
					_choice=0;
				}
			}
			if (WasJustPressed(SCE_CTRL_CROSS)){
				(*_chosenfile) = calloc(1,strlen(filenameholder[_choice])+1);
				memcpy(*_chosenfile,filenameholder[_choice],strlen(filenameholder[_choice])+1);
				break;		
			}
			if (WasJustPressed(SCE_CTRL_CIRCLE)){
				(*_chosenfile) = NULL;
				break;		
			}
	
			StartDrawingA();
			//DrawText(20,20+TextHeight(fontSize)+i*(TextHeight(fontSize)),currentMessages[i],fontSize);
			if (promptMessage!=NULL){
				DrawText(32,5,promptMessage,fontSize);
			}
			_tmpoffset=_choice+1-_maxPerNoScroll;
			if (_tmpoffset<0){
				_tmpoffset=0;
			}
			for (i=0;i<_maxPerNoScroll;i++){
				DrawText(32,5+_textheight*(i+2),filenameholder[i+_tmpoffset],fontSize);
			}
			DrawTextColored(32,5+_textheight*((_choice-_tmpoffset)+2),filenameholder[_choice],fontSize,0,255,0);
			DrawText(5,5+_textheight*((_choice-_tmpoffset)+2),">",fontSize);
			EndDrawingA();
			ControlsEnd();
			FpsCapWait();
		}
	}

	// Free memori
	for (i=0;i<50;i++){
		free(filenameholder[i]);
	}
	free(filenameholder);
}

void TitleScreen(){
	signed char _choice=0;
	int _textheight = TextHeight(fontSize);
	char _canShowRena=0;
	int _bustlocationcollinspacewidth = TextWidth(fontSize,"Bust location: ");

	// This checks if we have Rena busts in CG AND CGAlt
	char* _temppath = CombineStringsPLEASEFREE(STREAMINGASSETS,"CG/","re_se_de_a1.png","");
	if (CheckFileExist(_temppath)==1){
		free(_temppath);
		_temppath = CombineStringsPLEASEFREE(STREAMINGASSETS,"CGAlt/","re_se_de_a1.png","");
		if (CheckFileExist(_temppath)==1){
			_canShowRena=1;
		}
	}
	free(_temppath);

	CrossTexture* _renaImage=NULL;
	if (_canShowRena==1){
		_temppath = CombineStringsPLEASEFREE(STREAMINGASSETS,locationStrings[graphicsLocation],"re_se_de_a1.png","");
		_renaImage = LoadPNG(_temppath);
		free(_temppath);
	}


	//SetClearColor(255,255,255,255);
	while (currentGameStatus!=99){
		FpsCapStart();
		ControlsStart();

		if (WasJustPressed(SCE_CTRL_DOWN)){
			_choice++;
		}
		if (WasJustPressed(SCE_CTRL_UP)){
			_choice--;
		}
		// Left and right to change bust location
		if (WasJustPressed(SCE_CTRL_RIGHT) || WasJustPressed(SCE_CTRL_LEFT) || (_choice==2 && WasJustPressed(SCE_CTRL_CROSS))){
			if (_choice==2){
				if (graphicsLocation == LOCATION_CG){
					graphicsLocation = LOCATION_CGALT;
				}else if (graphicsLocation == LOCATION_CGALT){
					graphicsLocation = LOCATION_CG;
				}
				if (_canShowRena==1){
					FreeTexture(_renaImage);
					_temppath = CombineStringsPLEASEFREE(STREAMINGASSETS,locationStrings[graphicsLocation],"re_se_de_a1.png","");
					_renaImage = LoadPNG(_temppath);
					free(_temppath);
				}
			}
		}else{
			if (WasJustPressed(SCE_CTRL_CROSS)){
				PlayMenuSound();
				if (_choice==0){
					if (currentPresetFilename==NULL){
						currentPresetChapter=0;
						ControlsEnd();
						currentGameStatus=2;
					}
					break;
				}else if (_choice==1){
					printf("Manual script selection\n");
					ControlsEnd();
					FileSelector(SCRIPTFOLDER,&currentPresetFilename,"Select a script");
					if (currentPresetFilename!=NULL){
						currentGameStatus=3;
						RunScript(SCRIPTFOLDER,currentPresetFilename,0);
						currentGameStatus=0;
					}
				}else if (_choice==3){
					currentGameStatus=99;
					break;
				}else{
					_choice=0;
				}
			}
		}

		StartDrawingA();
		// Display sample Rena if changing bust location
		if (_choice==2){
			if (_canShowRena==1){
				DrawTexture(_renaImage,480,64);
			}
		}
		DrawText(32,5,"Main Menu",fontSize);

		DrawText(32,5+_textheight*(0+2),"Load preset and savefile",fontSize);
		DrawText(32,5+_textheight*(1+2),"Manual [Not recommended]",fontSize);
		DrawText(32,5+_textheight*(2+2),"Bust location: ",fontSize);
			if (graphicsLocation == LOCATION_CGALT){
				DrawText(32+_bustlocationcollinspacewidth,5+_textheight*(2+2),"CGAlt",fontSize);
			}else if (graphicsLocation==LOCATION_CG){
				DrawText(32+_bustlocationcollinspacewidth,5+_textheight*(2+2),"CG",fontSize);
			}
		DrawText(32,5+_textheight*(3+2),"Exit",fontSize);

		DrawText(5,5+_textheight*(_choice+2),">",fontSize);
		EndDrawingA();
		ControlsEnd();
		FpsCapWait();
	}

	if (_canShowRena==1){
		FreeTexture(_renaImage);
	}
}

void TipMenu(){
	if (currentPresetTipUnlockList.theArray[currentPresetChapter]==0){
		LazyMessage("No tips unlocked.",NULL,NULL,NULL);
		currentGameStatus=4;
		ControlsEnd();
		return;
	}
	signed char _choice=0;
	int _textheight = TextHeight(fontSize);
	int _tipcollinwidth = TextWidth(fontSize,"Tip: ");
	// The number for the tip the user has selected. Starts at 1. Subtract 1 if using this for an array
	unsigned char _chosenTip=1;
	char _chosenTipString[4]={48,0,0,0};
	char _chosenTipStringMax[4]={48,0,0,0};
	char _totalSelectedString[256]={0};
	itoa(currentPresetTipUnlockList.theArray[currentPresetChapter],&(_chosenTipStringMax[0]),10);
	
	if (tipNamesLoaded==1){
		strcpy(_totalSelectedString,currentPresentTipNameList.theArray[0]);
		strcat(_totalSelectedString," (1/");
		strcat(_totalSelectedString,_chosenTipStringMax);
		strcat(_totalSelectedString,")");
	}else{
		strcpy(_totalSelectedString,"1/");
		strcat(_totalSelectedString,_chosenTipStringMax);
	}

	while (currentGameStatus!=99){
		FpsCapStart();
		ControlsStart();
		if (WasJustPressed(SCE_CTRL_DOWN)){
			_choice++;
			if (_choice>1){
				_choice=0;
			}
		}
		if (WasJustPressed(SCE_CTRL_UP)){
			_choice--;
			if (_choice<0){
				_choice=1;
			}
		}
		if (WasJustPressed(SCE_CTRL_RIGHT)){
			_chosenTip++;
			if (_chosenTip>currentPresetTipUnlockList.theArray[currentPresetChapter]){
				_chosenTip=1;
			}
			// Update the string that is used to show what tip is selected
			if (tipNamesLoaded==1){
				itoa(_chosenTip,&(_chosenTipString[0]),10);
				strcpy(_totalSelectedString,currentPresentTipNameList.theArray[_chosenTip-1]);
				strcat(_totalSelectedString," (");
				strcat(_totalSelectedString,_chosenTipString);
				strcat(_totalSelectedString,"/");
				strcat(_totalSelectedString,_chosenTipStringMax);
				strcat(_totalSelectedString,")");
			}else{
				itoa(_chosenTip,&(_chosenTipString[0]),10);
				strcpy(_totalSelectedString,_chosenTipString);
				strcat(_totalSelectedString,"/");
				strcat(_totalSelectedString,_chosenTipStringMax);
			}
		}
		if (WasJustPressed(SCE_CTRL_LEFT)){
			_chosenTip--;
			if (_chosenTip<1){
				_chosenTip=currentPresetTipUnlockList.theArray[currentPresetChapter];
			}
			// Update the string that is used to show what tip is selected
			if (tipNamesLoaded==1){
				itoa(_chosenTip,&(_chosenTipString[0]),10);
				strcpy(_totalSelectedString,currentPresentTipNameList.theArray[_chosenTip-1]);
				strcat(_totalSelectedString," (");
				strcat(_totalSelectedString,_chosenTipString);
				strcat(_totalSelectedString,"/");
				strcat(_totalSelectedString,_chosenTipStringMax);
				strcat(_totalSelectedString,")");
			}else{
				itoa(_chosenTip,&(_chosenTipString[0]),10);
				strcpy(_totalSelectedString,_chosenTipString);
				strcat(_totalSelectedString,"/");
				strcat(_totalSelectedString,_chosenTipStringMax);
			}
		}
		if (WasJustPressed(SCE_CTRL_CROSS)){
			if (_choice==0){
				ControlsEnd();
				// This will trick the in between lines functions into thinking that we're in normal script execution mode and not quit
				currentGameStatus=3;
				RunScript(SCRIPTFOLDER, currentPresentTipList.theArray[_chosenTip-1],1);
				ControlsEnd();
				currentGameStatus=5;
				break;
			}else if (_choice==1){
				printf("Going back\n");
				currentGameStatus=4;
				ControlsEnd();
				break;
			}else{
				printf("INVALID SELECTION\n");
			}
		}
		ControlsEnd();
		StartDrawingA();
		DrawText(32,5+_textheight*(0+2),"Tip: ",fontSize);
		DrawText(32+_tipcollinwidth,5+_textheight*(0+2),_totalSelectedString,fontSize);
		DrawText(32,5+_textheight*(1+2),"Back",fontSize);
		DrawText(5,5+_textheight*(_choice+2),">",fontSize);


		DrawText(5,544-5-_textheight*2,"Left and Right - Change TIP",fontSize);
		DrawText(5,544-5-_textheight,"X - Select",fontSize);

		EndDrawingA();
		FpsCapWait();
	}
}

void ChapterJump(){
	//currentGameStatus=3;
	//RunScript
	//currentGameStatus=4;
	int _chapterChoice=0;
	unsigned char _choice=0;
	int _textheight = TextHeight(fontSize);
	char _tempNumberString[15];
	ControlsEnd();

	itoa(_chapterChoice,&(_tempNumberString[0]),10);
	strcpy(globalTempConcat,currentPresetFileList.theArray[_chapterChoice]);
	strcat(globalTempConcat," (");
	strcat(globalTempConcat,_tempNumberString);
	strcat(globalTempConcat,")");

	while (currentGameStatus!=99){
		FpsCapStart();
		ControlsStart();
		if (WasJustPressed(SCE_CTRL_RIGHT)){
			if (!IsDown(SCE_CTRL_R1)){
				_chapterChoice++;
			}else{
				_chapterChoice+=5;
			}
			if (_chapterChoice>currentPresetChapter){
				_chapterChoice=0;
			}

			itoa(_chapterChoice,&(_tempNumberString[0]),10);
			strcpy(globalTempConcat,currentPresetFileList.theArray[_chapterChoice]);
			strcat(globalTempConcat," (");
			strcat(globalTempConcat,_tempNumberString);
			strcat(globalTempConcat,")");
		}
		if (WasJustPressed(SCE_CTRL_LEFT)){
			if (!IsDown(SCE_CTRL_R1)){
				_chapterChoice--;
			}else{
				_chapterChoice-=5;
			}
			if (_chapterChoice<0){
				_chapterChoice=currentPresetChapter;
			}
			itoa(_chapterChoice,&(_tempNumberString[0]),10);
			strcpy(globalTempConcat,currentPresetFileList.theArray[_chapterChoice]);
			strcat(globalTempConcat," (");
			strcat(globalTempConcat,_tempNumberString);
			strcat(globalTempConcat,")");
		}
		if (WasJustPressed(SCE_CTRL_DOWN)){
			_choice++;
			if (_choice>1){
				_choice=0;
			}
		}
		if (WasJustPressed(SCE_CTRL_UP)){
			_choice--;
			if (_choice>=240){
				_choice=1;
			}
		}
		if (WasJustPressed(SCE_CTRL_CROSS)){
			if (_choice==0){
				ControlsEnd();
				currentGameStatus=3;
				RunScript(SCRIPTFOLDER, currentPresetFileList.theArray[_chapterChoice],1);
				ControlsEnd();
				currentGameStatus=5;
				break;
			}
			if (_choice==1){
				break;
			}
		}
		ControlsEnd();
		StartDrawingA();
		
		DrawText(32,5+_textheight*(0+2),globalTempConcat,fontSize);
		
		DrawText(32,5+_textheight*(1+2),"Back",fontSize);
		DrawText(5,5+_textheight*(_choice+2),">",fontSize);

		DrawText(5,544-5-_textheight*3,"Left and Right - Change chapter",fontSize);
		DrawText(5,544-5-_textheight*2,"R and Left or Right - Change chapter quickly",fontSize);
		DrawText(5,544-5-_textheight,"X - Select",fontSize);
		EndDrawingA();
		FpsCapWait();
	}
}

void NavigationMenu(){
	signed char _choice=0;
	int _textheight = TextHeight(fontSize);
	int _endofscriptwidth = TextWidth(fontSize,"End of script: ");
	char _endOfChapterString[10];
	itoa(currentPresetChapter,_endOfChapterString,10);
	while (currentGameStatus!=99){
		FpsCapStart();
		ControlsStart();
		if (WasJustPressed(SCE_CTRL_DOWN)){
			_choice++;
			if (_choice>3){
				_choice=0;
			}
		}
		if (WasJustPressed(SCE_CTRL_UP)){
			_choice--;
			if (_choice<0){
				_choice=3;
			}
		}
		if (WasJustPressed(SCE_CTRL_CROSS)){
			if (_choice==0){
				printf("Go to next chapter\n");
				currentPresetChapter++;
				WriteToDebugFile("Before set");
				WriteIntToDebugFile(currentPresetChapter);
				SetNextScriptName();
				WriteToDebugFile("Before statischange");
				WriteIntToDebugFile(currentPresetChapter);
				currentGameStatus=3;
				
				break;
			}else if (_choice==1){
				ChapterJump();
			}else if (_choice==2){
				printf("Viewing tips\n");
				currentGameStatus=5;
				ControlsEnd();
				break;
			}else if (_choice==3){
				printf("Exiting\n");
				currentGameStatus=99;
				break;
			}else{
				printf("INVALID SELECTION\n");
			}
		}
		ControlsEnd();
		StartDrawingA();

		DrawText(32,0,"End of script: ",fontSize);
		DrawText(_endofscriptwidth+32,0,_endOfChapterString,fontSize);

		DrawText(32,5+_textheight*(0+2),"Next",fontSize);
		DrawText(32,5+_textheight*(1+2),"Chapter Jump",fontSize);
		DrawText(32,5+_textheight*(2+2),"View Tips",fontSize);
		DrawText(32,5+_textheight*(3+2),"Exit",fontSize);
		DrawText(5,5+_textheight*(_choice+2),">",fontSize);
		EndDrawingA();
		FpsCapWait();
	}
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
	int i=0;
	for (i=0;i<MAXBUSTS;i++){
		ResetBustStruct(&(Busts[i]),0);
	}

	// Make the save files directory
	MakeDirectory(SAVEFOLDER);

	// Load the menu sound effect if it's present
	//char* tempstringconcat = CombineStringsPLEASEFREE(STREAMINGASSETS, "/SE/","wa_038",".ogg");
	//if (CheckFileExist(tempstringconcat)){
	//	menuSoundLoaded=1;
	//	menuSound = LoadSound(tempstringconcat);
	//}
	//free(tempstringconcat);
}

int main(int argc, char *argv[]){
	/* code */
	init();


	ClearDebugFile();

	// Fill with null char
	ClearMessageArray();

	// Initiate Lua
	L = luaL_newstate();
	luaL_openlibs(L);
	MakeLuaUseful();


	// Funky fresh stuff for me to use
	#if PLATFORM == PLAT_WINDOWS
		luaL_dofile(L,"./happy.lua");
	#elif PLATFORM == PLAT_VITA
		luaL_dofile(L,"app0:a/happy.lua");
	#endif
	lua_sethook(L, InBetweenLines, LUA_MASKLINE, 5);


	//CROSSMUSIC* testsong = LoadMusic("app0:a/testogg.ogg");
	//WriteSDLError();
	//PlayMusic(testsong);
	//WriteSDLError();
	//
	//FILE *fp;
	//fp = fopen("ux0:data/HIGURASHI/a.txt", "a");
	//fprintf(fp,"There are %d music deocoders available\n", Mix_GetNumMusicDecoders());
	//fclose(fp);

	//SetClearColor(255,0,0,255);
	//CrossTexture* testTex = LoadPNG("ux0:data/HIGURASHI/StreamingAssets/CG/black.png");
	//while (currentGameStatus!=99){
	//	StartDrawingA();
	//	vita2d_draw_texture(testTex,32,32);
	//	EndDrawingA();
	//}

	//printf("%s\n",currentPresetFilename);

	//fread
	//LoadPreset("./StreamingAssets/Presets/Watanagashi.txt");
	//return 1;
	currentGameStatus=0;

	//currentGameStatus=3;
	//strcpy(nextScriptToLoad,"wata_001");
int i=0;
	while (currentGameStatus!=99){
		switch (currentGameStatus){
			case 0:
				TitleScreen();
				break;
			case 1:
				// Create the string for the full path of the preset file and load it
				memset(&globalTempConcat,0,sizeof(globalTempConcat));
				strcpy(globalTempConcat,PRESETFOLDER);
				strcat(globalTempConcat,currentPresetFilename);
				LoadPreset(globalTempConcat);

				// Does not load the savefile, I promise.
				LoadGame();
				
				// If there is no save game, start a new one at chapter 0
				// Otherwise, go to the navigation menu
				if (currentPresetChapter==-1){
					currentPresetChapter=0;
					SetNextScriptName();
					currentGameStatus=3;
				}else{
					currentGameStatus=4;
				}
				
				break;
			case 2:
				FileSelector(PRESETFOLDER,&currentPresetFilename,"Select a preset");
				ControlsEnd();
				if (currentPresetFilename==NULL){
					printf("No files were found, or user quit\n");
					currentGameStatus=0;
				}else{
					currentGameStatus=1;
				}
				break;
			case 3:
				LuaThread(nextScriptToLoad);

				// If a preset is loaded, save the game. Otherwise, go to title
				if (currentPresetFileList.length!=0){
					currentGameStatus=4;
					SaveGame();
				}else{
					currentGameStatus=0;
				}
				break;
			case 4:
				// Menu for chapter jump, tip selection, and going to the next chapter
				NavigationMenu();
				break;
			case 5:
				// Menu for selecting tip to view
				TipMenu();
				break;
		}
	}
	printf("ENDGAME\n");
	return 0;
}
