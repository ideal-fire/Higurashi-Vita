/*
	
	(OPTIONAL TODO)
		TODO - (optional) Garbage collector won't collect functions made in script files??? i.e: function hima_tips_09_b()
			Maybe I can make a system similar to the one I made in MrDude that tracks all the global variables made between two points of time.
				If I could do that, I could dispose of the variables properly.
					Or I could just not do this and ignore the small problem. That's always an option.
			(I think this problem isn't worth the effort. So few files have multiple functions that it won't be a problem. If the user runs the same file with multiple functions over and over then the old functions will become garbage anyway, so they can't run out of memory that way. They need a lot of files with lots of functions of different names)
		TODO - (Optional) Italics
			OutputLine(NULL, "　……知レバ、…巻キ込マレテシマウ…。",
			   NULL, "...<i>If she found out... she would become involved</i>...", Line_Normal);

			(Here's the problem, It'll be hard to draw non italics text and italics in the same line)
			(A possible solution is to store x cordinates to start italics text)
				// Here's the plan.
				// Make another message array, but store text that is in italics in it
		TODO - Position markup
			At the very end of Onikakushi, I think that there's a markup that looks something like this <pos=36>Keechi</pos>
		TODO - Mod libvita2d to not inlcude characters with value 1 when getting text width. (This should be easy to do. There's a for loop)
		TODO - Inversion
			I could actually modify the loaded texture data. That would be for the best. I would need to store the filepaths of all busts and backgrounds loaded, though. Or, I could store backups in another texture.
	TODO - Inform user of errors in game specific Lua

	TODO - Implement all vnds commands
		SOUND,
		MUSIC,
		(useless commands?)
			SKIP,
			ENDSCRIPT,
			END_OF_FILE

	TODO - Remove scriptFolder variable
*/
#define SINGLELINEARRAYSIZE 121
#define PLAYTIPMUSIC 0
#include "GeneralGoodConfig.h"

// main.h
	void startDrawing();
	void Draw(char _shouldDrawMessageBox);
	void RecalculateBustOrder();
	void PlayBGM(const char* filename, int _volume, int _slot);
	void LazyMessage(const char* stra, const char* strb, const char* strc, const char* strd);
	void SaveSettings();
	void XOutFunction();
	void DrawHistory(unsigned char _textStuffToDraw[][SINGLELINEARRAYSIZE]);
	void SaveGameEditor();
	void SettingsMenu();
	void initializeNathanScript();
	void activateVNDSSettings();
	void activateHigurashiSettings();
	typedef struct grhuighruei{
		char** theArray;
		unsigned char length;
	}goodStringMallocArray;
	typedef struct grejgrkew{
		unsigned char* theArray;
		unsigned char length;
	}goodu8MallocArray;

// Libraries all need
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h> // toupper
//
#include <Lua/lua.h>
#include <Lua/lualib.h>
#include <Lua/lauxlib.h>
//

#define LOCATION_UNDEFINED 0
#define LOCATION_CG 1
#define LOCATION_CGALT 2

/////////////////////////////////////
#define MAXIMAGECHAR 20
#define MAXFILES 50
#define MAXFILELENGTH 51
#define MAXMESSAGEHISTORY 40
#define VERSIONSTRING "v2.4" // This
#define VERSIONNUMBER 4 // This
#define VERSIONCOLOR 0,208,138
#define USEUMA0 1
// Specific constants
#if PLATFORM != PLAT_3DS
	#define SELECTBUTTONNAME "X"
	#define BACKBUTTONNAME "O"
	#define ADVBOXHEIGHT 181
	#define VERSIONSTRINGSUFFIX ""
#else
	#define SELECTBUTTONNAME "A"
	#define BACKBUTTONNAME "B"
	#define ADVBOXHEIGHT 75
	#define cpuOverclocked textIsBottomScreen
	#define VERSIONSTRINGSUFFIX ""
#endif
#define HISTORYONONESCREEN ((int)((screenHeight-currentTextHeight*2-5)/currentTextHeight))
#define MENUCURSOR ">"
#define MENUCURSOROFFSET 5
#define MENUOPTIONOFFSET menuCursorSpaceWidth+5
////////////////////////////////////
#define MAXMUSICARRAY 10
#define MAXSOUNDEFFECTARRAY 10
#define IMAGECHARSPACESTRING "   "
#define MESSAGEEDGEOFFSET 10

#include "GeneralGoodExtended.h"
#include "GeneralGood.h"
#include "GeneralGoodGraphics.h"
#include "GeneralGoodText.h"
#include "GeneralGoodImages.h"
#include "GeneralGoodSound.h"
#include "GeneralGood.h"
#include "FpsCapper.h"

// System string
#if __UNIX__ || __linux__ || __gnu_linux__
	#define SYSTEMSTRING "GNULINUX"
#elif __WIN32__
	#define SYSTEMSTRING "WINDOWS"
#elif __vita__
	#define SYSTEMSTRING "VITA"
#elif _3DS
	#define SYSTEMSTRING "3DS"
#else
	#define SYSTEMSTRING "UNKNOWN"
#endif

// 1 is start
// 2 adds BGM and SE volume
// 3 adds voice volume
// 4 adds MessageBoxAlpha and textOnlyOverBackground
// 5 adds textSpeed
// 6 adds vndsClearAtBottom
#define OPTIONSFILEFORMAT 6

//#define LUAREGISTER(x,y) DebugLuaReg(y);
#define LUAREGISTER(x,y) lua_pushcfunction(L,x);\
	lua_setglobal(L,y);

////////////////////////////////////////
// PLatform specific variables
///////////////////////////////////////

//////////////
// The streamingAssets variable is only used for images and sound
char* streamingAssets;
char* presetFolder;
char* scriptFolder;
char* saveFolder;
char* gamesFolder;

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
	float cacheXOffsetScale;
	float cacheYOffsetScale;
	char* relativeFilename;
}bust;

bust* Busts;

#define MAXLINES 15

lua_State* L = NULL;
/*
	Line_ContinueAfterTyping=0; (No wait after text display, go right to next script line)
	Line_WaitForInput=1; (Wait for the player to click. Displays an arrow.)
	Line_Normal=2; (Wait for the player to click. DIsplays a page icon and almost 100% of the time has the screen cleared with the next command)
*/
#define Line_ContinueAfterTyping 0
#define Line_WaitForInput 1
#define Line_Normal 2
int endType;
signed char useVsync=0;
unsigned char currentMessages[MAXLINES][SINGLELINEARRAYSIZE];
int currentLine=0;
int place=0;

int lastBGMVolume = 128;

CrossTexture* currentBackground = NULL;
CROSSMUSIC* currentMusic[MAXMUSICARRAY] = {NULL};
CROSSPLAYHANDLE currentMusicHandle[MAXMUSICARRAY] = {0};
char* currentMusicFilepath[MAXMUSICARRAY]={NULL};
short currentMusicUnfixedVolume[MAXMUSICARRAY] = {0};
//CROSSMUSIC* currentMusic = NULL;
CROSSSFX* soundEffects[MAXSOUNDEFFECTARRAY] = {NULL};

CROSSSFX* menuSound=NULL;
signed char menuSoundLoaded=0;

// Alpha of black rectangle over screen
unsigned char MessageBoxAlpha = 100;

signed char MessageBoxEnabled=1;

unsigned int currentScriptLine=0;

// Order of busts drawn as organized by layer
// element in array is their bust slot
// first element in array is highest up
// so, when drawing, start from the end and go backwards to draw the first element last
unsigned char* bustOrder;

unsigned char* bustOrderOverBox;

char* locationStrings[3] = {(char*)"CG/",(char*)"CG/",(char*)"CGAlt/"};

goodStringMallocArray currentPresetFileList;
goodStringMallocArray currentPresetTipList;
goodStringMallocArray currentPresetTipNameList;
goodStringMallocArray currentPresetFileFriendlyList;
goodu8MallocArray currentPresetTipUnlockList;
int16_t currentPresetChapter=0;
// Both of these are made with malloc
char* currentPresetFilename=NULL;
// This may not be set because the user can choose to use the legacy preset folder mode.
char* currentGameFolderName=NULL;

#define GAMESTATUS_TITLE 0
#define GAMESTATUS_LOADPRESET 1
#define GAMESTATUS_PRESETSELECTION 2
#define GAMESTATUS_MAINGAME 3
#define GAMESTATUS_NAVIGATIONMENU 4
#define GAMESTATUS_TIPMENU 5
#define GAMESTATUS_GAMEFOLDERSELECTION 6
#define GAMESTATUS_QUIT 99
signed char currentGameStatus=GAMESTATUS_TITLE;

unsigned char nextScriptToLoad[256] = {0};
unsigned char globalTempConcat[256] = {0};

signed char tipNamesLoaded=0;
signed char chapterNamesLoaded=0;
unsigned char lastSelectionAnswer=0;

// The x position on screen of this image character
signed short imageCharX[MAXIMAGECHAR] = {0};
// The y position on screen of this image character
signed short imageCharY[MAXIMAGECHAR] = {0};
// The character that the image character is. The values in here are one of the IMAGECHAR constants
signed char imageCharType[MAXIMAGECHAR] = {0};
// The line number the image chars are at. This is used when displaying the message in OutputLine
unsigned short imageCharLines[MAXIMAGECHAR] = {0};
// The character positions within the lines they're at. This is used when displayng the message in OutputLine. Image characters are 3 spaces in the message. This will refer to the first spot. 
unsigned short imageCharCharPositions[MAXIMAGECHAR] = {0};

#define IMAGECHARSTAR 1
#define IMAGECHARNOTE 2
#define IMAGECHARUNKNOWN 0
CrossTexture* imageCharImages[3]; // PLEASE DON'T FORGET TO CHANGE THIS IF ANOTHER IMAGE CHAR IS ADDED

unsigned char filterR;
unsigned char filterG;
unsigned char filterB;
unsigned char filterA;
unsigned char filterActive=0;

signed char autoModeOn=0;
int32_t autoModeWait=500;

signed char cpuOverclocked=0;

#define TEXTMODE_NVL 0
#define TEXTMODE_AVD 1
char gameTextDisplayMode=TEXTMODE_NVL;
char hasOwnVoiceSetting=0;

unsigned char graphicsLocation = LOCATION_CGALT;

unsigned char messageHistory[MAXMESSAGEHISTORY][SINGLELINEARRAYSIZE];
unsigned char oldestMessage=0;

#define TOUCHMODE_NONE 0
#define TOUCHMODE_MAINGAME 1
#define TOUCHMODE_MENU 2
#define TOUCHMODE_LEFTRIGHTSELECT 3
unsigned char easyTouchControlMode = TOUCHMODE_MENU;

char presetsAreInStreamingAssets=1;

float bgmVolume = 0.75;
float seVolume = 1.0;
float voiceVolume = 1.0;

int currentTextHeight;
#if PLATFORM == PLAT_VITA
	pthread_t soundProtectThreadId;
#endif
#if PLATFORM == PLAT_3DS
	char _3dsSoundProtectThreadIsAlive=1;
	Thread _3dsSoundUpdateThread;
	char _bgmIsLock=0;
#endif
char isActuallyUsingUma0=0;
int MAXBUSTS = 9;
short textboxYOffset=0;
short textboxXOffset=0;
CrossTexture* currentCustomTextbox=NULL;
int outputLineScreenWidth;
int outputLineScreenHeight;
int messageInBoxXOffset=10;
int messageInBoxYOffset=0;
// 1 by default to retain compatibility with games converted before game specific Lua 
char gameHasTips=1;
char textOnlyOverBackground=0;
// A constant values between 0 and 127 that means that the text should be instantly displayed
#define TEXTSPEED_INSTANT 100
signed char textSpeed=1;
char isGameFolderMode;
char isEmbedMode;
int menuCursorSpaceWidth;
char canChangeBoxAlpha=1;
// When this variable is 1, we can assume that the current game is the default game because the user can't chose a different game when a default is set.
char defaultGameIsSet;
char nathanscriptIsInit=0;
char scriptUsesFileExtensions=0;
char bustsStartInMiddle=1;

// What scripts think the screen width and height is.
// For Higurashi, this is 640x480
// For vnds, this is the DS' screen resolution
int scriptScreenWidth=640;
int scriptScreenHeight=480;

// X and Y scale applied to graphics and their coordinates
#define USENEWSCALE 1
double graphicsScale=1.0;
char* lastBackgroundFilename=NULL;
char* currentScriptFilename=NULL;

char currentlyVNDSGame=0;
char nextVndsBustshotSlot=0;
// If all the text should be cleared when the text reached the bottom of the screen when playing a VNDS game
signed char vndsClearAtBottom=0;

