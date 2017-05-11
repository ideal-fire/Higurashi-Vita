#define PLAT_WINDOWS 1
#define PLAT_VITA 2
#define PLAT_3DS 3

#define REND_SDL 0
#define REND_VITA2D 1
#define REND_SF2D 2

#define SND_NONE 0
#define SND_SDL 1

#define REZ_UNDEFINED 0
#define REZ_480P 1
#define REZ_720P 2

#define LOCATION_UNDEFINED "/CG/"
#define LOCATION_CG "/CG/"
#define LOCATION_CGALT "/CGAlt/"

// Change these depending on the target platform
#define RENDERER REND_SDL
#define PLATFORM PLAT_WINDOWS
#define SOUNDPLAYER SND_SDL
#define SILENTMODE 1

// Change these depending on old art, new art, or PS3 art.
	// Old art (UNSCALED) is REZ_480P for both
	// New art (UNSCALED) is REZ_480P for background and REZ_720 for busts
	// PS3 art (UNSCALED) is REZ_720 for both
	//
	// !!!!!!!!!!!!!!!!!!!!!!!
	// !!!!!!! BUT !!!!!!!!!!!
	// !!!!!!!!!!!!!!!!!!!!!!!
	// This is the resolution of the image. I will resize the characters for 480p for the PS VITA
	// If images are resized, account for that in these variables.
#define BACKGROUNDREZ REZ_480P
#define BUSTREZ REZ_480P
#define WINDOWREZ REZ_480P

#define BACKGROUNDLOCATION LOCATION_CG
#define BUSTLOCATION LOCATION_CGALT

void Draw();
void Controls();
void YeOlMainLoop();

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

/*
wa_038 is menu sound
*/

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
	#include <psp2/io/fcntl.h>


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

#define BUST_STATUS_NORMAL 0
#define BUST_STATUS_FADEIN 1 // var 1 is alpha per frame. var 2 is time where 0 alpha
#define BUST_STATUS_FADEOUT 2 // var 1 is alpha per frame
#define BUST_STATUS_FADEOUT_MOVE 3 // var 1 is alpha per frame. var 2 is x per frame. var 3 is y per frame

typedef struct hauighrehrge{
	CrossTexture* image;
	signed int xOffset;
	signed int yOffset;
	char isActive;
	char isInvisible;
	int layer;
	signed short alpha;
	unsigned char bustStatus;
	unsigned int statusVariable;
	unsigned int statusVariable2;
	unsigned int statusVariable3;
	unsigned int lineCreatedOn;
}bust;

bust Busts[6];

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
CROSSSFX* lastSoundEffect = NULL;

int TextSpeed=10;

// Alpha of black rectangle over screen
int MessageBoxAlpha = 100;

char MessageBoxEnabled=1;

char InputValidity = 1;