/*
====================================================
*/
char* charToBoolString(char _boolValue){
	if (_boolValue){
		return "True";
	}else{
		return "False";
	}
}
double applyGraphicsScale(double _valueToScale){
	return _valueToScale*graphicsScale;
}
char* mallocForString(const char* _stringToPutInBuffer){
	char* _returnString = malloc(strlen(_stringToPutInBuffer)+1);
	strcpy(_returnString,_stringToPutInBuffer);
	return _returnString;
}
void changeMallocString(char** _stringToChange, const char* _newValue){
	if (*_stringToChange!=NULL){
		free(*_stringToChange);
	}
	*_stringToChange = malloc(strlen(_newValue)+1);
	strcpy(*_stringToChange,_newValue);
}
#if SUBPLATFORM == SUB_UNIX
char* itoa(int value, char* _buffer, int _uselessBase){
	sprintf(_buffer,"%d",value);
	return _buffer;
}
#endif
void XOutFunction(){
	#if PLATFORM == PLAT_3DS
		_3dsSoundProtectThreadIsAlive=0;
		threadJoin(_3dsSoundUpdateThread,30000000000);
	#endif
	quitGraphics();
	quitAudio();
	generalGoodQuit();
	exit(0);
}
char isForceQuit(){
	#if PLATFORM == PLAT_3DS
		return !(aptMainLoop());
	#else
		return 0;
	#endif
}
void exitIfForceQuit(){
	if (isForceQuit()==1){
		XOutFunction();
	}
}
// Give a function name to this. It will tell you if it's new.
// THIS DOES NOT ACCOUNT FOR FUNCTIONS MADE IN happy.lua
void DebugLuaReg(char* name){
	if (lua_getglobal(L,name)==0){
		//printf("%s is new!\n",name);
		printf("LUAREGISTER(L_NotYet,\"%s\")\n",name);
		lua_pop(L,-1);
		return;
	}
}
void PlayMenuSound(){
	if (menuSoundLoaded==1){
		playSound(menuSound,1,10);
	}
}
CrossTexture* SafeLoadPNG(const char* path){
	CrossTexture* _tempTex = loadPNG((char*)path);
	if (_tempTex==NULL){
		showErrorIfNull(_tempTex);
		LazyMessage("Failed to load image",path,"What will happen now?!",NULL);
	}
	return _tempTex;
}
CrossTexture* LoadEmbeddedPNG(const char* path){
	fixPath((char*)path,globalTempConcat,TYPE_EMBEDDED);
	CrossTexture* _tempTex = loadPNG((char*)globalTempConcat);
	if (_tempTex==NULL){
		showErrorIfNull(_tempTex);
		#if PLATFORM != PLAT_3DS
			LazyMessage("Failed to load image",path,"What will happen now?!","THIS IS SUPPOSED TO BE EMBEDDED!");
		#else
			LazyMessage("Failed to load image",path,"What will happen now?!","Check you set up everything correctly.");
		#endif
	}
	return _tempTex;
}
void DrawMessageBox(){
	#if PLATFORM == PLAT_3DS
		if (textIsBottomScreen==1){
			return;
		}
	#endif
	if (gameTextDisplayMode == TEXTMODE_NVL || currentCustomTextbox==NULL){
		drawRectangle(0,0,outputLineScreenWidth,outputLineScreenHeight,0,0,0,MessageBoxAlpha);
	}else{
		if (canChangeBoxAlpha){
			drawTextureScaleAlpha(currentCustomTextbox,textboxXOffset,textboxYOffset, (double)(outputLineScreenWidth-textboxXOffset)/(double)getTextureWidth(currentCustomTextbox), (double)(ADVBOXHEIGHT)/(double)getTextureHeight(currentCustomTextbox),MessageBoxAlpha);
		}else{
			drawTextureScale(currentCustomTextbox,textboxXOffset,textboxYOffset, (double)(outputLineScreenWidth-textboxXOffset)/(double)getTextureWidth(currentCustomTextbox), (double)(ADVBOXHEIGHT)/(double)getTextureHeight(currentCustomTextbox));
		}
	}
}
void DrawCurrentFilter(){
	drawRectangle(0,0,outputLineScreenWidth,outputLineScreenHeight,filterR,filterG,filterB,filterA);
	//drawRectangle(0,0,960,screenHeight,filterR,255-(filterG*filterA*.0011),filterB,255);
}
u64 waitwithCodeTarget;
void WaitWithCodeStart(int amount){
	waitwithCodeTarget = getTicks()+amount;
}
void WaitWithCodeEnd(int amount){
	if (getTicks()<waitwithCodeTarget){
		wait(waitwithCodeTarget-getTicks());
	}
}
void SetDefaultFontSize(){
	#if TEXTRENDERER == TEXT_DEBUG
		fontSize = 1;
	#endif
	#if TEXTRENDERER == TEXT_FONTCACHE
		fontSize = floor(screenWidth/40);
	#endif
	#if TEXTRENDERER == TEXT_VITA2D
		fontSize=32;
	#endif
}
void ReloadFont(){
	#if PLATFORM != PLAT_3DS
		fixPath("assets/LiberationSans-Regular.ttf",globalTempConcat,TYPE_EMBEDDED);
	#elif PLATFORM == PLAT_3DS
		fixPath("assets/Bitmap-LiberationSans-Regular",globalTempConcat,TYPE_EMBEDDED);
	#else
		#error whoops
	#endif
	loadFont(globalTempConcat);
	currentTextHeight = textHeight(fontSize);
}
char MenuControls(char _choice,int _menuMin, int _menuMax){
	if (wasJustPressed(SCE_CTRL_UP)){
		if (_choice!=_menuMin){
			return (_choice-1);
		}else{
			return _menuMax;
		}
	}
	if (wasJustPressed(SCE_CTRL_DOWN)){
		if (_choice!=_menuMax){
			return (_choice+1);
		}else{
			return _menuMin;
		}
	}
	return _choice;
}
char SafeLuaDoFile(lua_State* passedState, char* passedPath, char showMessage){
	if (checkFileExist(passedPath)==0){
		if (showMessage==1){
			LazyMessage("The LUA file",passedPath,"does not exist!","What will happen now?!");
		}
		return 0;
	}
	luaL_dofile(passedState,passedPath);
	return 1;
}
void WriteToDebugFile(const char* stuff){
	char *_tempDebugFileLocationBuffer = malloc(strlen(DATAFOLDER)+strlen("log.txt"));
	strcpy(_tempDebugFileLocationBuffer,DATAFOLDER);
	strcat(_tempDebugFileLocationBuffer,"log.txt");
	FILE *fp;
	fp = fopen(_tempDebugFileLocationBuffer, "a");
	if (!fp){
		LazyMessage("Failed to open debug file.",_tempDebugFileLocationBuffer,NULL,NULL);
		return;
	}
	fprintf(fp,"%s\n",stuff);
	fclose(fp);
}
void WriteSDLError(){
	#if RENDERER == REND_SDL || SOUNDPLAYER == SND_SDL
		WriteToDebugFile(SDL_GetError());
	#else
		WriteToDebugFile("Can't write SDL error because not using SDL.");
	#endif
}
size_t u_strlen(const unsigned char * array){
	return (const size_t)strlen((const char*)array);
}
// Returns one if they chose yes
// Returns zero if they chose no
int LazyChoice(const char* stra, const char* strb, const char* strc, const char* strd){
	int _choice=0;
	controlsStart();
	controlsEnd();
	while (currentGameStatus!=GAMESTATUS_QUIT){
		fpsCapStart();
		controlsStart();
		if (wasJustPressed(SCE_CTRL_CROSS)){
			PlayMenuSound();
			controlsStart();
			controlsEnd();
			return _choice;
		}
		if (wasJustPressed(SCE_CTRL_DOWN)){
			_choice++;
			if (_choice>1){
				_choice=0;
			}
		}
		if (wasJustPressed(SCE_CTRL_UP)){
			_choice--;
			if (_choice<0){
				_choice=1;
			}
		}
		controlsEnd();
		startDrawing();
		if (stra!=NULL){
			goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(0+2),stra,fontSize);
		}
		if (strb!=NULL){
			goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(1+2),strb,fontSize);
		}
		if (strc!=NULL){
			goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(2+2),strc,fontSize);
		}
		if (strd!=NULL){
			goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(3+2),strd,fontSize);
		}
		goodDrawText(0,screenHeight-32-currentTextHeight*(_choice+1),MENUCURSOR,fontSize);
		goodDrawText(MENUOPTIONOFFSET,screenHeight-32-currentTextHeight*2,"Yes",fontSize);
		goodDrawText(MENUOPTIONOFFSET,screenHeight-32-currentTextHeight,"No",fontSize);
		endDrawing();
		fpsCapWait();
	}
	return 0;
}
int FixVolumeArg(int _val){
	if (floor(_val/(float)2)>128){
		return 128;
	}else if (_val<0){
		return 0;
	}else{
		return floor(_val/(float)2);
	}
}
int FixBGMVolume(int _val){
	return FixVolumeArg(_val)*bgmVolume;
}
int FixSEVolume(int _val){
	return FixVolumeArg(_val)*seVolume;
}
int GenericFixSpecificVolume(int _val, double _scale){
	return FixVolumeArg(_val)*_scale;
}
int FixVoiceVolume(int _val){
	return FixVolumeArg(_val)*voiceVolume;
}
void addToMessageHistory(const char* _newWords){
	strcpy((char*)messageHistory[oldestMessage],_newWords);
	oldestMessage++;
	if (oldestMessage==MAXMESSAGEHISTORY){
		oldestMessage=0;
	}
}
void clearHistory(){
	int i;
	for (i=0;i<MAXMESSAGEHISTORY;i++){
		messageHistory[i][0]='\0';
	}
}
void ClearMessageArray(){
	currentLine=0;
	int i,j;
	for (i = 0; i < MAXLINES; i++){
		if (currentMessages[i][0]!='\0'){
			addToMessageHistory(currentMessages[i]);
		}
		for (j = 0; j < SINGLELINEARRAYSIZE; j++){
			currentMessages[i][j]='\0';
		}
	}
	for (i=0;i<MAXIMAGECHAR;i++){
		imageCharType[i]=-1;
	}
}
void SetAllMusicVolume(int _passedFixedVolume){
	int i;
	for (i = 0; i < MAXMUSICARRAY; i++){
		setMusicVolume(currentMusicHandle[i],_passedFixedVolume);
	}
}
int GetNextCharOnLine(int _linenum){
	return u_strlen(currentMessages[_linenum]);
}
void DrawMessageText(){
	int i;
	#if PLATFORM == PLAT_3DS
		if (textIsBottomScreen==1){
			startDrawingBottom();
			for (i = 0; i < MAXLINES; i++){
				goodDrawText(0,12+i*(currentTextHeight),(char*)currentMessages[i],fontSize);
			}
			for (i=0;i<MAXIMAGECHAR;i++){
				if (imageCharType[i]!=-1){
					drawTextureScale(imageCharImages[imageCharType[i]],imageCharX[i]-textboxXOffset-messageInBoxXOffset,imageCharY[i]-messageInBoxYOffset-textboxYOffset,((double)textWidth(fontSize,IMAGECHARSPACESTRING)/ getTextureWidth(imageCharImages[imageCharType[i]])),((double)textHeight(fontSize)/getTextureHeight(imageCharImages[imageCharType[i]])));
				}
			}
			return;
		}
	#endif
	for (i = 0; i < MAXLINES; i++){
		goodDrawText(textboxXOffset+messageInBoxXOffset,messageInBoxYOffset+12+textboxYOffset+i*(currentTextHeight),(char*)currentMessages[i],fontSize);
	}
	for (i=0;i<MAXIMAGECHAR;i++){
		if (imageCharType[i]!=-1){
			drawTextureScale(imageCharImages[imageCharType[i]],imageCharX[i],imageCharY[i],((double)textWidth(fontSize,IMAGECHARSPACESTRING)/ getTextureWidth(imageCharImages[imageCharType[i]])),((double)textHeight(fontSize)/getTextureHeight(imageCharImages[imageCharType[i]])));
		}
	}
}
int Password(int val, int _shouldHave){
	if (val==_shouldHave){
		return val+1;
	}else{
		return 0;
	}
}
void WriteIntToDebugFile(int a){
	char _tempCompleteNumberBuffer[9]; // 8 000 000 null
	sprintf(_tempCompleteNumberBuffer,"%d",a);
	WriteToDebugFile(_tempCompleteNumberBuffer);
}
// Does not clear the debug file at ux0:data/HIGURASHI/log.txt  , I promise.
void ClearDebugFile(){
	char* _tempDebugFileLocationBuffer = malloc(strlen(DATAFOLDER)+strlen("log.txt")+1);
	strcpy(_tempDebugFileLocationBuffer,DATAFOLDER);
	strcat(_tempDebugFileLocationBuffer,"log.txt");
	FILE *fp;
	fp = fopen(_tempDebugFileLocationBuffer, "w");
	if (!fp){
		LazyMessage("Failed to open debug file.",_tempDebugFileLocationBuffer,NULL,NULL);
		return;
	}
	fclose(fp);
	free(_tempDebugFileLocationBuffer);
}
void ResetBustStruct(bust* passedBust, int canfree){
	if (canfree==1 && passedBust->image!=NULL){
		freeTexture(passedBust->image);
		if (passedBust->relativeFilename!=NULL){
			free(passedBust->relativeFilename);
		}
	}
	passedBust->image=NULL;
	passedBust->xOffset=0;
	passedBust->yOffset=0;
	passedBust->isActive=0;
	passedBust->isInvisible=0;
	passedBust->alpha=255;
	passedBust->bustStatus = BUST_STATUS_NORMAL;
	passedBust->lineCreatedOn=0;
	passedBust->relativeFilename=NULL;
}
void DisposeOldScript(){
	// Frees the script main
	lua_getglobal(L,"FreeTrash");
	lua_call(L, 0, 0);
}
char StringStartWith(const char *a, const char *b){
	if(strncmp(a, b, strlen(b)) == 0) return 1;
	return 0;
}
// Give it a full script file path and it will return 1 if the file was converted beforehand
int DidActuallyConvert(char* filepath){
	if (checkFileExist(filepath)==0){
		LazyMessage("I was going to check if you converted this file,","but I can't find it!",filepath,"How strange.");
		return 1;
	}
	FILE* file = fopen(filepath, "r");
	char line[256];

	int _isConverted=0;

	startDrawing();
	goodDrawText(32,50,"Checking if you actually converted the script...",fontSize);
	goodDrawText(32,200,filepath,fontSize);
	endDrawing();

	while (fgets(line, sizeof(line), file)) {
		printf("%s", line);

		// //MyLegGuyisanoob is put in the first blank line found in converted script files
		if (StringStartWith(line,"//MyLegGuyisanoob")==1){
			_isConverted=1;
			break;
		}else if (StringStartWith(line,"function main()")==1){ // Every file should have this
			_isConverted=1;
			break;
		}
	}

	fclose(file);
	return _isConverted;
}
void SaveFontSizeFile(){
	fixPath("fontsize.noob",globalTempConcat,TYPE_DATA);
	FILE* fp = fopen((const char*)globalTempConcat,"w");
	fwrite(&fontSize,4,1,fp);
	fclose(fp);
}
void LoadFontSizeFile(){
	fixPath("fontsize.noob",globalTempConcat,TYPE_DATA);
	FILE* fp = fopen((const char*)globalTempConcat,"r");
	fread(&fontSize,4,1,fp);
	fclose(fp);
}
void DisplaypcallError(int val, const char* fourthMessage){
	switch (val){
		case LUA_ERRRUN:
			LazyMessage("lua_pcall failed with error","LUA_ERRSYNTAX, a runtime error.","Please report the bug on the thread.",fourthMessage);
		break;
		case LUA_ERRMEM:
			LazyMessage("lua_pcall failed with error","LUA_ERRMEM, an out of memory error","Please report the bug on the thread.",fourthMessage);
		break;
		case LUA_ERRERR:
			LazyMessage("lua_pcall failed with error","LUA_ERRMEM, a message handler error","Please report the bug on the thread.",fourthMessage);
		break;
		case LUA_ERRGCMM:
			LazyMessage("lua_pcall failed with error","LUA_ERRGCMM, an __gc metamethod error","Please report the bug on the thread.",fourthMessage);
		break;
		default:
			LazyMessage("lua_pcall failed with error","UNKNOWN ERROR, something that shouldn't happen.","Please report the bug on the thread.",fourthMessage);
		break;
	}
}
int _debugCount=0;
void PrintDebugCounter(){
	printf("DEBUG %d\n",_debugCount);
	_debugCount++;
}
// Returns 1 if it worked
char RunScript(const char* _scriptfolderlocation,char* filename, char addTxt){
	activateHigurashiSettings();
	// Hopefully, nobody tries to call a script from a script and wants to keep the current message display.
	ClearMessageArray();
	currentScriptLine=0;
	char tempstringconcat[strlen(_scriptfolderlocation)+strlen(filename)+strlen(".txt")+1];
	strcpy(tempstringconcat,_scriptfolderlocation);
	strcat(tempstringconcat,filename);
	if (addTxt==1){
		strcat(tempstringconcat,".txt");
	}
	// Free garbage and old main function if it existed
	DisposeOldScript();
	int _loadFileResult = luaL_loadfile(L, tempstringconcat);
	if (_loadFileResult!=LUA_OK){
		switch (_loadFileResult){
			case LUA_ERRSYNTAX:
				if (DidActuallyConvert(tempstringconcat)==1){
					LazyMessage("luaL_loadfile failed with error","LUA_ERRSYNTAX","You seem to have converted the files correctly...","Please report the bug on the thread.");
				}else{
					LazyMessage("luaL_loadfile failed with error","LUA_ERRSYNTAX","You probably forgot to convert the files with the","converter. Please make sure you did.");
				}
			break;
			case LUA_ERRMEM:
				LazyMessage("luaL_loadfile failed with error","LUA_ERRMEM","This is an out of memory error.","Please report the bug on the thread.");
			break;
			case LUA_ERRGCMM:
				LazyMessage("luaL_loadfile failed with error","LUA_ERRGCMM","This is a __gc metamethod error.","Please report the bug on the thread.");
			break;
			case LUA_ERRFILE:
				LazyMessage("luaL_loadfile failed with error","LUA_ERRFILE, this means the file failed to load.","Make sure the file exists.",tempstringconcat);
			break;
			default:
				LazyMessage("luaL_loadfile failed with error","UNKNOWN ERROR!","This is weird and should NEVER HAPPEN!","Please report the bug on the thread.");
			break;
		}
		currentGameStatus=GAMESTATUS_TITLE;
		return 0;
	}
	
	changeMallocString(&currentScriptFilename,tempstringconcat);
	int _pcallResult = lua_pcall(L, 0, LUA_MULTRET, 0);
	if (_pcallResult!=LUA_OK){
		printf("Failed pcall!\n");
		DisplaypcallError(_pcallResult,"This is the first lua_pcall in RunScript.");
		currentGameStatus=GAMESTATUS_TITLE;
		return 0;
	}

	//if (SafeLuaDoFile(L,tempstringconcat)==0){
	//	printf("Failed to load\n");
	//}

	// Adds function to stack
	lua_getglobal(L,"main");
	// Call funciton. Removes function from stack.
	_pcallResult = lua_pcall(L, 0, 0, 0);
	if (_pcallResult!=LUA_OK){
		DisplaypcallError(_pcallResult,"This is the second lua_pcall in RunScript.");
	}
	return 1;
}
char* CombineStringsPLEASEFREE(const char* first, const char* firstpointfive, const char* second, const char* third){
	char* tempstringconcat = (char*)calloc(1,strlen(first)+(firstpointfive!=NULL ? strlen(firstpointfive) : 0)+(second!=NULL ? strlen(second) : 0)+(third!=NULL ? strlen(third) : 0)+1);
	strcpy(tempstringconcat, first);
	if (firstpointfive!=NULL){
		strcat(tempstringconcat, firstpointfive);
	}
	if (second!=NULL){
		strcat(tempstringconcat, second);
	}
	if (third!=NULL){
		strcat(tempstringconcat, third);
	}
	return tempstringconcat;
}
signed char WaitCanSkip(int amount){
	int i=0;
	controlsStart();
	controlsEnd();
	for (i = 0; i < floor(amount/50); ++i){
		wait(50);
		controlsStart();
		if (wasJustPressed(SCE_CTRL_CROSS)){
			controlsEnd();
			printf("Skipped with %d left\n",amount-i);
			return 1;
		}
		controlsEnd();
	}
	wait(amount%50);
	return 0;
}
void DrawUntilX(){
	while (1){
		fpsCapStart();

		controlsStart();
		if (wasJustPressed(SCE_CTRL_CROSS) || isSkipping==1){
			break;
		}
		controlsEnd();

		startDrawing();
		Draw(MessageBoxEnabled);
		endDrawing();

		fpsCapWait();
	}
	controlsEnd();
}
// If we've run out of new lines, shift everything up.
void LastLineLazyFix(int* _line){
	if (*_line==MAXLINES){
		int i;
		addToMessageHistory(currentMessages[0]);
		for (i=1;i<MAXLINES;i++){
			strcpy(currentMessages[i-1],currentMessages[i]);
		}
		currentMessages[MAXLINES-1][0]=0;
		(*_line)--;

		for (i=0;i<MAXIMAGECHAR;i++){
			if (imageCharType[i]!=-1){
				imageCharY[i]-=currentTextHeight;
				// Delete image char if it goes offscreen
				if (imageCharY[i]<0){
					if (imageCharY[i]<(screenHeight*.20)*-1){
						imageCharType[i]=-1;
					}
				}
			}
		}

		//DrawUntilX();
		//ClearMessageArray();
		//*_line=0;
	}
}
void changeIfLazyLastLineFix(int* _line, int* _toChange){
	int _cacheLine = *_line;
	LastLineLazyFix(_line);
	if (*_line!=_cacheLine){
		(*_toChange)-=1;
	}
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
				printf("DONE SPRITE MOVING\n");
			}
		}
	}
}
int FixHistoryOldSub(int _val, int _scroll){
	if (_val+_scroll>=MAXMESSAGEHISTORY){
		return (_val+_scroll)-MAXMESSAGEHISTORY;
	}else{
		return _val+_scroll;
	}
}
void incrementScriptLineVariable(lua_State *L, lua_Debug *ar){
	currentScriptLine++;
}
void updateControlsGeneral(){
	if (wasJustPressed(SCE_CTRL_SQUARE)){
		isSkipping=1;
		endType=Line_ContinueAfterTyping;
	}
	if (wasJustPressed(SCE_CTRL_TRIANGLE)){
		SettingsMenu();
	}
	if (wasJustPressed(SCE_CTRL_SELECT)){
		PlayMenuSound();
		if (autoModeOn==1){
			autoModeOn=0;
		}else{
			autoModeOn=1;
		}
	}
}
void outputLineWait(){
	//if (currentGameStatus==GAMESTATUS_MAINGAME){
	if (isSkipping==1){
		controlsStart();
		#if PLATFORM != PLAT_COMPUTER
			if (!isDown(SCE_CTRL_SQUARE)){
				isSkipping=0;
			}
		#endif
		#if PLATFORM == PLAT_COMPUTER
			if ( /*(  !(isDown(SCE_TOUCH) && (touchX<screenWidth*.25 && touchY<screenHeight*.20)) ) && */!(isDown(SCE_CTRL_SQUARE))  ){
				isSkipping=0;
				PlayMenuSound();
			}
		#endif
		controlsEnd();
		if (isSkipping==1){
			endType=Line_ContinueAfterTyping;
		}
	}
	u64 _inBetweenLinesMilisecondsStart = getTicks();
	char _didPressCircle=0;
	do{
		fpsCapStart();
		controlsStart();
		Update();
		startDrawing();
		Draw(MessageBoxEnabled);
		endDrawing();

		if (wasJustPressed(SCE_CTRL_CROSS)){
			if (_didPressCircle==1){
				MessageBoxEnabled=1;
			}
			endType = Line_ContinueAfterTyping;
		}
		if (wasJustPressed(SCE_CTRL_CIRCLE)){
			if (_didPressCircle==1){
				MessageBoxEnabled = !MessageBoxEnabled;
			}else if (MessageBoxEnabled==1){
				MessageBoxEnabled=0;
				_didPressCircle=1;
			}
		}
		updateControlsGeneral();
		if (wasJustPressed(SCE_CTRL_START)){
			DrawHistory(messageHistory);
		}
		controlsEnd();
		fpsCapWait();
		if (autoModeOn==1){
			if (getTicks()>=(_inBetweenLinesMilisecondsStart+autoModeWait)){
				endType = Line_ContinueAfterTyping;
			}
		}
		exitIfForceQuit();
	}while(endType==Line_Normal || endType == Line_WaitForInput);
}
// This is used in background and bust drawing
// For Higurashi, this is used to get the center of the screen for all images.
// For vnds, this is just used to get the position of the background.
void GetXAndYOffset(CrossTexture* _tempImg, signed int* _tempXOffset, signed int* _tempYOffset){
	#if USENEWSCALE
		*_tempXOffset = floor((screenWidth-applyGraphicsScale(getTextureWidth(_tempImg)))/2);
		*_tempYOffset = floor((screenHeight-applyGraphicsScale(getTextureHeight(_tempImg)))/2);
	#else // TODO - Remove if all goes well
		*_tempXOffset = floor((screenWidth-getTextureWidth(_tempImg))/2);
		*_tempYOffset = floor((screenHeight-getTextureHeight(_tempImg))/2);
		// If they're bigger than the screen, assume that they're supposed to scroll or something
		if (*_tempXOffset<0){
			*_tempXOffset=0;
		}
		if (*_tempYOffset<0){
			*_tempYOffset=0;
		}
	#endif
}
float GetXOffsetScale(CrossTexture* _tempImg){
	#if USENEWSCALE
		return graphicsScale;
	#endif
	// TODO - Remove below code if all goes well.
	if (getTextureWidth(_tempImg)>screenWidth){
		return (screenWidth/scriptScreenWidth);
	}
	return (getTextureWidth(_tempImg)/(float)scriptScreenWidth);
}
float GetYOffsetScale(CrossTexture* _tempImg){
	#if USENEWSCALE
		return graphicsScale;
	#endif
	// TODO - Remove below code if all goes well
	if (getTextureHeight(_tempImg)>screenHeight){
		return (screenHeight/scriptScreenHeight);
	}
	return ( getTextureHeight(_tempImg)/(float)scriptScreenHeight);
}
void DrawBackgroundAlpha(CrossTexture* passedBackground, unsigned char passedAlpha){
	if (passedBackground!=NULL){
		signed int _tempXOffset;
		signed int _tempYOffset;
		GetXAndYOffset(passedBackground,&_tempXOffset,&_tempYOffset);
		drawTextureScaleAlpha(passedBackground,_tempXOffset,_tempYOffset, graphicsScale, graphicsScale, passedAlpha);
	}
}
void DrawBackground(CrossTexture* passedBackground){
	DrawBackgroundAlpha(passedBackground,255);
}
void DrawBust(bust* passedBust){
	signed int _tempXOffset=0;
	signed int _tempYOffset=0;
	if (bustsStartInMiddle){
		GetXAndYOffset(passedBust->image,&_tempXOffset,&_tempYOffset);
	}else{
		if (currentBackground!=NULL){
			GetXAndYOffset(currentBackground,&_tempXOffset,&_tempYOffset);
		}
	}
	if (passedBust->alpha==255){
		drawTextureScale(passedBust->image,_tempXOffset+passedBust->xOffset*passedBust->cacheXOffsetScale,_tempYOffset+passedBust->yOffset*passedBust->cacheYOffsetScale,graphicsScale,graphicsScale);
	}else{
		drawTextureScaleAlpha(passedBust->image,_tempXOffset+passedBust->xOffset*passedBust->cacheXOffsetScale,_tempYOffset+passedBust->yOffset*passedBust->cacheYOffsetScale, graphicsScale, graphicsScale, passedBust->alpha);
	}	
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
void MoveFilePointerPastNewline(CROSSFILE* fp){
	unsigned char _temp;
	crossfread(&_temp,1,1,fp);
	if (_temp==13){
		crossfseek(fp,1,SEEK_CUR);
	}
}
// LAST ARG IS WHERE THE LENGTH IS STORED
unsigned char* ReadNumberStringList(CROSSFILE *fp, unsigned char* arraysize){
	int numScripts;
	char currentReadNumber[4];
	// Add null for atoi
	currentReadNumber[4];
	memset(&currentReadNumber[0], 0, 4);
	crossfread(&currentReadNumber,3,1,fp);
	numScripts = atoi(currentReadNumber);
	MoveFilePointerPastNewline(fp);

	unsigned char* _thelist;
	
	_thelist = calloc(numScripts,sizeof(char));

	int i=0;
	for (i=0;i<numScripts;i++){
		crossfread(&currentReadNumber,3,1,fp);
		_thelist[i]=atoi(currentReadNumber);
		MoveFilePointerPastNewline(fp);
	}

	(*arraysize) = numScripts;
	return _thelist;
}
char** ReadFileStringList(CROSSFILE *fp, unsigned char* arraysize){
	char currentReadLine[200];
	char currentReadNumber[4];
	// Add null for atoi
	currentReadNumber[3]=0;
	int linePosition=0;
	int numScripts;

	currentReadNumber[4]='\0';
	crossfread(&currentReadNumber,3,1,fp);
	numScripts = atoi(currentReadNumber);
	MoveFilePointerPastNewline(fp);
	

	char** _thelist;
	
	_thelist = (char**)calloc(numScripts,sizeof(char*));
	unsigned char justreadbyte=00;

	int i=0;
	for (i=0;i<numScripts;i++){
		while (currentGameStatus!=GAMESTATUS_QUIT){
			if (crossfread(&justreadbyte,1,1,fp)!=1){
				break;
			}
			// Newline char
			// By some black magic, even though newline is two bytes, I can still detect it?
			if (isNewLine(fp,justreadbyte)==1){
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
	CROSSFILE *fp;
	fp = crossfopen(filename, "r");
	//fprintf(fp,"There are %d music deocoders available\n", Mix_GetNumMusicDecoders());
	//int i;

	currentPresetFileList.theArray = ReadFileStringList(fp,&currentPresetFileList.length);
	//for (i=0;i<currentPresetFileList.length;i++){
	//	printf("%s\n",currentPresetFileList.theArray[i]);
	//}

	if (gameHasTips==1){
		currentPresetTipList.theArray = ReadFileStringList(fp,&currentPresetTipList.length);
		//for (i=0;i<currentPresetTipList.length;i++){
		//	printf("%s\n",currentPresetTipList.theArray[i]);
		//}
		//printf("Is %s\n",currentReadNumber);
	
		currentPresetTipUnlockList.theArray = ReadNumberStringList(fp,&(currentPresetTipUnlockList.length));
		//for (i=0;i<currentPresetTipUnlockList.length;i++){
		//	printf("%d\n",currentPresetTipUnlockList.theArray[i]);
		//}
	}

	char tempreadstring[15] = {'\0'};

	if (gameHasTips==1){
		// Check for the
		// tipnames
		// string
		// If it exists, read the tip's names
		if (crossfread(tempreadstring,8,1,fp)==1){
			MoveFilePointerPastNewline(fp);
			if (strcmp(tempreadstring,"tipnames")==0){
				currentPresetTipNameList.theArray = ReadFileStringList(fp,&currentPresetTipNameList.length);
				tipNamesLoaded=1;
			}else{
				tipNamesLoaded=0;
			}
		}else{
			MoveFilePointerPastNewline(fp);
			tipNamesLoaded=0;
		}
	}

	if (crossfread(tempreadstring,12,1,fp)==1){
		MoveFilePointerPastNewline(fp);
		if (strcmp(tempreadstring,"chapternames")==0){
			currentPresetFileFriendlyList.theArray = ReadFileStringList(fp,&currentPresetFileFriendlyList.length);
			chapterNamesLoaded=1;
		}else{
			chapterNamesLoaded=0;
		}
	}else{
		MoveFilePointerPastNewline(fp);
		chapterNamesLoaded=0;
	}

	//chapterNamesLoaded
	//currentPresetFileFriendlyList


	crossfclose(fp);
}
void SetNextScriptName(){
	memset((char*)(nextScriptToLoad),'\0',sizeof(nextScriptToLoad));
	strcpy((char*)nextScriptToLoad,currentPresetFileList.theArray[currentPresetChapter]);
}
// Generates the default data paths for script, presets, etc
// Will use uma0 if possible
void ResetDataDirectory(){
	#if USEUMA0==1
		generateDefaultDataDirectory(&DATAFOLDER,1);
		if (!directoryExists(DATAFOLDER)){
			free(DATAFOLDER);
			generateDefaultDataDirectory(&DATAFOLDER,0);
		}else{
			isActuallyUsingUma0=1;
		}
	#else
		generateDefaultDataDirectory(&DATAFOLDER,0);
	#endif
}
void LazyMessage(const char* stra, const char* strb, const char* strc, const char* strd){
	controlsStart();
	controlsEnd();
	while (currentGameStatus!=GAMESTATUS_QUIT){
		fpsCapStart();
		controlsStart();
		if (wasJustPressed(SCE_CTRL_CROSS)){
			controlsStart();
			controlsEnd();
			break;
		}
		controlsEnd();
		startDrawing();
		if (stra!=NULL){
			goodDrawText(32,5+currentTextHeight*(0+2),stra,fontSize);
		}
		if (strb!=NULL){
			goodDrawText(32,5+currentTextHeight*(1+2),strb,fontSize);
		}
		if (strc!=NULL){
			goodDrawText(32,5+currentTextHeight*(2+2),strc,fontSize);
		}
		if (strd!=NULL){
			goodDrawText(32,5+currentTextHeight*(3+2),strd,fontSize);
		}
		goodDrawText(32,screenHeight-32-currentTextHeight,SELECTBUTTONNAME" to continue.",fontSize);
		endDrawing();
		fpsCapWait();
		exitIfForceQuit();
	}
}
void LoadGame(){
	strcpy((char*)globalTempConcat,saveFolder);
	strcat((char*)globalTempConcat,currentPresetFilename);
	currentPresetChapter=-1;
	if (checkFileExist((char*)globalTempConcat)==1){
		FILE *fp;
		fp = fopen((const char*)globalTempConcat, "r");
		fread(&currentPresetChapter,2,1,fp);
		fclose(fp);
	}
}
void SaveGame(){
	strcpy((char*)globalTempConcat,saveFolder);
	strcat((char*)globalTempConcat,currentPresetFilename);
	FILE *fp;
	fp = fopen((const char*)globalTempConcat, "w");
	fwrite(&currentPresetChapter,2,1,fp);
	fclose(fp);
}
void TryLoadMenuSoundEffect(char* _passedPathIdea){
	if (menuSound!=NULL){
		return;
	}
	char* tempstringconcat;
	if (_passedPathIdea==NULL){
		tempstringconcat = CombineStringsPLEASEFREE(streamingAssets, "SE/","wa_038",".ogg");
	}else{
		tempstringconcat = _passedPathIdea;
	}
	if (checkFileExist(tempstringconcat)){
		menuSoundLoaded=1;
		menuSound = loadSound(tempstringconcat);
		showErrorIfNull(menuSound);
		setSFXVolumeBefore(menuSound,FixSEVolume(256));
	}else{
		menuSoundLoaded=0;
	}
	if (_passedPathIdea==NULL){
		free(tempstringconcat);
	}
}
// realloc, but new memory is zeroed out
void* recalloc(void* _oldBuffer, int _newSize, int _oldSize){
	void* _newBuffer = realloc(_oldBuffer,_newSize);
	if (_newSize > _oldSize){
		void* _startOfNewData = ((char*)_newBuffer)+_oldSize;
		memset(_startOfNewData,0,_newSize-_oldSize);
	}
	return _newBuffer;
}
void updateTextPositions(CrossTexture* _passedBackground){
	if (textOnlyOverBackground){
		if (_passedBackground!=NULL){
			textboxXOffset = floor((float)(screenWidth- applyGraphicsScale(getTextureWidth(_passedBackground)))/2);
			outputLineScreenWidth = screenWidth - textboxXOffset;
		}
	}
	#if PLATFORM == PLAT_3DS
		if (textIsBottomScreen==1){
			outputLineScreenWidth=320;
			outputLineScreenHeight=240;
		}
	#endif
}
void updateGraphicsScale(CrossTexture* _passedBackground){
	#if USENEWSCALE
		if (((double)screenWidth)/getTextureWidth(_passedBackground) < ((double)screenHeight)/getTextureHeight(_passedBackground)){
			graphicsScale = ((double)screenWidth)/getTextureWidth(_passedBackground);
		}else{
			graphicsScale = ((double)screenHeight)/getTextureHeight(_passedBackground);
		}
	#endif
}
void setTextOnlyOverBackground(char _newValue){
	textOnlyOverBackground=_newValue;
	if (gameTextDisplayMode == TEXTMODE_AVD){
		textboxYOffset=screenHeight-ADVBOXHEIGHT;
	}else{
		textboxYOffset=0;
	}
	if (textOnlyOverBackground==0){
		textboxXOffset=0;
		outputLineScreenWidth = screenWidth;
	}else{
		updateTextPositions(currentBackground);
	}
}

// Returns the folder for CG or CGAlt depending on the user's settings
char* getUserPreferredImageDirectory(char _folderPreference){
	if (_folderPreference==LOCATION_CGALT){
		return locationStrings[LOCATION_CGALT];
	}else{
		return locationStrings[LOCATION_CG];
	}
}
// Returns the image folder the user didn't choose.
char* getUserPreferredImageDirectoryFallback(char _folderPreference){
	if (_folderPreference==LOCATION_CGALT){
		return locationStrings[LOCATION_CG];
	}else{
		return locationStrings[LOCATION_CGALT];
	}
}

// Location string fallback with a specific image format
char* _locationStringFallbackFormat(const char* filename, char _folderPreference, char* _fileFormat){
	char* _returnFoundString;
	// Try the user's first choice
	_returnFoundString = CombineStringsPLEASEFREE(streamingAssets, getUserPreferredImageDirectory(_folderPreference),filename,_fileFormat);
	
	if (checkFileExist(_returnFoundString)){
		return _returnFoundString;
	}

	// If not exist, try the other folder.
	free(_returnFoundString);
	_returnFoundString = CombineStringsPLEASEFREE(streamingAssets, getUserPreferredImageDirectoryFallback(_folderPreference),filename,_fileFormat);
	
	if (checkFileExist(_returnFoundString)){
		return _returnFoundString;
	}

	// If the file still doesn't exist, return NULL
	free(_returnFoundString);
	return NULL;
}

char* LocationStringFallback(const char* filename, char _folderPreference, char _extensionIncluded, char _isAllCaps){
	char* _foundFileExtension=NULL;
	char* _workableFilename = malloc(strlen(filename)+1);
	strcpy(_workableFilename,filename);
	
	if (_isAllCaps){
		signed short i=0;
		for (i=0;i<_workableFilename[i]!='\0';i++){
			_workableFilename[i] = toupper(_workableFilename[i]);
		}
	}

	if (_extensionIncluded){
		signed short i;
		short _cachedStrlen = strlen(_workableFilename);
		for (i=_cachedStrlen-1;i>=0;i--){
			if (_workableFilename[i]=='.' && i!=_cachedStrlen-1){
				_foundFileExtension = malloc(strlen(&(_workableFilename[i]))+1);
				strcpy(_foundFileExtension,&(_workableFilename[i]));
				_workableFilename[i]=0;
				break;
			}
		}
	}
	char* _returnFoundString;
	if (_extensionIncluded){
		// Try the included file extension
		_returnFoundString = _locationStringFallbackFormat(_workableFilename,_folderPreference,_foundFileExtension);
		if (_returnFoundString==NULL){
			// If not, try png
			_returnFoundString = _locationStringFallbackFormat(_workableFilename,_folderPreference, _isAllCaps==1 ? ".PNG" : ".png");
			if (_returnFoundString==NULL){
				// If not, try jpg
				_returnFoundString = _locationStringFallbackFormat(_workableFilename,_folderPreference, _isAllCaps==1 ? ".JPG" : ".jpg");
			}
		}
	}else{
		_returnFoundString = _locationStringFallbackFormat(_workableFilename,_folderPreference, _isAllCaps==1 ? ".PNG" : ".png");
		if (_returnFoundString==NULL && _extensionIncluded==0){
			_returnFoundString = _locationStringFallbackFormat(_workableFilename,_folderPreference, _isAllCaps==1 ? ".JPG" : ".jpg");
		}
	}

	if (_foundFileExtension!=NULL){
		free(_foundFileExtension);
	}
	free(_workableFilename);
	return _returnFoundString;
}
// Will load a PNG from CG or CGAlt
CrossTexture* safeLoadGamePNG(const char* filename, char _folderPreference, char _extensionIncluded){
	char* _tempFoundFilename;
	_tempFoundFilename = LocationStringFallback(filename,_folderPreference,_extensionIncluded,currentlyVNDSGame);
	if (_tempFoundFilename==NULL){
		LazyMessage("Image not found.",filename,"What will happen now?!",NULL);
		return NULL;
	}
	CrossTexture* _returnLoadedPNG = SafeLoadPNG(_tempFoundFilename);
	free(_tempFoundFilename);
	return _returnLoadedPNG;
}
//===================
void FadeBustshot(int passedSlot,int _time,char _wait){
	if (isSkipping==1){
		_time=0;
	}

	//int passedSlot = nathanvariableToInt(&_passedArguments[1)-1;
	//Busts[passedSlot].bustStatus = BUST_STATUS_FADEOUT;
	//Busts[passedSlot].statusVariable = 
	Busts[passedSlot].alpha=0;
	Busts[passedSlot].bustStatus = BUST_STATUS_FADEOUT;
	if (_time!=0){
		Busts[passedSlot].alpha=255;
		//int _time = floor(nathanvariableToInt(&_passedArguments[7));
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
				fpsCapStart();
				controlsStart();
				Update();
				startDrawing();
				Draw(MessageBoxEnabled);
				endDrawing();
				if (wasJustPressed(SCE_CTRL_CROSS)){
					Busts[passedSlot].alpha = 1;
				}
				controlsEnd();
				fpsCapWait();
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
			fpsCapStart();
			controlsStart();
			Update();
			startDrawing();
			Draw(MessageBoxEnabled);
			endDrawing();
			if (wasJustPressed(SCE_CTRL_CROSS)){
				for (i=0;i<MAXBUSTS;i++){
					if (Busts[i].isActive==1){
						Busts[i].alpha=1;
					}
				}
			}
			controlsEnd();
			fpsCapWait();
		}
	}
}
void DrawScene(const char* _filename, int time){
	if (isSkipping==1){
		time=0;
	}
	int _alphaPerFrame=255;
	int i=0;
	signed short _backgroundAlpha=0;

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

	if (lastBackgroundFilename!=NULL){
		if (strcmp(lastBackgroundFilename,_filename)==0){
			for (i=0;i<MAXBUSTS;i++){
				if (Busts[i].isActive==1 && Busts[i].lineCreatedOn != currentScriptLine-1){
					ResetBustStruct(&Busts[i], 1);
				}
			}
			return;
		}
	}

	changeMallocString(&lastBackgroundFilename,_filename);
	CrossTexture* newBackground = safeLoadGamePNG(_filename,graphicsLocation,scriptUsesFileExtensions);
	if (newBackground==NULL){
		freeTexture(currentBackground);
		currentBackground=NULL;
		return;
	}
	updateGraphicsScale(newBackground);
	updateTextPositions(newBackground);
	while (_backgroundAlpha<255){
		fpsCapStart();

		Update();
		_backgroundAlpha+=_alphaPerFrame;
		if (_backgroundAlpha>255){
			_backgroundAlpha=255;
		}
		startDrawing();
		
		if (currentBackground!=NULL){
			DrawBackground(currentBackground);
		}
		
		for (i = MAXBUSTS-1; i != -1; i--){
			if (bustOrder[i]!=255 && Busts[bustOrder[i]].isActive==1  && Busts[bustOrder[i]].lineCreatedOn != currentScriptLine-1){
				DrawBust(&(Busts[bustOrder[i]]));
			}
		}
		if (MessageBoxEnabled==1){
			DrawMessageBox();
		}
		for (i = MAXBUSTS-1; i != -1; i--){
			if (bustOrderOverBox[i]!=255 && Busts[bustOrderOverBox[i]].isActive==1 && Busts[bustOrderOverBox[i]].lineCreatedOn != currentScriptLine-1){
				DrawBust(&(Busts[bustOrderOverBox[i]]));
			}
		}

		
		DrawBackgroundAlpha(newBackground,_backgroundAlpha);
		
		for (i = MAXBUSTS-1; i != -1; i--){
			if (bustOrder[i]!=255 && Busts[bustOrder[i]].isActive==1  && Busts[bustOrder[i]].lineCreatedOn == currentScriptLine-1){
				DrawBust(&(Busts[bustOrder[i]]));
			}
		}

		if (filterActive==1){
			DrawCurrentFilter();
		}
		if (MessageBoxEnabled==1){
			DrawMessageText();
		}
		endDrawing();

		controlsStart();
		if (wasJustPressed(SCE_CTRL_CROSS)){
			_backgroundAlpha=254;
		}
		controlsEnd();

		fpsCapWait();
	}

	for (i=0;i<MAXBUSTS;i++){
		if (Busts[i].isActive==1 && Busts[i].lineCreatedOn != currentScriptLine-1){
			ResetBustStruct(&Busts[i], 1);
		}
	}

	if (currentBackground!=NULL){
		freeTexture(currentBackground);
		currentBackground=NULL;
	}
	currentBackground=newBackground;
}
void MoveBustSlot(unsigned char _sourceSlot, unsigned char _destSlot){
	ResetBustStruct(&(Busts[_destSlot]),1);
	memcpy(&(Busts[_destSlot]),&(Busts[_sourceSlot]),sizeof(bust));
	ResetBustStruct(&(Busts[_sourceSlot]),0);
	RecalculateBustOrder();
}
void DrawBustshot(unsigned char passedSlot, const char* _filename, int _xoffset, int _yoffset, int _layer, int _fadeintime, int _waitforfadein, int _isinvisible){
	if (passedSlot>=MAXBUSTS){
		printf("Increase max bust array to %d\n",passedSlot+1);
		int _oldMaxBusts = MAXBUSTS;
		MAXBUSTS = passedSlot+1;
		Busts = recalloc(Busts, MAXBUSTS * sizeof(bust), _oldMaxBusts * sizeof(bust));
		bustOrder = recalloc(bustOrder, MAXBUSTS * sizeof(char), _oldMaxBusts * sizeof(char));
		bustOrderOverBox = recalloc(bustOrderOverBox, MAXBUSTS * sizeof(char), _oldMaxBusts * sizeof(char));
		RecalculateBustOrder();
	}
	if (isSkipping==1 || _fadeintime==0){
		_fadeintime=0;
		_waitforfadein=0;
	}
	startDrawing();
	Draw(MessageBoxEnabled);
	endDrawing();
	int i;
	unsigned char skippedInitialWait=0;
	ResetBustStruct(&(Busts[passedSlot]),1); 
	Busts[passedSlot].image = safeLoadGamePNG(_filename,graphicsLocation,scriptUsesFileExtensions);
	Busts[passedSlot].relativeFilename = mallocForString(_filename);
	if (Busts[passedSlot].image==NULL){
		ResetBustStruct(&(Busts[passedSlot]),1);
		return;
	}
	Busts[passedSlot].xOffset = _xoffset;
	Busts[passedSlot].yOffset = _yoffset;
	Busts[passedSlot].cacheXOffsetScale = GetXOffsetScale(Busts[passedSlot].image);
	Busts[passedSlot].cacheYOffsetScale = GetYOffsetScale(Busts[passedSlot].image);

	if (_isinvisible!=0){
		Busts[passedSlot].isInvisible=1;
	}else{
		Busts[passedSlot].isInvisible=0;
	}
	Busts[passedSlot].layer = _layer;
	// The lineCreatedOn variable is used to know if the bustshot should stay after a scene change. The bustshot can only stay after a scene change if it's created the line before the scene change AND it doesn't wait for fadein completion.
	if (_waitforfadein==0){
		Busts[passedSlot].lineCreatedOn = currentScriptLine;
	}else{
		Busts[passedSlot].lineCreatedOn = 0;
	}
	Busts[passedSlot].isActive=1;
	RecalculateBustOrder();
	if (_fadeintime!=0){
		Busts[passedSlot].alpha=0;
		int _timeTotal = _fadeintime;
		int _time = floor(_fadeintime/2);
		int _totalFrames = floor(60*(_time/(double)1000));
		if (_totalFrames==0){
			_totalFrames=1;
		}
		int _alphaPerFrame=floor(255/_totalFrames);
		if (_alphaPerFrame==0){
			_alphaPerFrame=1;
		}
		Busts[passedSlot].statusVariable=_alphaPerFrame;
		Busts[passedSlot].statusVariable2 = _timeTotal -_time;
		Busts[passedSlot].bustStatus = BUST_STATUS_FADEIN;
	}else{
		Busts[passedSlot].bustStatus = BUST_STATUS_NORMAL;
	}

	i=1;
	if (_waitforfadein==1){
		while (Busts[passedSlot].alpha<255){
			fpsCapStart();
			controlsStart();
			Update();
			if (Busts[passedSlot].alpha>255){
				Busts[passedSlot].alpha=255;
			}
			startDrawing();
			Draw(MessageBoxEnabled);
			endDrawing();
			if (wasJustPressed(SCE_CTRL_CROSS) || skippedInitialWait==1){
				Busts[passedSlot].alpha = 255;
				Busts[passedSlot].bustStatus = BUST_STATUS_NORMAL;
				startDrawing();
				Draw(MessageBoxEnabled);
				endDrawing(); // Draw once more with the bust gone.
				controlsEnd();
				break;
			}
			controlsEnd();
			i++;
			fpsCapWait();
		}
	}
}
// strcpy, but it won't copy from src to dest if the value is 1.
// You can use this to exclude certian spots
// I do not mean the ASCII character 1, which is 49.
void strcpyNO1(char* dest, const char* src){
	int i;
	int _destCopyOffset=0;
	int _srcStrlen = strlen(src);
	for (i=0;i<_srcStrlen;i++){
		if (src[i]!=1){
			dest[_destCopyOffset++]=src[i];
		}
	}
	dest[_destCopyOffset++]=0;
}
// Same as strlen, but doesn't count any places with the value of 1 as a character.
int strlenNO1(char* src){
	int len=0;
	int i;
	for (i=0;;i++){
		if (src[i]=='\0'){
			break;
		}else if (src[i]!=1){
			len++;
		}
	}
	return len;
}
#if PLATFORM == PLAT_COMPUTER
	int _LagTestStart;
	void LagTestStart(){
		_LagTestStart = getTicks();
	}
	void LagTestEnd(){
		printf("lagometer: %d\n",getTicks()-_LagTestStart);
	}
#endif
char* getSpecificPossibleSoundFilename(const char* _filename, char* _folderName){
	char* tempstringconcat = CombineStringsPLEASEFREE(streamingAssets, _folderName, _filename, ".ogg");
	if (checkFileExist(tempstringconcat)==1){
		return tempstringconcat;
	}
	tempstringconcat[strlen(streamingAssets)+strlen(_folderName)+strlen(_filename)]='\0';
	strcat(tempstringconcat,".wav");
	if (checkFileExist(tempstringconcat)==1){
		return tempstringconcat;
	}
	if (scriptUsesFileExtensions){
		tempstringconcat[strlen(streamingAssets)+strlen(_folderName)+strlen(_filename)]='\0';
		if (checkFileExist(tempstringconcat)==1){
			return tempstringconcat;
		}
	}
	free(tempstringconcat);
	return NULL;
}
#define PREFER_DIR_BGM 0
#define PREFER_DIR_SE 1
#define PREFER_DIR_VOICE 2
// Will use fallbacks
// Return NULL if file not exist
char* getSoundFilename(const char* _filename, char _preferedDirectory){
	char* tempstringconcat;
	if (_preferedDirectory==PREFER_DIR_BGM){
		tempstringconcat = getSpecificPossibleSoundFilename(_filename,"BGM/");
		if (tempstringconcat==NULL){
			tempstringconcat = getSpecificPossibleSoundFilename(_filename,"SE/");
			if (tempstringconcat==NULL){
				tempstringconcat = getSpecificPossibleSoundFilename(_filename,"voice/");
			}
		}
	}else if (_preferedDirectory==PREFER_DIR_SE){
		tempstringconcat = getSpecificPossibleSoundFilename(_filename,"SE/");
		if (tempstringconcat==NULL){
			tempstringconcat = getSpecificPossibleSoundFilename(_filename,"BGM/");
			if (tempstringconcat==NULL){
				tempstringconcat = getSpecificPossibleSoundFilename(_filename,"voice/");
			}
		}
	}else if (_preferedDirectory==PREFER_DIR_VOICE){
		tempstringconcat = getSpecificPossibleSoundFilename(_filename,"voice/");
		if (tempstringconcat==NULL){
			tempstringconcat = getSpecificPossibleSoundFilename(_filename,"SE/");
			if (tempstringconcat==NULL){
				tempstringconcat = getSpecificPossibleSoundFilename(_filename,"BGM/");
			}
		}
	}else{
		printf("Invalid preference %d\n",_preferedDirectory);
	}
	return tempstringconcat;
}
// _dirRelativeToStreamingAssetsNoEndSlash should start AND END with a slash
// Example
// /SE/
// DO NOT FIX THE SE VOLUME BEFORE PASSING ARGUMENT
void GenericPlaySound(int passedSlot, const char* filename, int unfixedVolume, char _preferedDirectory, float _passedVolumeFixScale){
	if (passedSlot>=MAXSOUNDEFFECTARRAY){
		LazyMessage("Sound effect slot too high.","No action will be taken.",NULL,NULL);
		return;
	}
	if (strlen(filename)==0){
		printf("Sound effect filename empty.\n");
		return;
	}
	if (soundEffects[passedSlot]!=NULL){
		freeSound(soundEffects[passedSlot]);
		soundEffects[passedSlot]=NULL;
	}
	// Play WAV version if found.
	char* tempstringconcat = getSoundFilename(filename,_preferedDirectory);
	if (tempstringconcat==NULL){
		WriteToDebugFile("SE file not found");
		WriteToDebugFile(tempstringconcat);
	}else{
		soundEffects[passedSlot] = loadSound(tempstringconcat);
		//setSFXVolume(soundEffects[passedSlot],FixSEVolume(unfixedVolume));
		CROSSPLAYHANDLE _tempHandle = playSound(soundEffects[passedSlot],1,passedSlot+10);
		setSFXVolume(_tempHandle,GenericFixSpecificVolume(unfixedVolume,_passedVolumeFixScale));
	}
	free(tempstringconcat);
}
void OutputLine(const unsigned char* _tempMsg, char _endtypetemp, char _autoskip){
	if (strlen(_tempMsg)==0){
		return;
	}

	// 1 when finished displaying the text
	char _isDone=0;
	if (isSkipping==1 || _autoskip==1 || textSpeed==TEXTSPEED_INSTANT){
		_isDone=1;
	}
	MessageBoxEnabled=1;

	unsigned char message[strlen(_tempMsg)+1+strlen(currentMessages[currentLine])];
	// This will make the start of the message have whatever the start of the line says.
	// For example, if the line we're writing to already has "Keiichi is an idiot" written on it, that will be copied to the start of this message.
	strcpy(message,currentMessages[currentLine]);
	strcat(message,_tempMsg);
	int totalMessageLength=strlen(message);
	//printf("Total assembled: (START)%s(END)\n",message);
	if (totalMessageLength==0){
		endType = _endtypetemp;
		return;
	}
	// These are used when we're displaying the message to the user
	// Refer to the while loop near the end of this function.
	int _currentDrawLine = currentLine;
	int _currentDrawChar = GetNextCharOnLine(currentLine);
	int i, j;
	// This will loop through the entire message, looking for where I need to add new lines. When it finds a spot that
	// needs a new line, that spot in the message will become 0. So, when looking for the place to 
	int lastNewlinePosition=-1; // If this doesn't start at -1, the first character will be cut off. lastNewlinePosition+1 is always used, so a negative index won't be a problem.
	for (i = strlen(currentMessages[currentLine]); i < totalMessageLength; i++){
		if (message[i]==32){ // Only check when we meet a space. 32 is a space in ASCII
			message[i]='\0';
			// Check if the text has gone past the end of the screen OR we're out of array space for this line
			if (textWidth(fontSize,&(message[lastNewlinePosition+1]))>=outputLineScreenWidth-textboxXOffset-MESSAGEEDGEOFFSET-messageInBoxXOffset || i-lastNewlinePosition>=SINGLELINEARRAYSIZE-1){
				char _didWork=0;
				for (j=i-1;j>lastNewlinePosition+1;j--){
					//printf("J:%d, M:%c\n",j,message[j]);
					if (message[j]==32){
						// j in message will now be the end point
						message[j]='\0';
						_didWork=1;
						// Fix the thing we messed up, i is no longer the end point
						message[i]=32;
						// This code would copy 
						strcpyNO1(currentMessages[currentLine],&(message[lastNewlinePosition+1]));
						lastNewlinePosition=j;
						currentLine++;
						break;
					}
				}
				if (_didWork==0){
					//oh well. That's fine. We'll just copy it anyway.
					strcpyNO1(currentMessages[currentLine],&(message[lastNewlinePosition+1]));
					lastNewlinePosition=i;
					currentLine++;
				}
				changeIfLazyLastLineFix(&currentLine, &_currentDrawLine);
			}else{
				message[i]=32;
			}
		}else{
			// Don't do special checks if it is a normal English ASCII character
			// Here we have special checks for stuff like image characters and new lines.
			if (message[i]<65 || message[i]>122){
				if (message[i]=='<'){
					int k;
					// Loop and look for the end
					for (k=i+1;k<i+100;k++){
						if (message[k]=='>'){
							break;
						}
					}
					if (k!=i+100){
						memset(&(message[i]),1,k-i+1); // Because this starts at i, k being 11 with i as 10 would just write 1 byte, therefor missing the end '>'. THe fix is to add one.
						i+=(k-i-1);
					}
				}else if (message[i]==226 && message[i+1]==128 && message[i+2]==148){ // Weird hyphen replace
					memset(&(message[i]),45,1); // Replace it with a normal hyphen
					memset(&(message[i+1]),1,2); // Replace these with value 1
					i=i+2;
				}else if (message[i]==226){ // COde for special image character
					unsigned char _imagechartype;
					if (message[i+1]==153 && message[i+2]==170){ // ♪
						_imagechartype = IMAGECHARNOTE;
					}else if (message[i+1]==152 && message[i+2]==134){ // ☆
						_imagechartype = IMAGECHARSTAR;
					}else{
						printf("Unknown image char! %d;%d\n",message[i+1],message[i+2]);
						_imagechartype = IMAGECHARUNKNOWN;
					}
					message[i]=0; // So we can use textWidth
					for (j=0;j<MAXIMAGECHAR;j++){
						if (imageCharType[j]==-1){
							imageCharX[j] = textWidth(fontSize,&(message[lastNewlinePosition+1]))+textboxXOffset+messageInBoxXOffset;
							imageCharY[j] = messageInBoxYOffset+12+textboxYOffset+currentLine*(currentTextHeight);
							imageCharLines[j] = currentLine;
							message[i]='\0';
							imageCharCharPositions[j] = strlenNO1(&(message[lastNewlinePosition+1]));
							imageCharType[j] = _imagechartype;
							//printf("Asssigned line %d and pos %d\n",imageCharLines[j],imageCharCharPositions[j]);
							break;
						}
					}
					memset(&(message[i]),32,3);
					i+=2;
				}else if (message[i]=='\n'){
					message[i]='\0';
					strcpyNO1(currentMessages[currentLine],&(message[lastNewlinePosition+1]));
					currentLine++;
					lastNewlinePosition=i;
				}
			}
		}

		changeIfLazyLastLineFix(&currentLine, &_currentDrawLine);
	}
	// This code will make a new line if there needs to be one because of the last word
	if (textWidth(fontSize,&(message[lastNewlinePosition+1]))>=outputLineScreenWidth-textboxXOffset-MESSAGEEDGEOFFSET-messageInBoxXOffset){
		char _didWork=0;
		for (j=totalMessageLength-1;j>lastNewlinePosition+1;j--){
			if (message[j]==32){
				// WWWWWWWWWWWWWWWWWWWWWWWW MION
				message[j]='\0';
				// Copy stuff before the split, this would copy the W characters
				strcpyNO1(currentMessages[currentLine],&(message[lastNewlinePosition+1]));
				currentLine++;
				changeIfLazyLastLineFix(&currentLine, &_currentDrawLine);
				lastNewlinePosition=j;
				_didWork=1;
				// Copy stuff after the split, this would copy the MION
				strcpyNO1(currentMessages[currentLine],&(message[lastNewlinePosition+1]));
				break;
			}
		}
		// Were we able to find a place to put a new line? If not, do this.
		if (_didWork==0){
			printf("did not work.\n");
			// Just put as much as possible on one line.
			for (i=lastNewlinePosition+1;i<totalMessageLength;i++){
				char _tempCharCache = message[i];
				message[i]='\0';
				if (textWidth(fontSize,&(message[lastNewlinePosition+1]))>outputLineScreenWidth-textboxXOffset-MESSAGEEDGEOFFSET-messageInBoxXOffset){
					// What this means is that when only the string UP TO the last character was small enough. Now we have to replicate the behavior of the previous loop to get the shorter string.
					char _tempCharCache2 = message[i-1];
					message[i-1]='\0';
					strcpyNO1(currentMessages[currentLine],&(message[lastNewlinePosition+1]));
					message[i-1]=_tempCharCache2;
					currentLine++;
					changeIfLazyLastLineFix(&currentLine, &_currentDrawLine);
					lastNewlinePosition=i-2;
				}else{
					//printf("%d;%s\n",textWidth(fontSize,&(message[lastNewlinePosition+1])));
				}
				message[i] = _tempCharCache;
			}
			// Copy whatever remains
			strcpyNO1(currentMessages[currentLine],&(message[lastNewlinePosition+1]));
		}
	}else{
		// Copy whatever is left. In a
		// WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW(\n)NOOB
		// example, NOOB will be copied. This is seperate from the code above. NOOB by itself does not need a new line.
		strcpyNO1(currentMessages[currentLine],&(message[lastNewlinePosition+1]));
	}
	changeIfLazyLastLineFix(&currentLine, &_currentDrawLine);
	if (_currentDrawLine<0){
		_currentDrawLine=0;
	}
	#if PLATFORM == PLAT_3DS
		int _oldMessageXOffset=textboxXOffset;
		int _oldMessageInBoxXOffset=messageInBoxXOffset;
		int _oldMessageYOffset=textboxYOffset;
		int _oldMessageInBoxYOffset=messageInBoxYOffset;
		if (textIsBottomScreen==1){
			textboxXOffset=0;
			messageInBoxXOffset=0;
			textboxYOffset=0;
			messageInBoxYOffset=0;
		}
	#endif
	char _slowTextSpeed=0;
	while(_isDone==0){
		#if PLATFORM != PLAT_VITA
			fpsCapStart();
		#endif

		// The first one that takes two bytes is U+0080, or 0xC2 0x80
		// If it is a two byte character, we don't want to try and draw when there's only one byte. Skip to include the next one.
		if (currentMessages[_currentDrawLine][_currentDrawChar]>=0xC2){
			_currentDrawChar++;
		}

		controlsStart();
		if (wasJustPressed(SCE_CTRL_CROSS) || capEnabled==0){
			_isDone=1;
		}
		updateControlsGeneral();
		controlsEnd();
		
		startDrawing();
		if (currentBackground!=NULL){
			DrawBackground(currentBackground);
		}
		
		for (i = MAXBUSTS-1; i != -1; i--){
			if (bustOrder[i]!=255 && Busts[bustOrder[i]].isActive==1){
				DrawBust(&(Busts[bustOrder[i]]));
			}
		}
		if (filterActive==1){
			DrawCurrentFilter();
		}
		if (MessageBoxEnabled==1){
			DrawMessageBox();
		}
		
		for (i = MAXBUSTS-1; i != -1; i--){
			if (bustOrderOverBox[i]!=255 && Busts[bustOrderOverBox[i]].isActive==1){
				DrawBust(&(Busts[bustOrderOverBox[i]]));
			}
		}
		#if PLATFORM == PLAT_3DS
			if (textIsBottomScreen==1){
				startDrawingBottom();
			}
		#endif
		if (MessageBoxEnabled==1){
			char _tempCharCache = currentMessages[_currentDrawLine][_currentDrawChar+1];
			currentMessages[_currentDrawLine][_currentDrawChar+1]='\0';
			for (i = 0; i <= _currentDrawLine; i++){
				goodDrawText(textboxXOffset+messageInBoxXOffset,12+messageInBoxYOffset+textboxYOffset+i*(currentTextHeight),(char*)currentMessages[i],fontSize);
			}
			currentMessages[_currentDrawLine][_currentDrawChar+1]=_tempCharCache;
			for (i=0;i<MAXIMAGECHAR;i++){
				if (imageCharType[i]!=-1){
					if ((imageCharLines[i]<_currentDrawLine) || (imageCharLines[i]==_currentDrawLine && imageCharCharPositions[i]<=_currentDrawChar)){
						drawTextureScale(imageCharImages[imageCharType[i]],imageCharX[i],imageCharY[i],((double)textWidth(fontSize,IMAGECHARSPACESTRING)/ getTextureWidth(imageCharImages[imageCharType[i]])),((double)textHeight(fontSize)/getTextureHeight(imageCharImages[imageCharType[i]])));
					}
				}
			}
		}
		endDrawing();
		if (_isDone==0 && ( (textSpeed>0) || (_slowTextSpeed++ == abs(textSpeed)) )){
			_slowTextSpeed=0;
			for (i=0;i<(textSpeed>0 ? textSpeed : 1);i++){
				_currentDrawChar++;
				// If the next char we're about to display is the end of the line
				if (currentMessages[_currentDrawLine][_currentDrawChar]=='\0'){
					_currentDrawLine++;
					// If we just passed the line we'll be writing to next time then we're done
					if (_currentDrawLine==currentLine+1){
						_isDone=1; // We will no longer increment the current character
						_currentDrawLine-=1; // Fix this variable as we passed where we wanted to be
						_currentDrawChar=strlen(currentMessages[_currentDrawLine]); // The character we're displaying is at the end
						break;
					}else{ // Otherwise, start displaying at the start of the next line
						_currentDrawChar=0;
					}
				}
			}
		}
		#if PLATFORM != PLAT_VITA
			fpsCapWait();
		#endif
	}
	#if PLATFORM == PLAT_3DS
		if (textIsBottomScreen==1){
			textboxXOffset=_oldMessageXOffset;
			messageInBoxXOffset=_oldMessageInBoxXOffset;
			textboxYOffset=_oldMessageYOffset;
			messageInBoxYOffset=_oldMessageInBoxYOffset;
		}
	#endif
	// End of function
	endType = _endtypetemp;
}
void FreeBGM(int _slot){
	#if PLATFORM == PLAT_3DS
		// Wait for BGM to be unlocked.
		while (_bgmIsLock){
			wait(1);
		}
		_bgmIsLock = 1;
	#endif
	if (currentMusic[_slot]!=NULL){
		stopMusic(currentMusicHandle[_slot]);
		freeMusic(currentMusic[_slot]);
		currentMusic[_slot]=NULL;
		currentMusicHandle[_slot]=0;
		if (currentMusicFilepath[_slot]!=NULL){
			free(currentMusicFilepath[_slot]);
			currentMusicFilepath[_slot]=NULL;
			currentMusicUnfixedVolume[_slot] = 0;
		}
	}
	#if PLATFORM == PLAT_3DS
		_bgmIsLock = 0;
	#endif
}
void StopBGM(int _slot){
	#if PLATFORM == PLAT_3DS
		// Wait for BGM to be unlocked.
		while (_bgmIsLock){
			wait(1);
		}
	#endif
	if (currentMusic[_slot]!=NULL){
		stopMusic(currentMusicHandle[_slot]);
	}
}
// Unfixed bgm
void PlayBGM(const char* filename, int _volume, int _slot){
	#if PLATFORM == PLAT_3DS
		// Wait for BGM to be unlocked.
		while (_bgmIsLock){
			wait(1);
		}
		_bgmIsLock = 1;
	#endif
	if (bgmVolume==0){
		printf("BGM volume is 0, ignore music change.");
		return;
	}
	if (_slot>=MAXMUSICARRAY){
		LazyMessage("Music slot too high.","No action will be taken.",NULL,NULL);
		return;
	}
	char* tempstringconcat = getSoundFilename(filename,PREFER_DIR_BGM);
	if (tempstringconcat==NULL){
		printf("BGM file not found.\n");
		FreeBGM(_slot);
	}else{
		char* _tempHoldFilepathConcat = malloc(strlen(filename)+1);
		strcpy(_tempHoldFilepathConcat,filename);
		CROSSMUSIC* _tempHoldSlot = loadMusic(tempstringconcat);
		showErrorIfNull(_tempHoldSlot);
		// FreeBGM is right here so the player can listen to the old BGM as the new one loads.
		FreeBGM(_slot);
		currentMusic[_slot] = _tempHoldSlot;
		currentMusicFilepath[_slot] = _tempHoldFilepathConcat;
		currentMusicUnfixedVolume[_slot] = _volume;
		currentMusicHandle[_slot] = playMusic(currentMusic[_slot],_slot);
		setMusicVolume(currentMusicHandle[_slot],FixBGMVolume(_volume));
		lastBGMVolume=_volume;
		
		free(tempstringconcat);
	}
	#if PLATFORM == PLAT_3DS
		_bgmIsLock = 0;
	#endif
	return;
}
// Settings file format:
// OPTIONSFILEFORMAT, 1 byte
// cpuOverclocked, 1 byte
// graphicsLocation, 1 byte
// autoModeWait, 4 bytes
// BGM volume, 1 byte, multiply it by 4 so it's a whole number when writing to save file
// SE volume, 1 byte, multiply it by 4 so it's a whole number when writing to save file
// Voice volume, 1 byte, multiply it by 4 so it's a whole number when writing to save file
// MessageBoxAlpha, 1 byte
// textOnlyOverBackground, 1 byte
// textSpeed, 1 byte
// vndsClearAtBottom, 1 byte
void SaveSettings(){
	FILE* fp;
	fixPath("settings.noob",globalTempConcat,TYPE_DATA);
	fp = fopen ((const char*)globalTempConcat, "w");

	unsigned char _bgmTemp = floor(bgmVolume*4);
	unsigned char _seTemp = floor(seVolume*4);
	unsigned char _voiceTemp = floor(voiceVolume*4);

	unsigned char _tempOptionsFormat = OPTIONSFILEFORMAT;
	fwrite(&_tempOptionsFormat,1,1,fp);
	fwrite(&cpuOverclocked,1,1,fp);
	fwrite(&graphicsLocation,1,1,fp);
	fwrite(&autoModeWait,4,1,fp);

	fwrite(&_bgmTemp,1,1,fp);
	fwrite(&_seTemp,1,1,fp);
	fwrite(&_voiceTemp,1,1,fp);
	fwrite(&MessageBoxAlpha,1,1,fp);
	fwrite(&textOnlyOverBackground,1,1,fp);
	fwrite(&textSpeed,1,1,fp);
	fwrite(&vndsClearAtBottom,sizeof(signed char),1,fp);

	fclose(fp);
	printf("SAved settings file.\n");
}
void LoadSettings(){
	fixPath("fontsize.noob",globalTempConcat,TYPE_DATA);
	if (checkFileExist((const char*)globalTempConcat)==0){
		SetDefaultFontSize();
	}else{
		LoadFontSizeFile();
	}
	fixPath("settings.noob",globalTempConcat,TYPE_DATA);
	if (checkFileExist((const char*)globalTempConcat)==1){
		FILE* fp;
		fp = fopen ((const char*)globalTempConcat, "r");
		unsigned char _tempOptionsFormat = 255;
		// This is the version of the format of the options file.
		fread(&_tempOptionsFormat,1,1,fp);
		if (_tempOptionsFormat>=1){
			fread(&cpuOverclocked,1,1,fp);
			fread(&graphicsLocation,1,1,fp);
			fread(&autoModeWait,4,1,fp);
		}
		if (_tempOptionsFormat>=2){
			unsigned char _bgmTemp;
			unsigned char _seTemp;

			fread(&_bgmTemp,1,1,fp);
			fread(&_seTemp,1,1,fp);
			bgmVolume = (float)_bgmTemp/4;
			seVolume = (float)_seTemp/4;
		}
		if (_tempOptionsFormat>=3){
			unsigned char _voiceTemp;
			fread(&_voiceTemp,1,1,fp);
			voiceVolume = (float)_voiceTemp/4;
		}
		if (_tempOptionsFormat>=4){
			fread(&MessageBoxAlpha,1,1,fp);
			fread(&textOnlyOverBackground,1,1,fp);
		}
		if (_tempOptionsFormat>=5){
			fread(&textSpeed,1,1,fp);
		}
		if (_tempOptionsFormat>=6){
			fread(&vndsClearAtBottom,sizeof(signed char),1,fp);
		}
		fclose(fp);

		if (cpuOverclocked==1){
			#if PLATFORM == PLAT_VITA
				scePowerSetArmClockFrequency(444);
			#endif
			#if PLATFORM == PLAT_3DS
				outputLineScreenWidth = 320;
				outputLineScreenHeight = 240;
			#endif
		}
		setTextOnlyOverBackground(textOnlyOverBackground);
		printf("Loaded settings file.\n");
	}
}
#define HISTORYSCROLLBARHEIGHT (((double)HISTORYONONESCREEN/(double)MAXMESSAGEHISTORY)*screenHeight)
//#define HISTORYSCROLLRATE (floor((double)MAXMESSAGEHISTORY/15))
#define HISTORYSCROLLRATE 1
void DrawHistory(unsigned char _textStuffToDraw[][SINGLELINEARRAYSIZE]){
	controlsEnd();
	int _noobHeight = textHeight(fontSize);
	int _controlsStringWidth = textWidth(fontSize,"UP and DOWN to scroll, "BACKBUTTONNAME" to return");
	int _scrollOffset=MAXMESSAGEHISTORY-HISTORYONONESCREEN;

	int i;
	while (1){
		fpsCapStart();

		controlsStart();
		if (wasJustPressed(SCE_CTRL_UP) || wasJustPressed(SCE_CTRL_LEFT)){
			_scrollOffset-=wasJustPressed(SCE_CTRL_LEFT) ? HISTORYSCROLLRATE*2 : HISTORYSCROLLRATE;;
			if (_scrollOffset<0){
				_scrollOffset=0;
			}
		}
		if (wasJustPressed(SCE_CTRL_DOWN) || wasJustPressed(SCE_CTRL_RIGHT)){
			_scrollOffset+=wasJustPressed(SCE_CTRL_RIGHT) ? HISTORYSCROLLRATE*2 : HISTORYSCROLLRATE;
			if (_scrollOffset>MAXMESSAGEHISTORY-HISTORYONONESCREEN){
				_scrollOffset=MAXMESSAGEHISTORY-HISTORYONONESCREEN;
			}
		}
		if (wasJustPressed(SCE_CTRL_CIRCLE) || wasJustPressed(SCE_CTRL_START)){
			controlsEnd();
			break;
		}
		controlsEnd();

		startDrawing();

		Draw(0);



		drawRectangle(textboxXOffset,0,outputLineScreenWidth-textboxXOffset,screenHeight,0,230,255,200);
		for (i = 0; i < HISTORYONONESCREEN; i++){
			goodDrawTextColored(textboxXOffset,textHeight(fontSize)+i*(textHeight(fontSize)),(const char*)_textStuffToDraw[FixHistoryOldSub(i+_scrollOffset,oldestMessage)],fontSize,0,0,0);
		}
		if (outputLineScreenWidth == screenWidth){
			goodDrawTextColored(3,screenHeight-_noobHeight-5,"TEXTLOG",fontSize,0,0,0);
			goodDrawTextColored(screenWidth-10-_controlsStringWidth,screenHeight-_noobHeight-5,"UP and DOWN to scroll, "BACKBUTTONNAME" to return",fontSize,0,0,0);
		}
		drawRectangle((screenWidth-5),0,5,screenHeight,0,0,0,255);
		drawRectangle((screenWidth-5),floor((screenHeight-HISTORYSCROLLBARHEIGHT)*((double)_scrollOffset/(MAXMESSAGEHISTORY-HISTORYONONESCREEN))),5,HISTORYSCROLLBARHEIGHT,255,0,0,255);

		endDrawing();

		fpsCapWait();
	}
}
void ChangeEasyTouchMode(int _newControlValue){
	controlsStart();
	controlsEnd();
	easyTouchControlMode = _newControlValue;
}

// FOLDER NAME SHOULD NOT END WITH SLASH
void GenerateStreamingAssetsPaths(char* _streamingAssetsFolderName, char _isRelativeToData){
	free(streamingAssets);
	free(scriptFolder);

	if (_isRelativeToData){
		streamingAssets = malloc(strlen(DATAFOLDER)+strlen(_streamingAssetsFolderName)+2);
		strcpy(streamingAssets,DATAFOLDER);
		strcat(streamingAssets,_streamingAssetsFolderName);
		strcat(streamingAssets,"/");
	
		scriptFolder = malloc(strlen(DATAFOLDER)+strlen(_streamingAssetsFolderName)+strlen("/Scripts/")+1);
		strcpy(scriptFolder,DATAFOLDER);
		strcat(scriptFolder,_streamingAssetsFolderName);
		strcat(scriptFolder,"/Scripts/");
	}else{
		streamingAssets = malloc(strlen(_streamingAssetsFolderName)+2);
		strcpy(streamingAssets,_streamingAssetsFolderName);
		strcat(streamingAssets,"/");
		
		scriptFolder = malloc(strlen(_streamingAssetsFolderName)+strlen("/Scripts/")+1);
		strcpy(scriptFolder,_streamingAssetsFolderName);
		strcat(scriptFolder,"/Scripts/");
	}

	presetFolder = malloc(strlen(DATAFOLDER)+strlen(_streamingAssetsFolderName)+strlen("/Presets/")+1);
	strcpy(presetFolder,DATAFOLDER);
	strcat(presetFolder,"Presets/");
	if (!directoryExists(presetFolder)){ // If the normal preset folder doesn't exist
		presetFolder[0]='\0';
		if (_isRelativeToData){
			strcat(presetFolder,DATAFOLDER);
		}
		strcat(presetFolder,_streamingAssetsFolderName); // Look 4 lines above for why this is okay
		strcat(presetFolder,"/Presets/");
		presetsAreInStreamingAssets=1;
	}else{
		presetsAreInStreamingAssets=0;
	}
}
void UpdatePresetStreamingAssetsDir(char* filename){
	char _tempNewStreamingAssetsPathbuffer[strlen(DATAFOLDER)+strlen("StreamingAssets_")+strlen(filename)+1];
	strcpy(_tempNewStreamingAssetsPathbuffer,DATAFOLDER);
	strcat(_tempNewStreamingAssetsPathbuffer,"StreamingAssets_");
	strcat(_tempNewStreamingAssetsPathbuffer,filename);
	if (directoryExists(_tempNewStreamingAssetsPathbuffer)){
		// The directory does exist. Construct the string for the new StreamingAssets folder and regenerate the path strings
		strcpy(_tempNewStreamingAssetsPathbuffer,"StreamingAssets_");
		strcat(_tempNewStreamingAssetsPathbuffer,filename);
		GenerateStreamingAssetsPaths(_tempNewStreamingAssetsPathbuffer,1);
	}
}
void RunGameSpecificLua(){
	char _completedSpecificLuaPath[strlen(scriptFolder)+strlen("_GameSpecific.lua")+1];
	strcpy(_completedSpecificLuaPath,scriptFolder);
	strcat(_completedSpecificLuaPath,"_GameSpecific.lua");
	if (checkFileExist(_completedSpecificLuaPath)){
		printf("Game specific LUA found.\n");
		luaL_dofile(L,_completedSpecificLuaPath);
	}
}
void generateADVBoxPath(char* _passedStringBuffer, char* _passedSystemString){
	strcpy(_passedStringBuffer,streamingAssets);
	strcat(_passedStringBuffer,"GameSpecificAdvBox");
	strcat(_passedStringBuffer,_passedSystemString);
	strcat(_passedStringBuffer,".png");
}
void loadADVBox(){
	if (currentCustomTextbox!=NULL){
		freeTexture(currentCustomTextbox);
		currentCustomTextbox=NULL;
	}
	char _tempFilepathBuffer[strlen(streamingAssets)+strlen("GameSpecificAdvBox.png")+( strlen("DEFAULT")>strlen(SYSTEMSTRING) ? strlen("DEFAULT") : strlen(SYSTEMSTRING) )+1];
	generateADVBoxPath(_tempFilepathBuffer,SYSTEMSTRING);
	if (!checkFileExist(_tempFilepathBuffer)){
		generateADVBoxPath(_tempFilepathBuffer,"DEFAULT");
	}
	if (checkFileExist(_tempFilepathBuffer)){
		currentCustomTextbox = loadPNG(_tempFilepathBuffer);
	}else{
		#if PLATFORM != PLAT_3DS
			currentCustomTextbox = LoadEmbeddedPNG("assets/DefaultAdvBox.png");
		#else
			currentCustomTextbox = LoadEmbeddedPNG("assets/DefaultAdvBoxLowRes.png");
		#endif
	}
	setTextOnlyOverBackground(textOnlyOverBackground);
}
void LoadGameSpecificStupidity(){
	TryLoadMenuSoundEffect(NULL);
	RunGameSpecificLua();
}
void resetSettings(){
	autoModeWait=500;
	graphicsLocation = LOCATION_CGALT;
	cpuOverclocked=0; // We don't actually change the CPU speed. They'll never notice. ;)
	bgmVolume=.75;
	seVolume=1.0;
	voiceVolume=1.0;
	MessageBoxAlpha=100;
	textOnlyOverBackground=0;
	textSpeed=1;
	// Update music volume using new default setting
	SetAllMusicVolume(FixBGMVolume(lastBGMVolume));
}
// This will assume that trying to create a directory that already exists is okay.
// Must call this function after paths are set up.
void createRequiredDirectories(){
	// These directories need to be made if it's CIA version
	#if PLATFORM == PLAT_3DS
		createDirectory("/3ds/");
		createDirectory("/3ds/data/");
		createDirectory("/3ds/data/HIGURASHI/");
	#endif
	createDirectory(saveFolder);
	createDirectory(DATAFOLDER);
}
void startLoadPresetSpecifiedInFile(char* _presetFilenameFile){
	FILE* fp;
	char _tempReadPresetFilename[50];
	fp = fopen (_presetFilenameFile, "r");
 	fgets (_tempReadPresetFilename, 50, fp);
	fclose (fp);

	removeNewline(_tempReadPresetFilename);
	currentPresetFilename = malloc(strlen(_tempReadPresetFilename)+1);
	strcpy(currentPresetFilename,_tempReadPresetFilename);

	currentGameStatus = GAMESTATUS_LOADPRESET;
}
// Also starts loading the preset file
void startLoadingGameFolder(char* _chosenGameFolder){
	char _fileWithPresetFilenamePath[strlen(gamesFolder)+strlen(_chosenGameFolder)+strlen("/includedPreset.txt")+1];
	strcpy(_fileWithPresetFilenamePath,gamesFolder);
	strcat(_fileWithPresetFilenamePath,_chosenGameFolder);
	strcat(_fileWithPresetFilenamePath,"/includedPreset.txt");

	if (!checkFileExist(_fileWithPresetFilenamePath)){
		LazyMessage("Invalid game folder.","I know this because the includedPreset.txt","is not exist.",NULL);
	}
	startLoadPresetSpecifiedInFile(_fileWithPresetFilenamePath);

	free(presetFolder);
	presetFolder = malloc(strlen(gamesFolder)+strlen(_chosenGameFolder)+strlen("/")+1);
	strcpy(presetFolder,gamesFolder);
	strcat(presetFolder,_chosenGameFolder);
	strcat(presetFolder,"/");
	 
	free(streamingAssets);
	streamingAssets = malloc(strlen(presetFolder)+1);
	strcpy(streamingAssets,presetFolder);
	
	free(scriptFolder);
	scriptFolder = malloc(strlen(streamingAssets)+strlen("Scripts/")+1);
	strcpy(scriptFolder,streamingAssets);
	strcat(scriptFolder,"Scripts/");
}
void setDefaultGame(char* _defaultGameFolderName){
	char _defaultGameSaveFilenameBuffer[strlen(saveFolder)+strlen("/_defaultGame")+1];
	strcpy(_defaultGameSaveFilenameBuffer,saveFolder);
	strcat(_defaultGameSaveFilenameBuffer,"/defaultGame");

	FILE* fp;
	fp = fopen(_defaultGameSaveFilenameBuffer, "w");
	if (!fp){
		LazyMessage("Failed to open default game save file.",_defaultGameSaveFilenameBuffer,NULL,NULL);
		return;
	}
	fwrite(_defaultGameFolderName,strlen(_defaultGameFolderName),1,fp);
	fclose(fp);
}
void makeTextSpeedString(char* _textSpeedStringBuffer, signed char _newTextSpeed){
	if (_newTextSpeed==1){
		strcpy(_textSpeedStringBuffer,"Default");
	}else if (_newTextSpeed==TEXTSPEED_INSTANT){
		strcpy(_textSpeedStringBuffer,"Instant");
	}else{
		itoa(_newTextSpeed,_textSpeedStringBuffer,10);
	}
}
void activateVNDSSettings(){
	currentlyVNDSGame=1;
	scriptUsesFileExtensions=1;
	bustsStartInMiddle=0;
	scriptScreenWidth=256;
	scriptScreenHeight=192;
}
void activateHigurashiSettings(){
	currentlyVNDSGame=0;
	scriptUsesFileExtensions=0;
	bustsStartInMiddle=1;
	scriptScreenWidth=640;
	scriptScreenHeight=480;
}
#if PLATFORM == PLAT_VITA
	char wasJustPressedSpecific(SceCtrlData _currentPad, SceCtrlData _lastPad, int _button){
	if (_currentPad.buttons & _button){
		if (!(_lastPad.buttons & _button)){
			return 1;
		}
	}
	return 0;
	}
	// Wait for the user to suspend the game, save them, and be happy.
	void* soundProtectThread(void *arg){
	if (isActuallyUsingUma0==1){
		return NULL;
	}
	SceCtrlData _currentPad;
	SceCtrlData _lastPad;
	sceCtrlPeekBufferPositive(0, &_lastPad, 1);
	sceCtrlPeekBufferPositive(0, &_currentPad, 1);
	while (1){
		sceCtrlPeekBufferPositive(0, &_currentPad, 1);
		if (wasJustPressedSpecific(_currentPad,_lastPad,SCE_CTRL_PSBUTTON) || wasJustPressedSpecific(_currentPad,_lastPad,SCE_CTRL_POWER)){
			// Stop with WAV
			int i;
			for (i=0;i<MAXMUSICARRAY;i++){
				StopBGM(i);
			}
			for (i=0;i<MAXSOUNDEFFECTARRAY;i++){
				if (soundEffects[i]!=NULL){
					stopSound(soundEffects[i]);
				}
			}
			// Wait for the user to return.
			SceRtcTick _firstPressedButtonTick;
			sceRtcGetCurrentTick(&_firstPressedButtonTick);
			SceRtcTick _checkForReturnTick;
			sceRtcGetCurrentTick(&_checkForReturnTick);
			uint32_t _rtcTickResolution = sceRtcGetTickResolution();
			while (1){
				sceRtcGetCurrentTick(&_checkForReturnTick);
				// Wait a second, literally
				if (_checkForReturnTick.tick>_firstPressedButtonTick.tick+_rtcTickResolution){
					break;
				}
				sceKernelDelayThread(1);
			}
			// The user has returned.
			// Load and play
			for (i=0;i<MAXMUSICARRAY;i++){
				if (currentMusicFilepath[i]==NULL){
					continue;
				}
				char* _tempHoldBuffer = malloc(strlen(currentMusicFilepath[i])+1);
				strcpy(_tempHoldBuffer,currentMusicFilepath[i]);
				PlayBGM(_tempHoldBuffer,currentMusicUnfixedVolume[i],i);
				free(_tempHoldBuffer);
			}
		}
		_lastPad=_currentPad;
		sceKernelDelayThread(16);
	}
	return NULL;
	}
#endif
#if PLATFORM == PLAT_3DS
	char getIsCiaBuild(){
		if (checkFileExist("romfs:/assets/star.png")){
			return 1;
		}
		return 0;
	}
	void soundUpdateThread(void *arg){
		int i;
		while (_3dsSoundProtectThreadIsAlive){
			_bgmIsLock=1;
			for (i=0;i<10;i++){
				if (currentMusic[i]!=NULL){
					nathanUpdateMusicIfNeeded(currentMusic[i]);
				}
			}
			_bgmIsLock=0;
			svcSleepThread(500000000); // Wait half a second
		}
	}
#endif
#include "NathanDoubleScripting.h"
void writeLengthStringToFile(FILE* fp, char* _stringToWrite){
	if (_stringToWrite==NULL){
		short _tempHoldWriteData=0;
		fwrite(&_tempHoldWriteData,sizeof(short),1,fp);
		return;
	}
	short _tempFoundStrlen = strlen(_stringToWrite);
	fwrite(&_tempFoundStrlen,sizeof(short),1,fp);
	if (_tempFoundStrlen>0){
		fwrite(_stringToWrite,_tempFoundStrlen,1,fp);
	}
}
void saveVariableList(FILE* fp, nathanscriptGameVariable* _listToSave, int _totalListLength){
	int i;
	fwrite(&_totalListLength,sizeof(int),1,fp);
	for (i=0;i<_totalListLength;i++){
		char _correctVariableType = _listToSave[i].variable.variableType;
		fwrite(&(_correctVariableType),sizeof(char),1,fp);
		writeLengthStringToFile(fp,_listToSave[i].name);
		writeLengthStringToFile(fp,nathanvariableToString(&(_listToSave[i].variable)));
		nathanscriptConvertVariable(&(_listToSave[i].variable),_correctVariableType);
	}
}
char* readLengthStringFromFile(FILE* fp){
	short _tempFoundStrlen;
	fread(&_tempFoundStrlen,sizeof(short),1,fp);
	char* _returnString;
	if (_tempFoundStrlen==0){
		_returnString = malloc(1);
		_returnString[0]='\0';
	}else{
		_returnString = malloc(_tempFoundStrlen+1);
		fread(&(_returnString[0]),_tempFoundStrlen,1,fp);
		_returnString[_tempFoundStrlen]=0;
	}
	return _returnString;
}
void loadVariableList(FILE* fp, nathanscriptGameVariable** _listToLoad, int* _totalListLength){
	int i;
	int _readTotalVariables;
	fread(&_readTotalVariables,sizeof(int),1,fp);
	freeNathanGamevariableArray(*_listToLoad,*_totalListLength);
	*_listToLoad=NULL;
	*_totalListLength=0;
	for (i=0;i<_readTotalVariables;i++){
		int _newVariableIndex = nathanscriptAddNewVariableToList(_listToLoad,_totalListLength);
		char _readCorrectVariableType;
		fread(&_readCorrectVariableType,sizeof(char),1,fp);
		(*_listToLoad)[_newVariableIndex].name = readLengthStringFromFile(fp);
		(*_listToLoad)[_newVariableIndex].variable.value = readLengthStringFromFile(fp);
		(*_listToLoad)[_newVariableIndex].variable.variableType = NATHAN_TYPE_STRING;
		nathanscriptConvertVariable(&((*_listToLoad)[_newVariableIndex].variable),_readCorrectVariableType);
	}
}
#define VNDSSAVEFORMAT 1
// unsigned char - format version
// script filename relative to script folder
// long int - position in the file
// int - number of messgae strings
// message strings
// current background filename
// int - MAXBUSTS
// bust x, bust y, bust filename
// game variables
void vndsNormalSave(char* _filename){
	FILE* fp = fopen(_filename,"w");
	unsigned char _tempOptionsFormat = VNDSSAVEFORMAT;
	fwrite(&_tempOptionsFormat,sizeof(unsigned char),1,fp); //
	writeLengthStringToFile(fp,currentScriptFilename);
	long int _currentFilePosition = ftell(nathanscriptCurrentOpenFile);
	fwrite(&_currentFilePosition,sizeof(long int),1,fp); //
	int i;
	i=MAXLINES;
	fwrite(&i,sizeof(int),1,fp); //
	for (i=0;i<MAXLINES;i++){
		writeLengthStringToFile(fp, currentMessages[i]); //
	}
	writeLengthStringToFile(fp,lastBackgroundFilename); //

	fwrite(&MAXBUSTS,sizeof(int),1,fp); //
	for (i=0;i<MAXBUSTS;i++){
		fwrite(&(Busts[i].xOffset),sizeof(signed int),1,fp); //
		fwrite(&(Busts[i].yOffset),sizeof(signed int),1,fp); //
		if (Busts[i].relativeFilename==NULL){
			writeLengthStringToFile(fp,""); //
		}else{
			writeLengthStringToFile(fp,Busts[i].relativeFilename); //
		}
	}

	saveVariableList(fp,nathanscriptGamevarList,nathanscriptTotalGamevar); //
	fclose(fp);
}
void vndsNormalLoad(char* _filename){
	FILE* fp = fopen(_filename,"r");
	unsigned char _readFileFormat;
	fread(&_readFileFormat,sizeof(unsigned char),1,fp); //
	if (_readFileFormat!=1){
		LazyMessage("Bad file format version.\n",NULL,NULL,NULL);
		fclose(fp);
	}
	char* _foundScriptFilename = readLengthStringFromFile(fp); //
	changeMallocString(&currentScriptFilename,_foundScriptFilename);
	long int _readFilePosition;
	fread(&_readFilePosition,sizeof(long int),1,fp); //
	int _readMaxLines;
	fread(&_readMaxLines,sizeof(int),1,fp); //
	currentLine = _readMaxLines;
	int i;
	for (i=0;i<_readMaxLines;i++){
		char* _tempReadLine = readLengthStringFromFile(fp); //
		strcpy(&(currentMessages[i][0]),_tempReadLine);
		free(_tempReadLine);
	}
	char* _foundBackgroundFilename;
	_foundBackgroundFilename = readLengthStringFromFile(fp); //
	if (strlen(_foundBackgroundFilename)!=0){
		DrawScene(_foundBackgroundFilename,30);
	}
	free(_foundBackgroundFilename);

	int _maxReadBusts;
	nextVndsBustshotSlot=0;
	fread(&_maxReadBusts,sizeof(int),1,fp); //
	for (i=0;i<_maxReadBusts;i++){
		signed int _tempReadX;
		signed int _tempReadY;
		char* _tempReadFilename;
		fread(&_tempReadX,sizeof(signed int),1,fp); //
		fread(&_tempReadY,sizeof(signed int),1,fp); //
		_tempReadFilename = readLengthStringFromFile(fp); //
		if (_tempReadFilename[0]!='\0'){
			nextVndsBustshotSlot = i+1;
			DrawBustshot(i,_tempReadFilename,_tempReadX,_tempReadY,0,0,0,0);
		}
		free(_tempReadFilename);
	}

	loadVariableList(fp,&nathanscriptGamevarList,&nathanscriptTotalGamevar);
	fclose(fp);

	// ============================================
	// END
	// ============================================

	char _tempLoadedFilename[strlen(scriptFolder)+strlen(_foundScriptFilename)+1];
	strcpy(_tempLoadedFilename,scriptFolder);
	strcat(_tempLoadedFilename,_foundScriptFilename);

	endType=Line_Normal;	
	outputLineWait();

	nathanscriptDoScript(_tempLoadedFilename,_readFilePosition);
}
/*
=================================================
*/
void scriptDisplayWindow(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	MessageBoxEnabled=1;
	return;
}
void scriptClearMessage(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	currentLine=0;
	ClearMessageArray();
	return;
}
void scriptOutputLine(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	if (_passedArguments[3].variableType!=NATHAN_TYPE_NULL){
		if (strcmp(nathanvariableToString(&_passedArguments[3]),"0")==0){
			return;
		}
		OutputLine((unsigned const char*)nathanvariableToString(&_passedArguments[3]),nathanvariableToInt(&_passedArguments[4]),0);
		outputLineWait();
	}
	return;
}
// OutputLineAll(NULL, "\n", Line_ContinueAfterTyping);
// Null, text, line type
void scriptOutputLineAll(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	if (_passedArguments[1].variableType!=NATHAN_TYPE_NULL){
		if (strcmp(nathanvariableToString(&_passedArguments[1]),"0")==0){
			return;
		}
		OutputLine((unsigned const char*)nathanvariableToString(&_passedArguments[1]),nathanvariableToInt(&_passedArguments[2]),1);
		outputLineWait();
	}
	return;
}
//
void scriptWait(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	if (isSkipping!=1 && capEnabled==1){
		wait(nathanvariableToInt(&_passedArguments[0]));
	}
	return;
}
// filename, filter, unknown, unknown, time
void scriptDrawSceneWithMask(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	DrawScene(nathanvariableToString(&_passedArguments[0]),nathanvariableToInt(&_passedArguments[4]));
	return;
}
// filename
// fadein
void scriptDrawScene(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	DrawScene(nathanvariableToString(&_passedArguments[0]),nathanvariableToInt(&_passedArguments[1]));
	return;
}
// Placeholder for unimplemented function
void scriptNotYet(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	printf("An unimplemented Lua function was just executed!\n");
	return;
}
// Fist arg seems to be a channel arg.
	// Usually 1 for msys
	// Usually 2 for lsys
// Second arg is path in BGM folder without extension
// Third arg is volume. 128 seems to be average. I can hardly hear 8 with computer volume on 10.
// Fourth arg is unknown
void scriptPlayBGM(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	PlayBGM(nathanvariableToString(&_passedArguments[1]),nathanvariableToInt(&_passedArguments[2]),nathanvariableToInt(&_passedArguments[0]));

	if (nathanvariableToInt(&_passedArguments[3])!=0){
		printf("*************** VERY IMPORTANT *******************\nThe last PlayBGM call didn't have 0 for the fourth argument! This is a good place to investigate!\n");
	}

	return;
}
// Some int argument
// Maybe music slot
void scriptStopBGM(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	char _slot = nathanvariableToInt(&_passedArguments[0]);
	if (currentMusic[_slot]!=NULL){
		StopBGM(_slot);
		FreeBGM(_slot);
	}
	return;
}
#if SOUNDPLAYER == SND_3DS
	void nathanSetChannelVolume(unsigned char _a, float _b);
	// slot, time, should wair
	void scriptFadeoutBGM(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
		if (currentMusic[(int)nathanvariableToInt(&_passedArguments[0])]==NULL){
			return;
		}
		if (nathanvariableToBool(&_passedArguments[2])==0){
			stopMusic(currentMusicHandle[(int)nathanvariableToInt(&_passedArguments[0])]);
			return;
		}
		float _perTenthSecond=(float)((float)(1*bgmVolume)/((double)nathanvariableToInt(&_passedArguments[1])/(double)100));
		//float _perTenthSecond=.1;
		if (_perTenthSecond==0){
			_perTenthSecond=.00001;
		}
		float _currentFadeoutVolume=((1*((float)currentMusicUnfixedVolume[(int)nathanvariableToInt(&_passedArguments[0])]/(float)256))*bgmVolume);
		unsigned char _passedHandle = currentMusicHandle[(int)nathanvariableToInt(&_passedArguments[0])];
		while (_currentFadeoutVolume>0){
			if (_currentFadeoutVolume<_perTenthSecond){
				_currentFadeoutVolume=0;
			}else{
				_currentFadeoutVolume-=_perTenthSecond;
			}
			nathanSetChannelVolume(_passedHandle,_currentFadeoutVolume);
			svcSleepThread(100000000); // Wait for one tenth of a second
		}
		ndspChnWaveBufClear(_passedHandle);
	}
#else
	// slot, time, (bool) should wait for finish
	void scriptFadeoutBGM(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
		if (currentMusic[(int)nathanvariableToInt(&_passedArguments[0])]!=NULL){
			#if SOUNDPLAYER == SND_SOLOUD
				if (currentMusicHandle[(int)nathanvariableToInt(&_passedArguments[0])]==0){
					return;
				}
			#endif
			fadeoutMusic(currentMusicHandle[(int)nathanvariableToInt(&_passedArguments[0])],nathanvariableToInt(&_passedArguments[1]));
			if (nathanvariableToBool(&_passedArguments[2])==1){
				wait(nathanvariableToInt(&_passedArguments[1]));
			}
		}
		return;
	}
#endif
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
void scriptDrawBustshotWithFiltering(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	int i;
	for (i=8;i!=12;i++){
		if (nathanvariableToInt(&_passedArguments[i-1])!=0){
			printf("***********************IMPORTANT INFORMATION***************************\nAn argument I know nothing about was just used in DrawBustshotWithFiltering!\n***********************************************\n");
		}
	}
	DrawBustshot(nathanvariableToInt(&_passedArguments[0]), nathanvariableToString(&_passedArguments[1]), nathanvariableToInt(&_passedArguments[4]), nathanvariableToInt(&_passedArguments[5]), nathanvariableToInt(&_passedArguments[12]), nathanvariableToInt(&_passedArguments[13]), nathanvariableToBool(&_passedArguments[14]), nathanvariableToInt(&_passedArguments[11]));
	return;
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
	//
	// * The bustshot can only stay after a scene change if it's created the line before the scene change AND it doesn't wait for fadein completion.
void scriptDrawBustshot(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	if (!currentlyVNDSGame){
		startDrawing();
		Draw(MessageBoxEnabled);
		endDrawing();
	}

	int i;
	for (i=8;i!=12;i++){
		if (nathanvariableToInt(&_passedArguments[i-1])!=0){
			printf("***********************IMPORTANT INFORMATION***************************\nAn argument I know nothing about was just used in DrawBustshotWithFiltering!\n***********************************************\n");
		}
	}
	
	//void DrawBustshot(unsigned char passedSlot, char* _filename, int _xoffset, int _yoffset, int _layer, int _fadeintime, int _waitforfadein, int _isinvisible){
	DrawBustshot(nathanvariableToInt(&_passedArguments[0]), nathanvariableToString(&_passedArguments[1]), nathanvariableToInt(&_passedArguments[2]), nathanvariableToInt(&_passedArguments[3]), nathanvariableToInt(&_passedArguments[13]), nathanvariableToInt(&_passedArguments[14]), nathanvariableToBool(&_passedArguments[15]), nathanvariableToInt(&_passedArguments[12]));
	return;
}
void scriptSetValidityOfInput(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	if (nathanvariableToBool(&_passedArguments[0])==1){
		InputValidity=1;
	}else{
		InputValidity=0;
	}
	return;
}
// Fadeout time
// Wait for completely fadeout
void scriptFadeAllBustshots(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	FadeAllBustshots(nathanvariableToInt(&_passedArguments[0]),nathanvariableToBool(&_passedArguments[1]));
	//int i;
	//for (i=0;i<MAXBUSTS;i++){
	//	if (Busts[i].isActive==1){
	//		freeTexture(Busts[i].image);
	//		ResetBustStruct(&(Busts[i]));
	//	}
	//}
	return;
}
void scriptDisableWindow(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	MessageBoxEnabled=0;
	return;
}
void scriptFadeBustshotWithFiltering(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	FadeBustshot(nathanvariableToInt(&_passedArguments[0]),nathanvariableToInt(&_passedArguments[6]),nathanvariableToBool(&_passedArguments[7]));
	return;
}
//FadeBustshot( 2, FALSE, 0, 0, 0, 0, 0, TRUE );
//FadeBustshot( SLOT, MOVE, X, Y, UNKNOWNA, UNKNOWNB, FADETIME, WAIT );
void scriptFadeBustshot(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	FadeBustshot(nathanvariableToInt(&_passedArguments[0]),nathanvariableToInt(&_passedArguments[6]),nathanvariableToBool(&_passedArguments[7]));
	return;
}
// Slot, file, volume
void scriptPlaySE(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	if (isSkipping==0 && seVolume>0){
		GenericPlaySound(nathanvariableToInt(&_passedArguments[0]),nathanvariableToString(&_passedArguments[1]),nathanvariableToInt(&_passedArguments[2]),PREFER_DIR_SE,seVolume);
	}
	return;
}
// PlayVoice(channel, filename, volume)
void scriptPlayVoice(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	if (isSkipping==0 && (hasOwnVoiceSetting==1 ? voiceVolume : seVolume)>0){
		GenericPlaySound(nathanvariableToInt(&_passedArguments[0]),nathanvariableToString(&_passedArguments[1]),nathanvariableToInt(&_passedArguments[2]),PREFER_DIR_VOICE, hasOwnVoiceSetting==1 ? voiceVolume : seVolume);
	}
	return;
}
// Loads a script file
void scriptCallScript(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	const char* filename = nathanvariableToString(&_passedArguments[0]);

	char* tempstringconcat = CombineStringsPLEASEFREE(scriptFolder, "",filename,".txt");
	char tempstring2[strlen(tempstringconcat)+1];
	strcpy(tempstring2,tempstringconcat);
	free(tempstringconcat);

	if (checkFileExist(tempstring2)==1){
		printf("Do script %s\n",tempstring2);
		RunScript("",tempstring2,0);
	}else{
		printf("Failed to find script\n");
	}
	return;
}
// "bg_166", 7, 200, 0
void scriptChangeScene(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	DrawScene(nathanvariableToString(&_passedArguments[0]),0);
	return;
}
// DrawSprite(slot, filename, ?, x, y, ?, ?, ?, ?, ?, ?, ?, ?, LAYER, FADEINTIME, WAITFORFADEIN)
// x is relative to -320
	// y is relative to -240???
	// DrawSprite(slot, filename, ?, x, y, ?, ?, ?, ?, ?, ?, ?, ?, LAYER, FADEINTIME, WAITFORFADEIN)
void scriptDrawSprite(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	//void DrawBustshot(unsigned char passedSlot, char* _filename, int _xoffset, int _yoffset, int _layer, int _fadeintime, int _waitforfadein, int _isinvisible){
	DrawBustshot(nathanvariableToInt(&_passedArguments[0]),nathanvariableToString(&_passedArguments[1]),320+nathanvariableToInt(&_passedArguments[3]),240+nathanvariableToInt(&_passedArguments[4]),nathanvariableToInt(&_passedArguments[13]), nathanvariableToInt(&_passedArguments[14]),nathanvariableToBool(&_passedArguments[15]),0);
	//DrawBustshot(nathanvariableToInt(&_passedArguments[1)-1, nathanvariableToString(&_passedArguments[2), nathanvariableToInt(&_passedArguments[3), nathanvariableToInt(&_passedArguments[4), nathanvariableToInt(&_passedArguments[14), nathanvariableToInt(&_passedArguments[15), nathanvariableToBool(&_passedArguments[16), nathanvariableToInt(&_passedArguments[13));
	return;
}
//MoveSprite(slot, destinationx, destinationy, ?, ?, ?, ?, ?, timeittakes, waitforcompletion)
	// MoveSprite(5,-320,-4500,0,0,0,0,0,101400, TRUE)
void scriptMoveSprite(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	int _totalTime = nathanvariableToInt(&_passedArguments[8]);
	int _passedSlot = nathanvariableToInt(&_passedArguments[0]);
	// Number of x pixles the sprite has to move by the end

	printf("arg2:%d\n",(int)nathanvariableToInt(&_passedArguments[1]));
	printf("x:%d\n",Busts[_passedSlot].xOffset);

	int _xTengoQue = nathanvariableToInt(&_passedArguments[1])-(Busts[_passedSlot].xOffset-320);
	int _yTengoQue = nathanvariableToInt(&_passedArguments[2])-(Busts[_passedSlot].yOffset-240);
	char _waitforcompletion = nathanvariableToBool(&_passedArguments[9]);

	Busts[_passedSlot].statusVariable3 = nathanvariableToInt(&_passedArguments[1])+320;
	Busts[_passedSlot].statusVariable4 = nathanvariableToInt(&_passedArguments[2])+240;

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
		Busts[_passedSlot].xOffset=nathanvariableToInt(&_passedArguments[1])+320;
		Busts[_passedSlot].yOffset=nathanvariableToInt(&_passedArguments[2])+240;
	}

	if (_waitforcompletion==1){
		while(Busts[_passedSlot].bustStatus!=BUST_STATUS_NORMAL){
			fpsCapStart();
			controlsStart();
			if (wasJustPressed(SCE_CTRL_CROSS)){
				Busts[_passedSlot].xOffset=nathanvariableToInt(&_passedArguments[1])+320;
				Busts[_passedSlot].yOffset=nathanvariableToInt(&_passedArguments[2])+240;
			}
			controlsEnd();
			Update();
			startDrawing();
			Draw(MessageBoxEnabled);
			endDrawing();
			fpsCapWait();
		}
	}
	return;
}
//FadeSprite(slot, time, waitfrocompletion)
		// FadeSprite(5,700,FALSE)
void scriptFadeSprite(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	FadeBustshot(nathanvariableToInt(&_passedArguments[0]),nathanvariableToInt(&_passedArguments[1]),nathanvariableToBool(&_passedArguments[2]));	
	return;
}
#define MENUSELECTIONHIGHLIGHTOFFSET 10
// Select(numoptions, arrayofstring)
//		Let's the user make a choice and have this not be a sound novel anymore. :/
//		First arg is the number of options and the second arg is a string of the names of the options
//		Result can be found in LoadValueFromLocalWork("SelectResult")
//			Choice result is zero based
//				First choice is zero, second is one
void scriptSelect(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	ChangeEasyTouchMode(TOUCHMODE_MENU);
	int _totalOptions = nathanvariableToInt(&_passedArguments[0]);
	char* noobOptions[_totalOptions];
	int i;
	for (i=0;i<_totalOptions;i++){
		noobOptions[i] = malloc(strlen(nathanvariableGetArray(&_passedArguments[1],i))+1);
		strcpy(noobOptions[i],nathanvariableGetArray(&_passedArguments[1],i));
	}

	// This is the actual loop for choosing the choice
	signed char _choice=0;
	while (1){
		fpsCapStart();
		controlsStart();

		_choice = MenuControls(_choice,0,_totalOptions-1);

		if (wasJustPressed(SCE_CTRL_CROSS)){
			lastSelectionAnswer = _choice;
			break;
		}
		controlsEnd();
		startDrawing();
		Draw(0);
		DrawMessageBox();
		for (i=0;i<_totalOptions;i++){
			if (_choice!=i){
				goodDrawText(MENUOPTIONOFFSET,i*currentTextHeight,noobOptions[i],fontSize);
			}
		}
		goodDrawText(MENUOPTIONOFFSET+MENUCURSOROFFSET,_choice*currentTextHeight,noobOptions[_choice],fontSize);
		goodDrawText(MENUCURSOROFFSET*2,_choice*currentTextHeight,MENUCURSOR,fontSize);

		endDrawing();
		fpsCapWait();
	}

	// Free strings that were made with calloc earlier
	for (i=0;i<_totalOptions;i++){
		free(noobOptions[i]);
	}
	ChangeEasyTouchMode(TOUCHMODE_MAINGAME);
	return;
}
// Loads a special variable
void scriptLoadValueFromLocalWork(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	
	const char* _wordWant = nathanvariableToString(&_passedArguments[0]);
	//printf("%s\n",_wordWant);
	if ( strcmp(_wordWant,"SelectResult")==0){
		makeNewReturnArray(_returnedReturnArray,_returnArraySize,1);
		nathanvariableArraySetFloat(*_returnedReturnArray,0,lastSelectionAnswer);
		return;
	}else{
		LazyMessage("Unknown LoadValueFromLocalWork!",_wordWant,"Please report to MyLegGuy!","thx");
	}
	return;
}
// Calls a function that was made in a script
void scriptCallSection(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	char buf[256];
	strcpy(buf, nathanvariableToString(&_passedArguments[0]));
	strcat(buf,"()");
	printf("%s\n",buf);
	luaL_dostring(L,buf);
	return;
}
// I CAN DO THIS EZ-PZ WITH DRAWING RECTANGLES OVER THE SCREEN
// DrawFilm( 2,  0, 255, 0, 255, 0, 1000, TRUE );
// DrawFilm (slot?, r, g, b, filer's alpha, ?, fadein time, wait for fadein) <-- Guess
// DrawFilm ( type, r, g, b, a, style?, fadein time, wait for fadein )
void scriptDrawFilm(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	// 0 is none, defaults to 1.
	// 1 is "EffectColorMix"
	// 2 is DrainColor
	// 3 is Negative
	// 10 is HorizontalBlur2
	// 12 is GaussianBlur
	char _filterType = nathanvariableToInt(&_passedArguments[0]);
	filterActive=1;
	if (_filterType<=1){
		filterR = nathanvariableToInt(&_passedArguments[1]);
		filterG = nathanvariableToInt(&_passedArguments[2]);
		filterB = nathanvariableToInt(&_passedArguments[3]);
		filterA = nathanvariableToInt(&_passedArguments[4]);
	}else{ // For these, we'll just draw a white filter.
		filterR = 255;
		filterG = 255;
		filterB = 255;
		filterA = 127;
	}
	return;
}
// I think this just has a time argument and a blocking argument. I've implemented neither.
void scriptFadeFilm(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	filterActive=0;
	return;
}
// This command is used so unoften that I didn't bother to make it look good.
// FadeBG( 3000, TRUE );
void scriptFadeBG(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	if (currentBackground!=NULL){
		freeTexture(currentBackground);
		currentBackground=NULL;
	}
	return;
}
void scriptMoveBust(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	MoveBustSlot(nathanvariableToInt(&_passedArguments[0]),nathanvariableToInt(&_passedArguments[1]));
	return;
}
void scriptGetScriptLine(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	makeNewReturnArray(_returnedReturnArray,_returnArraySize,1);
	nathanvariableArraySetFloat(*_returnedReturnArray,0,currentScriptLine);
	return;
}
void scriptDebugFile(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	WriteToDebugFile(nathanvariableToString(&_passedArguments[0]));
	return;
}
void scriptOptionsEnableVoiceSetting(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	if (_numArguments==0){
		hasOwnVoiceSetting=1;
	}else{
		hasOwnVoiceSetting = nathanvariableToBool(&_passedArguments[0]);
	}
	return;
}
void scriptOptionsSetTextMode(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	gameTextDisplayMode = nathanvariableToInt(&_passedArguments[0]);
	setTextOnlyOverBackground(textOnlyOverBackground);
	return;
}
void scriptLoadADVBox(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	loadADVBox();
	return;
}
void scriptOptionsSetTips(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	gameHasTips=nathanvariableToBool(&_passedArguments[0]);
	return;
}
void scriptOptionsCanChangeBoxAlpha(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	canChangeBoxAlpha = nathanvariableToBool(&_passedArguments[0]);
	return;
}
// normal image 1, hover image 1, select image 1, normal image 2, hover image 2, select image 2
void scriptImageChoice(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	int i;
	// When you are not interacting with that choice
	CrossTexture** _passedNormalImages;
	// When your cursor is over that choice
	CrossTexture** _passedHoverImages;
	// When you've selected that choice
	CrossTexture** _passedSelectImages;
	int _numberOfChoices = _numArguments/3;
	printf("Found %d choices\n",_numberOfChoices);
	_passedNormalImages = malloc(sizeof(CrossTexture*)*_numberOfChoices);
	_passedHoverImages = malloc(sizeof(CrossTexture*)*_numberOfChoices);
	_passedSelectImages = malloc(sizeof(CrossTexture*)*_numberOfChoices);

	for (i=0;i<_numberOfChoices;i++){
		_passedNormalImages[i] = safeLoadGamePNG(nathanvariableToString(&_passedArguments[i*3+1-1]),graphicsLocation,scriptUsesFileExtensions);
		_passedHoverImages[i] = safeLoadGamePNG(nathanvariableToString(&_passedArguments[i*3+2-1]),graphicsLocation,scriptUsesFileExtensions);
		_passedSelectImages[i] = safeLoadGamePNG(nathanvariableToString(&_passedArguments[i*3+3-1]),graphicsLocation,scriptUsesFileExtensions);
	}

	// Y position of the first choice graphic
	int _startDrawY;
	// X position of every choice graphic
	int _startDrawX;
	int _spaceBetweenChoices;

	int _firstChoiceWidth = getTextureWidth(_passedNormalImages[0]);
	int _firstChoiceHeight = getTextureHeight(_passedNormalImages[0]);
	int _halfFirstChoiceHeight = _firstChoiceHeight/2.0;
	char _userChoice=0;
	char _isHoldSelect=0;


	_startDrawX = (screenWidth - _firstChoiceWidth)/2.0;
	_startDrawY = ((textboxYOffset!=0 ? textboxYOffset : screenHeight)/2.0)-((_numberOfChoices)*_firstChoiceHeight+(_numberOfChoices-1)*_halfFirstChoiceHeight)/2.0;
	while (1){
		fpsCapStart();

		controlsStart();
		_userChoice = MenuControls(_userChoice,0,_numberOfChoices-1);
		if (wasJustPressed(SCE_CTRL_CROSS)){
			_isHoldSelect=1;
		}
		if (wasJustPressed(SCE_CTRL_UP) || wasJustPressed(SCE_CTRL_DOWN)){
			_isHoldSelect=0;
		}		
		if (wasJustPressed(SCE_CTRL_CIRCLE)){
			break;
		}
		if (wasJustReleased(SCE_CTRL_CROSS)){
			if (_isHoldSelect==1){
				controlsEnd();
				break;
			}
		}
		controlsEnd();

		startDrawing();
		Draw(1);
		for (i=0;i<_numberOfChoices;i++){
			drawTexture(_passedNormalImages[i],0,_startDrawY+_firstChoiceHeight*i+_halfFirstChoiceHeight*i);
		}
		if (_isHoldSelect==0){
			drawTexture(_passedHoverImages[_userChoice],0,_startDrawY+_firstChoiceHeight*_userChoice+_halfFirstChoiceHeight*_userChoice);
		}else{
			drawTexture(_passedSelectImages[_userChoice],0,_startDrawY+_firstChoiceHeight*_userChoice+_halfFirstChoiceHeight*_userChoice);
		}
		endDrawing();

		fpsCapWait();
	}

	for (i=0;i<_numberOfChoices;i++){
		freeTexture(_passedNormalImages[i]);
		freeTexture(_passedHoverImages[i]);
		freeTexture(_passedSelectImages[i]);
	}
	free(_passedNormalImages);
	free(_passedHoverImages);
	free(_passedSelectImages);

	makeNewReturnArray(_returnedReturnArray,_returnArraySize,1);
	nathanvariableArraySetFloat(*_returnedReturnArray,0,_userChoice);
	return;
}
#include "LuaWrapperDefinitions.h"
//======================================================
void Draw(char _shouldDrawMessageBox){
	int i;
	if (currentBackground!=NULL){
		DrawBackground(currentBackground);
	}
	for (i = MAXBUSTS-1; i != -1; i--){
		if (bustOrder[i]!=255 && Busts[bustOrder[i]].isActive==1){
			DrawBust(&(Busts[bustOrder[i]]));
		}
	}
	if (filterActive==1){
		DrawCurrentFilter();
	}
	if (_shouldDrawMessageBox==1){
		DrawMessageBox();
	}
	for (i = MAXBUSTS-1; i != -1; i--){
		if (bustOrderOverBox[i]!=255 && Busts[bustOrderOverBox[i]].isActive==1){
			DrawBust(&(Busts[bustOrderOverBox[i]]));
		}
	}
	if (_shouldDrawMessageBox==1){
		DrawMessageText();
	}
}
// Returns 0 if normal
// Returns 1 if user quit
// Returns 2 if no files found
char FileSelector(char* directorylocation, char** _chosenfile, char* promptMessage){

	int i=0;
	int totalFiles=0;

	int _returnVal=0;

	// Can hold MAXFILES filenames
	// Each with no more than 50 characters (51 char block of memory. extra one for null char)
	char** filenameholder;
	filenameholder = (char**)calloc(MAXFILES,sizeof(char*));
	for (i=0;i<MAXFILES;i++){
		filenameholder[i]=(char*)calloc(1,MAXFILELENGTH);
	}

	CROSSDIR dir;
	CROSSDIRSTORAGE lastStorage;
	dir = openDirectory (directorylocation);

	if (dirOpenWorked(dir)==0){
		LazyMessage("Failed to open directory",directorylocation,NULL,NULL);
		// Free memori
		for (i=0;i<MAXFILES;i++){
			free(filenameholder[i]);
		}
		free(filenameholder);
		*_chosenfile = NULL;
		return 2;
	}

	for (i=0;i<MAXFILES;i++){
		if (directoryRead(&dir,&lastStorage) == 0){
			break;
		}
		memcpy((filenameholder[i]),getDirectoryResultName(&lastStorage),strlen(getDirectoryResultName(&lastStorage))+1);
	}
	directoryClose (dir);
	
	totalFiles = i;

	if (totalFiles==0){
		LazyMessage("No files found.",NULL,NULL,NULL);
		*_chosenfile=NULL;
		_returnVal=2;
	}else{
		int _choice=0;
		int _maxPerNoScroll=floor((screenHeight-5-currentTextHeight*2)/(currentTextHeight));
		if (totalFiles<_maxPerNoScroll){
			_maxPerNoScroll=totalFiles;
		}
		int _tmpoffset=0;
		while (currentGameStatus!=GAMESTATUS_QUIT){
			fpsCapStart();
			controlsStart();
	
			if (wasJustPressed(SCE_CTRL_UP)){
				_choice--;
				if (_choice<0){
					_choice=totalFiles-1;
				}
			}
			if (wasJustPressed(SCE_CTRL_DOWN)){
				_choice++;
				if (_choice>=totalFiles){
					_choice=0;
				}
			}
			if (wasJustPressed(SCE_CTRL_RIGHT)){
				_choice+=5;
				if (_choice>=totalFiles){
					_choice=totalFiles-1;
				}
			}
			if (wasJustPressed(SCE_CTRL_LEFT)){
				_choice-=5;
				if (_choice<0){
					_choice=0;
				}
			}
			if (wasJustPressed(SCE_CTRL_CROSS)){
				(*_chosenfile) = (char*)calloc(1,strlen(filenameholder[_choice])+1);
				memcpy(*_chosenfile,filenameholder[_choice],strlen(filenameholder[_choice])+1);
				PlayMenuSound();
				break;		
			}
			if (wasJustPressed(SCE_CTRL_CIRCLE)){
				(*_chosenfile) = NULL;
				_returnVal=1;
				break;		
			}
	
			startDrawing();
			//DrawText(20,20+textHeight(fontSize)+i*(textHeight(fontSize)),currentMessages[i],fontSize);
			if (promptMessage!=NULL){
				goodDrawText(MENUOPTIONOFFSET,5,promptMessage,fontSize);
			}
			_tmpoffset=_choice+1-_maxPerNoScroll;
			if (_tmpoffset<0){
				_tmpoffset=0;
			}
			for (i=0;i<_maxPerNoScroll;i++){
				goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(i+2),filenameholder[i+_tmpoffset],fontSize);
			}
			goodDrawTextColored(MENUOPTIONOFFSET,5+currentTextHeight*((_choice-_tmpoffset)+2),filenameholder[_choice],fontSize,0,255,0);
			goodDrawText(5,5+currentTextHeight*((_choice-_tmpoffset)+2),MENUCURSOR,fontSize);
			endDrawing();
			controlsEnd();
			fpsCapWait();
		}
	}

	// Free memori
	for (i=0;i<MAXFILES;i++){
		free(filenameholder[i]);
	}
	free(filenameholder);
	return _returnVal;
}
void FontSizeSetup(){
	ChangeEasyTouchMode(TOUCHMODE_MENU);
	char _choice=0;
	char _tempNumberString[10];
	itoa(fontSize,_tempNumberString,10);
	while (1){
		fpsCapStart();
		controlsStart();
		_choice = MenuControls(_choice,0,2);

		if (wasJustPressed(SCE_CTRL_CROSS) || wasJustPressed(SCE_CTRL_RIGHT)){
			if (_choice==0){
				#if PLATFORM != PLAT_3DS
					fontSize++;
				#else
					fontSize+=.1;
				#endif
				itoa(fontSize,_tempNumberString,10);
				#if PLATFORM == PLAT_VITA || PLATFORM == PLAT_3DS
					currentTextHeight = textHeight(fontSize);
				#endif
			}else if (_choice==1){
				ReloadFont();
			}else if (_choice==2){
				ChangeEasyTouchMode(TOUCHMODE_MENU);
				ReloadFont();
				break;
			}
		}
		if (wasJustPressed(SCE_CTRL_CIRCLE) || wasJustPressed(SCE_CTRL_LEFT)){
			if (_choice==0){
				#if PLATFORM != PLAT_3DS
					fontSize--;
					if (fontSize<=5){
						fontSize=6;
					}
				#else
					fontSize-=.1;
					if (fontSize<.8){
						fontSize=.8;
					}
				#endif
				itoa(fontSize,_tempNumberString,10);
				#if PLATFORM == PLAT_VITA || PLATFORM == PLAT_3DS
					currentTextHeight = textHeight(fontSize);
				#endif
			}
		}
		controlsEnd();
		startDrawing();
		goodDrawText(MENUOPTIONOFFSET,currentTextHeight,"Font Size: ",fontSize);
			goodDrawText(MENUOPTIONOFFSET+textWidth(fontSize,"Font Size: "),currentTextHeight,_tempNumberString,fontSize);
		#if PLATFORM == PLAT_VITA
			goodDrawText(MENUOPTIONOFFSET,currentTextHeight*2,"Test",fontSize);
			goodDrawText(MENUOPTIONOFFSET,currentTextHeight*5,"Press the \"Test\" button to ",fontSize);
			goodDrawText(MENUOPTIONOFFSET,currentTextHeight*6,"make the text look good. 32 is default.",fontSize);
		#endif
		goodDrawText(MENUOPTIONOFFSET,currentTextHeight*3,"Done",fontSize);
		#if PLATFORM != PLAT_VITA
			goodDrawText(MENUOPTIONOFFSET,currentTextHeight*5,"You should be able to see this entire line. It shouldn't cut off.",fontSize);
	
			goodDrawText(MENUOPTIONOFFSET,currentTextHeight*8,"Press the BACK button to see the controls. Green and red are used",fontSize);
			goodDrawText(MENUOPTIONOFFSET,currentTextHeight*9,"to change the font size when you're on the first option.",fontSize);
	
			goodDrawText(MENUOPTIONOFFSET,currentTextHeight*11,"You have to select \"Test\" to see the new size.",fontSize);
	
			goodDrawText(MENUOPTIONOFFSET,currentTextHeight*13,"aeiouthnaeiouthnaeiouthnaeiouthnaeiouthnaeiouthnaeiouthnaeiouthn",fontSize);
		#endif
		goodDrawText(5,currentTextHeight*(_choice+1),MENUCURSOR,fontSize);
		endDrawing();
		fpsCapWait();
	}
	SaveFontSizeFile();
}
#define ISTEXTSPEEDBAR 0
#define MAXOPTIONSSETTINGS 15
void SettingsMenu(){
	PlayMenuSound();
	signed char _choice=0;
	int i;
	char _artBefore=graphicsLocation; // This variable is used to check if the player changed the bust location after exiting
	CrossTexture* _renaImage=NULL;
	int _singleSpaceWidth = textWidth(fontSize," ");
	char _tempItoaHoldBGM[5] = {'\0'};
	char _tempItoaHoldSE[5] = {'\0'};
	char _tempItoaHoldVoice[5] = {'\0'};
	char _tempItoaHoldBoxAlpha[5] = {'\0'};
	char _tempItoaHoldTextSpeed[8] = {'\0'}; // Needs to be big enough to hold "instant"
	char _tempAutoModeString[10] = {'\0'};
	char _maxOptionSlotUsed=0;

	// Dynamic slots
	signed char _vndsSaveOptionsSlot=-2;
	signed char _vndsHitBottomActionSlot=-2;

	char* _settingsOptionsMainText[MAXOPTIONSSETTINGS];
	char* _settingsOptionsValueText[MAXOPTIONSSETTINGS];
	if (currentGameStatus == GAMESTATUS_TITLE){
		_settingsOptionsMainText[0] = "Back";
	}else{
		_settingsOptionsMainText[0] = "Resume";
	}
	if (hasOwnVoiceSetting){
		_settingsOptionsMainText[1] = "Voice Volume:";
	}else{
		_settingsOptionsMainText[1] = "===";
	}
	_settingsOptionsMainText[2] = "Auto Mode Speed:";
	_settingsOptionsMainText[3] = "Bust Location:";
	#if PLATFORM == PLAT_VITA
		_settingsOptionsMainText[4] = "Overclock CPU";
	#elif PLATFORM == PLAT_3DS
		_settingsOptionsMainText[4] = "Text:";
		if (cpuOverclocked==1){
			_settingsOptionsValueText[4] = "Bottom Screen";
		}else{
			_settingsOptionsValueText[4] = "Top Screen";
		}
	#else
		_settingsOptionsMainText[4] = "Nothing";
	#endif
	_settingsOptionsMainText[5] = "BGM Volume:";
	_settingsOptionsMainText[6] = "SE Volume:";
	_settingsOptionsMainText[7] = "Font Size";
	_settingsOptionsMainText[8] = "Defaults";
	if (canChangeBoxAlpha){
		_settingsOptionsMainText[9] = "Message Box Alpha:";
	}else{
		_settingsOptionsMainText[9] = "===";
	}
	_settingsOptionsMainText[10] = "Textbox:";
	_settingsOptionsMainText[11] = "Text Speed:";
	_maxOptionSlotUsed=11;
	// Add new settings here
	if (currentlyVNDSGame){
		_settingsOptionsMainText[++_maxOptionSlotUsed] = "=Save Game=";
		_vndsSaveOptionsSlot = _maxOptionSlotUsed;

		_settingsOptionsMainText[++_maxOptionSlotUsed] = "Clear at bottom:";
		_vndsHitBottomActionSlot = _maxOptionSlotUsed;
	}
	// Quit button is always last
	if (currentGameStatus!=GAMESTATUS_TITLE){
		_settingsOptionsMainText[++_maxOptionSlotUsed] = "Quit";
	}

	// Set pointers to menu option value text
	for (i=0;i<MAXOPTIONSSETTINGS;i++){
		_settingsOptionsValueText[i]=NULL;
	}
	if (hasOwnVoiceSetting){
		_settingsOptionsValueText[1] = &(_tempItoaHoldVoice[0]);
	}
	_settingsOptionsValueText[2]=&(_tempAutoModeString[0]);
	if (graphicsLocation == LOCATION_CG){
		_settingsOptionsValueText[3]="CG";
	}else if (graphicsLocation == LOCATION_CGALT){
		_settingsOptionsValueText[3]="CGAlt";
	}
	_settingsOptionsValueText[5] = &(_tempItoaHoldBGM[0]);
	_settingsOptionsValueText[6] = &(_tempItoaHoldSE[0]);
	if (canChangeBoxAlpha){
		_settingsOptionsValueText[9] = &(_tempItoaHoldBoxAlpha[0]);
	}
	if (textOnlyOverBackground){
		_settingsOptionsValueText[10] = "Small";
	}else{
		_settingsOptionsValueText[10] = "Full";
	}
	_settingsOptionsValueText[11] = &(_tempItoaHoldTextSpeed[0]);
	if (currentlyVNDSGame){
		_settingsOptionsValueText[_vndsHitBottomActionSlot] = charToBoolString(vndsClearAtBottom);
	}

	// Make strings
	itoa(autoModeWait,_tempAutoModeString,10);
	itoa(bgmVolume*4,_tempItoaHoldBGM,10);
	itoa(seVolume*4, _tempItoaHoldSE,10);
	itoa(voiceVolume*4, _tempItoaHoldVoice,10);
	itoa(MessageBoxAlpha, _tempItoaHoldBoxAlpha,10);
	makeTextSpeedString(_tempItoaHoldTextSpeed,textSpeed);
	if (textOnlyOverBackground){
		_settingsOptionsValueText[10] = "Small";
	}else{
		_settingsOptionsValueText[10] = "Full";
	}

	// This checks if we have Rena busts in CG AND CGAlt also loads Rena, if possible
	char* _tempRenaPath = CombineStringsPLEASEFREE(streamingAssets,"CG/","re_se_de_a1.png","");
	if (checkFileExist(_tempRenaPath)==1){
		free(_tempRenaPath);
		_tempRenaPath = CombineStringsPLEASEFREE(streamingAssets,"CGAlt/","re_se_de_a1.png","");
		if (checkFileExist(_tempRenaPath)==1){
			free(_tempRenaPath);
			_tempRenaPath = CombineStringsPLEASEFREE(streamingAssets,locationStrings[graphicsLocation],"re_se_de_a1.png",""); // New path for the user's specific graphic choice
			_renaImage = SafeLoadPNG(_tempRenaPath);
		}
		free(_tempRenaPath);
	}
	
	while (currentGameStatus!=GAMESTATUS_QUIT){
		fpsCapStart();
		controlsStart();
		_choice = MenuControls(_choice,0,_maxOptionSlotUsed);
		
		if (wasJustPressed(SCE_CTRL_CIRCLE)){
			break;
		}
		if (wasJustPressed(SCE_CTRL_LEFT)){
			if (_choice==2){
				if (isDown(SCE_CTRL_LTRIGGER)){
					autoModeWait-=200;
				}else{
					autoModeWait-=500;
				}
				if (autoModeWait<=0){
					autoModeWait=500;
				}
				itoa(autoModeWait,_tempAutoModeString,10);
			}else if (_choice==1 && hasOwnVoiceSetting){
				if (voiceVolume==0){
					voiceVolume=1.25;
				}
				voiceVolume-=.25;
				itoa(voiceVolume*4,_tempItoaHoldVoice,10);
			}else if (_choice==5){
				if (bgmVolume==0){
					bgmVolume=1.25;
				}
				bgmVolume-=.25;
				itoa(bgmVolume*4,_tempItoaHoldBGM,10);
				SetAllMusicVolume(FixBGMVolume(lastBGMVolume));
			}else if (_choice==6){
				if (seVolume==0){
					seVolume=1.25;
				}
				seVolume-=.25;
				itoa(seVolume*4,_tempItoaHoldSE,10);
				if (menuSoundLoaded==1){
					setSFXVolumeBefore(menuSound,FixSEVolume(256));
				}
				PlayMenuSound();
			}else if (_choice==9 && canChangeBoxAlpha){
				// char will wrap, we don't want that
				int _tempHoldChar = MessageBoxAlpha;
				if (isDown(SCE_CTRL_LTRIGGER)){
					_tempHoldChar-=15;
				}else{
					_tempHoldChar-=25;
				}
				if (_tempHoldChar<=0){
					_tempHoldChar=0;
				}
				MessageBoxAlpha = _tempHoldChar;
				itoa(MessageBoxAlpha,_tempItoaHoldBoxAlpha,10);
			}else if (_choice==11){
				textSpeed--;
				if (textSpeed==-11){
					textSpeed=-10;
				}else if (textSpeed==TEXTSPEED_INSTANT-1){
					textSpeed=10;
				}else if (textSpeed==0){
					textSpeed=-1;
				}
				makeTextSpeedString(_tempItoaHoldTextSpeed,textSpeed);
			}
		}
		if (wasJustPressed(SCE_CTRL_CROSS) || wasJustPressed(SCE_CTRL_RIGHT)){
			if (_choice==0){ // Resume
				PlayMenuSound();
				break;
			}else if (_choice==1 && hasOwnVoiceSetting){
				if (voiceVolume==1){
					voiceVolume=0;
				}else{
					voiceVolume+=.25;
				}
				itoa(voiceVolume*4,_tempItoaHoldVoice,10);
			}else if (_choice==2){
				if (isDown(SCE_CTRL_LTRIGGER)){
					autoModeWait+=200;
				}else{
					autoModeWait+=500;
				}
				itoa(autoModeWait,_tempAutoModeString,10);
			}else if (_choice==3){
				PlayMenuSound();
				if (graphicsLocation == LOCATION_CG){
					graphicsLocation = LOCATION_CGALT;
					_settingsOptionsValueText[3]="CGAlt";
				}else if (graphicsLocation == LOCATION_CGALT){
					graphicsLocation = LOCATION_CG;
					_settingsOptionsValueText[3]="CG";
				}
				if (_renaImage!=NULL){
					freeTexture(_renaImage);
					_tempRenaPath = CombineStringsPLEASEFREE(streamingAssets,locationStrings[graphicsLocation],"re_se_de_a1.png","");
					_renaImage = SafeLoadPNG(_tempRenaPath);
					free(_tempRenaPath);
				}
			}else if (_choice==4){ // CPU speed
				PlayMenuSound();
				if (cpuOverclocked==0){
					cpuOverclocked=1;
					#if PLATFORM == PLAT_VITA
						scePowerSetArmClockFrequency(444);
					#elif PLATFORM == PLAT_3DS
						_settingsOptionsValueText[4] = "Bottom Screen";
					#endif
				}else if (cpuOverclocked==1){
					cpuOverclocked=0;
					#if PLATFORM == PLAT_VITA
						scePowerSetArmClockFrequency(333);
					#elif PLATFORM == PLAT_3DS
						_settingsOptionsValueText[4] = "Top Screen";
					#endif
				}
			}else if (_choice==5){
				if (bgmVolume==1){
					bgmVolume=0;
				}else{
					bgmVolume+=.25;
				}
				itoa(bgmVolume*4,_tempItoaHoldBGM,10);
				SetAllMusicVolume(FixBGMVolume(lastBGMVolume));
			}else if (_choice==6){
				if (seVolume==1){
					seVolume=0;
				}else{
					seVolume+=.25;
				}
				itoa(seVolume*4,_tempItoaHoldSE,10);

				if (menuSoundLoaded==1){
					setSFXVolumeBefore(menuSound,FixSEVolume(256));
				}
				PlayMenuSound();
			}else if (_choice==7){
				FontSizeSetup();
				currentTextHeight = textHeight(fontSize);
			}else if (_choice==8){
				PlayMenuSound();
				if (LazyChoice("This will reset your settings.","Is this okay?",NULL,NULL)==1){
					resetSettings();
					// Some need to have their strings changed so the user can actually see the changes
					itoa(autoModeWait,_tempAutoModeString,10);
					itoa(bgmVolume*4,_tempItoaHoldBGM,10);
					itoa(seVolume*4, _tempItoaHoldSE,10);
					itoa(voiceVolume*4, _tempItoaHoldVoice,10);
					itoa(MessageBoxAlpha, _tempItoaHoldBoxAlpha,10);
				}
			}else if (_choice==9 && canChangeBoxAlpha){
				int _tempHoldChar = MessageBoxAlpha;
				if (isDown(SCE_CTRL_LTRIGGER)){
					_tempHoldChar+=15;
				}else{
					_tempHoldChar+=25;
				}
				if (_tempHoldChar>255){
					_tempHoldChar=255;
				}
				MessageBoxAlpha = _tempHoldChar;
				itoa(_tempHoldChar,_tempItoaHoldBoxAlpha,10);
			}else if (_choice==10){
				setTextOnlyOverBackground(!textOnlyOverBackground);
				if (textOnlyOverBackground){
					_settingsOptionsValueText[10] = "Small";
				}else{
					_settingsOptionsValueText[10] = "Full";
				}
			}else if (_choice==11){
				textSpeed++;
				if (textSpeed==11){
					textSpeed=TEXTSPEED_INSTANT;
				}else if (textSpeed==TEXTSPEED_INSTANT+1){
					textSpeed=TEXTSPEED_INSTANT;
				}else if (textSpeed==0){
					textSpeed=1;
				}
				makeTextSpeedString(_tempItoaHoldTextSpeed,textSpeed);
			}else if (_choice==_vndsSaveOptionsSlot){ // VNDS Save
				PlayMenuSound();
				char _tempSavefilePath[strlen(streamingAssets)+strlen("sav0")+1];
				strcpy(_tempSavefilePath,streamingAssets);
				strcat(_tempSavefilePath,"sav0");
				vndsNormalSave(_tempSavefilePath);
				LazyMessage("Saved to",_tempSavefilePath,NULL,NULL);
			}else if (_choice==_vndsHitBottomActionSlot){
				vndsClearAtBottom = !vndsClearAtBottom;
				_settingsOptionsValueText[_vndsHitBottomActionSlot] = charToBoolString(vndsClearAtBottom);
			}else if (_choice==_maxOptionSlotUsed){ // Quit
				endType = Line_ContinueAfterTyping;
				if (_choice==99){
					currentGameStatus=GAMESTATUS_NAVIGATIONMENU;
				}else{
					currentGameStatus=GAMESTATUS_QUIT;
				}
				exit(0);
				break;
			}
		}
		controlsEnd();
		startDrawing();
		goodDrawText(5,5+_choice*currentTextHeight,MENUCURSOR,fontSize);
		for (i=0;i<=_maxOptionSlotUsed;i++){
			goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*i,_settingsOptionsMainText[i],fontSize);
			if (_settingsOptionsValueText[i]!=NULL){
				goodDrawText(MENUOPTIONOFFSET+textWidth(fontSize,_settingsOptionsMainText[i])+_singleSpaceWidth,5+currentTextHeight*i,_settingsOptionsValueText[i],fontSize);
			}
		}
		if (cpuOverclocked){
			goodDrawTextColored(MENUOPTIONOFFSET,5+currentTextHeight*4,_settingsOptionsMainText[4],fontSize,0,255,0);
		}
		if (MessageBoxAlpha>=230 && canChangeBoxAlpha){
			goodDrawTextColored(MENUOPTIONOFFSET,5+currentTextHeight*9,_settingsOptionsMainText[9],fontSize,255,0,0);
		}
		// Display sample Rena if changing bust location
		#if PLATFORM == PLAT_3DS
			startDrawingBottom();
			if (_choice==3){
				if (_renaImage!=NULL){
					drawTexture(_renaImage,0,screenHeight-getTextureHeight(_renaImage));
				}
			}
		#else
			if (_choice==3){
				if (_renaImage!=NULL){
					drawTexture(_renaImage,screenWidth-getTextureWidth(_renaImage)-5,screenHeight-getTextureHeight(_renaImage));
				}
			}
		#endif
		endDrawing();
		fpsCapWait();
		exitIfForceQuit();
	}
	SaveSettings();
	if (_renaImage!=NULL){
		freeTexture(_renaImage);
	}
	if (currentGameStatus!=GAMESTATUS_TITLE){
		if (_artBefore != graphicsLocation){
			LazyMessage("You changed the character art location.","The next time a character is loaded,","it will load from",locationStrings[graphicsLocation]);
		}
	}
	#if PLATFORM == PLAT_3DS
		if (textIsBottomScreen==1){
			outputLineScreenWidth = 320;
			outputLineScreenHeight = 240;
		}else{
			outputLineScreenWidth = 400;
			outputLineScreenHeight = 240;
		}
	#endif
}
void TitleScreen(){
	signed char _choice=0;
	
	signed char _titlePassword=0;

	int _versionStringWidth = textWidth(fontSize,VERSIONSTRING VERSIONSTRINGSUFFIX);

	//SetClearColor(255,255,255,255);
	while (currentGameStatus!=GAMESTATUS_QUIT){
		fpsCapStart();
		controlsStart();

		// Password right left down up square
			if (wasJustPressed(SCE_CTRL_RIGHT)){
				_titlePassword=1;
			}else if (wasJustPressed(SCE_CTRL_LEFT)){
				_titlePassword = Password(_titlePassword,1);
			}else if (wasJustPressed(SCE_CTRL_DOWN)){
				_titlePassword = Password(_titlePassword,2);
			}else if (wasJustPressed(SCE_CTRL_UP)){
				_titlePassword = Password(_titlePassword,3);
			}else if (wasJustPressed(SCE_CTRL_SQUARE)){
				_titlePassword = Password(_titlePassword,4);
				if (_titlePassword==5){
					if (LazyChoice("Would you like to activate top secret","speedy mode for MyLegGuy's testing?",NULL,NULL)==1){
						capEnabled=0;
						autoModeWait=50;
					}
				}
			}

		if (wasJustPressed(SCE_CTRL_DOWN)){
			_choice++;
			if (_choice>3){
				_choice=0;
			}
		}
		if (wasJustPressed(SCE_CTRL_UP)){
			_choice--;
			if (_choice<0){
				_choice=3;
			}
		}

		if (wasJustPressed(SCE_CTRL_CROSS)){
			if (_choice==0){
				PlayMenuSound(); 
				if (currentPresetFilename==NULL){
					currentPresetChapter=0;
					controlsEnd();
					if (isGameFolderMode){
						currentGameStatus=GAMESTATUS_GAMEFOLDERSELECTION;
					}else{
						currentGameStatus=GAMESTATUS_PRESETSELECTION;
					}
				}
				break;
			}else if (_choice==1){
				PlayMenuSound();
				controlsEnd();
				if (isGameFolderMode){
					char* _chosenGameFolder;
					if (FileSelector(gamesFolder,&_chosenGameFolder,(char*)"Select a game")==2 || _chosenGameFolder==NULL){
						continue;
					}
					char _tempNewStreamingAssetsPathbuffer[strlen(gamesFolder)+strlen(_chosenGameFolder)+1];
					strcpy(_tempNewStreamingAssetsPathbuffer,gamesFolder);
					strcat(_tempNewStreamingAssetsPathbuffer,_chosenGameFolder);
					GenerateStreamingAssetsPaths(_tempNewStreamingAssetsPathbuffer,0);
					free(_chosenGameFolder);
				}
				if (!directoryExists(scriptFolder)){
					controlsEnd();
					char* _tempChosenFile;
					if (FileSelector(presetFolder,&_tempChosenFile,(char*)"Select a preset to choose StreamingAssets folder")==2){
						LazyMessage(scriptFolder,"does not exist and no files in",presetFolder,"Do you have any files?");
						continue;
					}else{
						if (_tempChosenFile==NULL){
							continue;
						}
						UpdatePresetStreamingAssetsDir(_tempChosenFile);
						free(_tempChosenFile);
					}
				}
				controlsEnd();
				char* _tempManualFileSelectionResult;
				FileSelector(scriptFolder,&_tempManualFileSelectionResult,(char*)"Select a script");
				if (_tempManualFileSelectionResult!=NULL){
					if (strlen(_tempManualFileSelectionResult)>4 && strcmp(&(_tempManualFileSelectionResult[strlen(_tempManualFileSelectionResult)-4]),".scr")==0){
						currentGameStatus=GAMESTATUS_MAINGAME;
						initializeNathanScript();

						activateVNDSSettings();

						char _tempFilepathBuffer[strlen(scriptFolder)+strlen(_tempManualFileSelectionResult)+1];
						strcpy(_tempFilepathBuffer,scriptFolder);
						strcat(_tempFilepathBuffer,_tempManualFileSelectionResult);

						changeMallocString(&currentScriptFilename,_tempManualFileSelectionResult);
						nathanscriptDoScript(_tempFilepathBuffer,0);

						free(_tempManualFileSelectionResult);
						currentGameStatus=GAMESTATUS_TITLE;
						ChangeEasyTouchMode(TOUCHMODE_MENU);
					}else{
						currentGameStatus=GAMESTATUS_MAINGAME;
						RunScript(scriptFolder,_tempManualFileSelectionResult,0);
						free(_tempManualFileSelectionResult);
						currentGameStatus=GAMESTATUS_TITLE;
						ChangeEasyTouchMode(TOUCHMODE_MENU);
					}
				}
				if (presetsAreInStreamingAssets==0){ // If the presets are not specific to a StreamingAssets folder, that means that the user could be using a different StreamingAssets folder. Reset paths just in case.
					GenerateStreamingAssetsPaths("StreamingAssets",1);
				}
			}else if (_choice==3){ // Quit button
				currentGameStatus=GAMESTATUS_QUIT;
				break;
			}else if (_choice==2){ // Go to setting menu
				controlsEnd();
				SettingsMenu();
				controlsEnd();
				break;
			}else{
				_choice=0;
			}
		}
		

		startDrawing();

		goodDrawText(MENUOPTIONOFFSET,5,"Main Menu",fontSize);

		goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(0+2),"Load game",fontSize);
		goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(1+2),"Manual mode",fontSize);
		goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(2+2),"Settings",fontSize);
		goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(3+2),"Exit",fontSize);

		goodDrawTextColored((screenWidth-5)-_versionStringWidth,screenHeight-5-currentTextHeight,VERSIONSTRING VERSIONSTRINGSUFFIX,fontSize,VERSIONCOLOR);
		goodDrawText(5,screenHeight-5-currentTextHeight,SYSTEMSTRING,fontSize);

		goodDrawText(5,5+currentTextHeight*(_choice+2),MENUCURSOR,fontSize);
		#if PLATFORM == PLAT_3DS
			startDrawingBottom();
		#endif
		endDrawing();
		controlsEnd();
		fpsCapWait();
		exitIfForceQuit();
	}
}
void tipMenuChangeDisplay(char* _passedCurrentName, char* _passedCurrentSlot, char* _passedMaxSlot){
	ClearMessageArray();
	char _tempNameBuffer[strlen(_passedCurrentName)+1+1+3+1+3+1+1]; // main name + space + left parentheses + three digit number + slash + three digit number + right parentheses + null
	if (tipNamesLoaded==0){
		_passedCurrentName="???";
	}
	sprintf(_tempNameBuffer,"%s (%s/%s)",_passedCurrentName,_passedCurrentSlot,_passedMaxSlot);
	OutputLine(_tempNameBuffer,Line_ContinueAfterTyping,1);
}
void TipMenu(){
	ClearMessageArray();
	if (currentPresetTipUnlockList.theArray[currentPresetChapter]==0){
		LazyMessage("No tips unlocked.",NULL,NULL,NULL);
		currentGameStatus=GAMESTATUS_NAVIGATIONMENU;
		controlsEnd();
		return;
	}
	// The number for the tip the user has selected. Starts at 1. Subtract 1 if using this for an array
	unsigned char _chosenTip=1;
	char _chosenTipString[4]={49,0,0,0};
	char _chosenTipStringMax[4]={48,0,0,0};
	itoa(currentPresetTipUnlockList.theArray[currentPresetChapter],&(_chosenTipStringMax[0]),10);
	tipMenuChangeDisplay(currentPresetTipNameList.theArray[_chosenTip-1],_chosenTipString,_chosenTipStringMax);
	int i;
	signed char _choice=0;

	ChangeEasyTouchMode(TOUCHMODE_LEFTRIGHTSELECT);

	while (currentGameStatus!=GAMESTATUS_QUIT){
		fpsCapStart();
		controlsStart();

		if (wasJustPressed(SCE_CTRL_DOWN)){
			_choice++;
			if (_choice>1){
				_choice=0;
			}
		}
		if (wasJustPressed(SCE_CTRL_UP)){
			_choice--;
			if (_choice<0){
				_choice=1;
			}
		}

		if (wasJustPressed(SCE_CTRL_RIGHT)){
			_chosenTip++;
			if (_chosenTip>currentPresetTipUnlockList.theArray[currentPresetChapter]){
				_chosenTip=1;
			}
			itoa(_chosenTip,&(_chosenTipString[0]),10);
			tipMenuChangeDisplay(currentPresetTipNameList.theArray[_chosenTip-1],_chosenTipString,_chosenTipStringMax);
		}
		if (wasJustPressed(SCE_CTRL_LEFT) ){
			_chosenTip--;
			if (_chosenTip<1){
				_chosenTip=currentPresetTipUnlockList.theArray[currentPresetChapter];
			}
			itoa(_chosenTip,&(_chosenTipString[0]),10);
			tipMenuChangeDisplay(currentPresetTipNameList.theArray[_chosenTip-1],_chosenTipString,_chosenTipStringMax);
		}
		if (wasJustPressed(SCE_CTRL_CROSS)){
			ChangeEasyTouchMode(TOUCHMODE_MAINGAME);
			controlsEnd();
			// This will trick the in between lines functions into thinking that we're in normal script execution mode and not quit
			currentGameStatus=GAMESTATUS_MAINGAME;
			RunScript(scriptFolder, currentPresetTipList.theArray[_chosenTip-1],1);
			controlsEnd();
			ChangeEasyTouchMode(TOUCHMODE_MENU);
			currentGameStatus=GAMESTATUS_TIPMENU;
			// Fix display after it's been cleared by the TIP
			itoa(_chosenTip,&(_chosenTipString[0]),10);
			tipMenuChangeDisplay(currentPresetTipNameList.theArray[_chosenTip-1],_chosenTipString,_chosenTipStringMax);
		}
		if (wasJustPressed(SCE_CTRL_CIRCLE)){
			ChangeEasyTouchMode(TOUCHMODE_MENU);
			ClearMessageArray();
			currentGameStatus=GAMESTATUS_NAVIGATIONMENU;
			#if PLAYTIPMUSIC == 1
				StopBGM();
			#endif
			break;
		}
		controlsEnd();
		startDrawing();
		for (i = 0; i < 3; i++){
			goodDrawText(MENUOPTIONOFFSET,currentTextHeight+i*(currentTextHeight),(char*)currentMessages[i],fontSize);
		}
		goodDrawText(5,screenHeight-5-currentTextHeight*3,"Left and Right - Change TIP",fontSize);
		goodDrawText(5,screenHeight-5-currentTextHeight*2,BACKBUTTONNAME" - Back",fontSize);
		goodDrawText(5,screenHeight-5-currentTextHeight,SELECTBUTTONNAME" - Select",fontSize);

		endDrawing();
		fpsCapWait();
	}
}
void ChapterJump(){
	ChangeEasyTouchMode(TOUCHMODE_LEFTRIGHTSELECT);
	//currentGameStatus=3;
	//RunScript
	//currentGameStatus=4;
	int _chapterChoice=0;
	unsigned char _choice=0;
	char _tempNumberString[15];
	controlsEnd();

	itoa(_chapterChoice,&(_tempNumberString[0]),10);
	strcpy((char*)globalTempConcat,currentPresetFileList.theArray[_chapterChoice]);
	strcat((char*)globalTempConcat," (");
	strcat((char*)globalTempConcat,_tempNumberString);
	strcat((char*)globalTempConcat,")");

	while (currentGameStatus!=GAMESTATUS_QUIT){
		fpsCapStart();
		controlsStart();
		if (wasJustPressed(SCE_CTRL_RIGHT)){
			if (!isDown(SCE_CTRL_RTRIGGER)){
				_chapterChoice++;
			}else{
				_chapterChoice+=5;
			}
			if (_chapterChoice>currentPresetChapter){
				_chapterChoice=0;
			}

			itoa(_chapterChoice,&(_tempNumberString[0]),10);
			strcpy((char*)globalTempConcat,currentPresetFileList.theArray[_chapterChoice]);
			strcat((char*)globalTempConcat," (");
			strcat((char*)globalTempConcat,_tempNumberString);
			strcat((char*)globalTempConcat,")");
		}
		if (wasJustPressed(SCE_CTRL_LEFT)){
			if (!isDown(SCE_CTRL_RTRIGGER)){
				_chapterChoice--;
			}else{
				_chapterChoice-=5;
			}
			if (_chapterChoice<0){
				_chapterChoice=currentPresetChapter;
			}
			itoa(_chapterChoice,&(_tempNumberString[0]),10);
			strcpy((char*)globalTempConcat,currentPresetFileList.theArray[_chapterChoice]);
			strcat((char*)globalTempConcat," (");
			strcat((char*)globalTempConcat,_tempNumberString);
			strcat((char*)globalTempConcat,")");
		}
		if (wasJustPressed(SCE_CTRL_DOWN)){
			_choice++;
			if (_choice>1){
				_choice=0;
			}
		}
		if (wasJustPressed(SCE_CTRL_UP)){
			_choice--;
			if (_choice>=240){
				_choice=1;
			}
		}
		if (wasJustPressed(SCE_CTRL_CROSS)){
			ChangeEasyTouchMode(TOUCHMODE_MAINGAME);
			if (_choice==0){
				controlsEnd();
				currentGameStatus=GAMESTATUS_MAINGAME;
				RunScript(scriptFolder, currentPresetFileList.theArray[_chapterChoice],1);
				controlsEnd();
				ChangeEasyTouchMode(TOUCHMODE_MENU);
				currentGameStatus=GAMESTATUS_TIPMENU;
				break;
			}
			if (_choice==1){
				break;
			}
		}
		if (wasJustPressed(SCE_CTRL_CIRCLE)){
			ChangeEasyTouchMode(TOUCHMODE_MENU);
			break;
		}
		controlsEnd();
		startDrawing();
		
		if (chapterNamesLoaded==0){
			goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(0+2),(const char*)globalTempConcat,fontSize);
		}else{
			goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(0+2),currentPresetFileFriendlyList.theArray[_chapterChoice],fontSize);
		}

		goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(1+2),"Back",fontSize);
		goodDrawText(5,5+currentTextHeight*(_choice+2),MENUCURSOR,fontSize);

		goodDrawText(5,screenHeight-5-currentTextHeight*4,"Left and Right - Change chapter",fontSize);
		goodDrawText(5,screenHeight-5-currentTextHeight*3,"R and Left or Right - Change chapter quickly",fontSize);
		goodDrawText(5,screenHeight-5-currentTextHeight*2,BACKBUTTONNAME" - Back",fontSize);
		goodDrawText(5,screenHeight-5-currentTextHeight,SELECTBUTTONNAME" - Select",fontSize);
		endDrawing();
		fpsCapWait();
	}
}
void SaveGameEditor(){
	ChangeEasyTouchMode(TOUCHMODE_LEFTRIGHTSELECT);
	char _endOfChapterString[10];
	itoa(currentPresetChapter,_endOfChapterString,10);
	controlsEnd();
	while (1){
		fpsCapStart();

		controlsStart();
		if (wasJustPressed(SCE_CTRL_RIGHT)){
			currentPresetChapter++;
			if (currentPresetChapter>currentPresetFileList.length-1){
				currentPresetChapter=0;
			}
			itoa(currentPresetChapter,_endOfChapterString,10);
		}
		if (wasJustPressed(SCE_CTRL_LEFT)){
			currentPresetChapter--;
			if (currentPresetChapter<0){
				currentPresetChapter=currentPresetFileList.length-1;
			}
			itoa(currentPresetChapter,_endOfChapterString,10);
		}
		if (wasJustPressed(SCE_CTRL_CROSS)){
			ChangeEasyTouchMode(TOUCHMODE_MENU);
			SaveGame();
			controlsEnd();
			break;
		}
		controlsEnd();
		startDrawing();
		if (chapterNamesLoaded==0){
			goodDrawText(MENUOPTIONOFFSET, currentTextHeight, _endOfChapterString, fontSize);
		}else{
			goodDrawText(MENUOPTIONOFFSET, currentTextHeight, currentPresetFileFriendlyList.theArray[currentPresetChapter], fontSize);
		}

		
		goodDrawText(MENUOPTIONOFFSET, screenHeight-currentTextHeight*3, "Welcome to the save file editor!", fontSize);
		goodDrawText(MENUOPTIONOFFSET, screenHeight-currentTextHeight*2, SELECTBUTTONNAME" - Finish and save", fontSize);
		goodDrawText(MENUOPTIONOFFSET, screenHeight-currentTextHeight, "Left and Right - Change last completed chapter", fontSize);
		endDrawing();
		fpsCapWait();
	}
}
void NavigationMenu(){
	ChangeEasyTouchMode(TOUCHMODE_MENU);
	signed char _choice=0;
	int _endofscriptwidth = textWidth(fontSize,(char*)"End of script: ");
	char _endOfChapterString[10];
	itoa(currentPresetChapter,_endOfChapterString,10);

	char _nextChapterExist=0;
	// Checks if there is another chapter left in the preset file. If so, set the variable accordingly
	if (!(currentPresetChapter+1>=currentPresetFileList.length)){
		_nextChapterExist=1;
	}else{
		// This is the default value. I just put this line of code here so I can remember
		_nextChapterExist=0;
	}

	unsigned char _codeProgress=0;
	char _maxListSlot=1; // For chapter jump and exit
	if (_nextChapterExist==1){
		_maxListSlot++;
	}
	if (gameHasTips==1){
		_maxListSlot++;
	}
	char _nextButtonSlot, _chapterButtonSlot, _tipButtonSlot, _quitButtonSlot, _slotAssignIndex = 0;
	if (_nextChapterExist==1){
		_nextButtonSlot=_slotAssignIndex;
		_slotAssignIndex++;
	}else{
		_nextButtonSlot=99;
	}
	_chapterButtonSlot = _slotAssignIndex;
	_slotAssignIndex++;
	if (gameHasTips==1){
		_tipButtonSlot=_slotAssignIndex;
		_slotAssignIndex++;
	}else{
		_tipButtonSlot=99;
	}
	_quitButtonSlot=_slotAssignIndex;
	while (currentGameStatus!=GAMESTATUS_QUIT){
		fpsCapStart();
		controlsStart();

		// Editor secret code
			if (wasJustPressed(SCE_CTRL_UP)){
				_codeProgress = Password(_codeProgress,0);
			}
			if (wasJustPressed(SCE_CTRL_DOWN)){
				_codeProgress = Password(_codeProgress,1);
			}
			if (wasJustPressed(SCE_CTRL_LEFT)){
				_codeProgress = Password(_codeProgress,2);
			}
			if (wasJustPressed(SCE_CTRL_RIGHT)){
				_codeProgress = Password(_codeProgress,3);
				if (_codeProgress==4){
					SaveGameEditor();
					itoa(currentPresetChapter,_endOfChapterString,10);
					_nextChapterExist=1;
					_codeProgress=0;
				}
			}

		if (wasJustPressed(SCE_CTRL_DOWN)){
			_choice++;
			if (_choice>_maxListSlot){
				_choice=0;
			}
		}
		if (wasJustPressed(SCE_CTRL_UP)){
			_choice--;
			if (_choice<0){
				_choice=_maxListSlot;
			}
		}
		if (wasJustPressed(SCE_CTRL_CROSS)){
			if (_choice==_nextButtonSlot){
				printf("Go to next chapter\n");
				if (currentPresetChapter+1==currentPresetFileList.length){
					LazyMessage("There is no next chapter.", NULL, NULL, NULL);
				}else{
					easyTouchControlMode = TOUCHMODE_MAINGAME;
					ChangeEasyTouchMode(TOUCHMODE_MAINGAME);
					currentPresetChapter++;
					SetNextScriptName();
					currentGameStatus=GAMESTATUS_MAINGAME;
					break;
				}
			}else if (_choice==_chapterButtonSlot){
				ChapterJump();
			}else if (_choice==_tipButtonSlot){
				printf("Viewing tips\n");
				currentGameStatus=GAMESTATUS_TIPMENU;
				controlsEnd();
				#if PLAYTIPMUSIC == 1
					PlayBGM("lsys14",256);
				#endif
				break;
			}else if (_choice==_quitButtonSlot){
				currentGameStatus=GAMESTATUS_QUIT;
				break;
			}else{
				printf("INVALID SELECTION\n");
			}
		}
		if (wasJustPressed(SCE_CTRL_TRIANGLE)){
			if (isGameFolderMode && !isEmbedMode && LazyChoice(defaultGameIsSet ? "Unset this game as the default?" : "Set this game as the default game?",NULL,NULL,NULL)){
				defaultGameIsSet = !defaultGameIsSet;
				setDefaultGame(defaultGameIsSet ? currentGameFolderName : "NONE");
			}
		}
		controlsEnd();
		startDrawing();

		goodDrawText(MENUOPTIONOFFSET,0,"End of script: ",fontSize);
		if (chapterNamesLoaded==0){
			goodDrawText(_endofscriptwidth+MENUOPTIONOFFSET,0,_endOfChapterString,fontSize);
		}else{
			goodDrawText(_endofscriptwidth+MENUOPTIONOFFSET,0,currentPresetFileFriendlyList.theArray[currentPresetChapter],fontSize);
		}

		char _currentListDrawPosition=0;
		if (_nextChapterExist==1){
			goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(_currentListDrawPosition+2),"Next",fontSize);
			_currentListDrawPosition++;
		}
		goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(_currentListDrawPosition+2),"Chapter Jump",fontSize);
		if (gameHasTips==1){
			_currentListDrawPosition++;
			goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(_currentListDrawPosition+2),"View Tips",fontSize);
		}
		_currentListDrawPosition++;
		goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(_currentListDrawPosition+2),"Exit",fontSize);
		goodDrawText(5,5+currentTextHeight*(_choice+2),MENUCURSOR,fontSize);
		endDrawing();
		fpsCapWait();
		exitIfForceQuit();
	}
}
void NewGameMenu(){
	char _choice=0;
	ChangeEasyTouchMode(TOUCHMODE_MENU);
	while (1){
		fpsCapStart();

		controlsStart();
		_choice = MenuControls(_choice,0,1);

		if (wasJustPressed(SCE_CTRL_CROSS)){
			if (_choice==0){
				break;
			}else{
				currentPresetChapter=0;
				controlsEnd();
				SaveGameEditor();
				break;
			}
		}
		controlsEnd();

		startDrawing();
		goodDrawText(MENUOPTIONOFFSET,currentTextHeight,"NEW GAME",fontSize);
		goodDrawText(MENUOPTIONOFFSET,currentTextHeight*3,"Start from beginning",fontSize);
		goodDrawText(MENUOPTIONOFFSET,currentTextHeight*4,"Savegame Editor",fontSize);
		goodDrawText(5,currentTextHeight*(_choice+3),MENUCURSOR,fontSize);
		endDrawing();

		fpsCapWait();
	}
}
void VNDSNavigationMenu(){
	controlsEnd();
	signed char _choice=0;

	char* _loadedNovelName=NULL;

	CrossTexture* _loadedThumbnail=NULL;

	char _possibleThunbnailPath[strlen(streamingAssets)+strlen("/thumbnail.png")+1];
	strcpy(_possibleThunbnailPath,streamingAssets);
	strcat(_possibleThunbnailPath,"/thumbnail.png");
	if (checkFileExist(_possibleThunbnailPath)){
		_loadedThumbnail = SafeLoadPNG(_possibleThunbnailPath);
	}

	_possibleThunbnailPath[strlen(streamingAssets)]=0;
	strcat(_possibleThunbnailPath,"/info.txt");
	if (checkFileExist(_possibleThunbnailPath)){
		FILE* fp = fopen(_possibleThunbnailPath,"r");
		char _tempReadLine[256];
		fgets(_tempReadLine,256,fp);
		if (strlen(_tempReadLine)>6){ // If string is long enough to contain title string
			char* _foundTitleString = &(_tempReadLine[6]);
			_loadedNovelName = malloc(strlen(_foundTitleString)+1);
			strcpy(_loadedNovelName,_foundTitleString);
		}
	}
	if (_loadedNovelName==NULL){
		_loadedNovelName = malloc(strlen("VNDS")+1);
		strcpy(_loadedNovelName,"VNDS");
	}

	while (currentGameStatus!=GAMESTATUS_QUIT){
		fpsCapStart();
		controlsStart();

		_choice = MenuControls(_choice,0,2);
		if (wasJustPressed(SCE_CTRL_CROSS)){
			if (_choice==0){
				char _vndsSaveFileConcat[strlen(streamingAssets)+strlen("sav0")+1];
				strcpy(_vndsSaveFileConcat,streamingAssets);
				strcat(_vndsSaveFileConcat,"sav0");
				if (checkFileExist(_vndsSaveFileConcat)){
					vndsNormalLoad(_vndsSaveFileConcat);
				}else{
					LazyMessage("Save file",_vndsSaveFileConcat,"not exist.",NULL);
				}
			}else if (_choice==1){
				char _vndsMainScriptConcat[strlen(streamingAssets)+strlen("/Scripts/main.scr")+1];
				strcpy(_vndsMainScriptConcat,streamingAssets);
				strcat(_vndsMainScriptConcat,"/Scripts/main.scr");
				if (checkFileExist(_vndsMainScriptConcat)){
					currentGameStatus = GAMESTATUS_MAINGAME;
					changeMallocString(&currentScriptFilename,"main.scr");
					nathanscriptDoScript(_vndsMainScriptConcat,0);
					currentGameStatus = GAMESTATUS_NAVIGATIONMENU;
				}else{
					LazyMessage("Main script file",_vndsMainScriptConcat,"not exist.",NULL);
				}
			}else if (_choice==2){
				currentGameStatus = GAMESTATUS_QUIT;
			}
		}
		controlsEnd();
		startDrawing();

		if (_loadedThumbnail!=NULL){
			drawTexture(_loadedThumbnail,screenWidth-getTextureWidth(_loadedThumbnail),screenHeight-getTextureHeight(_loadedThumbnail));
		}

		goodDrawText(MENUOPTIONOFFSET,0,_loadedNovelName,fontSize);

		goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(0+2),"Load Save",fontSize);
		goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(1+2),"New Game",fontSize);
		goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(2+2),"Exit",fontSize);

		goodDrawText(5,5+currentTextHeight*(_choice+2),MENUCURSOR,fontSize);

		endDrawing();
		fpsCapWait();
		exitIfForceQuit();
	}
	free(_loadedNovelName);
}
// =====================================================
char initializeLua(){
	if (L==NULL){
		// Initialize Lua
		L = luaL_newstate();
		luaL_openlibs(L);
		initLuaWrappers();
	
		// happy.lua contains functions that both Higurashi script files use and my C code
		char _didLoadHappyLua;
		fixPath("assets/happy.lua",globalTempConcat,TYPE_EMBEDDED);
		_didLoadHappyLua = SafeLuaDoFile(L,globalTempConcat,0);
		lua_sethook(L, incrementScriptLineVariable, LUA_MASKLINE, 5);
		if (_didLoadHappyLua==0){
			#if PLATFORM == PLAT_VITA
				LazyMessage("happy.lua is missing for some reason.","Redownload the VPK.","If that doesn't fix it,","report the problem to MyLegGuy.");
			#else
				LazyMessage("happy.lua missing.",NULL,NULL,NULL);
			#endif
			return 2;
		}
	}
	return 0;
}
// These functions do some stuff with the data and usually eventually call the same functions that the Lua functions do.
#include "VNDSScriptWrappers.h"
void initializeNathanScript(){
	if (!nathanscriptIsInit){
		nathanscriptIsInit=1;
		nathanscriptInit();
		// TODO - Pre-realloc to number of functions we need.
		nathanscriptAddFunction(vndswrapper_text,nathanscriptMakeConfigByte(1,0),"text");
		nathanscriptAddFunction(vndswrapper_choice,nathanscriptMakeConfigByte(1,0),"choice");
		nathanscriptAddFunction(vndswrapper_delay,0,"delay");
		nathanscriptAddFunction(vndswrapper_cleartext,0,"cleartext");
		nathanscriptAddFunction(vndswrapper_bgload,0,"bgload");
		nathanscriptAddFunction(vndswrapper_setimg,0,"setimg");
		nathanscriptAddFunction(vndswrapper_jump,0,"jump");
		nathanscriptAddFunction(vndswrapper_music,0,"music");
		nathanscriptAddFunction(vndswrapper_gsetvar,nathanscriptMakeConfigByte(0,1),"gsetvar");
		// Load global variables
		char _globalsSaveFilePath[strlen(saveFolder)+strlen("vndsGlobals")+1];
		strcpy(_globalsSaveFilePath,saveFolder);
		strcat(_globalsSaveFilePath,"vndsGlobals");
		if (checkFileExist(_globalsSaveFilePath)){
			FILE* fp = fopen(_globalsSaveFilePath,"r");
			loadVariableList(fp,&nathanscriptGlobalvarList,&nathanscriptTotalGlobalvar);
			fclose(fp);
		}
	}
}
// Please exit if this function returns 2
// Go ahead as normal if it returns 0
signed char init(){
	srand (time(NULL));
	int i;
	for (i=0;i<3;i++){
		printf("====================================================\n");
	}
	generalGoodInit();
	initGraphics(960,544,&screenWidth,&screenHeight);
	setClearColor(0,0,0,255);

	outputLineScreenWidth = screenWidth;
	outputLineScreenHeight = screenHeight;

	// Make buffers for busts
	Busts = calloc(1,sizeof(bust)*MAXBUSTS);
	bustOrder = calloc(1,sizeof(char)*MAXBUSTS);
	bustOrderOverBox = calloc(1,sizeof(char)*MAXBUSTS);

	// Setup DATAFOLDER variable. Defaults to uma0 if it exists and it's unsafe build
	ResetDataDirectory();

	// These will soon be freed
	streamingAssets = malloc(1);
	presetFolder = malloc(1);
	scriptFolder = malloc(1);

	saveFolder = malloc(strlen(DATAFOLDER)+strlen("Saves/")+1);
	strcpy(saveFolder,DATAFOLDER);
	strcat(saveFolder,"Saves/");

	gamesFolder = malloc(strlen(DATAFOLDER)+strlen("Games/")+1);
	strcpy(gamesFolder,DATAFOLDER);
	strcat(gamesFolder,"Games/");

	// Make file paths with default StreamingAssets folder
	GenerateStreamingAssetsPaths("StreamingAssets",1);

	// Save folder, data folder, and others
	createRequiredDirectories();

	//
	ClearDebugFile();

	// This will also load the font size file and therefor must come before font loading
	// Will not crash if no settings found
	LoadSettings();

	// Check if the application came with a game embedded. If so, load it.
	fixPath("isEmbedded.txt",globalTempConcat,TYPE_EMBEDDED);
	if (checkFileExist(globalTempConcat)){
		isEmbedMode=1;
		startLoadPresetSpecifiedInFile(globalTempConcat);

		free(streamingAssets);
		fixPath("game/",globalTempConcat,TYPE_EMBEDDED);
		streamingAssets = malloc(strlen(globalTempConcat)+1);
		strcpy(streamingAssets,globalTempConcat);

		free(scriptFolder);
		fixPath("game/Scripts/",globalTempConcat,TYPE_EMBEDDED);
		scriptFolder = malloc(strlen(globalTempConcat)+1);
		strcpy(scriptFolder,globalTempConcat);

		free(presetFolder);
		fixPath("",globalTempConcat,TYPE_EMBEDDED);
		presetFolder = malloc(strlen(globalTempConcat)+1);
		strcpy(presetFolder,globalTempConcat);

		currentGameStatus = GAMESTATUS_LOADPRESET;
	}else{
		char _defaultGameSaveFilenameBuffer[strlen(saveFolder)+strlen("/_defaultGame")+1];
		strcpy(_defaultGameSaveFilenameBuffer,saveFolder);
		strcat(_defaultGameSaveFilenameBuffer,"/defaultGame");
		if (checkFileExist(_defaultGameSaveFilenameBuffer)){
			FILE* fp;
			fp = fopen(_defaultGameSaveFilenameBuffer,"r");
			char _readGameFolderName[256];
			fgets(_readGameFolderName,256,fp);
			fclose(fp);
			if (strcmp(_readGameFolderName,"NONE")!=0){
				defaultGameIsSet=1;
				currentGameFolderName = malloc(strlen(_readGameFolderName)+1);
				strcpy(currentGameFolderName,_readGameFolderName);
				startLoadingGameFolder(currentGameFolderName);
				currentGameStatus=GAMESTATUS_LOADPRESET;
			}
		}
	}
	// Check if this is the new game folder mode or the old preset file mode.
	fixPath("Games/",globalTempConcat,TYPE_DATA);
	if (directoryExists(globalTempConcat)==1){
		isGameFolderMode=1;
	}else{
		// On 3ds, only disable game folder mode if preset folder is there.
		fixPath("Presets/",globalTempConcat,TYPE_DATA);
		if (directoryExists(globalTempConcat)==1){
			// Maybe, one day, I'll make it so it's 1 by default, so I'll have to have this here.
			isGameFolderMode=0;
		}else{
			#if PLATFORM == PLAT_3DS
				isGameFolderMode=1;
			#else
				isGameFolderMode=0;
			#endif
		}
	}

	// Check for star picture in 3ds data directory to verify that they put the required files there.
	#if PLATFORM == PLAT_3DS
		osSetSpeedupEnable(1);
		fixPath("assets/star.png",globalTempConcat,TYPE_EMBEDDED);
		if (checkFileExist(globalTempConcat)==0){
			while(1){
				exitIfForceQuit();
				startDrawing();
				drawRectangle(0,0,20,100,255,0,0,255);
				drawRectangle(20,0,30,15,255,0,0,255);
				drawRectangle(20,35,30,15,255,0,0,255);
				endDrawing();
			}
		}
	#endif

	ReloadFont();
	if (initAudio()==0){
		#if PLATFORM == PLAT_3DS
			LazyMessage("dsp init failed. Do you have dsp","firm dumped and in","/3ds/dspfirm.cdc","?");
		#else
			LazyMessage("...but it not worked?",NULL,NULL,NULL);
		#endif
	}

	menuCursorSpaceWidth = textWidth(fontSize,MENUCURSOR" ");

	// Load the menu sound effect if it's present
	fixPath("assets/wa_038.ogg",globalTempConcat,TYPE_EMBEDDED);
	TryLoadMenuSoundEffect(globalTempConcat);
	if (menuSound==NULL){
		TryLoadMenuSoundEffect(NULL);
	}

	// Needed for any advanced message display
	imageCharImages[IMAGECHARUNKNOWN] = LoadEmbeddedPNG("assets/unknown.png");
	imageCharImages[IMAGECHARNOTE] = LoadEmbeddedPNG("assets/note.png");
	imageCharImages[IMAGECHARSTAR] = LoadEmbeddedPNG("assets/star.png");

	// Zero the image char arrray
	for (i=0;i<MAXIMAGECHAR;i++){
		imageCharType[i]=-1;
	}

	// Fill with null char
	ClearMessageArray();
	if (initializeLua()==2){
		return 2;
	}

	for (i=0;i<MAXBUSTS;i++){
		ResetBustStruct(&(Busts[i]),0);
	}

	#if PLATFORM == PLAT_VITA && SOUNDPLAYER == SND_SOLOUD
		// Create the protection thread.
		if (pthread_create(&soundProtectThreadId, NULL, &soundProtectThread, NULL) != 0){
			return 2;
		}
	#endif
	#if PLATFORM == PLAT_3DS
		// Create the sound update thread
		s32 _foundMainThreadPriority = 0;
		svcGetThreadPriority(&_foundMainThreadPriority, CUR_THREAD_HANDLE);
		_3dsSoundUpdateThread = threadCreate(soundUpdateThread, NULL, 4 * 1024, _foundMainThreadPriority-1, -2, false);
	#endif

	return 0;
}
int main(int argc, char *argv[]){
	/* code */
	if (init()==2){
		currentGameStatus = GAMESTATUS_QUIT;
	}
	// Put stupid test stuff here
	while (currentGameStatus!=GAMESTATUS_QUIT){
		switch (currentGameStatus){
			case GAMESTATUS_TITLE:
				TitleScreen();
				break;
			case GAMESTATUS_LOADPRESET:
				// Next, we can try to switch the StreamingAssets directory to ux0:data/HIGURASHI/StreamingAssets_FILENAME/ if that directory exists
				UpdatePresetStreamingAssetsDir(currentPresetFilename);
				LoadGameSpecificStupidity();
				// Create the string for the full path of the preset file and load it
				strcpy((char*)globalTempConcat,presetFolder);
				strcat((char*)globalTempConcat,currentPresetFilename);
				LoadPreset((char*)globalTempConcat);
				// Does not load the savefile, I promise.
				LoadGame();
				// If there is no save game, start a new one at chapter 0
				// Otherwise, go to the navigation menu
				if (currentPresetChapter==-1){
					controlsEnd();
					NewGameMenu();
					controlsEnd();
					if (currentPresetChapter==-1){
						controlsEnd();
						currentPresetChapter=0;
						SetNextScriptName();
						currentGameStatus=GAMESTATUS_MAINGAME;
					}else{
						currentGameStatus=GAMESTATUS_NAVIGATIONMENU;
					}
				}else{
					currentGameStatus=GAMESTATUS_NAVIGATIONMENU;
				}
				break;
			case GAMESTATUS_PRESETSELECTION:
				if (FileSelector(presetFolder,&currentPresetFilename,(char*)"Select a preset")==2){
					LazyMessage("No presets found.","If you ran the converter, you should've gotten some.","You can manually put presets in:",presetFolder);
					break;
				}
				controlsEnd();
				if (currentPresetFilename==NULL){
					currentGameStatus=GAMESTATUS_TITLE;
				}else{
					currentGameStatus=GAMESTATUS_LOADPRESET;
				}
				break;
			case GAMESTATUS_MAINGAME:
				; // This blank statement is here to allow me to declare a variable. Variables can not be declared directly after a label.
				char _didWork = RunScript(scriptFolder, nextScriptToLoad, 1);
				if (currentPresetFileList.length!=0){
					if (_didWork==0){ // If the script didn't run, don't advance the game
						currentPresetChapter--; // Go back a script
						if (currentPresetChapter<0 || currentPresetChapter==255){ // o, no, we've gone back too far!
							LazyMessage("So... the first script failed to launch.","You now have the info on why, so go","try and fix it.","Pressing "SELECTBUTTONNAME" will close the application.");
							currentGameStatus=GAMESTATUS_QUIT;
						}else{
							currentGameStatus=GAMESTATUS_NAVIGATIONMENU;
						}
					}else{
						SaveGame();
					}
					if (currentGameStatus!=GAMESTATUS_QUIT){
						currentGameStatus=GAMESTATUS_NAVIGATIONMENU;
					}
				}else{
					currentGameStatus=GAMESTATUS_TITLE;
				}
				break;
			case GAMESTATUS_NAVIGATIONMENU:
				// Menu for chapter jump, tip selection, and going to the next chapter
				NavigationMenu();
				break;
			case GAMESTATUS_TIPMENU:
				// Menu for selecting tip to view
				TipMenu();
				break;
			case GAMESTATUS_GAMEFOLDERSELECTION:
				;
				char* _chosenGameFolder;
				if (FileSelector(gamesFolder,&_chosenGameFolder,(char*)"Select a game")==2){
					LazyMessage("No folders found.","After running the script converter","you should've put the converted files in",gamesFolder);
					currentGameStatus = GAMESTATUS_TITLE;
					break;
				}
				if (_chosenGameFolder==NULL){
					currentGameStatus = GAMESTATUS_TITLE;
					break;
				}
				char _possibleVNDSStatusFile[strlen(gamesFolder)+strlen(_chosenGameFolder)+strlen("/Scripts/main.scr")+1];
				strcpy(_possibleVNDSStatusFile,gamesFolder);
				strcat(_possibleVNDSStatusFile,_chosenGameFolder);
				strcat(_possibleVNDSStatusFile,"/isvnds");
				if (checkFileExist(_possibleVNDSStatusFile)){
					initializeNathanScript();
					// Special settings for vnds
					activateVNDSSettings();
					// Setup StreamingAssets path
					_possibleVNDSStatusFile[strlen(gamesFolder)+strlen(_chosenGameFolder)]=0;
					GenerateStreamingAssetsPaths(_possibleVNDSStatusFile,0);
					currentGameStatus = GAMESTATUS_NAVIGATIONMENU;
					// VNDS games also support game specific lua
					LoadGameSpecificStupidity();
					VNDSNavigationMenu();
				}else{
					if (_chosenGameFolder==NULL){
						currentGameStatus=GAMESTATUS_TITLE;
					}else{
						if (strcmp(_chosenGameFolder,"PLACEHOLDER.txt")==0){ // TODO - Either automatically delete this, or delete it when it's selected.
							LazyMessage("Feel free to delete this file,","PLACEHOLDER.txt","in",gamesFolder);
							free(_chosenGameFolder);
							break;
						}
						startLoadingGameFolder(_chosenGameFolder);
						currentGameFolderName = _chosenGameFolder; // Do not free _chosenGameFolder
						currentGameStatus=GAMESTATUS_LOADPRESET;
					}
				}
				break;
		}
	}
	printf("ENDGAME\n");
	//QuitApplication(L);
	quitGraphics();
	quitAudio();
	generalGoodQuit();
	return 0;
}