unsigned int currentScriptLine=0;

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
	if (InputValidity==1){
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
	if (InputValidity==1){
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
		sceCtrlPeekBufferPositive(0, &pad, 1);
		//sceTouchPeek(SCE_TOUCH_PORT_FRONT, &currentTouch, 1);
	#endif
	#if RENDERER == REND_SDL
		SDL_Event e;
		while( SDL_PollEvent( &e ) != 0 ){
			if( e.type == SDL_QUIT ){
				place=2;
				luaL_error(L,"game stopped\n");
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

void WriteDebugInfo(){
	FILE *fp;
	fp = fopen("ux0:data/HIGURASHI/a.txt", "a");
	fprintf(fp,"There are %d music deocoders available\n", Mix_GetNumMusicDecoders());
	fclose(fp);
}


void ResetBustStruct(bust* passedBust){
	passedBust->image=NULL;
	passedBust->xOffset=0;
	passedBust->yOffset=0;
	passedBust->isActive=0;
	passedBust->isInvisible=0;
	passedBust->alpha=255;
	passedBust->bustStatus = BUST_STATUS_NORMAL;
}

char* CombineStringsPLEASEFREE(const char* first, const char* second, const char* third){
	char* tempstringconcat = calloc(1,strlen(first)+strlen(second)+strlen(third)+1);
	strcat(tempstringconcat, first);
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
	for (i = 0; i < 6; i++){
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
				ResetBustStruct(&(Busts[i]));
				Busts[i].bustStatus = BUST_STATUS_NORMAL;
			}
			
		}
	}
}

void InBetweenLines(lua_State *L, lua_Debug *ar) {
	currentScriptLine++;
	do{
		FpsCapStart();
		ControlsStart();
		Update();
		Draw();
		Controls();
		if (WasJustPressed(SCE_CTRL_CROSS)){
			endType = Line_ContinueAfterTyping;
		}
		ControlsEnd();
		FpsCapWait();
	}while(endType==Line_Normal || endType == Line_WaitForInput);
}

void DrawBust(bust* passedBust){
	if (passedBust->alpha==255){
		#if BUSTREZ == REZ_480P
			DrawTexture(passedBust->image,160+passedBust->xOffset,passedBust->yOffset+32);
		#elif BUSTREZ == REZ_720P
			DrawTextureScale(passedBust->image,160+passedBust->xOffset,passedBust->yOffset+32,.5,.5);
		#endif	
	}else{
		#if BUSTREZ == REZ_480P
			DrawTextureAlpha(passedBust->image,160+passedBust->xOffset,passedBust->yOffset+32,passedBust->alpha);
		#elif BUSTREZ == REZ_720P
			DrawTextureScaleAlpha(passedBust->image,160+passedBust->xOffset,passedBust->yOffset+32,.5,.5,passedBust->alpha);
		#endif
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

//===================

void FadeBustshot(int passedSlot,int _time,char _wait){
	//int passedSlot = lua_tonumber(passedState,1)-1;
	//Busts[passedSlot].bustStatus = BUST_STATUS_FADEOUT;
	//Busts[passedSlot].statusVariable = 
	Busts[passedSlot].alpha=0;
	Busts[passedSlot].bustStatus = BUST_STATUS_FADEOUT;
	if (_time!=0){
		Busts[passedSlot].alpha=255;
		//int _time = floor(lua_tonumber(passedState,7));
		int _totalFrames = floor(60*(_time/(double)1000));
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
	int i=0;
	for (i=0;i<6;i++){
		if (Busts[i].isActive==1){
			FadeBustshot(i,_time,0);
		}
	}
	if (_wait==1){
		char _isDone=0;
		while (_isDone==0){
			_isDone=1;
			for (i=0;i<6;i++){
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
				for (i=0;i<6;i++){
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
	int _alphaPerFrame=255;
	//FadeAllBustshots(time,0);

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

	char* tempstringconcat = CombineStringsPLEASEFREE(STREAMINGASSETS BACKGROUNDLOCATION,filename,".png");
	CrossTexture* newBackground = LoadPNG(tempstringconcat);
	free(tempstringconcat);

	while (_backgroundAlpha>0){
		FpsCapStart();

		Update();
		_backgroundAlpha-=_alphaPerFrame;
		int i;
		StartDrawingA();
		if (currentBackground!=NULL){
			if (currentBackground!=NULL){
				#if BACKGROUNDREZ == REZ_480P
					DrawTexture(newBackground,160,32);
				#elif BACKGROUNDREZ == REZ_720P
					DrawTextureScale(newBackground,160,32,.5,.5);
				#endif
			
				#if BACKGROUNDREZ == REZ_480P
					DrawTextureAlpha(currentBackground,160,32,_backgroundAlpha);
				#elif BACKGROUNDREZ == REZ_720P
					DrawTextureScaleAlpha(currentBackground,160,32,.5,.5,_backgroundAlpha);
				#endif
			}else{
				#if BACKGROUNDREZ == REZ_480P
					DrawTextureAlpha(newBackground,160,32,_backgroundAlpha);
				#elif BACKGROUNDREZ == REZ_720P
					DrawTextureScaleAlpha(newBackground,160,32,.5,.5,_backgroundAlpha);
				#endif
			}
		}
		for (i = 0; i < 6; i++){
			if (Busts[i].isActive==1){
				DrawBust(&(Busts[i]));
			}
		}
		if (MessageBoxEnabled==1){
			DrawRectangle(160,32,640,480,0,0,0,MessageBoxAlpha);
		}
		if (MessageBoxEnabled==1){
			DrawMessageText();
		}
		EndDrawingA();

		ControlsStart();
		if (WasJustPressed(SCE_CTRL_CROSS)){
			for (i=0;i<6;i++){
				Busts[i].alpha=1;
			}
			_backgroundAlpha=1;
		}
		ControlsEnd();

		FpsCapWait();
	}

	if (currentBackground!=NULL){
		printf("Dispose old background\n");
		FreeTexture(currentBackground);
		currentBackground=NULL;
	}
	currentBackground=newBackground;
}


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
	MessageBoxEnabled=1;
	int i,j;
	int currentChar = GetNextCharOnLine(currentLine);
	unsigned const char* message = (unsigned const char*)lua_tostring(passedState,4);
	endType = lua_tonumber(passedState,5);

	char waitingIsForShmucks=0;

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
			Controls();
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
			Controls();
			ControlsEnd();
			FpsCapWait();
		}
		
	}
	return 0;
}

//
int L_Wait(lua_State* passedState){
	Wait(lua_tonumber(passedState,1));
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
	
	char* tempstringconcat = CombineStringsPLEASEFREE(STREAMINGASSETS"/BGM/", lua_tostring(passedState,2), ".ogg");
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
		Wait(lua_tonumber(passedState,2));
	}
	return 0;
}

void DrawBustshot(unsigned char passedSlot, const char* _filename, int _xoffset, int _yoffset, int _layer, int _fadeintime, int _waitforfadein, int _isinvisible){
	Draw();
	int i;
	unsigned char skippedInitialWait=0;

	WaitWithCodeStart(_fadeintime);

	// Don't draw while loading.
	if (Busts[passedSlot].image!=NULL){
		printf("Free old bust\n");
		FreeTexture(Busts[passedSlot].image);
		Busts[passedSlot].image=NULL;
		ResetBustStruct(&(Busts[passedSlot]));
	}

	char* tempstringconcat = CombineStringsPLEASEFREE(STREAMINGASSETS BUSTLOCATION,_filename,".png");

	if (CheckFileExist(tempstringconcat)==0){
		free(tempstringconcat);
		if (strlen(BUSTLOCATION) == 7){
			printf("Switching to cg\n");
			tempstringconcat = CombineStringsPLEASEFREE(STREAMINGASSETS "/CG/",_filename,".png");
		}else{
			tempstringconcat = CombineStringsPLEASEFREE(STREAMINGASSETS "/CGAlt/",_filename,".png");
		}
	}

	Busts[passedSlot].image = LoadPNG(tempstringconcat);
	free(tempstringconcat);

	Busts[passedSlot].xOffset = _xoffset;
	Busts[passedSlot].yOffset = _yoffset;

	if (_isinvisible!=0){
		Busts[passedSlot].isInvisible=1;
	}else{
		Busts[passedSlot].isInvisible=0;
	}
	Busts[passedSlot].layer = _layer;

	Busts[passedSlot].isActive=1;
	if ((int)_fadeintime!=0){
		Busts[passedSlot].alpha=0;
		int _timeTotal = _fadeintime;
		int _time = floor(_fadeintime/2);
		int _totalFrames = floor(60*(_time/(double)1000));
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

// Bustshot slot? (Normally 1-3 used, 5 for black, 6 for cinema)
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
	DrawBustshot(lua_tonumber(passedState,1), lua_tostring(passedState,2), lua_tonumber(passedState,5), lua_tonumber(passedState,6), lua_tonumber(passedState,13), lua_tonumber(passedState,14), lua_toboolean(passedState,15), lua_tonumber(passedState,12));

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
	DrawBustshot(lua_tonumber(passedState,1), lua_tostring(passedState,2), lua_tonumber(passedState,3), lua_tonumber(passedState,4), lua_tonumber(passedState,14), lua_tonumber(passedState,15), lua_toboolean(passedState,16), lua_tonumber(passedState,13));
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
	//for (i=0;i<6;i++){
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
	int passedSlot = lua_tonumber(passedState,1)-1;
	if (Busts[passedSlot].image!=NULL){
		FreeTexture(Busts[passedSlot].image);
	}
	ResetBustStruct(&(Busts[passedSlot]));
	return 0;
}

//FadeBustshot( 2, FALSE, 0, 0, 0, 0, 0, TRUE );
//FadeBustshot( SLOT, MOVE, X, Y, UNKNOWNA, UNKNOWNB, FADETIME, WAIT );
int L_FadeBustshot(lua_State* passedState){
	FadeBustshot(lua_tonumber(passedState,1)-1,lua_tonumber(passedState,7),lua_toboolean(passedState,8));
	return 0;
}

int L_PlaySE(lua_State* passedState){
	#if SILENTMODE != 1
		if (lastSoundEffect!=NULL){
			FreeSound(lastSoundEffect);
			lastSoundEffect=NULL;
		}
		char* tempstringconcat = CombineStringsPLEASEFREE(STREAMINGASSETS "/SE/",lua_tostring(passedState,2),".ogg");
		lastSoundEffect = LoadSound(tempstringconcat);
		free(tempstringconcat);
		PlaySound(lastSoundEffect,1);
	#endif
	return 0;
}

/*
TODO - For some reason, this doesn't clear the portrait

	OutputLine(NULL, "　そう言って倒れこむ魅音。",
		   NULL, "Mion said before collapsing.", Line_Normal);
	ClearMessage();

	DisableWindow();
	DrawBustshot( 3, "sa_se_aw_a1", 160, 0, 0, FALSE, 0, 0, 0, 0, 0, 0, 0, 20, 0, FALSE );
	DrawScene( "bg_108", 400 );
*/
/*
THIS CRASHES
	DisableWindow();
	DrawBustshotWithFiltering( 5, "black", "down", 0, 0, 0, FALSE, 0, 0, 0, 0, 0, 25, 300, TRUE );
	FadeBustshot( 1, FALSE, 0, 0, 0, 0, 0, TRUE );
	DrawBG( "bg_109", 0, TRUE );
	DrawBustshot( 2, "me_se_th_a1", 0, 0, 0, FALSE, 0, 0, 0, 0, 0, 0, 0, 0, 0, TRUE );
	FadeBustshotWithFiltering( 5, "down", 0, FALSE, 0, 0, 300, TRUE );
*/
// TODO - Don't clear the bustshot if it was the last command ONLY IF IT'S THE LAST COMMAND
// EVEN IF THERES TWO BUSTSHOTS SHOWN before scene change, only recent one stays
//look here

////「取り合えず生きてますわね＠ゲーム続行は可能でございますわぁ！！＠
//	OutputLine(NULL, "「取り合えず生きてますわね。",
//		   NULL, "\"She's alive, at least.", Line_WaitForInput);

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

	//  TEMP
	LUAREGISTER(L_NotYet,"SetSpeedOfMessage")
	LUAREGISTER(L_NotYet,"ShakeScreen")
}
//======================================================
void Draw(){
	int i;
	//DrawTexture(testtex,32,32);
	StartDrawingA();

	if (currentBackground!=NULL){
		#if BACKGROUNDREZ == REZ_480P
			DrawTexture(currentBackground,160,32);
		#elif BACKGROUNDREZ == REZ_720P
			DrawTextureScale(currentBackground,160,32,.5,.5);
		#endif
	}

	for (i = 0; i < 6; i++){
		if (Busts[i].isActive==1){
			DrawBust(&(Busts[i]));
		}
	}

	if (MessageBoxEnabled==1){
		// This is the message box
		DrawRectangle(160,32,640,480,0,0,0,MessageBoxAlpha);
	}

	if (MessageBoxEnabled==1){
		DrawMessageText();
	}
	EndDrawingA();
}

void Controls(){
	if (InputValidity==1){
	}
}

void LuaThread(){
	#if PLATFORM == PLAT_VITA
		luaL_dofile(L,"app0:a/happy.lua");
		lua_sethook(L, InBetweenLines, LUA_MASKLINE, 5);
		luaL_dofile(L,"app0:a/test3.lua");
	#elif PLATFORM == PLAT_WINDOWS
		luaL_dofile(L,"./happy.lua");
		lua_sethook(L, InBetweenLines, LUA_MASKLINE, 5);
		luaL_dofile(L,"./test3.lua");
	#endif
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
	for (i=0;i<6;i++){
		ResetBustStruct(&(Busts[i]));
	}
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
    //pthread_create(&tid, NULL, LuaThread, NULL);
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

	//SetClearColor(255,0,0,255);
	//CrossTexture* testTex = LoadPNG("ux0:data/HIGURASHI/StreamingAssets/CG/black.png");
	//while (1){
	//	StartDrawingA();
	//	vita2d_draw_texture(testTex,32,32);
	//	EndDrawingA();
	//}
	
	
	
	
	LuaThread();
	printf("ENDGAME\n");
	return 0;
}
