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
		TODO - Remove scriptFolder variable
		TODO - Fix LazyMessage system. Let it take a variable number of arguments to put together. Maybe even make it printf style.
		TODO - Inspect SetDrawingPointOfMessage
			It appears to just set the line to draw on, or that's at least what it's usually used for.
			Inspect what the max line I can use in it is.
			Think about how I could implement this command if the value given is bigger than the total number of lines
				Change the actual text box X and text box Y and use the input arg as a percentage of the screen?
			How does this work in ADV mode?
				Actually, the command is removed in ADV mode.
		TODO - Expression changes look odd.
	TODO - Allow VNDS sound command to stop all sounds
	TODO - SetSpeedOfMessage
	TODO - With my setvar and if statement changes, I broke hima tip 09. But VNDSx acts the same as my program does when I run the script... VNDS Android exclusive features? Never worked in the first place?

	Colored text example:
		text x1b[<colorID>;1m<restoftext>
		text x1b[0m

	sizeof(unsigned char); //1
	sizeof(long int); // 4 on Vita, 8 on my computer
	sizeof(short); // 2
	sizeof(int); // 4
	sizeof(signed int); // 4
*/
// This is pretty long because foreign characters can take two bytes
#define SINGLELINEARRAYSIZE 300
#define PLAYTIPMUSIC 0
#include "GeneralGoodConfig.h"

#include "main.h"

// Libraries all need
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h> // toupper
#include <stdarg.h>
//
#include <Lua/lua.h>
#include <Lua/lualib.h>
#include <Lua/lauxlib.h>
//
#include "legarchive.h"

#define LOCATION_UNDEFINED 0
#define LOCATION_CG 1
#define LOCATION_CGALT 2
/////////////////////////////////////
#define MAXBUSTCACHE 6
#define MAXIMAGECHAR 20
#define MAXFILES 50
#define MAXFILELENGTH 51
#define MAXMESSAGEHISTORY 40
#define VERSIONSTRING "forgotversionnumber" // This
#define VERSIONNUMBER 8 // This
#define VERSIONCOLOR 255,135,53 // It's Rena colored!
#define USEUMA0 1
// Specific constants
#if PLATFORM != PLAT_3DS
	#define SELECTBUTTONNAME "X"
	#define BACKBUTTONNAME "O"
	int advboxHeight = 181;
	#define VERSIONSTRINGSUFFIX ""
#else
	#define SELECTBUTTONNAME "A"
	#define BACKBUTTONNAME "B"
	int advboxHeight = 75;
	#define cpuOverclocked textIsBottomScreen
	#define VERSIONSTRINGSUFFIX ""
#endif
#if PLATFORM == PLAT_VITA
	#define CANINVERT 1
#elif PLATFORM == PLAT_3DS
	#define CANINVERT 0
#elif PLATFORM == PLAT_COMPUTER
	#define CANINVERT 0
#endif
#define HISTORYONONESCREEN ((int)((screenHeight-currentTextHeight*2-5)/currentTextHeight))
#define MENUCURSOR ">"
#define MENUCURSOROFFSET 5
#define MENUOPTIONOFFSET menuCursorSpaceWidth+5
#define CLEARMESSAGEFADEOUTTIME 100
////////////////////////////////////
#define TEXTBOXFADEOUTTIME 200 // In milliseconds
#define TEXTBOXFADEINTIME 150
#define TEXTBOXFADEOUTUPDATES(x) ((((double)x)/1000)*60) // Update frames
////////////////////////////////////
#define MAXMUSICARRAY 10
#define MAXSOUNDEFFECTARRAY 10
#define IMAGECHARSPACESTRING "   "
#define MESSAGEEDGEOFFSET 10

#define PREFER_DIR_BGM 0
#define PREFER_DIR_SE 1
#define PREFER_DIR_VOICE 2
#define PREFER_DIR_NOMEIMPORTA 3

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
	#define SYSTEMSTRING "LINUX"
#elif __WIN32__
	#define SYSTEMSTRING "WINDOWS"
#elif __vita__
	#define SYSTEMSTRING "VITA"
#elif _3DS
	#define SYSTEMSTRING "3DS"
#else
	#warning please make platform string
	#define SYSTEMSTRING "UNKNOWN"
#endif

// TODO - Proper libGeneralGood support for this
#if SOUNDPLAYER == SND_VITA
	char mlgsndIsPlaying(NathanAudio* _passedAudio);
#endif

// 1 is start
// 2 adds BGM and SE volume
// 3 adds voice volume
// 4 adds MessageBoxAlpha and textOnlyOverBackground
// 5 adds textSpeed
// 6 adds vndsClearAtBottom
// 7 adds showVNDSWarnings
// 8 adds higurashiUsesDynamicScale
// 9 adds preferredTextDisplayMode
// 10 adds autoModeVoicedWait
// 11 adds vndsSpritesFade
// 12 adds vndsVitaTouch
#define OPTIONSFILEFORMAT 12

#define VNDSSAVEFORMAT 1

#define VNDSGLOBALSSAVEFORMAT 1

//#define LUAREGISTER(x,y) DebugLuaReg(y);
#define LUAREGISTER(x,y) lua_pushcfunction(L,x);\
	lua_setglobal(L,y);

// Make a lua function and all it does is set a number variable to what you give to it
#define EASYLUAINTSETFUNCTION(scriptFunctionName,varname) \
	int L_##scriptFunctionName(lua_State* passedState){ \
		varname = lua_tonumber(passedState,1); \
		return 0; \
	}
// Easily push stuff made with EASYLUAINTSETFUNCTION
#define PUSHEASYLUAINTSETFUNCTION(scriptFunctionName) \
	LUAREGISTER(L_##scriptFunctionName,#scriptFunctionName)
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
#define BUST_STATUS_FADEIN 1 // var 1 is alpha per frame. var 2 is the time until we're actually going to start the fadein. Until var 2 is at 0, this bust will have 0 alpha. For some reason, the Higurashi engine wastes half of the specified time with the bust just not there.
#define BUST_STATUS_FADEOUT 2 // var 1 is alpha per frame
#define BUST_STATUS_FADEOUT_MOVE 3 // var 1 is alpha per frame. var 2 is x per frame. var 3 is y per frame
#define BUST_STATUS_SPRITE_MOVE 4 // var 1 is x per frame, var 2 is y per frame
#define BUST_STATUS_TRANSFORM_FADEIN 5 // The bust is fading into an already used slot. image is what the new texture is going to be thats fading in, transformTexture is the old texture that is fading out. var 1 is alpha per frame. added to image, subtracted from transformTexture.

#if PLATFORM == PLAT_VITA
	#include <psp2/touch.h>
	SceTouchData touch_old[SCE_TOUCH_PORT_MAX_NUM];
	SceTouchData touch[SCE_TOUCH_PORT_MAX_NUM];
	signed char vndsVitaTouch=1;
#endif

void invertImage(CrossTexture* _passedImage, signed char _doInvertAlpha);
typedef struct{
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
	CrossTexture* transformTexture; // See BUST_STATUS_TRANSFORM_FADEIN. This is the texture that is transforming
	unsigned int lineCreatedOn;
	double cacheXOffsetScale;
	double cacheYOffsetScale;
	char* relativeFilename; // Filename passed by the script
}bust;
typedef struct{
	CrossTexture* image;
	char* filename;
}cachedImage;

/*
Can cache up to MAXBUSTCACHE busts

bustA - Loaded into bust struct
bustB - Loaded into bust struct
bgload - All bust structs have their data put into bustCache then bust structs are reset
bustC - Loaded into bust struct
bustA - Loaded from bust cache and element removed, but not freed, from bust cache
bgload - First remove bustB from bust cache and then do the same as before.
*/
cachedImage bustCache[MAXBUSTCACHE];

bust* Busts;

#define MAXLINES 15

lua_State* L = NULL;
/*
	Line_ContinueAfterTyping=0; (No wait after text display, go right to next script line)
	Line_WaitForInput=1; (Wait for the player to click. Displays an arrow.)
	Line_Normal=2; (Wait for the player to click. DIsplays a page icon and almost 100% of the time has the screen cleared with the next command)
	LINE_RESERVED=3; (This is a value that is guaranteed not to represent an actual line end type.)
*/
#define Line_ContinueAfterTyping 0
#define Line_WaitForInput 1
#define Line_Normal 2
#define LINE_RESERVED 3
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

// Set by the graphics init function
int screenWidth=0;
int screenHeight=0;

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
#define GAMESTATUS_LOADGAMEFOLDER 7
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

#define FILTERTYPE_INACTIVE 0 // This one is different from Higurashi, in Higurashi it defaults to FILTERTYPE_EFFECTCOLORMIX
#define FILTERTYPE_EFFECTCOLORMIX 1
#define FILETRTYPE_DRAINCOLOR 2
#define FILTERTYPE_NEGATIVE 3
#define FILTERTYPE_HORIZONTALBLUR2 10
#define FILETRTYPE_GAUSSIANBLUR 12

unsigned char filterR;
unsigned char filterG;
unsigned char filterB;
unsigned char filterA;
unsigned char filterActive=0;
signed char currentFilterType=FILTERTYPE_INACTIVE;

signed char autoModeOn=0;
int autoModeWait=500;
int autoModeVoicedWait=500;

signed char cpuOverclocked=0;

#define TEXTMODE_NVL 0
#define TEXTMODE_AVD 1 // Wrong spelling
#define TEXTMODE_ADV 1
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
int singleSpaceWidth;
#if PLATFORM == PLAT_VITA
	pthread_t soundProtectThreadId;

	// For Vita specific font workaround
	extern CrossFont* fontImage;
	void* _loadedFontBuffer=NULL;
#endif
#if PLATFORM == PLAT_3DS
	char _3dsSoundProtectThreadIsAlive=1;
	Thread _3dsSoundUpdateThread;
	char _bgmIsLock=0;
#endif
char isActuallyUsingUma0=0;
int maxBusts = 9;
short textboxYOffset=0;
short textboxXOffset=0;
CrossTexture* currentCustomTextbox=NULL;
int outputLineScreenWidth;
int outputLineScreenHeight;
int messageInBoxXOffset=10;
int messageInBoxYOffset=0;
// 1 by default to retain compatibility with games converted before game specific Lua 
char gameHasTips=1;
char textOnlyOverBackground=1;
// This is a constant value between 0 and 127 that means that the text should be instantly displayed
#define TEXTSPEED_INSTANT 100
signed char textSpeed=1;
char isGameFolderMode=1;
char isEmbedMode;
int menuCursorSpaceWidth;
char canChangeBoxAlpha=1;
// When this variable is 1, we can assume that the current game is the default game because the user can't chose a different game when a default is set.
char defaultGameIsSet;
char nathanscriptIsInit=0;
char scriptUsesFileExtensions=0;
char scriptForceResourceUppercase=0;
char bustsStartInMiddle=1;
//char shouldUseBustCache=0;

// What scripts think the screen width and height is for sprite positions
// For Higurashi, this is 640x480
// For vnds, this is the DS' screen resolution
int scriptScreenWidth=640;
int scriptScreenHeight=480;

// X and Y scale applied to graphics size
double graphicsScale=1.0;
signed char dynamicScaleEnabled=1;
signed char higurashiUsesDynamicScale=0;

// This assumes the background is the biggest image. These values decide how much to scale all other elements, such as characters.
int actualBackgroundWidth;
int actualBackgroundHeight;
// If these values should not change because we're sure they're right. We can be sure these are right if they're specified in vnds ini file.
char actualBackgroundSizesConfirmedForSmashFive=0;

char* lastBackgroundFilename=NULL;
char* currentScriptFilename=NULL;
char* lastBGMFilename=NULL;

char currentlyVNDSGame=0;
char nextVndsBustshotSlot=0;
// If all the text should be cleared when the text reached the bottom of the screen when playing a VNDS game
signed char vndsClearAtBottom=0;
signed char showVNDSWarnings=1;
signed char imagesAreJpg=0;
signed char dynamicAdvBoxHeight=0;
// Will only be used in games it can be used in
signed char preferredTextDisplayMode=TEXTMODE_NVL;
signed char useSoundArchive=0;
legArchive soundArchive;
signed char lastVoiceSlot=-1;
int foundSetImgIndex = -1;
signed char vndsSpritesFade=1;
char textDisplayModeOverriden=0; // If the text display mode has been changed manually by the script

//
signed char forceShowQuit=-1;
signed char forceShowVNDSSettings=-1;
signed char forceShowVNDSSave=-1;
signed char forceShowRestartBGM=-1;
signed char forceArtLocationSlot=-1;
signed char forceScalingOption=-1;
signed char forceTextBoxModeOption=-1;
signed char forceVNDSFadeOption=-1;
signed char forceDebugButton=-1;
signed char forceResettingsButton = 1;
signed char forceTextOverBGOption = 1;
signed char forceFontSizeOption = 1;

/*
====================================================
*/
#include "../stolenCode/goodvita2ddraw.h"
CrossTexture* _loadGameImage(const char* path){
	if (imagesAreJpg){
		return loadJPG((char*)path);
	}else{
		return loadPNG((char*)path);
	}
}
char shouldShowWarnings(){
	if (currentlyVNDSGame && !showVNDSWarnings){
		return 0;
	}
	return 1;
}
// Directly remove file extension from string, string should not be const.
void removeFileExtension(char* _passedFilename){
	signed short i;
	for (i=strlen(_passedFilename)-1;i>=0;--i){
		if (_passedFilename[i]=='.'){
			_passedFilename[i]='\0';
			break;
		}
	}
}
/*
// This is what my brother made when I asked him to make a file extension chopper for me because he never helps me with code in return.
void removeFileExtension(char* String){
	int I=0, Size=strlen(String);
	for(I=Size-1; I>=0; I--){
	    if(String[I]=='.'){
	        String[I]=0;
	        break;
	    }
	    String[I]=0;
	}
}
*/
char* charToBoolString(char _boolValue){
	if (_boolValue){
		return "True";
	}
	return "False";
}
char* charToSwitch(char _boolValue){
	if (_boolValue){
		return "On";
	}
	return "Off";
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
	if (_newValue!=NULL){
		*_stringToChange = mallocForString(_newValue);
	}else{
		*_stringToChange=NULL;
	}
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
	if (menuSoundLoaded==1 && seVolume>0){
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
// Number of lines to draw is not zero based
void DrawMessageText(unsigned char _alpha, int _maxDrawLine){
	if (_maxDrawLine==-1){
		_maxDrawLine=MAXLINES;
	}
	int i;
	#if PLATFORM == PLAT_3DS
		if (textIsBottomScreen==1){
			startDrawingBottom();
			if (strlen(currentMessages[i])==0){
				goodDrawText(0,0,".",fontSize); // Hotfix to fix crash when no text on bottom screen.
			}
			for (i = 0; i < _maxDrawLine; i++){
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
	if (_alpha==255){
		for (i = 0; i < _maxDrawLine; i++){
			goodDrawText(textboxXOffset+messageInBoxXOffset,messageInBoxYOffset+12+textboxYOffset+i*(currentTextHeight),(char*)currentMessages[i],fontSize);
		}
	}else{
		for (i = 0; i < _maxDrawLine; i++){
			goodDrawTextColoredAlpha(textboxXOffset+messageInBoxXOffset,messageInBoxYOffset+12+textboxYOffset+i*(currentTextHeight),(char*)currentMessages[i],fontSize,255,255,255,_alpha);
		}
	}
	
	for (i=0;i<MAXIMAGECHAR;i++){
		if (imageCharType[i]!=-1){
			drawTextureScale(imageCharImages[imageCharType[i]],imageCharX[i],imageCharY[i],((double)textWidth(fontSize,IMAGECHARSPACESTRING)/ getTextureWidth(imageCharImages[imageCharType[i]])),((double)textHeight(fontSize)/getTextureHeight(imageCharImages[imageCharType[i]])));
		}
	}
}
void DrawMessageBox(char _textmodeToDraw){
	#if PLATFORM == PLAT_3DS
		if (textIsBottomScreen==1){
			return;
		}
	#endif
	if (_textmodeToDraw == TEXTMODE_NVL || currentCustomTextbox==NULL){
		drawRectangle(0,0,outputLineScreenWidth,outputLineScreenHeight,0,0,0,MessageBoxAlpha);
	}else{
		if (canChangeBoxAlpha){
			drawTextureScaleAlpha(currentCustomTextbox,textboxXOffset,textboxYOffset, (double)(outputLineScreenWidth-textboxXOffset)/(double)getTextureWidth(currentCustomTextbox), (double)(advboxHeight)/(double)getTextureHeight(currentCustomTextbox),MessageBoxAlpha);
		}else{
			drawTextureScale(currentCustomTextbox,textboxXOffset,textboxYOffset, (double)(outputLineScreenWidth-textboxXOffset)/(double)getTextureWidth(currentCustomTextbox), (double)(advboxHeight)/(double)getTextureHeight(currentCustomTextbox));
		}
	}
}
void DrawCurrentFilter(){
	if (currentFilterType!=FILTERTYPE_NEGATIVE){
		drawRectangle(0,0,outputLineScreenWidth,outputLineScreenHeight,filterR,filterG,filterB,filterA);
	}
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
void _loadSpecificFont(char* _filename){
	#if PLATFORM != PLAT_VITA
		loadFont(_filename);
	#else
		// Here I put custom code for loading fonts on the Vita. I need this for fonts with a lot of characters. Why? Well, if the font has a lot of characters, FreeType won't load all of them at once. It'll stream the characters from disk. At first that sounds good, but remember that the Vita breaks its file handles after sleep mode. So new text wouldn't work after sleep mode. I could fix this by modding libvita2d and making it use my custom IO commands, but I just don't feel like doing that right now.
		if (fontImage!=NULL){
			vita2d_free_font(fontImage);
		}
		if (_loadedFontBuffer!=NULL){
			free(_loadedFontBuffer);
		}
		FILE* fp = fopen(_filename, "rb");
		// Get file size
		fseek(fp, 0, SEEK_END);
		long _foundFilesize = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		// Read file into memory
		_loadedFontBuffer = malloc(_foundFilesize);
		fread(_loadedFontBuffer, _foundFilesize, 1, fp);
		fclose(fp);
		fontImage = vita2d_load_font_mem(_loadedFontBuffer,_foundFilesize);
		//fontImage = vita2d_load_font_file(filename);
	#endif
	currentTextHeight = textHeight(fontSize);
	singleSpaceWidth = textWidth(fontSize," ");
}
void ReloadFont(){
	#if PLATFORM != PLAT_3DS
		fixPath("assets/LiberationSans-Regular.ttf",globalTempConcat,TYPE_EMBEDDED);
	#elif PLATFORM == PLAT_3DS
		fixPath("assets/Bitmap-LiberationSans-Regular",globalTempConcat,TYPE_EMBEDDED);
	#else
		#error whoops
	#endif
	//_loadSpecificFont("sa0:data/font/pvf/ltn4.pvf");
	_loadSpecificFont(globalTempConcat);
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
// Return 1 if value was changed
char altMenuControls(char* _choice, int _menuMin, int _menuMax){
	char _newValue = MenuControls(*_choice,_menuMin,_menuMax);
	if (*_choice!=_newValue){
		*_choice = _newValue;
		return 1;
	}
	return 0;
}
char SafeLuaDoFile(lua_State* passedState, char* passedPath, char showMessage){
	if (checkFileExist(passedPath)==0){
		if (showMessage==1){
			LazyMessage("The LUA file",passedPath,"does not exist!","What will happen now?!");
		}
		return 0;
	}
	return lazyLuaError(luaL_dofile(passedState,passedPath));
}
void WriteToDebugFile(const char* stuff){
	#if PLATFORM == PLAT_COMPUTER
		printf("%s\n",stuff);
	#endif
	char* _tempDebugFileLocationBuffer = malloc(strlen(DATAFOLDER)+strlen("log.txt")+1);
	strcpy(_tempDebugFileLocationBuffer,DATAFOLDER);
	strcat(_tempDebugFileLocationBuffer,"log.txt");
	FILE *fp;
	fp = fopen(_tempDebugFileLocationBuffer, "a");
	if (!fp){
		LazyMessage("Failed to open debug file.",_tempDebugFileLocationBuffer,NULL,NULL);
	}else{
		fprintf(fp,"%s\n",stuff);
		fclose(fp);
	}
	free(_tempDebugFileLocationBuffer);
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
signed int getChangePerFrame(signed int _totalRequiredChange, signed int _totalTime){
	return atLeastOne(floor(((_totalRequiredChange)/(60*((double)_totalTime/1000)))));
}
void ClearMessageArray(char _doFadeTransition){
	currentLine=0;
	int i;
	int _totalAddedToHistory=0;
	for (i = 0; i < MAXLINES; i++){
		if (currentMessages[i][0]!='\0'){
			addToMessageHistory(currentMessages[i]);
			_totalAddedToHistory++;
		}
	}
	if (_totalAddedToHistory!=0 && MessageBoxEnabled && !isSkipping && _doFadeTransition){ // If we actually added stuff
		signed int _changePerFrame = getChangePerFrame(255,CLEARMESSAGEFADEOUTTIME);
		signed int _currentTextAlpha = 255;
		while (_currentTextAlpha>0){
			fpsCapStart();
			_currentTextAlpha-=_changePerFrame;
			if (_currentTextAlpha<0){
				_currentTextAlpha=0;
			}
			startDrawing();
			drawAdvanced(1,1,1,MessageBoxEnabled,1,0);
			DrawMessageText(_currentTextAlpha,-1);
			endDrawing();
			fpsCapWait();
		}
	}
	for (i = 0; i < MAXLINES; ++i){
		currentMessages[i][0]='\0';
	}
	for (i=0;i<MAXIMAGECHAR;i++){
		imageCharType[i]=-1;
	}
}
void SetAllMusicVolume(int _passedFixedVolume){
	int i;
	for (i = 0; i < MAXMUSICARRAY; i++){
		if (currentMusicHandle[i]!=0){
			setMusicVolume(currentMusicHandle[i],_passedFixedVolume);
		}
	}
}
int GetNextCharOnLine(int _linenum){
	return u_strlen(currentMessages[_linenum]);
}
int Password(int val, int _shouldHave){
	if (val==_shouldHave){
		return val+1;
	}else{
		return 0;
	}
}
void WriteIntToDebugFile(int a){
	char _tempCompleteNumberBuffer[11];
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
	if (canfree==1){
		if (passedBust->image!=NULL){
			freeTexture(passedBust->image);
		}
		if (passedBust->relativeFilename!=NULL){
				free(passedBust->relativeFilename);
			}
		if (passedBust->transformTexture!=NULL){
			freeTexture(passedBust->transformTexture);
		}
	}
	
	passedBust->image=NULL;
	passedBust->transformTexture=NULL;
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
	ClearMessageArray(0);
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
// Update what bustshots are doing depending on their bustStatus
void Update(){
	int i=0;
	for (i = 0; i < maxBusts; i++){
		if (Busts[i].bustStatus == BUST_STATUS_FADEIN){
			if (Busts[i].statusVariable2>0){
				Busts[i].statusVariable2-=17;
				if (Busts[i].statusVariable2>4000000){ // If we went back too far, causing the int to wrap.
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
		if (Busts[i].bustStatus == BUST_STATUS_TRANSFORM_FADEIN){
			Busts[i].alpha+=Busts[i].statusVariable;
			if (Busts[i].alpha>=255){
				Busts[i].alpha=255;
				Busts[i].bustStatus = BUST_STATUS_NORMAL;
				freeTexture(Busts[i].transformTexture);
				Busts[i].transformTexture=NULL;
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
	if (isSkipping==1 && !isDown(SCE_CTRL_SQUARE)){
		isSkipping=0;
	}
	if (wasJustPressed(SCE_CTRL_TRIANGLE)){
		SettingsMenu(1,currentlyVNDSGame,currentlyVNDSGame,!isActuallyUsingUma0 && PLATFORM != PLAT_VITA,!currentlyVNDSGame,0,currentlyVNDSGame,currentlyVNDSGame,(strcmp(VERSIONSTRING,"forgotversionnumber")==0));
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
		updateControlsGeneral();
		controlsEnd();
		if (isSkipping==1){
			endType=LINE_RESERVED;
		}
	}
	u64 _toggledTextboxTime=0;
	// 0 if we need to wait for sound to end.
	u64 _inBetweenLinesMilisecondsStart;
	int _chosenAutoWait;
	#if SOUNDPLAYER == SND_VITA
		if (lastVoiceSlot!=-1 && soundEffects[lastVoiceSlot]!=NULL && mlgsndIsPlaying(soundEffects[lastVoiceSlot])){
			_inBetweenLinesMilisecondsStart=0;
			_chosenAutoWait = autoModeVoicedWait;
		}else{
			_inBetweenLinesMilisecondsStart = getTicks();
			if (_inBetweenLinesMilisecondsStart==0){
				_inBetweenLinesMilisecondsStart=1;
			}
			_chosenAutoWait = autoModeWait;
		}
	#else
		_inBetweenLinesMilisecondsStart = getTicks();
		_chosenAutoWait = autoModeWait;
		if (_inBetweenLinesMilisecondsStart==0){
			_inBetweenLinesMilisecondsStart=1;
		}
	#endif
	// On PS Vita, prevent sleep mode if using auto mode
	#if PLATFORM == PLAT_VITA
		if (autoModeOn){
			sceKernelPowerTick(0);
		}
	#endif
	char _didPressCircle=0;
	do{
		fpsCapStart();
		controlsStart();
		Update();
		startDrawing();
		Draw(MessageBoxEnabled);
		// Easy save menu
		if (currentlyVNDSGame && isDown(SCE_CTRL_RTRIGGER)){
			drawRectangle(0,0,screenWidth,currentTextHeight*4,0,0,0,255);
			goodDrawText(0,currentTextHeight*0,"UP: Save slot 1",fontSize);
			goodDrawText(0,currentTextHeight*1,"DOWN: Save slot 2",fontSize);
			goodDrawText(0,currentTextHeight*2,"LEFT: Save slot 3",fontSize);
			goodDrawText(0,currentTextHeight*3,"RIGHT: Save slot 4",fontSize);
			if (wasJustPressed(SCE_CTRL_UP) || wasJustPressed(SCE_CTRL_DOWN) || wasJustPressed(SCE_CTRL_LEFT) || wasJustPressed(SCE_CTRL_RIGHT)){
				unsigned char _selectedSlot=1;
				if (wasJustPressed(SCE_CTRL_UP)){
					_selectedSlot=1;
				}else if (wasJustPressed(SCE_CTRL_DOWN)){
					_selectedSlot=2;
				}else if (wasJustPressed(SCE_CTRL_LEFT)){
					_selectedSlot=3;
				}else if (wasJustPressed(SCE_CTRL_RIGHT)){
					_selectedSlot=4;
				}
				easyVNDSSaveSlot(_selectedSlot);
				PlayMenuSound();
				drawRectangle(0,0,screenWidth,screenHeight,0,255,0,255);
			}
		}
		endDrawing();

		int touch_bool = 0;
		#if PLATFORM == PLAT_VITA
			memcpy(touch_old, touch, sizeof(touch_old));
			if (vndsVitaTouch){
				int port;
				for (port = 0; port < SCE_TOUCH_PORT_MAX_NUM; port++){
					sceTouchPeek(port, &touch[port], 1);
				}
				touch_bool = vndsVitaTouch && ((touch[SCE_TOUCH_PORT_FRONT].reportNum == 1) && (touch_old[SCE_TOUCH_PORT_FRONT].reportNum == 0));
			}
		#endif

		if (wasJustPressed(SCE_CTRL_CROSS) || touch_bool ){
			if (_didPressCircle==1){
				showTextbox();
			}
			endType = LINE_RESERVED;
		}
		if (wasJustPressed(SCE_CTRL_CIRCLE)){
			if (_didPressCircle==1){
				if (_toggledTextboxTime!=0){
					_inBetweenLinesMilisecondsStart+=getTicks()-_toggledTextboxTime;
					_toggledTextboxTime=0;
				}
				MessageBoxEnabled = !MessageBoxEnabled;
			}else if (MessageBoxEnabled==1){
				MessageBoxEnabled=0;
				_didPressCircle=1;
				_toggledTextboxTime=getTicks();
			}
		}
		updateControlsGeneral();
		if (wasJustPressed(SCE_CTRL_START)){
			DrawHistory(messageHistory);
		}
		controlsEnd();
		fpsCapWait();
		if (autoModeOn==1 && _toggledTextboxTime==0){
			if (_inBetweenLinesMilisecondsStart!=0){ // If we're not waiting for audio to end
				if (getTicks()>=(_inBetweenLinesMilisecondsStart+_chosenAutoWait)){
					endType = LINE_RESERVED;
				}
			}else{
				#if PLATFORM == PLAT_VITA
					// Check if audio has ended yet.
					if (mlgsndIsPlaying(soundEffects[lastVoiceSlot])==0){
						_inBetweenLinesMilisecondsStart = getTicks();
					}
				#endif
			}
		}
		exitIfForceQuit();
	}while(endType==Line_Normal || endType == Line_WaitForInput);
	// If we pressed a button to continue the text and we're doing VNDS ADV mode
	if (currentlyVNDSGame && gameTextDisplayMode==TEXTMODE_ADV && endType==LINE_RESERVED){
		ClearMessageArray(1);
	}

	endType=Line_ContinueAfterTyping;
	lastVoiceSlot=-1;
}
// This is used in background and bust drawing
// For Higurashi, this is used to get the center of the screen for all images.
// For VNDS, this is just used to get the position of the background.
void GetXAndYOffset(CrossTexture* _tempImg, signed int* _tempXOffset, signed int* _tempYOffset){
	if (dynamicScaleEnabled){
		*_tempXOffset = floor((screenWidth-applyGraphicsScale(getTextureWidth(_tempImg)))/2);
		*_tempYOffset = floor((screenHeight-applyGraphicsScale(getTextureHeight(_tempImg)))/2);
	}else{
		*_tempXOffset = floor((screenWidth-getTextureWidth(_tempImg))/2);
		*_tempYOffset = floor((screenHeight-getTextureHeight(_tempImg))/2);
		// If they're bigger than the screen, assume that they're supposed to scroll or something
		if (*_tempXOffset<0){
			*_tempXOffset=0;
		}
		if (*_tempYOffset<0){
			*_tempYOffset=0;
		}
	}
}
double GetXOffsetScale(CrossTexture* _tempImg){
	if (dynamicScaleEnabled){
		return applyGraphicsScale(actualBackgroundWidth)/(double)scriptScreenWidth;
	}else{
		if (getTextureWidth(_tempImg)>screenWidth){
			return (screenWidth/scriptScreenWidth);
		}
		return (getTextureWidth(_tempImg)/(float)scriptScreenWidth);
	}
}
double GetYOffsetScale(CrossTexture* _tempImg){
	if (dynamicScaleEnabled){
		return applyGraphicsScale(actualBackgroundHeight)/(double)scriptScreenHeight;
	}else{
		if (getTextureHeight(_tempImg)>screenHeight){
			return (screenHeight/scriptScreenHeight);
		}
		return ( getTextureHeight(_tempImg)/(float)scriptScreenHeight);
	}
}
void DrawBackgroundAlpha(CrossTexture* passedBackground, unsigned char passedAlpha){
	if (passedBackground!=NULL){
		signed int _tempXOffset;
		signed int _tempYOffset;
		GetXAndYOffset(passedBackground,&_tempXOffset,&_tempYOffset);
		//printf("%f;%d;%d;%d;%d\n",graphicsScale,_tempXOffset,_tempYOffset,(int)(getTextureWidth(passedBackground)*graphicsScale),(int)(getTextureHeight(passedBackground)*graphicsScale));
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
		// If busts don't start in the middle, they start at the start of the background
		if (currentBackground!=NULL){
			GetXAndYOffset(currentBackground,&_tempXOffset,&_tempYOffset);
		}
	}

	//printf("=====\n");
	//printf("GraphicsScale:%f\n",graphicsScale);
	//printf("TempYOffset:%d\n",_tempYOffset);
	//printf("YOffset;%d\n",passedBust->yOffset);
	//printf("cacheYOffsetScale:%f\n",passedBust->cacheYOffsetScale);
	//printf("TotalYOffset:%d\n",(int)(_tempYOffset+passedBust->yOffset*passedBust->cacheYOffsetScale));
	//printf("ImageHeight:%d\n",getTextureHeight(passedBust->image));
	//printf("SclaedImageHeight:%d\n",(int)(getTextureHeight(passedBust->image)*graphicsScale));

	// If the busts end one pixel off again, it may be because these are now int instead of float.
	float _drawBustX = ceil(_tempXOffset+passedBust->xOffset*passedBust->cacheXOffsetScale);
	float _drawBustY = ceil(_tempYOffset+passedBust->yOffset*passedBust->cacheYOffsetScale);
	if (passedBust->alpha==255){
		drawTextureScaleAlphaGood(passedBust->image,_drawBustX,_drawBustY,graphicsScale,graphicsScale,255);
	}else{
		if (passedBust->bustStatus==BUST_STATUS_TRANSFORM_FADEIN){
			drawTextureScaleAlphaGood(passedBust->transformTexture,_drawBustX,_drawBustY, graphicsScale, graphicsScale, 255-passedBust->alpha);
		}
		drawTextureScaleAlphaGood(passedBust->image,_drawBustX,_drawBustY, graphicsScale, graphicsScale, passedBust->alpha);
	}
}
void RecalculateBustOrder(){
	int i, j, k;
	for (i=0;i<maxBusts;i++){
		bustOrder[i]=255;
		bustOrderOverBox[i]=255;
	}
	// This generates the orderOfAction list
	// i is the the current orderOfAction slot.
	for (i=0;i<maxBusts;i++){
		// j is the current fighter we're testig
		for (j=0;j<maxBusts;j++){
			// If current entity speed greater than
			if ((bustOrder[i]==255 || Busts[j].layer>Busts[bustOrder[i]].layer) && (Busts[j].layer<=31)){
				// Loops through all of orderOfAction to make sure you're not already in orderOfAction
				for (k=0;k<maxBusts;k++){
					if (bustOrder[k]==j){
						break;
					}else{
						if (k==maxBusts-1){
							bustOrder[i]=j;
						}
					}
				}
			}
		}
	}
	// Do another calculation for busts that have a layer greater than 31 and therefor are over the message box
	for (i=0;i<maxBusts;i++){
		// j is the current fighter we're testig
		for (j=0;j<maxBusts;j++){
			// If current entity speed greater than
			if ((bustOrderOverBox[i]==255 || Busts[j].layer>Busts[bustOrderOverBox[i]].layer) && (Busts[j].layer>31)){
				// Loops through all of orderOfAction to make sure you're not already in orderOfAction
				for (k=0;k<maxBusts;k++){
					if (bustOrderOverBox[k]==j){
						break;
					}else{
						if (k==maxBusts-1){
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

	currentReadNumber[3]='\0';
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
void _LazyMessage(const char* stra, const char* strb, const char* strc, const char* strd, char _doWait){
	controlsStart();
	controlsEnd();
	do{
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
		if (_doWait){
			goodDrawText(32,screenHeight-32-currentTextHeight,SELECTBUTTONNAME" to continue.",fontSize);
		}
		endDrawing();
		fpsCapWait();
		exitIfForceQuit();
	}while (currentGameStatus!=GAMESTATUS_QUIT && _doWait);
}
/*
void error( const char* _stringFormat, ... ) {
	va_list _getLengthArgs;
	va_list _doWriteArgs;
	char* _completeString;

	va_start( _getLengthArgs, _stringFormat );
	va_copy(_doWriteArgs,_getLengthArgs); // vsnprintf modifies the state of _getLengthArgs so that we can't use it anymore, copy it so we can use it twice in total
	_completeString = malloc(vsnprintf(NULL,0,_stringFormat,_getLengthArgs)+1); // Get the size it would've written
	va_end( _getLengthArgs );
	vsprintf(_completeString,_stringFormat,_doWriteArgs); // This should not overflow because we already got the exact size we'll need
	va_end( _doWriteArgs ); // Even though it's a copy we still need to va_end it.

	printf("%s\n",_completeString);
	free(_completeString);
}

-- make a method called like wrapText, returns array of strings and int with length of aray
*/
void LazyMessage(const char* stra, const char* strb, const char* strc, const char* strd){
	_LazyMessage(stra,strb,strc,strd,1);
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
		if (menuSound!=NULL){
			showErrorIfNull(menuSound);
			setSFXVolumeBefore(menuSound,FixSEVolume(256));
		}
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
// If we're doing textOnlyOverBackground, make the textbox start at the right place according to global variables for background sizes
void updateTextPositions(){
	if (textOnlyOverBackground){
		textboxXOffset = floor((float)(screenWidth-applyGraphicsScale(actualBackgroundWidth))/2);
		outputLineScreenWidth = screenWidth - textboxXOffset;
	}
	#if PLATFORM == PLAT_3DS
		if (textIsBottomScreen==1){
			outputLineScreenWidth=320;
			outputLineScreenHeight=240;
		}
	#endif
}
// Using the global variables for background sizes, update the global graphics scale factor
// Used for dynamic graphic scaling!
void updateGraphicsScale(){
	if (dynamicScaleEnabled){
		if (((double)screenWidth)/actualBackgroundWidth < ((double)screenHeight)/actualBackgroundHeight){
			graphicsScale = (((double)screenWidth)/actualBackgroundWidth);
		}else{
			graphicsScale = (((double)screenHeight)/actualBackgroundHeight);
		}
	}else{
		graphicsScale=1;
	}
}
void setTextOnlyOverBackground(char _newValue){
	textOnlyOverBackground=_newValue;
	if (gameTextDisplayMode == TEXTMODE_ADV){
		textboxYOffset=screenHeight-advboxHeight;
	}else{
		textboxYOffset=0;
	}
	if (textOnlyOverBackground==0){
		textboxXOffset=0;
		outputLineScreenWidth = screenWidth;
	}else{
		updateTextPositions();
	}
}
void applyTextboxChanges(){
	setTextOnlyOverBackground(textOnlyOverBackground);
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
		for (i=0;_workableFilename[i]!='\0';i++){
			_workableFilename[i] = toupper(_workableFilename[i]);
		}
	}

	// Remove file extension and put it in _foundFileExtension if file extension is included
	if (_extensionIncluded){
		signed short i;
		short _cachedStrlen = strlen(_workableFilename);
		for (i=_cachedStrlen-1;i>=0;i--){
			if (_workableFilename[i]=='.' && i!=_cachedStrlen-1){
				_foundFileExtension = malloc(strlen(&(_workableFilename[i]))+1);
				strcpy(_foundFileExtension,&(_workableFilename[i]));
				_workableFilename[i]='\0';
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
CrossTexture* safeLoadGameImage(const char* filename, char _folderPreference, char _extensionIncluded){
	char* _tempFoundFilename;
	_tempFoundFilename = LocationStringFallback(filename,_folderPreference,_extensionIncluded,scriptForceResourceUppercase);
	if (_tempFoundFilename==NULL){
		if (shouldShowWarnings()){
			LazyMessage("Image not found.",filename,"What will happen now?!",NULL);
		}
		return NULL;
	}
	CrossTexture* _returnLoadedPNG = _loadGameImage(_tempFoundFilename);
	free(_tempFoundFilename);
	#if PLATFORM == PLAT_VITA
		// Smooth scaling
		if (currentlyVNDSGame){
			vita2d_texture_set_filters(_returnLoadedPNG,SCE_GXM_TEXTURE_FILTER_LINEAR,SCE_GXM_TEXTURE_FILTER_LINEAR);
		}
	#endif
	return _returnLoadedPNG;
}
int decrementWithMax(int _passedCurrentValue, int _passedMax){
	if (_passedCurrentValue==0){
		return _passedMax;
	}
	return --_passedCurrentValue;
}
int incrementWithMax(int _passedCurrentValue, int _passedMax){
	if (_passedCurrentValue==_passedMax){
		return 0;
	}
	return ++_passedCurrentValue;
}
void _freeCachedImage(cachedImage* _passedImage){
	changeMallocString(&(_passedImage->filename),NULL);
	if (_passedImage->image!=NULL){
		freeTexture(_passedImage->image);
		_passedImage->image=NULL;
	}
}
// For all active slots in the bust cache, free them.
// Checks if something is NULL before freeing it multiple times because I just shove checks everywhere hoping nothing breaks.
void freeBustCache(){
	unsigned char i;
	for (i=0;i<MAXBUSTCACHE;++i){
		if (bustCache[i].filename!=NULL){
			_freeCachedImage(&(bustCache[i]));
		}
	}
}
cachedImage* getFreeBustCacheSlot(){
	unsigned char i;
	for (i=0;i<MAXBUSTCACHE;++i){
		if (bustCache[i].filename==NULL){
			return &(bustCache[i]);
		}
	}
	// If this one is full then overwrite the first slot
	_freeCachedImage(&(bustCache[0]));
	return &(bustCache[0]);
}
cachedImage* searchBustCache(const char* _passedFilename){
	unsigned char i;
	for (i=0;i<MAXBUSTCACHE;++i){
		if (bustCache[i].filename!=NULL){
			if (strcmp(_passedFilename,bustCache[i].filename)==0){
				return &(bustCache[i]);
			}
		}
	}
	return NULL;
}
void increaseBustArraysSize(int _oldMaxBusts, int _newMaxBusts){
	printf("Increase max bust array to %d\n",_newMaxBusts);
	Busts = recalloc(Busts, _newMaxBusts * sizeof(bust), _oldMaxBusts * sizeof(bust));
	bustOrder = recalloc(bustOrder, _newMaxBusts * sizeof(char), _oldMaxBusts * sizeof(char));
	bustOrderOverBox = recalloc(bustOrderOverBox, _newMaxBusts * sizeof(char), _oldMaxBusts * sizeof(char));
	RecalculateBustOrder();
}
signed int atLeastOne(signed int _input){
	return _input==0 ? 1 : _input;
}
#define ADV_DYNAMICBOX_MILLISECONDSTRETCH 200
void smoothADVBoxHeightTransition(int _oldHeight, int _newHeight, int _maxDrawLine){
	if (_oldHeight==_newHeight){
		return;
	}
	if (!isSkipping){
		// Absolute value
		signed int _changePerFrame = atLeastOne(floor(((_newHeight-_oldHeight)/(60*((double)ADV_DYNAMICBOX_MILLISECONDSTRETCH/1000)))));
		char _isGoingDown = _changePerFrame<0;
		_changePerFrame = _changePerFrame < 0 ? _changePerFrame*-1 : _changePerFrame;
		while (1){
			fpsCapStart();
			// Apply the changes
			if (_isGoingDown){
				advboxHeight-=_changePerFrame;
				if (advboxHeight<_newHeight){
					break;
				}
			}else{
				advboxHeight+=_changePerFrame;
				if (advboxHeight>_newHeight){
					break;
				}
			}
			applyTextboxChanges();
			// Draw the changes
			startDrawing();
			drawAdvanced(1,1,1,1,1,0); // Don't draw message text during transitions
			if (_maxDrawLine!=0){
				DrawMessageText(255,_maxDrawLine);
			}
			endDrawing();
			fpsCapWait();
		}
	}
	advboxHeight=_newHeight;
	applyTextboxChanges();
}
void updateDynamicADVBox(int _maxDrawLine, int _overrideNewHeight){
	if (_maxDrawLine==-1){
		_maxDrawLine=MAXLINES;
	}
	if (_overrideNewHeight==-1){
		_overrideNewHeight=1; // One extra line to be safe
		short i;
		for (i=0;i<MAXLINES;++i){
			if (currentMessages[i][0]!='\0'){
				_overrideNewHeight=i+2; // Last non-empty line. Adding 1 is for the free line, adding another 1 is because line index is 0 based
			}
		}
	}
	int _newAdvBoxHeight = _overrideNewHeight*currentTextHeight;
	if (_maxDrawLine!=0){
		smoothADVBoxHeightTransition(advboxHeight,_newAdvBoxHeight,_maxDrawLine);
	}else{
		advboxHeight=_newAdvBoxHeight;
		applyTextboxChanges();
	}
}
void enableVNDSADVMode(){
	gameTextDisplayMode=TEXTMODE_ADV;
	dynamicAdvBoxHeight=1;
	applyTextboxChanges();
	loadADVBox();
	updateDynamicADVBox(0,-1);
}
void disableVNDSADVMode(){
	gameTextDisplayMode=TEXTMODE_NVL;
	dynamicAdvBoxHeight=0;
	applyTextboxChanges();
}
char* getFileExtension(char* _passedFilename){
	return &(_passedFilename[strlen(_passedFilename)-3]);
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
	for (i=0;i<maxBusts;i++){
		if (Busts[i].isActive==1){
			FadeBustshot(i,_time,0);
		}
	}
	if (_wait==1){
		char _isDone=0;
		while (_isDone==0){
			_isDone=1;
			for (i=0;i<maxBusts;i++){
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
				for (i=0;i<maxBusts;i++){
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
void waitForBustFade(){
	int i;
	while(1){
		char _didBreak=0;
		for (i=0;i<maxBusts;i++){
			//printf("%d is not done.",i);
			if (Busts[i].isActive==1){
				if (Busts[i].bustStatus!=BUST_STATUS_NORMAL){
					_didBreak=1;
					break;
				}
			}
		}
		if (!_didBreak){
			break;
		}
		fpsCapStart();
		Update();
		startDrawing();
		Draw(MessageBoxEnabled);
		endDrawing();
		fpsCapWait();
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
		int _totalFrames = floor(60*(time/(double)1000));
		if (_totalFrames==0){
			_totalFrames=1;
		}
		_alphaPerFrame=floor(255/_totalFrames);
		if (_alphaPerFrame==0){
			_alphaPerFrame=1;
		}
	}
	// If we're NOT doing the VNDS easy bust reset trick
	if (!(lastBackgroundFilename!=NULL && strcmp(lastBackgroundFilename,_filename)==0)){
		changeMallocString(&lastBackgroundFilename,_filename);
		CrossTexture* newBackground = safeLoadGameImage(_filename,graphicsLocation,scriptUsesFileExtensions);
		if (newBackground==NULL){
			freeTexture(currentBackground);
			currentBackground=NULL;
			return;
		}
		if (filterActive && currentFilterType==FILTERTYPE_NEGATIVE){
			invertImage(newBackground,0);
		}
		if (actualBackgroundSizesConfirmedForSmashFive==0){
			actualBackgroundWidth = getTextureWidth(newBackground);
			actualBackgroundHeight = getTextureHeight(newBackground);
			updateGraphicsScale();
			updateTextPositions();
		}
		while (_backgroundAlpha<255){
			fpsCapStart();
	
			Update();
			_backgroundAlpha+=_alphaPerFrame;
			if (_backgroundAlpha>255){
				_backgroundAlpha=255;
			}
			// Change alpha of busts made on the last line
			for (i = maxBusts-1; i != -1; i--){
				if (bustOrder[i]!=255 && Busts[bustOrder[i]].isActive==1 && Busts[bustOrder[i]].lineCreatedOn == currentScriptLine-1){
					Busts[bustOrder[i]].alpha = _backgroundAlpha;
				}
			}
			startDrawing();
			drawAdvanced(1,1,0,0,1,0);
			DrawBackgroundAlpha(newBackground,_backgroundAlpha);
			// Draw busts created on the last line at the same alpha as the new background
			for (i = maxBusts-1; i != -1; i--){
				if (bustOrder[i]!=255 && Busts[bustOrder[i]].isActive==1 && Busts[bustOrder[i]].lineCreatedOn == currentScriptLine-1){
					DrawBust(&(Busts[bustOrder[i]]));
				}
			}
			drawAdvanced(0,0,1,MessageBoxEnabled,0,MessageBoxEnabled);
			endDrawing();
	
			controlsStart();
			if (wasJustPressed(SCE_CTRL_CROSS)){
				_backgroundAlpha=254;
			}
			controlsEnd();
			fpsCapWait();
		}

		if (currentBackground!=NULL){
			freeTexture(currentBackground);
		}
		currentBackground=newBackground;
	}
	// Fix alpha for busts created on the last line
	for (i = maxBusts-1; i != -1; i--){
		if (bustOrder[i]!=255 && Busts[bustOrder[i]].isActive==1 && Busts[bustOrder[i]].lineCreatedOn == currentScriptLine-1){
			Busts[bustOrder[i]].alpha = 255;
		}
	}
	// Delete old bust cache before putting new ones in it
	freeBustCache();
	// Update the bust cache will all our current busts that we're about to free
	for (i=0;i<maxBusts;++i){
		if (Busts[i].isActive==1 && Busts[i].lineCreatedOn != currentScriptLine-1){
			cachedImage* _slotToUse = getFreeBustCacheSlot();
			_slotToUse->filename = Busts[i].relativeFilename; // Already malloc'd, I think.
			_slotToUse->image = Busts[i].image;

			// Must set these to NULL because we free later
			Busts[i].relativeFilename=NULL;
			Busts[i].image=NULL;
		}
	}
	// Fix the bust cache if cached images are inverted.
	if (filterActive && currentFilterType==FILTERTYPE_NEGATIVE){
		for (i=0;i<maxBusts;++i){
			if (Busts[i].isActive==1 && Busts[i].lineCreatedOn != currentScriptLine-1){
				invertImage(Busts[i].image,0);
			}
		}
	}
	for (i=0;i<maxBusts;i++){
		if (Busts[i].isActive==1 && Busts[i].lineCreatedOn != currentScriptLine-1){
			ResetBustStruct(&Busts[i], 1); // Won't double free the busts in the cache because we set the references in the busts to NULL
		}
	}
}
void MoveBustSlot(unsigned char _sourceSlot, unsigned char _destSlot){
	if (_sourceSlot==_destSlot){
		return;
	}
	ResetBustStruct(&(Busts[_destSlot]),1);
	memcpy(&(Busts[_destSlot]),&(Busts[_sourceSlot]),sizeof(bust));
	ResetBustStruct(&(Busts[_sourceSlot]),0);
	RecalculateBustOrder();
}
// Returns number of old bust slots if bust slots were added
int DrawBustshot(unsigned char passedSlot, const char* _filename, int _xoffset, int _yoffset, int _layer, int _fadeintime, int _waitforfadein, int _isinvisible){
	if (passedSlot>=maxBusts){
		int _oldMaxBusts = maxBusts;
		maxBusts = passedSlot+1;
		increaseBustArraysSize(_oldMaxBusts,maxBusts);
		if (DrawBustshot(passedSlot,_filename,_xoffset,_yoffset,_layer,_fadeintime,_waitforfadein,_isinvisible)!=0){
			printf("Strange error.\n");
		}
		return _oldMaxBusts;
	}
	if (isSkipping==1 || _fadeintime==0){
		_fadeintime=0;
		_waitforfadein=0;
	}
	if (!currentlyVNDSGame && _fadeintime!=0){ // HACK, just don't do this for VNDS because we don't have bust fadeout
		// I wonder why these three lines are here. Probably something to do with the lack of redraw between frames.
		startDrawing();
		Draw(MessageBoxEnabled);
		endDrawing();
	}
	unsigned char skippedInitialWait=0;
	if (Busts[passedSlot].isActive){ // Detect if we need to do a fadein transition
		if (_fadeintime!=0){ // Only do all this fade stupidity if we're going to fade in the first place.
			CrossTexture* _cachedOldTexture = Busts[passedSlot].image;
			Busts[passedSlot].image=NULL;
			ResetBustStruct(&(Busts[passedSlot]),1);
			Busts[passedSlot].transformTexture = _cachedOldTexture;
			Busts[passedSlot].bustStatus = BUST_STATUS_TRANSFORM_FADEIN;
		}else{
			ResetBustStruct(&(Busts[passedSlot]),1);
		}
	}else{
		ResetBustStruct(&(Busts[passedSlot]),0); // Remove leftover data, nothing in here right now should be malloc'd
	}

	cachedImage* _possibleCachedImage = searchBustCache(_filename);
	if (_possibleCachedImage!=NULL){
		Busts[passedSlot].image = _possibleCachedImage->image;
		Busts[passedSlot].relativeFilename=_possibleCachedImage->filename;
		// Remove from cache so we don't free it early
		_possibleCachedImage->image = NULL;
		_possibleCachedImage->filename = NULL;
	}else{
		Busts[passedSlot].image = safeLoadGameImage(_filename,graphicsLocation,scriptUsesFileExtensions);
		Busts[passedSlot].relativeFilename=mallocForString(_filename);
		if (Busts[passedSlot].image==NULL){
			free(Busts[passedSlot].relativeFilename);
			Busts[passedSlot].relativeFilename=NULL;
			ResetBustStruct(&(Busts[passedSlot]),0);
			return 0;
		}
	}
	if (filterActive==1 && currentFilterType==FILTERTYPE_NEGATIVE){
		invertImage(Busts[passedSlot].image,0);
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
		int _time;
		if (Busts[passedSlot].bustStatus == BUST_STATUS_TRANSFORM_FADEIN){
			_time = _fadeintime; // Transform fadein doesn't waste time
		}else{
			_time = floor(_fadeintime/2); // Normal fadein wastes half the time for no reason
		}
		int _totalFrames = floor(60*(_time/(double)1000));
		if (_totalFrames==0){
			_totalFrames=1;
		}
		int _alphaPerFrame=floor(255/_totalFrames);
		if (_alphaPerFrame==0){
			_alphaPerFrame=1;
		}
		Busts[passedSlot].statusVariable=_alphaPerFrame;
		if (Busts[passedSlot].bustStatus != BUST_STATUS_TRANSFORM_FADEIN){ // If we're not transforming, go ahead and set this as a normal fadein
			Busts[passedSlot].statusVariable2 = _timeTotal -_time;
			Busts[passedSlot].bustStatus = BUST_STATUS_FADEIN;
		}else{
			Busts[passedSlot].statusVariable2 = 0;
		}
	}else{
		Busts[passedSlot].bustStatus = BUST_STATUS_NORMAL;
	}
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
			fpsCapWait();
		}
	}
	return 0;
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
		printf("lagometer: %ld\n",getTicks()-_LagTestStart);
	}
#endif
char* getSpecificPossibleSoundFilename(const char* _filename, char* _folderName){
	char* tempstringconcat = malloc(strlen(streamingAssets)+strlen(_folderName)+strlen(_filename)+1+4);
	strcpy(tempstringconcat,streamingAssets);
	strcat(tempstringconcat,_folderName);
	strcat(tempstringconcat,_filename);
	//
	if (scriptUsesFileExtensions){
		if (checkFileExist(tempstringconcat)==1){
			return tempstringconcat;
		}
		removeFileExtension(tempstringconcat);
	}
	//
	strcat(tempstringconcat,".ogg");
	if (checkFileExist(tempstringconcat)==1){
		return tempstringconcat;
	}
	//
	#if SOUNDPLAYER != SND_VITA
		removeFileExtension(tempstringconcat);
		strcat(tempstringconcat,".wav");
		if (checkFileExist(tempstringconcat)==1){
			return tempstringconcat;
		}
	#endif
	//
	#if SOUNDPLAYER == SND_VITA
		removeFileExtension(tempstringconcat);
		strcat(tempstringconcat,".mp3");
		if (checkFileExist(tempstringconcat)==1){
			return tempstringconcat;
		}
	#endif
	//
	free(tempstringconcat);
	return NULL;
}
#if SOUNDPLAYER == SND_VITA
	char getProbableSoundFormat(const char* _passedFilename){
		if (strlen(_passedFilename)>=4){
			// Copy file extension to another buffer so we can modify it
			char _copiedExtension[5];
			strcpy(_copiedExtension,&(_passedFilename[strlen(_passedFilename)-4]));

			// Convert the extension to lower case
			char i;
			for (i=0;i<4;++i){
				if (_copiedExtension[i]<='Z' && _copiedExtension[i]>='A'){
					_copiedExtension[i]+=32;
				}
			}
			// Do
			if (strcmp(_copiedExtension,".mp3")==0){
				return FILE_FORMAT_MP3;
			}/*else if (strcmp(_copiedExtension,".wav")==0){
				return FILE_FORMAT_WAV;
			}*/else if (strcmp(_copiedExtension,".ogg")==0){
				return FILE_FORMAT_OGG;
			}
		}
		LazyMessage("File format not found.",_passedFilename,NULL,NULL);
		return FILE_FORMAT_NONE;
	}
#else
	char getProbableSoundFormat(const char* _passedFilename){
		LazyMessage("Error with getProbableSoundFormat being the wrong version. Contact MyLegGuy.",_passedFilename,NULL,NULL);
		return 0;
	}
#endif
legArchiveFile soundArchiveGetFilename(const char* _filename, char* _foundFormat){
	legArchiveFile _possibleResult;
	*_foundFormat=0;
	_possibleResult.fp=NULL; // How we know if file not found
	if (scriptUsesFileExtensions){
		_possibleResult = getAdvancedFile(soundArchive,_filename);
		if (_possibleResult.fp!=NULL){
			*_foundFormat = getProbableSoundFormat(_filename);
			return _possibleResult;
		}	
	}
	//
	char* tempstringconcat = malloc(strlen(_filename)+4+1);
	strcpy(tempstringconcat,_filename);
	if (scriptUsesFileExtensions){
		removeFileExtension(tempstringconcat); // Remove the file extension we had on it before, we're adding new ones
	}
	strcat(tempstringconcat,".ogg");
	_possibleResult = getAdvancedFile(soundArchive,tempstringconcat);
	if (_possibleResult.fp!=NULL){
		*_foundFormat = getProbableSoundFormat(tempstringconcat);
		free(tempstringconcat);
		return _possibleResult;
	}
	//
	#if SOUNDPLAYER != SND_VITA
		removeFileExtension(tempstringconcat);
		strcat(tempstringconcat,".wav");
		_possibleResult = getAdvancedFile(soundArchive,tempstringconcat);
		if (_possibleResult.fp!=NULL){
			*_foundFormat = getProbableSoundFormat(tempstringconcat);
			free(tempstringconcat);
			return _possibleResult;
		}
	#endif
	//
	#if SOUNDPLAYER == SND_VITA
		removeFileExtension(tempstringconcat);
		strcat(tempstringconcat,".mp3");
		_possibleResult = getAdvancedFile(soundArchive,tempstringconcat);
		if (_possibleResult.fp!=NULL){
			*_foundFormat = getProbableSoundFormat(tempstringconcat);
			free(tempstringconcat);
			return _possibleResult;
		}
	#endif
	//
	free(tempstringconcat);
	return _possibleResult;
}
// Will use fallbacks
// Return NULL if file not exist
char* getSoundFilename(const char* _filename, char _preferedDirectory){
	char* tempstringconcat=NULL;
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
	}else if (_preferedDirectory==PREFER_DIR_NOMEIMPORTA){
		return getSoundFilename(_filename,PREFER_DIR_SE);
	}else{
		printf("Invalid preference %d\n",_preferedDirectory);
		return getSoundFilename(_filename,PREFER_DIR_VOICE);
	}
	return tempstringconcat;
}
// Cast returned pointer
void* loadGameAudio(const char* _filename, char _preferedDirectory, char _isSE){
	void* _tempHoldSlot=NULL;
	
	// First try and find the file in the folders
	char* tempstringconcat = getSoundFilename(_filename,_preferedDirectory);
	if (tempstringconcat!=NULL && checkFileExist(tempstringconcat)){
		if (_isSE){
			_tempHoldSlot = loadSound(tempstringconcat);
		}else{
			_tempHoldSlot = loadMusic(tempstringconcat);
		}
		showErrorIfNull(_tempHoldSlot);
	}else{
		//LazyMessage("[DEBUG MESSAGE] not found in SE folder.",_filename,tempstringconcat,"will fallback on archive.");
	}
	free(tempstringconcat);
	// If we didn't find the file in the folders and we can use the sound archive, try that
	if (_tempHoldSlot==NULL && useSoundArchive){
		#if SOUNDPLAYER == SND_VITA
			char _foundFormat=0;
			legArchiveFile _foundArchiveFile = soundArchiveGetFilename(_filename,&_foundFormat);
			if (_foundArchiveFile.fp==NULL){
				//LazyMessage(filename,"not found in archive",NULL,NULL);
			}else{
				_tempHoldSlot = _mlgsnd_loadAudioFILE(_foundArchiveFile, _foundFormat, !_isSE, 1);
			}
		#else
			LazyMessage("sound archive not supported.",NULL,NULL,NULL);
		#endif
	}
	if (_tempHoldSlot==NULL){
		if (shouldShowWarnings()){
			LazyMessage(_filename,"Sound not found in folders.",useSoundArchive ? "Not found in archive either." : NULL,NULL);
		}
	}
	return _tempHoldSlot;
}
// _dirRelativeToStreamingAssetsNoEndSlash should start AND END with a slash
// Example
// /SE/
// DO NOT FIX THE SE VOLUME BEFORE PASSING ARGUMENT
void GenericPlayGameSound(int passedSlot, const char* filename, int unfixedVolume, char _preferedDirectory, float _passedVolumeFixScale){
	if (passedSlot>=MAXSOUNDEFFECTARRAY){
		LazyMessage("Sound effect slot too high.","No action will be taken.",NULL,NULL);
		return;
	}
	if (strlen(filename)==0){
		printf("Sound effect filename empty.\n");
		return;
	}
	if (soundEffects[passedSlot]!=NULL){
		stopSound(soundEffects[passedSlot]);
		freeSound(soundEffects[passedSlot]);
		soundEffects[passedSlot]=NULL;
	}
	soundEffects[passedSlot] = loadGameAudio(filename,_preferedDirectory,1);
	if (soundEffects[passedSlot]==NULL){
		//// Don't try if it's .aac file
		//if (currentlyVNDSGame && strlen(filename)>=4){
		//	if (strcmp(&(filename[strlen(filename)-4]),".aac")==0){ // No aac support
		//		return;
		//	}
		//}
		//WriteToDebugFile("SE file not found");
		//WriteToDebugFile(filename);
	}else{
		CROSSPLAYHANDLE _tempHandle = playSound(soundEffects[passedSlot],1,passedSlot+10);
		setSFXVolume(_tempHandle,GenericFixSpecificVolume(unfixedVolume,_passedVolumeFixScale));
		// Used for auto mode
		lastVoiceSlot=passedSlot;
	}
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
	if (!MessageBoxEnabled){
		showTextbox();
	}

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
					if (_imagechartype != IMAGECHARUNKNOWN){
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
					}
				}else if (message[i]=='\n'){
					message[i]='\0';
					strcpyNO1(currentMessages[currentLine],&(message[lastNewlinePosition+1]));
					currentLine++;
					lastNewlinePosition=i;
				}
			}else{
				//http://jafrog.com/2013/11/23/colors-in-terminal.html
				if (message[i]=='\\' || message[i]=='x'){ // I saw that Umineko VNDS doesn't use a backslash before
					#if __GNUC__==8 && __GNUC_MINOR__==2
						#warning Needed to disable color markup check because gcc bug
					#else
						if (totalMessageLength-i>=strlen("x1b[0m")+(message[i]=='\\')){
							if (strncmp(&(message[i+(message[i]=='\\')]),"x1b[",strlen("x1b["))==0){
								int _oldIndex=i;
								// Advance to the x character if we chose to use backslash
								if (message[i]=='\\'){
									i++;
								}
								i+=4; // We're now in the parameters
								int _mSearchIndex;
								for (_mSearchIndex=i;_mSearchIndex<totalMessageLength;++_mSearchIndex){
									if (message[_mSearchIndex]=='m'){
										break;
									}
								}
								// If found the ending
								if (message[_mSearchIndex]=='m'){
									// TODO - Do stuff with the found color code
									if (message[i]=='0'){ // If we're resetting the color
	
									}else{
										int _semiColonSearchIndex;
										for (_semiColonSearchIndex=i;_semiColonSearchIndex<_mSearchIndex;++_semiColonSearchIndex){
											if (message[_semiColonSearchIndex]==';'){
												break;
											}
										}
										message[_semiColonSearchIndex]=0;
										printf("the number is %s\n",&(message[i]));
										message[_semiColonSearchIndex]=';';
									}
									i=_oldIndex;
									memset(&(message[i]),1,_mSearchIndex-i+1);
								}else{
									printf("Failed to parse color markup");
									i=_oldIndex; // Must be invalid otherwise
								}
							}
						}
					#endif
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
	if (dynamicAdvBoxHeight){
		if (0){
			// Don't add 1 to this value. For VNDS games, this always represents the next line.
			updateDynamicADVBox(_currentDrawLine,-1);
		}else{
			// TODO - Untested
			// If not VNDS, there is a chance _currentDrawLine won't represent the next line, so we need to make sure we're actually drawing the _currentDrawLine and that it doesn't draw newly added text.
			char _archivedCharacter = currentMessages[_currentDrawLine][_currentDrawChar];
			currentMessages[_currentDrawLine][_currentDrawChar] = '\0';
			updateDynamicADVBox(_currentDrawLine+1,currentLine+2);
			currentMessages[_currentDrawLine][_currentDrawChar] = _archivedCharacter;
		}
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
		drawAdvanced(1,1,1,MessageBoxEnabled,1,0);
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
#if PLATFORM == PLAT_3DS
	// Wait for BGM to be unlocked and then lock it
	void lockBGM(){
		while (_bgmIsLock){
			wait(10);
		}
		_bgmIsLock = 1;
	}
#endif
void __freeBGMNoLock(int _slot){
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
}
void FreeBGM(int _slot){
	#if PLATFORM == PLAT_3DS
		lockBGM();
	#endif
	__freeBGMNoLock(_slot);
	#if PLATFORM == PLAT_3DS
		_bgmIsLock = 0;
	#endif
}
void StopBGM(int _slot){
	#if PLATFORM == PLAT_3DS
		lockBGM();
	#endif
	changeMallocString(&lastBGMFilename,NULL);
	if (currentMusic[_slot]!=NULL){
		stopMusic(currentMusicHandle[_slot]);
	}
	#if PLATFORM == PLAT_3DS
		_bgmIsLock = 0;
	#endif
}
// Unfixed bgm
void PlayBGM(const char* filename, int _volume, int _slot){
	#if PLATFORM == PLAT_3DS
		lockBGM();
	#endif
	if (filename!=lastBGMFilename){ // HACK
		changeMallocString(&lastBGMFilename,filename);
	}
	if (bgmVolume==0){
		printf("BGM volume is 0, ignore music change.\n");
	}else if (_slot>=MAXMUSICARRAY){
		LazyMessage("Music slot too high.","No action will be taken.",NULL,NULL);
	}else{
		CROSSMUSIC* _tempHoldSlot=loadGameAudio(filename,PREFER_DIR_BGM,0);
		if (_tempHoldSlot==NULL){
			__freeBGMNoLock(_slot);
		}else{
			char* _tempHoldFilepathConcat = malloc(strlen(filename)+1);
			strcpy(_tempHoldFilepathConcat,filename);
			// FreeBGM is right here, after load but before play, so the player can listen to the old BGM as the new one loads.
			__freeBGMNoLock(_slot);
			currentMusic[_slot] = _tempHoldSlot;
			currentMusicFilepath[_slot] = _tempHoldFilepathConcat;
			currentMusicUnfixedVolume[_slot] = _volume;
			currentMusicHandle[_slot] = playMusic(currentMusic[_slot],_slot);
			setMusicVolume(currentMusicHandle[_slot],FixBGMVolume(_volume));
			lastBGMVolume=_volume;
		}
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
// showVNDSWarnings, 1 byte
// higurashiUsesDynamicScale, 1 byte
// preferredTextDisplayMode, 1 byte
// autoModeVoicedWait, 4 bytes
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
	fwrite(&autoModeWait,sizeof(int),1,fp);

	fwrite(&_bgmTemp,1,1,fp);
	fwrite(&_seTemp,1,1,fp);
	fwrite(&_voiceTemp,1,1,fp);
	fwrite(&MessageBoxAlpha,1,1,fp);
	fwrite(&textOnlyOverBackground,1,1,fp);
	fwrite(&textSpeed,1,1,fp);
	fwrite(&vndsClearAtBottom,sizeof(signed char),1,fp);
	fwrite(&showVNDSWarnings,sizeof(signed char),1,fp);
	fwrite(&higurashiUsesDynamicScale,sizeof(signed char),1,fp);
	fwrite(&preferredTextDisplayMode,sizeof(signed char),1,fp);
	fwrite(&autoModeVoicedWait,sizeof(int),1,fp);
	fwrite(&vndsSpritesFade,sizeof(signed char),1,fp);
	#if PLATFORM == PLAT_VITA
		fwrite(&vndsVitaTouch,sizeof(signed char),1,fp);
	#endif

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
			fread(&autoModeWait,sizeof(int),1,fp);
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
		if (_tempOptionsFormat>=7){
			fread(&showVNDSWarnings,sizeof(signed char),1,fp);
		}
		if (_tempOptionsFormat>=8){
			fread(&higurashiUsesDynamicScale,sizeof(signed char),1,fp);
		}
		if (_tempOptionsFormat>=9){
			fread(&preferredTextDisplayMode,sizeof(signed char),1,fp);
		}
		if (_tempOptionsFormat>=10){
			fread(&autoModeVoicedWait,sizeof(int),1,fp);
		}
		if (_tempOptionsFormat>=11){
			fread(&vndsSpritesFade,sizeof(signed char),1,fp);
		}
		if (_tempOptionsFormat>=12){
			#if PLATFORM == PLAT_VITA
				fread(&vndsVitaTouch,sizef(signed char),1,fp);
			#endif
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
		applyTextboxChanges();
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

		drawRectangle(textboxXOffset,0,outputLineScreenWidth-textboxXOffset,screenHeight,230,255,200,150);
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
	free(presetFolder);

	streamingAssets = malloc(strlen(DATAFOLDER)+strlen(_streamingAssetsFolderName)+2);
	scriptFolder = malloc(strlen(DATAFOLDER)+strlen(_streamingAssetsFolderName)+strlen("/Scripts/")+1);
	streamingAssets[0]='\0';
	scriptFolder[0]='\0';
	if (_isRelativeToData){
		strcat(streamingAssets,DATAFOLDER); 
		strcat(scriptFolder,DATAFOLDER); 
	}
	//
	strcat(streamingAssets,_streamingAssetsFolderName);
	strcat(streamingAssets,"/");
	//
	strcat(scriptFolder,_streamingAssetsFolderName);
	strcat(scriptFolder,"/Scripts/");

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
		if(luaL_dofile(L,_completedSpecificLuaPath)==1){
			LazyMessage("Error in Lua file",_completedSpecificLuaPath,NULL,lua_tostring(L,-1));
			printf("%s\n",lua_tostring(L,-1));
		}
	}
	// Just in case
	applyTextboxChanges();
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
	applyTextboxChanges();
}
void LoadGameSpecificStupidity(){
	TryLoadMenuSoundEffect(NULL);
	RunGameSpecificLua();
}
void resetSettings(){
	fixPath("settings.noob",globalTempConcat,TYPE_DATA);
	if (checkFileExist(globalTempConcat)){
		remove(globalTempConcat);
	}
	fixPath("fontsize.noob",globalTempConcat,TYPE_DATA);
	if (checkFileExist(globalTempConcat)){
		remove(globalTempConcat);
	}
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
	_tempReadPresetFilename[0]='\0';
	fgets (_tempReadPresetFilename, 50, fp);
	fclose (fp);

	removeNewline(_tempReadPresetFilename);
	currentPresetFilename = malloc(strlen(_tempReadPresetFilename)+1);
	strcpy(currentPresetFilename,_tempReadPresetFilename);

	currentGameStatus = GAMESTATUS_LOADPRESET;
}
// Also starts loading the preset file
// returns 1 if okay to keep going
char startLoadingGameFolder(char* _chosenGameFolder){
	char _fileWithPresetFilenamePath[strlen(gamesFolder)+strlen(_chosenGameFolder)+strlen("/includedPreset.txt")+1];
	strcpy(_fileWithPresetFilenamePath,gamesFolder);
	strcat(_fileWithPresetFilenamePath,_chosenGameFolder);
	strcat(_fileWithPresetFilenamePath,"/includedPreset.txt");

	if (!checkFileExist(_fileWithPresetFilenamePath)){
		LazyMessage("Invalid game folder.","I know this because the includedPreset.txt","does not exist.","Did you remember to convert this folder before moving it?");
		return 0;
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
	return 1;
}
void setDefaultGame(char* _defaultGameFolderName){
	char _defaultGameSaveFilenameBuffer[strlen(saveFolder)+strlen("_defaultGame")+1];
	strcpy(_defaultGameSaveFilenameBuffer,saveFolder);
	strcat(_defaultGameSaveFilenameBuffer,"_defaultGame");

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
	scriptForceResourceUppercase=0;
	dynamicScaleEnabled=1;
	//shouldUseBustCache=1;
}
void activateHigurashiSettings(){
	currentlyVNDSGame=0;
	scriptUsesFileExtensions=0;
	bustsStartInMiddle=1;
	scriptScreenWidth=640;
	scriptScreenHeight=480;
	scriptForceResourceUppercase=0;
	dynamicScaleEnabled=higurashiUsesDynamicScale;
	//shouldUseBustCache=0;
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
	#if SOUNDPLAYER != SND_VITA
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
			lockBGM();
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
// unsigned char - format version
// script filename relative to script folder
// long int - position in the file
// int - number of messgae strings
// int - currentLine
// message strings
// current background filename
// int - maxBusts
// bust x, bust y, bust filename
// string - lastBGMFilename
// int - lastBGMVolume
// game variables
void vndsNormalSave(char* _filename){
	FILE* fp = fopen(_filename,"wb");
	
	// Save options file format
	unsigned char _tempOptionsFormat = VNDSSAVEFORMAT;
	fwrite(&_tempOptionsFormat,sizeof(unsigned char),1,fp); //

	// Save the current script
	writeLengthStringToFile(fp,currentScriptFilename);

	// Save the position in the current script
	long int _currentFilePosition = crossftell(nathanscriptCurrentOpenFile);
	fwrite(&_currentFilePosition,sizeof(long int),1,fp); //

	// Save the max number of lines we can have on screen, this makes the saves safe even if I change this number
	int i;
	i=MAXLINES;
	fwrite(&i,sizeof(int),1,fp); //
	// Save our current line
	fwrite(&currentLine,sizeof(int),1,fp); //

	// Save the current messages
	for (i=0;i<MAXLINES;i++){
		writeLengthStringToFile(fp, currentMessages[i]); //
	}

	// Save the background filename
	writeLengthStringToFile(fp,lastBackgroundFilename); //

	// Write the number of busts we're saving
	fwrite(&maxBusts,sizeof(int),1,fp); //

	// Write the bust data
	for (i=0;i<maxBusts;i++){
		fwrite(&(Busts[i].xOffset),sizeof(signed int),1,fp); //
		fwrite(&(Busts[i].yOffset),sizeof(signed int),1,fp); //
		if (Busts[i].relativeFilename==NULL){
			writeLengthStringToFile(fp,""); //
		}else{
			writeLengthStringToFile(fp,Busts[i].relativeFilename); //
		}
	}

	writeLengthStringToFile(fp,lastBGMFilename);
	fwrite(&lastBGMVolume,sizeof(int),1,fp);

	// Write game specific var list
	saveVariableList(fp,nathanscriptGamevarList,nathanscriptTotalGamevar); //
	fclose(fp);
}
void vndsNormalLoad(char* _filename){
	FILE* fp = fopen(_filename,"rb");
	unsigned char _readFileFormat;
	fread(&_readFileFormat,sizeof(unsigned char),1,fp); //
	if (_readFileFormat!=1){
		LazyMessage("Bad file format version.",NULL,NULL,NULL);
		fclose(fp);
	}
	char* _foundScriptFilename = readLengthStringFromFile(fp); //
	changeMallocString(&currentScriptFilename,_foundScriptFilename);
	long int _readFilePosition;
	fread(&_readFilePosition,sizeof(long int),1,fp); //
	int _readMaxLines;
	fread(&_readMaxLines,sizeof(int),1,fp); //
	fread(&currentLine,sizeof(int),1,fp); //
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
		printf("Read %d/%d\n",i,_maxReadBusts);
		signed int _tempReadX;
		signed int _tempReadY;
		char* _tempReadFilename;
		fread(&_tempReadX,sizeof(signed int),1,fp); //
		fread(&_tempReadY,sizeof(signed int),1,fp); //
		_tempReadFilename = readLengthStringFromFile(fp); //
		if (_tempReadFilename[0]!='\0'){
			nextVndsBustshotSlot = i+1;
			DrawBustshot(i,_tempReadFilename,_tempReadX,_tempReadY,i,0,0,0);
		}
		free(_tempReadFilename);
	}

	char* _foundBGMFilename = readLengthStringFromFile(fp);
	fread(&lastBGMVolume,sizeof(int),1,fp);
	if (strlen(_foundBGMFilename)!=0){
		PlayBGM(_foundBGMFilename,lastBGMVolume,0);
	}
	free(_foundBGMFilename);

	loadVariableList(fp,&nathanscriptGamevarList,&nathanscriptTotalGamevar);
	fclose(fp);

	// ============================================
	// END
	// ============================================

	char _tempLoadedFilename[strlen(scriptFolder)+strlen(_foundScriptFilename)+1];
	strcpy(_tempLoadedFilename,scriptFolder);
	strcat(_tempLoadedFilename,_foundScriptFilename);

	// Have to free here because this variable is used above
	free(_foundScriptFilename);

	controlsStart();
	controlsEnd();

	if (gameTextDisplayMode==TEXTMODE_ADV && dynamicAdvBoxHeight){
		updateDynamicADVBox(-1,-1);
	}

	endType=Line_Normal;	
	outputLineWait();

	nathanscriptDoScript(_tempLoadedFilename,_readFilePosition,inBetweenVNDSLines);
}
char* easyVNDSSaveSlot(int _slot){
	char* _tempSavefilePath = malloc(strlen(streamingAssets)+strlen("sav")+3+1);
	sprintf(_tempSavefilePath,"%ssav%d",streamingAssets,_slot);
	vndsNormalSave(_tempSavefilePath);
	return _tempSavefilePath;
}
void _textboxTransition(char _isOn, int _totalTime){
	if (MessageBoxEnabled!=_isOn && !isSkipping){
		signed short _fadeoutPerUpdate = ceil(MessageBoxAlpha/(double)TEXTBOXFADEOUTUPDATES(_totalTime));
		unsigned char _oldMessageBoxAlpha = MessageBoxAlpha;
		if (_isOn==0){
			_fadeoutPerUpdate*=-1;
		}
		if (_isOn==1){
			MessageBoxAlpha=0;
		}
		MessageBoxAlpha+=_fadeoutPerUpdate;
		while (1){
			fpsCapStart();
			if (!(MessageBoxAlpha+_fadeoutPerUpdate<=0 || MessageBoxAlpha+_fadeoutPerUpdate>=_oldMessageBoxAlpha)){
				MessageBoxAlpha+=_fadeoutPerUpdate;
			}else{
				break;
			}
			startDrawing();
			drawAdvanced(1,1,1,1,1,0); // Don't draw text
			endDrawing();
			fpsCapWait();
		}
		MessageBoxAlpha = _oldMessageBoxAlpha;
	}
	MessageBoxEnabled=_isOn;
}
void hideTextbox(){
	_textboxTransition(0,TEXTBOXFADEOUTTIME);
}
void showTextbox(){
	_textboxTransition(1,TEXTBOXFADEINTIME);
}
#if PLATFORM == PLAT_VITA
	void invertImage(vita2d_texture* _passedImage, signed char _doInvertAlpha){
		// Don't recalculate these every time
		uint32_t _cachedImageWidth = vita2d_texture_get_width(_passedImage);
		uint32_t _cachedImageHeight = vita2d_texture_get_height(_passedImage);
		uint8_t _cachedValuesToInvert = 3+_doInvertAlpha; // Last 8 bits are alpha value for some reason
		// Pixels stored in uint32_t
		void* _currentImageData = vita2d_texture_get_datap(_passedImage);
		uint32_t y;
		for (y=0;y<_cachedImageHeight;++y) {
			uint32_t x;
			for (x=0;x< _cachedImageWidth;++x) {
				uint8_t i;
				for (i=0;i<_cachedValuesToInvert;++i){
					((uint8_t*)_currentImageData)[(x + _cachedImageWidth * y)*4+i]=255-((uint8_t*)_currentImageData)[(x + _cachedImageWidth * y)*4+i];
				}
			}
		}
	}
#else
	void invertImage(CrossTexture* _passedImage, signed char _doInvertAlpha){
		printf("Invert image at %p. Alpha change: %d\n",_passedImage,_doInvertAlpha);
	}
#endif
void applyNegative(int _actionTime, signed char _waitforcompletion){
	filterActive=1;
	#if CANINVERT
		// Invert all images
		currentFilterType = FILTERTYPE_NEGATIVE;
		unsigned char i;
		for (i=0;i<maxBusts;++i){
			if (Busts[i].isActive==1){
				invertImage(Busts[i].image,0);
			}
		}
		if (currentBackground!=NULL){
			invertImage(currentBackground,0);
		}
	#else
		currentFilterType = FILTERTYPE_EFFECTCOLORMIX;
		filterR = 255;
		filterG = 255;
		filterB = 255;
		filterA = 127;
	#endif
}
void removeNegative(int _actionTime, signed char _waitforcompletion){
	#if CANINVERT
		// Inverting everything again fixes it
		applyNegative(_actionTime,_waitforcompletion);
	#endif
	filterActive=0;
	currentFilterType=FILTERTYPE_INACTIVE;
}
void addGamePresetToLegacyFolder(char* _streamingAssetsRoot, char* _presetFilenameRelative){
	if (!directoryExists(_streamingAssetsRoot)){
		LazyMessage(_streamingAssetsRoot,"does not exist. This means","you probably don't have to worry about",_presetFilenameRelative);
		return;
	}

	char _presetFilenameAbsolute[strlen(presetFolder)+strlen(_presetFilenameRelative)+1];
	strcpy(_presetFilenameAbsolute,presetFolder);
	strcat(_presetFilenameAbsolute,_presetFilenameRelative);

	char _includedPresetTxtLocation[strlen(_streamingAssetsRoot)+strlen("includedPreset.txt")+1];
	strcpy(_includedPresetTxtLocation,_streamingAssetsRoot);
	strcat(_includedPresetTxtLocation,"includedPreset.txt");

	char _newStreamingAssetsPresetFilenameAbsolute[strlen(_streamingAssetsRoot)+strlen(_presetFilenameRelative)+1];
	strcpy(_newStreamingAssetsPresetFilenameAbsolute,_streamingAssetsRoot);
	strcat(_newStreamingAssetsPresetFilenameAbsolute,_presetFilenameRelative);

	// Make includedPreset.txt
	FILE* fp = fopen(_includedPresetTxtLocation,"wb");
	fwrite(_presetFilenameRelative,strlen(_presetFilenameRelative),1,fp);
	fclose(fp);

	// Copy preset
	FILE* fpr = fopen(_presetFilenameAbsolute,"rb");
	FILE* fpw = fopen(_newStreamingAssetsPresetFilenameAbsolute,"wb");
	while (1){
		char _lastReadByte;
		if (fread(&_lastReadByte,1,1,fpr)!=1){
			break;
		}
		fwrite(&_lastReadByte,1,1,fpw);
	}
	fclose(fpr);
	fclose(fpw);
}
char* readSpecificIniLine(FILE* fp, char* _prefix){
	char _tempReadLine[256];
	_tempReadLine[0]='\0';
	fgets(_tempReadLine,256,fp);
	removeNewline(_tempReadLine);
	if (strlen(_tempReadLine)>strlen(_prefix)){ // If string is long enough to contain title string
		if (strncmp(_tempReadLine,_prefix,strlen(_prefix))==0){ // If the line starts with what we want it to
			char* _foundValueString = &(_tempReadLine[strlen(_prefix)]);
			char* _returnString = malloc(strlen(_foundValueString)+1);
			strcpy(_returnString,_foundValueString);
			return _returnString;
		}else{
			printf("Error, doesn't start with %s\n",_prefix);
		}
	}
	return NULL;
}
char isNumberString(char* _inputString){
	while (*(_inputString)!='\0'){
		if (!(*(_inputString)>=48 && *(_inputString)<=57)){
			return 0;
		}
		_inputString++;
	}
	return 1;
}
char lazyLuaError(int _loadResult){
	if (_loadResult!=0){
		switch (_loadResult){
			case LUA_ERRSYNTAX:
				LazyMessage("LUA_ERRSYNTAX",NULL,NULL,NULL);
			break;
			case LUA_ERRMEM:
				LazyMessage("LUA_ERRMEM",NULL,NULL,NULL);
			break;
			case LUA_ERRGCMM:
				LazyMessage("LUA_ERRGCMM",NULL,NULL,NULL);
			break;
			case LUA_ERRFILE:
				LazyMessage("LUA_ERRFILE",NULL,NULL,NULL);
			break;
			case LUA_ERRRUN:
				LazyMessage("LUA_ERRRUN",NULL,NULL,NULL);
			break;
			case LUA_ERRERR:
				LazyMessage("LUA_ERRERR",NULL,NULL,NULL);
			break;
			case 1:
				LazyMessage("Lua error.",NULL,NULL,NULL);
			break;
			default:
				LazyMessage("UNKNOWN ERROR!",NULL,NULL,NULL);
			break;
		}
		return 1;
	}
	return 0;
}
/*
=================================================
*/
void scriptDisplayWindow(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	showTextbox();
	return;
}
void scriptClearMessage(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	currentLine=0;
	ClearMessageArray(1);
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
	printf("An unimplemented Lua function was just executed.\n");
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
	//for (i=0;i<maxBusts;i++){
	//	if (Busts[i].isActive==1){
	//		freeTexture(Busts[i].image);
	//		ResetBustStruct(&(Busts[i]));
	//	}
	//}
	return;
}
void scriptDisableWindow(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	hideTextbox();
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
		GenericPlayGameSound(nathanvariableToInt(&_passedArguments[0]),nathanvariableToString(&_passedArguments[1]),nathanvariableToInt(&_passedArguments[2]),PREFER_DIR_SE,seVolume);
	}
	return;
}
// PlayVoice(channel, filename, volume)
void scriptPlayVoice(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	if (isSkipping==0 && (hasOwnVoiceSetting==1 ? voiceVolume : seVolume)>0){
		GenericPlayGameSound(nathanvariableToInt(&_passedArguments[0]),nathanvariableToString(&_passedArguments[1]),nathanvariableToInt(&_passedArguments[2]),PREFER_DIR_VOICE, hasOwnVoiceSetting==1 ? voiceVolume : seVolume);
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

#define CHOICESCROLLTIMEOFFSET 500 // How long you have to be on a choice before it starts scrolling
#define CHOICESCROLLTIMEINTERVAL 100 // How often a new character
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

	if (currentlyVNDSGame){
		nathanscriptBackLine(); // Fix file position for saving so when we load it reloads the choice command
	}

	// Text that is too long to fit will scroll character by character
	signed char _needScrolling=-1;
	char _isScrollingText;
	u64 _lastScrollTime;
	int _scrollOffset=0; // In characters. Initialize this to get rid of gcc warning
	signed char _scrollRight; // 0 if going left
	// This is the actual loop for choosing the choice
	signed char _choice=0;
	while (1){
		fpsCapStart();
		controlsStart();
		char _oldIndex = _choice;
		_choice = MenuControls(_choice,0,_totalOptions-1);
		// Init scrolling if we changed menu index or just started the loop
		if (_needScrolling==-1 || _oldIndex!=_choice){
			_scrollOffset=0;
			if (textWidth(fontSize,noobOptions[_choice])>screenWidth-MENUOPTIONOFFSET-MENUCURSOROFFSET){
				_needScrolling=1;
				_lastScrollTime = getTicks();
				_isScrollingText=0;
				_scrollRight=1;
			}else{
				_needScrolling=0;
			}
		}

		// Process scrolling
		if (_needScrolling){
			if (_isScrollingText){
				if (getTicks()>_lastScrollTime+CHOICESCROLLTIMEINTERVAL){
					char _oldDirection = _scrollRight;
					if (_scrollRight){
						if (textWidth(fontSize,&(noobOptions[_choice][_scrollOffset]))>screenWidth-MENUOPTIONOFFSET-MENUCURSOROFFSET){
							++_scrollOffset;
						}else{
							_scrollRight=0;
						}
					}else{
						if (_scrollOffset==0){
							_scrollRight=1;
						}else{
							--_scrollOffset;
						}
					}
					if (_oldDirection!=_scrollRight){
						_lastScrollTime = getTicks()+CHOICESCROLLTIMEOFFSET-CHOICESCROLLTIMEINTERVAL; // Trick the program into waiting CHOICESCROLLTIMEOFFSET before it goes back the other way
					}else{
						_lastScrollTime = getTicks();
					}
				}
			}else{
				if (getTicks()>_lastScrollTime+CHOICESCROLLTIMEOFFSET){
					_isScrollingText=1;
					_lastScrollTime = 0; // So we'll scroll next frame
				}
			}
		}
		updateControlsGeneral();
		if (wasJustPressed(SCE_CTRL_CROSS)){
			lastSelectionAnswer = _choice;
			break;
		}
		controlsEnd();
		startDrawing();
		Draw(0);
		DrawMessageBox(TEXTMODE_NVL);
		for (i=0;i<_totalOptions;i++){
			if (_choice!=i){
				goodDrawText(MENUOPTIONOFFSET,i*currentTextHeight,noobOptions[i],fontSize);
			}
		}
		goodDrawText(MENUOPTIONOFFSET+MENUCURSOROFFSET,_choice*currentTextHeight,&(noobOptions[_choice][_scrollOffset]),fontSize);
		goodDrawText(MENUCURSOROFFSET*2,_choice*currentTextHeight,MENUCURSOR,fontSize);

		endDrawing();
		fpsCapWait();
	}

	if (currentlyVNDSGame){
		nathanscriptAdvanceLine(); // Fix what we did at the start of the function
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
	// Make sure we don't have some horrible situation where the player is stuck in Hell
	if (currentFilterType == FILTERTYPE_NEGATIVE){
		removeNegative(0,0);
	}
	// 0 is none, defaults to 1.
	// 1 is "EffectColorMix"
	// 2 is DrainColor
	// 3 is Negative
	// 10 is HorizontalBlur2
	// 12 is GaussianBlur
	currentFilterType = nathanvariableToInt(&_passedArguments[0]);
	if (currentFilterType==0){ // Default
		currentFilterType = FILTERTYPE_EFFECTCOLORMIX;
	}
	filterActive=1;
	if (currentFilterType<=1){
		filterR = nathanvariableToInt(&_passedArguments[1]);
		filterG = nathanvariableToInt(&_passedArguments[2]);
		filterB = nathanvariableToInt(&_passedArguments[3]);
		filterA = nathanvariableToInt(&_passedArguments[4]);
	}else if (currentFilterType==3 && CANINVERT){
		applyNegative(nathanvariableToInt(&_passedArguments[6]),nathanvariableToBool(&_passedArguments[7]));
	}else{ // For these, we'll just draw a white filter.
		filterR = 255;
		filterG = 255;
		filterB = 255;
		filterA = 127;
	}
	return;
}
// Seems to be very similar to using DrawFilm with type of 3
// Negative(fadein time, wait for fadein)
// Translates to DrawFilm(3, 1, 1, 1, 255, 0, arg 0, arg 1);
void scriptNegative(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	applyNegative(nathanvariableToInt(&_passedArguments[0]),nathanvariableToBool(&_passedArguments[1]));
}
// I think this just has a time argument and a blocking argument. I've implemented neither.
void scriptFadeFilm(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	if (currentFilterType==FILTERTYPE_NEGATIVE){
		removeNegative(nathanvariableToInt(&_passedArguments[0]),nathanvariableToBool(&_passedArguments[1]));
	}
	filterActive=0;
	currentFilterType = FILTERTYPE_INACTIVE;
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
	textDisplayModeOverriden=1;
	gameTextDisplayMode = nathanvariableToInt(&_passedArguments[0]);
	applyTextboxChanges();
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

// Srttings menu override
EASYLUAINTSETFUNCTION(oMenuQuit,forceShowQuit)
EASYLUAINTSETFUNCTION(oMenuVNDSSettings,forceShowVNDSSettings)
EASYLUAINTSETFUNCTION(oMenuVNDSSave,forceShowVNDSSave)
EASYLUAINTSETFUNCTION(oMenuRestartBGM,forceShowRestartBGM)
EASYLUAINTSETFUNCTION(oMenuArtLocations,forceArtLocationSlot)
EASYLUAINTSETFUNCTION(oMenuScalingOption,forceScalingOption)
EASYLUAINTSETFUNCTION(oMenuVNDSBustFade,forceVNDSFadeOption)
EASYLUAINTSETFUNCTION(oMenuDebugButton,forceDebugButton)
EASYLUAINTSETFUNCTION(oMenuTextboxMode,forceTextBoxModeOption) // ADV or NVL
EASYLUAINTSETFUNCTION(oMenuTextOverBG,forceTextOverBGOption) // text only over background
// Manually set the options if you've chosen to disable the menu option
EASYLUAINTSETFUNCTION(textOnlyOverBackground,textOnlyOverBackground);
EASYLUAINTSETFUNCTION(dynamicAdvBoxHeight,dynamicAdvBoxHeight);
EASYLUAINTSETFUNCTION(advboxHeight,advboxHeight) 

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
		_passedNormalImages[i] = safeLoadGameImage(nathanvariableToString(&_passedArguments[i*3+1-1]),graphicsLocation,scriptUsesFileExtensions);
		_passedHoverImages[i] = safeLoadGameImage(nathanvariableToString(&_passedArguments[i*3+2-1]),graphicsLocation,scriptUsesFileExtensions);
		_passedSelectImages[i] = safeLoadGameImage(nathanvariableToString(&_passedArguments[i*3+3-1]),graphicsLocation,scriptUsesFileExtensions);
	}

	// Y position of the first choice graphic
	int _startDrawY;
	// X position of every choice graphic
	int _startDrawX;
	//int _spaceBetweenChoices;

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
			drawTexture(_passedNormalImages[i],_startDrawX,_startDrawY+_firstChoiceHeight*i+_halfFirstChoiceHeight*i);
		}
		if (_isHoldSelect==0){
			drawTexture(_passedHoverImages[_userChoice],_startDrawX,_startDrawY+_firstChoiceHeight*_userChoice+_halfFirstChoiceHeight*_userChoice);
		}else{
			drawTexture(_passedSelectImages[_userChoice],_startDrawX,_startDrawY+_firstChoiceHeight*_userChoice+_halfFirstChoiceHeight*_userChoice);
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
	if (currentlyVNDSGame && nathanscriptIsInit){
		char _numberToStringBuffer[5];
		sprintf(_numberToStringBuffer,"%d",_userChoice+1);
		genericSetVar("selected","=",_numberToStringBuffer,&nathanscriptGamevarList,&nathanscriptTotalGamevar);
	}
	return;
}
// Sets the size of the screen that positions are relative to. For example, this would be the DS' screen resolution for vnds games. It's 640x480 for Higurashi
// Sets scriptScreenWidth and scriptScreenHeight
void scriptSetPositionsSize(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	scriptScreenWidth = nathanvariableToInt(&_passedArguments[0]);
	scriptScreenHeight = nathanvariableToInt(&_passedArguments[1]);
	return;
}
void scriptSetIncludedFileExtensions(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	scriptUsesFileExtensions = nathanvariableToBool(&_passedArguments[0]);
	return;
}
void scriptSetForceCapFilenames(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	scriptForceResourceUppercase = nathanvariableToBool(&_passedArguments[0]);
	return;
}
#include "LuaWrapperDefinitions.h"
//======================================================
void drawAdvanced(char _shouldDrawBackground, char _shouldDrawLowBusts, char _shouldDrawFilter, char _shouldDrawMessageBox, char _shouldDrawHighBusts, char _shouldDrawMessageText){
	int i;
	if (currentBackground!=NULL && _shouldDrawBackground){
		DrawBackground(currentBackground);
	}
	if (_shouldDrawLowBusts){
		for (i = maxBusts-1; i != -1; i--){
			if (bustOrder[i]!=255 && Busts[bustOrder[i]].isActive==1){
				DrawBust(&(Busts[bustOrder[i]]));
			}
		}
	}
	if (filterActive==1 && _shouldDrawFilter){
		DrawCurrentFilter();
	}
	if (_shouldDrawMessageBox==1){
		DrawMessageBox(gameTextDisplayMode);
	}
	if (_shouldDrawHighBusts){
		for (i = maxBusts-1; i != -1; i--){
			if (bustOrderOverBox[i]!=255 && Busts[bustOrderOverBox[i]].isActive==1){
				DrawBust(&(Busts[bustOrderOverBox[i]]));
			}
		}
	}
	if (_shouldDrawMessageText==1){
		DrawMessageText(255,-1);
	}
}
void Draw(char _shouldDrawMessageBox){
	drawAdvanced(1,1,1,_shouldDrawMessageBox,1,_shouldDrawMessageBox);
}
char upgradeToGameFolder(){
	controlsEnd();
	char* _tempChosenFile;
	char _didUpgradeOne=0;
	while (1){
		if (FileSelector(presetFolder,&_tempChosenFile,(char*)"Select the preset to use or press circle to be done.")==2){
			LazyMessage("What? No preset files?",presetFolder,"is empty.",NULL);
			return 0;
		}else{
			if (_tempChosenFile==NULL){
				break;
			}
			UpdatePresetStreamingAssetsDir(_tempChosenFile);
			if (LazyChoice("Add the preset file",_tempChosenFile,"to",streamingAssets)){
				addGamePresetToLegacyFolder(streamingAssets,_tempChosenFile);\
				_didUpgradeOne=1;
				if (!LazyChoice("Done.","Upgrade another folder?",NULL,NULL)){
					free(_tempChosenFile);
					break;
				}
			}
			free(_tempChosenFile);
		}
	}
	if (_didUpgradeOne){
		ClearMessageArray(0);
		controlsStart();
		controlsEnd();

		char _bigMessageBuffer[strlen("Now that you've upgraded one or more of your StreamingAssets folders to include the preset file, you need to move all your StreamingAssets folder(s) using VitaShell or MolecularShell to\n\nYou will need to create that games folder first. After you create that game folder, you won't be able to use preset mode anymore, so make sure you've upgraded all of your StreamingAssets folders before.")+strlen(gamesFolder)+1];
		sprintf(_bigMessageBuffer,"Now that you've upgraded one or more of your StreamingAssets folders to include the preset file, you need to move all your StreamingAssets folder(s) using VitaShell or MolecularShell to\n%s\nYou will need to create that games folder first. After you create that game folder, you won't be able to use preset mode anymore, so make sure you've upgraded all of your StreamingAssets folders before.",gamesFolder);

		OutputLine(_bigMessageBuffer,Line_WaitForInput,0);
		while (!wasJustPressed(SCE_CTRL_CROSS)){
			fpsCapStart();
			controlsEnd();
			startDrawing();
			DrawMessageText(255,-1);
			endDrawing();
			controlsStart();
			fpsCapWait();
		}
	}else{
		LazyMessage("You did not upgrade any folders.",NULL,NULL,NULL);
	}
	return _didUpgradeOne;
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
void debugMenuOption(){
	//
}
// Will change the global variables for you
void switchTextDisplayMode(signed char _newMode){
	if (currentlyVNDSGame){
		if (_newMode==TEXTMODE_ADV){
			enableVNDSADVMode();
		}else if (_newMode==TEXTMODE_NVL){
			disableVNDSADVMode();
		}
	}else{
		LazyMessage("TODO",NULL,NULL,NULL);
	}
}
void _settingsChangeAuto(int* _storeValue, char* _storeString){
	signed char _isNegative = wasJustPressed(SCE_CTRL_LEFT) ? -1 : 1;
	if (isDown(SCE_CTRL_LTRIGGER)){
		*_storeValue+=(int)(_isNegative*50);
	}else{
		*_storeValue+=(int)(_isNegative*100);
	}
	if (*_storeValue<=0){
		*_storeValue=0;
	}
	itoa(*_storeValue,_storeString,10);
}

void overrideIfSet(signed char* _possibleTarget, signed char _possibleOverride){
	if (_possibleOverride!=-1){
		*_possibleTarget = _possibleOverride;
	}
}

#define ISTEXTSPEEDBAR 0
#define MAXOPTIONSSETTINGS 22
#define SETTINGSMENU_EASYADDOPTION(a,b) \
	_settingsOptionsMainText[++_maxOptionSlotUsed] = a; \
	b = _maxOptionSlotUsed;
void SettingsMenu(signed char _shouldShowQuit, signed char _shouldShowVNDSSettings, signed char _shouldShowVNDSSave, signed char _shouldShowRestartBGM, signed char _showArtLocationSlot, signed char _showScalingOption, signed char _showTextBoxModeOption, signed char _showVNDSFadeOption, signed char _showDebugButton){
	// Allow global overide for settings
	overrideIfSet(&_shouldShowQuit,forceShowQuit);
	overrideIfSet(&_shouldShowVNDSSettings,forceShowVNDSSettings);
	overrideIfSet(&_shouldShowVNDSSave,forceShowVNDSSave);
	overrideIfSet(&_shouldShowRestartBGM,forceShowRestartBGM);
	overrideIfSet(&_showArtLocationSlot,forceArtLocationSlot);
	overrideIfSet(&_showScalingOption,forceScalingOption);
	overrideIfSet(&_showTextBoxModeOption,forceTextBoxModeOption);
	overrideIfSet(&_showVNDSFadeOption,forceVNDSFadeOption);
	overrideIfSet(&_showDebugButton,forceDebugButton);

	controlsStart();
	controlsEnd();
	PlayMenuSound();
	signed char _choice=0;
	signed char _scrollOffset=0;
	signed char _optionsOnScreen;
	signed char _needToScroll=0;
	static unsigned char _chosenSaveSlot=0;
	int i;
	char _artBefore=graphicsLocation; // This variable is used to check if the player changed the bust location after exiting
	CrossTexture* _renaImage=NULL;
	char _tempItoaHoldBGM[5] = {'\0'};
	char _tempItoaHoldSE[5] = {'\0'};
	char _tempItoaHoldVoice[5] = {'\0'};
	char _tempItoaHoldBoxAlpha[5] = {'\0'};
	char _tempItoaHoldTextSpeed[8] = {'\0'}; // Needs to be big enough to hold "instant"
	char _tempAutoModeString[10] = {'\0'};
	char _tempAutoModeVoiceString[10] = {'\0'};
	char _tempHoldSaveSlotSelection[5] = {'\0'};
	char _maxOptionSlotUsed=0;

	// Dynamic slots
	signed char _resettingsSlot=-2;
	signed char _textOverBGSlot=-2;
	signed char _fontSizeSlot=-2;
	signed char _textSpeedSlot=-2;
	signed char _vndsSaveOptionsSlot=-2;
	signed char _vndsHitBottomActionSlot=-2;
	signed char _restartBgmActionSlot=-2;
	signed char _vndsErrorShowToggleSlot=-2;
	signed char _voiceVolumeSlot=-2;
	signed char _bustLocationSlot=-2;
	signed char _messageBoxAlphaSlot=-2;
	signed char _higurashiScalingSlot=-2;
	signed char _textboxModeSlot=-2;
	signed char _autoModeSpeedSlot=-2;
	signed char _autoModeSpeedVoiceSlot=-2;
	signed char _vndsBustFadeEnableSlot=-2;
	signed char _debugButtonSlot=-2;
	#if PLATFORM == PLAT_VITA
		signed char _vndsVitaTouchSlot=-2;
	#endif
	
	char* _settingsOptionsMainText[MAXOPTIONSSETTINGS];
	char* _settingsOptionsValueText[MAXOPTIONSSETTINGS];
	// Init value slots
	for (i=0;i<MAXOPTIONSSETTINGS;i++){
		_settingsOptionsValueText[i]=NULL;
	}
	if (currentGameStatus == _shouldShowQuit){
		_settingsOptionsMainText[0] = "Back";
	}else{
		_settingsOptionsMainText[0] = "Resume";
	}
	#if PLATFORM == PLAT_VITA
		_settingsOptionsMainText[1] = "Overclock CPU";
	#elif PLATFORM == PLAT_3DS
		_settingsOptionsMainText[1] = "Text:";
		if (textIsBottomScreen==1){
			_settingsOptionsValueText[1] = "Bottom Screen";
		}else{
			_settingsOptionsValueText[1] = "Top Screen";
		}
	#else
		_settingsOptionsMainText[1] = "Nothing";
	#endif
	_settingsOptionsMainText[2] = "BGM Volume:";
	_settingsOptionsMainText[3] = "SE Volume:";
	_maxOptionSlotUsed=3;

	// Add new, optional settings here
	if (hasOwnVoiceSetting){
		SETTINGSMENU_EASYADDOPTION("Voice Volume:",_voiceVolumeSlot);
	}
	SETTINGSMENU_EASYADDOPTION("Text Speed:",_textSpeedSlot);
	if (forceTextOverBGOption){
		SETTINGSMENU_EASYADDOPTION("Textbox:",_textOverBGSlot);
	}
	SETTINGSMENU_EASYADDOPTION("Auto Speed:",_autoModeSpeedSlot);
	SETTINGSMENU_EASYADDOPTION("Auto Voiced Speed:",_autoModeSpeedVoiceSlot);
	if (canChangeBoxAlpha){
		SETTINGSMENU_EASYADDOPTION("Message Box Alpha:",_messageBoxAlphaSlot);
	}
	if (_showArtLocationSlot){
		SETTINGSMENU_EASYADDOPTION("Bust Location:",_bustLocationSlot);
	}
	if (_shouldShowRestartBGM==1){
		SETTINGSMENU_EASYADDOPTION("Restart BGM",_restartBgmActionSlot);
	}
	if (_shouldShowVNDSSave){
		SETTINGSMENU_EASYADDOPTION("=Save Game ",_vndsSaveOptionsSlot);
	}
	if (_shouldShowVNDSSettings){
		SETTINGSMENU_EASYADDOPTION("Clear at bottom:",_vndsHitBottomActionSlot);
		SETTINGSMENU_EASYADDOPTION("VNDS Warnings:",_vndsErrorShowToggleSlot);
	}
	if (_showScalingOption){
		SETTINGSMENU_EASYADDOPTION("Dynamic Scaling: ",_higurashiScalingSlot);
	}
	if (_showTextBoxModeOption){
		SETTINGSMENU_EASYADDOPTION("Text Mode: ",_textboxModeSlot);
	}
	if (_showVNDSFadeOption){
		SETTINGSMENU_EASYADDOPTION("VNDS Image Fade:",_vndsBustFadeEnableSlot);
	}
	if (_showDebugButton){
		SETTINGSMENU_EASYADDOPTION("Debug",_debugButtonSlot);
	}
	if (forceResettingsButton){
		SETTINGSMENU_EASYADDOPTION("Defaults",_resettingsSlot);
	}
	if (forceFontSizeOption){
		SETTINGSMENU_EASYADDOPTION("Font Size",_fontSizeSlot);
	}
	#if PLATFORM == PLAT_VITA
		SETTINGSMENU_EASYADDOPTION("Vita Touch:",_vndsVitaTouchSlot);
	#endif
	// Quit button is always last
	if (currentGameStatus!=GAMESTATUS_TITLE){
		_settingsOptionsMainText[++_maxOptionSlotUsed] = "Quit";
	}

	//////////////////
	// Set values here
	//////////////////
	// Set pointers to menu option value text
	if (forceTextOverBGOption){
		_settingsOptionsValueText[_textOverBGSlot] = textOnlyOverBackground ? "Small" : "Full";
	}
	_settingsOptionsValueText[_autoModeSpeedSlot]=&(_tempAutoModeString[0]);
	_settingsOptionsValueText[_autoModeSpeedVoiceSlot]=&(_tempAutoModeVoiceString[0]);
	if (hasOwnVoiceSetting){
		_settingsOptionsValueText[_voiceVolumeSlot] = &(_tempItoaHoldVoice[0]);
	}
	if (_showArtLocationSlot){
		if (graphicsLocation == LOCATION_CG){
			_settingsOptionsValueText[_bustLocationSlot]="CG";
		}else if (graphicsLocation == LOCATION_CGALT){
			_settingsOptionsValueText[_bustLocationSlot]="CGAlt";
		}
	}
	_settingsOptionsValueText[2] = &(_tempItoaHoldBGM[0]);
	_settingsOptionsValueText[3] = &(_tempItoaHoldSE[0]);
	if (canChangeBoxAlpha){
		_settingsOptionsValueText[_messageBoxAlphaSlot] = &(_tempItoaHoldBoxAlpha[0]);
	}
	_settingsOptionsValueText[_textSpeedSlot] = &(_tempItoaHoldTextSpeed[0]);
	if (_shouldShowVNDSSave){
		_settingsOptionsValueText[_vndsSaveOptionsSlot]=&(_tempHoldSaveSlotSelection[0]);
	}
	if (_shouldShowVNDSSettings){
		_settingsOptionsValueText[_vndsHitBottomActionSlot] = charToBoolString(vndsClearAtBottom);
		if (showVNDSWarnings){
			_settingsOptionsValueText[_vndsErrorShowToggleSlot] = "Show";
		}else{
			_settingsOptionsValueText[_vndsErrorShowToggleSlot] = "Hide";
		}
	}
	if (_showScalingOption){
		_settingsOptionsValueText[_higurashiScalingSlot]=charToSwitch(higurashiUsesDynamicScale);
	}
	if (_showTextBoxModeOption){
		_settingsOptionsValueText[_textboxModeSlot]=(preferredTextDisplayMode==TEXTMODE_ADV ? "ADV" : "NVL");
	}
	if (_showVNDSFadeOption){
		if (vndsSpritesFade){
			_settingsOptionsValueText[_vndsBustFadeEnableSlot]="On";
		}else{
			_settingsOptionsValueText[_vndsBustFadeEnableSlot]="Off";
		}
	}

	#if PLATFORM == PLAT_VITA
		if(vndsVitaTouch){
			_settingsOptionsValueText[_vndsVitaTouchSlot] = "On";
		}else{
			_settingsOptionsValueText[_vndsVitaTouchSlot] = "Off";
		}
	#endif

	// Make strings
	itoa(autoModeWait,_tempAutoModeString,10);
	itoa(autoModeVoicedWait,_tempAutoModeVoiceString,10);
	itoa(bgmVolume*4,_tempItoaHoldBGM,10);
	itoa(seVolume*4, _tempItoaHoldSE,10);
	itoa(voiceVolume*4, _tempItoaHoldVoice,10);
	itoa(MessageBoxAlpha, _tempItoaHoldBoxAlpha,10);
	makeTextSpeedString(_tempItoaHoldTextSpeed,textSpeed);
	sprintf(_tempHoldSaveSlotSelection,"%d=",_chosenSaveSlot);

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
	}
	free(_tempRenaPath);
	
	_optionsOnScreen = (screenHeight/(double)currentTextHeight)-1;
	if (_optionsOnScreen>_maxOptionSlotUsed){
		_optionsOnScreen = _maxOptionSlotUsed+1;
	}else{
		_needToScroll=1;
	}
	while (currentGameStatus!=GAMESTATUS_QUIT){
		fpsCapStart();
		controlsStart();
		if (altMenuControls(&_choice,0,_maxOptionSlotUsed)){
			if (_choice>=_optionsOnScreen){
				_scrollOffset = _choice-(_optionsOnScreen-1);
			}else if (_choice<_optionsOnScreen){
				_scrollOffset=0;
			}
		}
		if (wasJustPressed(SCE_CTRL_CIRCLE)){
			break;
		}
		if (wasJustPressed(SCE_CTRL_TRIANGLE)){
			if (_choice==_vndsSaveOptionsSlot){
				_chosenSaveSlot=0;
			}else{
				break;
			}
		}
		if (wasJustPressed(SCE_CTRL_LEFT)){
			if (_choice==2){
				if (bgmVolume==0){
					bgmVolume=1.25;
				}
				bgmVolume-=.25;
				itoa(bgmVolume*4,_tempItoaHoldBGM,10);
				SetAllMusicVolume(FixBGMVolume(lastBGMVolume));
			}else if (_choice==3){
				if (seVolume==0){
					seVolume=1.25;
				}
				seVolume-=.25;
				itoa(seVolume*4,_tempItoaHoldSE,10);
				if (menuSoundLoaded==1){
					setSFXVolumeBefore(menuSound,FixSEVolume(256));
				}
				PlayMenuSound();
			}else if (_choice==_textSpeedSlot){
				textSpeed--;
				if (textSpeed==-11){
					textSpeed=-10;
				}else if (textSpeed==TEXTSPEED_INSTANT-1){
					textSpeed=10;
				}else if (textSpeed==0){
					textSpeed=-1;
				}
				makeTextSpeedString(_tempItoaHoldTextSpeed,textSpeed);
			}else if (_choice==_autoModeSpeedSlot){
				_settingsChangeAuto(&autoModeWait,_tempAutoModeString);
			}else if (_choice==_autoModeSpeedVoiceSlot){
				_settingsChangeAuto(&autoModeVoicedWait,_tempAutoModeVoiceString);
			}else if (_choice==_messageBoxAlphaSlot){ /////////////////////////////////////////////
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
			}else if (_choice==_voiceVolumeSlot){
				if (voiceVolume==0){
					voiceVolume=1.25;
				}
				voiceVolume-=.25;
				itoa(voiceVolume*4,_tempItoaHoldVoice,10);
			}else if (_choice==_vndsSaveOptionsSlot){
				_chosenSaveSlot--;
				sprintf(_tempHoldSaveSlotSelection,"%d=",_chosenSaveSlot);
			}
		}
		if (wasJustPressed(SCE_CTRL_CROSS) || wasJustPressed(SCE_CTRL_RIGHT)){
			if (_choice==0){ // Resume
				PlayMenuSound();
				break;
			}/*else if (_choice==1){
				if (isDown(SCE_CTRL_LTRIGGER)){
					autoModeWait+=50;
				}else{
					autoModeWait+=100;
				}
				itoa(autoModeWait,_tempAutoModeString,10);
			}*/else if (_choice==1){ // CPU speed
				PlayMenuSound();
				if (cpuOverclocked==0){
					cpuOverclocked=1;
					#if PLATFORM == PLAT_VITA
						scePowerSetArmClockFrequency(444);
					#elif PLATFORM == PLAT_3DS
						_settingsOptionsValueText[1] = "Bottom Screen";
					#endif
				}else if (cpuOverclocked==1){
					cpuOverclocked=0;
					#if PLATFORM == PLAT_VITA
						scePowerSetArmClockFrequency(333);
					#elif PLATFORM == PLAT_3DS
						_settingsOptionsValueText[1] = "Top Screen";
					#endif
				}
			}else if (_choice==2){
				if (bgmVolume==1){
					bgmVolume=0;
				}else{
					bgmVolume+=.25;
				}
				itoa(bgmVolume*4,_tempItoaHoldBGM,10);
				SetAllMusicVolume(FixBGMVolume(lastBGMVolume));
			}else if (_choice==3){
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
			}else if (_choice==_fontSizeSlot){
				FontSizeSetup();
				currentTextHeight = textHeight(fontSize);
			}else if (_choice==_resettingsSlot){
				PlayMenuSound();
				if (LazyChoice("This will reset your settings.","Is this okay?",NULL,NULL)==1){
					resetSettings();
					LazyMessage("Restart for the changes to","take effect.",NULL,NULL);
				}
			}else if (_choice==_textOverBGSlot){
				setTextOnlyOverBackground(!textOnlyOverBackground);
				if (textOnlyOverBackground){
					_settingsOptionsValueText[_textOverBGSlot] = "Small";
				}else{
					_settingsOptionsValueText[_textOverBGSlot] = "Full";
				}
			}else if (_choice==_textSpeedSlot){
				textSpeed++;
				if (textSpeed==11){
					textSpeed=TEXTSPEED_INSTANT;
				}else if (textSpeed==TEXTSPEED_INSTANT+1){
					textSpeed=TEXTSPEED_INSTANT;
				}else if (textSpeed==0){
					textSpeed=1;
				}
				makeTextSpeedString(_tempItoaHoldTextSpeed,textSpeed);
			}else if (_choice==_autoModeSpeedSlot){
				_settingsChangeAuto(&autoModeWait,_tempAutoModeString);
			}else if (_choice==_autoModeSpeedVoiceSlot){
				_settingsChangeAuto(&autoModeVoicedWait,_tempAutoModeVoiceString);
			}else if (_choice==_messageBoxAlphaSlot){ /////////////////////////////////////////////
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
			}else if (_choice==_bustLocationSlot){
				PlayMenuSound();
				if (graphicsLocation == LOCATION_CG){
					graphicsLocation = LOCATION_CGALT;
					_settingsOptionsValueText[_bustLocationSlot]="CGAlt";
				}else if (graphicsLocation == LOCATION_CGALT){
					graphicsLocation = LOCATION_CG;
					_settingsOptionsValueText[_bustLocationSlot]="CG";
				}
				if (_renaImage!=NULL){
					freeTexture(_renaImage);
					_tempRenaPath = CombineStringsPLEASEFREE(streamingAssets,locationStrings[graphicsLocation],"re_se_de_a1.png","");
					_renaImage = SafeLoadPNG(_tempRenaPath);
					free(_tempRenaPath);
				}
			}else if (_choice==_voiceVolumeSlot){
				if (voiceVolume==1){
					voiceVolume=0;
				}else{
					voiceVolume+=.25;
				}
				itoa(voiceVolume*4,_tempItoaHoldVoice,10);
			}else if (_choice==_restartBgmActionSlot){
				PlayBGM(lastBGMFilename,lastBGMVolume,1);
			}else if (_choice==_vndsSaveOptionsSlot){ // VNDS Save
				if (!wasJustPressed(SCE_CTRL_RIGHT)){
					PlayMenuSound();
					char* _savedPath = easyVNDSSaveSlot(_chosenSaveSlot);
					LazyMessage("Saved to",_savedPath,NULL,NULL);
					free(_savedPath);
				}else{
					_chosenSaveSlot++;
					sprintf(_tempHoldSaveSlotSelection,"%d=",_chosenSaveSlot);
				}
			}else if (_choice==_vndsHitBottomActionSlot){
				vndsClearAtBottom = !vndsClearAtBottom;
				_settingsOptionsValueText[_vndsHitBottomActionSlot] = charToBoolString(vndsClearAtBottom);
			}else if (_choice==_vndsErrorShowToggleSlot){
				showVNDSWarnings = !showVNDSWarnings;
				if (showVNDSWarnings){
					_settingsOptionsValueText[_vndsErrorShowToggleSlot] = "Show";
				}else{
					_settingsOptionsValueText[_vndsErrorShowToggleSlot] = "Hide";
				}
			}else if (_choice==_higurashiScalingSlot){
				higurashiUsesDynamicScale=!higurashiUsesDynamicScale;
				dynamicScaleEnabled=higurashiUsesDynamicScale;
				updateGraphicsScale();
				updateTextPositions();
				_settingsOptionsValueText[_higurashiScalingSlot]=charToSwitch(higurashiUsesDynamicScale);
				for (i=0;i<maxBusts;++i){
					if (Busts[i].isActive){
						Busts[i].cacheXOffsetScale = GetXOffsetScale(Busts[i].image);
						Busts[i].cacheYOffsetScale = GetYOffsetScale(Busts[i].image);
					}
				}
			}else if (_choice==_textboxModeSlot){
				preferredTextDisplayMode = (preferredTextDisplayMode==TEXTMODE_ADV ? TEXTMODE_NVL : TEXTMODE_ADV);
				_settingsOptionsValueText[_textboxModeSlot]=(preferredTextDisplayMode==TEXTMODE_ADV ? "ADV" : "NVL");
				switchTextDisplayMode(preferredTextDisplayMode);
			}else if (_choice==_vndsBustFadeEnableSlot){
				vndsSpritesFade=!vndsSpritesFade;
				if (_showVNDSFadeOption){
					if (vndsSpritesFade){
						_settingsOptionsValueText[_vndsBustFadeEnableSlot]="On";
					}else{
						_settingsOptionsValueText[_vndsBustFadeEnableSlot]="Off";
					}
				}
			}else if (_choice==_debugButtonSlot){
				debugMenuOption();
			}
			#if PLATFORM == PLAT_VITA
				else if (_choice==_vndsVitaTouchSlot){
					vndsVitaTouch=!vndsVitaTouch;
					if (vndsVitaTouch){
						_settingsOptionsValueText[_vndsVitaTouchSlot]="On";
					}else{
						_settingsOptionsValueText[_vndsVitaTouchSlot]="Off";
					}
				}
			#endif
			else if (_choice==_maxOptionSlotUsed){ // Quit
				#if PLATFORM == PLAT_3DS
					lockBGM();
				#endif
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
		goodDrawText(5,5+(_choice-_scrollOffset)*currentTextHeight,MENUCURSOR,fontSize);
		for (i=0;i<_optionsOnScreen;i++){
			if (i>_maxOptionSlotUsed){
				break;
			}
			goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*i,_settingsOptionsMainText[i+_scrollOffset],fontSize);
			if (_settingsOptionsValueText[i+_scrollOffset]!=NULL){
				goodDrawText(MENUOPTIONOFFSET+textWidth(fontSize,_settingsOptionsMainText[i+_scrollOffset])+singleSpaceWidth,5+currentTextHeight*i,_settingsOptionsValueText[i+_scrollOffset],fontSize);
			}
		}
		if (_needToScroll && _choice!=_maxOptionSlotUsed){
			goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*_optionsOnScreen,"\\/\\/\\/\\/",fontSize); // First option is at 0, so don't subtract one from _optionsOnScreen
		}
		
		// Color CPU overclock text if enabled
		if (cpuOverclocked && PLATFORM != PLAT_3DS){
			goodDrawTextColored(MENUOPTIONOFFSET,5+currentTextHeight*(1-_scrollOffset),_settingsOptionsMainText[1],fontSize,0,255,0);
		}
		// If message box alpha is very high or text is on the bottom screen then make the message box alpha text red
		if ( (MessageBoxAlpha>=230 && canChangeBoxAlpha) || (PLATFORM == PLAT_3DS && cpuOverclocked)){
			goodDrawTextColored(MENUOPTIONOFFSET,5+currentTextHeight*(_messageBoxAlphaSlot-_scrollOffset),_settingsOptionsMainText[_messageBoxAlphaSlot],fontSize,255,0,0);
		}
		if (_shouldShowVNDSSettings && currentlyVNDSGame && gameTextDisplayMode == TEXTMODE_ADV){
			goodDrawTextColored(MENUOPTIONOFFSET,5+currentTextHeight*(_vndsHitBottomActionSlot-_scrollOffset),_settingsOptionsMainText[_vndsHitBottomActionSlot],fontSize,255,0,0);
		}
		// Display sample Rena if changing bust location
		#if PLATFORM == PLAT_3DS
			startDrawingBottom();
			if (_choice==_bustLocationSlot){
				if (_renaImage!=NULL){
					drawTexture(_renaImage,0,screenHeight-getTextureHeight(_renaImage));
				}
			}
		#else
			if (_choice==_bustLocationSlot){
				if (_renaImage!=NULL){
					drawTexture(_renaImage,screenWidth-getTextureWidth(_renaImage)-5,screenHeight-getTextureHeight(_renaImage));
				}
			}
		#endif
		endDrawing();
		fpsCapWait();
		exitIfForceQuit();
	}
	controlsEnd();
	SaveSettings();
	if (_renaImage!=NULL){
		freeTexture(_renaImage);
	}
	if (currentGameStatus!=GAMESTATUS_TITLE){
		// If we changed art location, reload busts
		if (_artBefore != graphicsLocation){
			for (i=0;i<maxBusts;++i){
				if (Busts[i].isActive){
					char* _cacheFilename = mallocForString(Busts[i].relativeFilename);
					DrawBustshot(i,_cacheFilename,Busts[i].xOffset,Busts[i].yOffset,Busts[i].layer,0,0,Busts[i].isInvisible);
					free(_cacheFilename);
				}
			}
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

	char _bottomConfigurationString[13+strlen(SYSTEMSTRING)];
	strcpy(_bottomConfigurationString,SYSTEMSTRING);
	if (isGameFolderMode){
		strcat(_bottomConfigurationString,";Games");
	}else{
		strcat(_bottomConfigurationString,";Presets");
	}
	#if PLATFORM != PLAT_3DS
		if (isActuallyUsingUma0){
			strcat(_bottomConfigurationString,";uma0");
		}else{
			strcat(_bottomConfigurationString,";ux0");
		}
	#endif
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
					if (LazyChoice("Would you like to activate top secret","speedy mode for testing?",NULL,NULL)==1){
						capEnabled=0;
						autoModeWait=50;
					}
				}
			}

		_choice = MenuControls(_choice, 0, isGameFolderMode ? 3 : 4);

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
						nathanscriptDoScript(_tempFilepathBuffer,0,inBetweenVNDSLines);

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
			}else if (_choice==2){ // Go to setting menu
				controlsEnd();
				SettingsMenu(0,0,0,0,1,0,0,0,0);
				controlsEnd();
				break;
			}else if (_choice==3){ // Quit button
				currentGameStatus=GAMESTATUS_QUIT;
				break;
			}else if (_choice==4){
				if (isGameFolderMode){
					LazyMessage("You really shouldn't be here.","You haven't escaped, you know?","You're not even going the right way.",NULL);
				}else{
					ClearMessageArray(0);
					controlsStart();
					controlsEnd();
					OutputLine("This process will convert your legacy preset & StreamingAssets setup to the new game folder setup. It's makes everything easier, so you should do it.\n\nHere's how this will work:\n1) Select a preset file\n2) That preset file will be put in the SteamingAssets folder for you. If you already upgraded the StreamingAssets folder, the preset file just overwrite the old one.\n3) Repeat for all of your games.\n4) You must manually move the StreamingAssets folder(s) using VitaShell or MolecularShell to the games folder.\n\nIf it sounds too hard for you, there's also a video tutorial on the Wololo thread.",Line_WaitForInput,0);

					while (!wasJustPressed(SCE_CTRL_CROSS)){
						fpsCapStart();
						controlsEnd();
						startDrawing();
						DrawMessageText(255,-1);
						endDrawing();
						controlsStart();
						fpsCapWait();
					}
					if (LazyChoice("Upgrade to game folder mode?",NULL,NULL,NULL)){
						if (upgradeToGameFolder()){
							currentGameStatus=GAMESTATUS_QUIT;
							break;
						}
					}
				}
			}else{
				_choice=0;
			}
		}
		

		startDrawing();

		goodDrawText(MENUOPTIONOFFSET,5,"Main Menu",fontSize);

		// Menu options
		goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(0+2),"Load game",fontSize);
		goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(1+2),"Manual mode",fontSize);
		goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(2+2),"Basic Settings",fontSize);
		goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(3+2),"Exit",fontSize);
		if (!isGameFolderMode){
			goodDrawTextColored(MENUOPTIONOFFSET,5+currentTextHeight*(4+2),"Upgrade to game folder mode",fontSize,0,255,0);
		}

		// Extra bottom data
		goodDrawTextColored((screenWidth-5)-_versionStringWidth,screenHeight-5-currentTextHeight,VERSIONSTRING VERSIONSTRINGSUFFIX,fontSize,VERSIONCOLOR);
		goodDrawText(5,screenHeight-5-currentTextHeight,_bottomConfigurationString,fontSize);

		// Cursor
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
	ClearMessageArray(0);
	char _tempNameBuffer[strlen(_passedCurrentName)+1+1+3+1+3+1+1]; // main name + space + left parentheses + three digit number + slash + three digit number + right parentheses + null
	if (tipNamesLoaded==0){
		_passedCurrentName="???";
	}
	sprintf(_tempNameBuffer,"%s (%s/%s)",_passedCurrentName,_passedCurrentSlot,_passedMaxSlot);
	OutputLine(_tempNameBuffer,Line_ContinueAfterTyping,1);
}
void TipMenu(){
	ClearMessageArray(0);
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
			ClearMessageArray(0);
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
void controls_setDefaultGame(){
	if (wasJustPressed(SCE_CTRL_TRIANGLE)){
		if (isGameFolderMode && !isEmbedMode && LazyChoice(defaultGameIsSet ? "Unset this game as the default?" : "Set this game as the default game?",NULL,NULL,NULL)){
			defaultGameIsSet = !defaultGameIsSet;
			setDefaultGame(defaultGameIsSet ? currentGameFolderName : "NONE");
		}
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
		controls_setDefaultGame();
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
		_currentListDrawPosition++;
		if (gameHasTips==1){	
			goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(_currentListDrawPosition+2),"View Tips",fontSize);
			_currentListDrawPosition++;
		}
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
// Hold L to disable font loading
// Hold R to disable all optional loading
void VNDSNavigationMenu(){
	if (!textDisplayModeOverriden){
		switchTextDisplayMode(preferredTextDisplayMode);
	}
	// If the ADV box height won't change make sure the user doesn't make the font size huge
	if (dynamicAdvBoxHeight==0){
		forceFontSizeOption=0;
	}
	controlsStart();
	signed char _choice=0;
	unsigned char _chosenSaveSlot=0;
	char* _loadedNovelName=NULL;

	CrossTexture* _loadedThumbnail=NULL;
	char _chosenSlotAsString[4] = "0";

	//
	char _possibleThunbnailPath[strlen(streamingAssets)+strlen("/SEArchive.legArchive.legList")+1];
	strcpy(_possibleThunbnailPath,streamingAssets);
	strcat(_possibleThunbnailPath,"/vndsvitaproperties");
	if (checkFileExist(_possibleThunbnailPath)){
		_LazyMessage("Loading properties",NULL,NULL,NULL,0);
		FILE* fp = fopen(_possibleThunbnailPath,"rb");
		char _loadedVersionNumber;
		fread(&_loadedVersionNumber,1,1,fp);
		fclose(fp);
		imagesAreJpg=0;
	}else{
		if (!defaultGameIsSet && !isEmbedMode){
			LazyMessage("VNDSVita Game Converter < v1.1",NULL,"Game may crash.",NULL);
		}
		printf("Is old game converter.\n");
	}
	//
	strcpy(_possibleThunbnailPath,streamingAssets);
	strcat(_possibleThunbnailPath,"/thumbnail.png");
	if (checkFileExist(_possibleThunbnailPath) && !isDown(SCE_CTRL_RTRIGGER)){
		_LazyMessage("Loading thumbnail",NULL,NULL,NULL,0);
		_loadedThumbnail = _loadGameImage(_possibleThunbnailPath);
	}
	//
	_possibleThunbnailPath[strlen(streamingAssets)]=0;
	strcat(_possibleThunbnailPath,"/info.txt");
	if (checkFileExist(_possibleThunbnailPath) && !isDown(SCE_CTRL_RTRIGGER)){
		_LazyMessage("Loading info.txt",NULL,NULL,NULL,0);
		FILE* fp = fopen(_possibleThunbnailPath,"r");
		_loadedNovelName = readSpecificIniLine(fp,"title=");
		fclose(fp);
	}
	if (_loadedNovelName==NULL){
		_loadedNovelName = malloc(strlen("VNDS")+1);
		strcpy(_loadedNovelName,"VNDS");
	}
	//
	_possibleThunbnailPath[strlen(streamingAssets)]=0;
	strcat(_possibleThunbnailPath,"/img.ini");
	if (checkFileExist(_possibleThunbnailPath) && !isDown(SCE_CTRL_RTRIGGER)){
		_LazyMessage("Loading img.ini",NULL,NULL,NULL,0);
		FILE* fp = fopen(_possibleThunbnailPath,"r");
		char* _widthString = readSpecificIniLine(fp,"width=");
		char* _heightString = readSpecificIniLine(fp,"height=");
		if (_widthString!=NULL && _heightString!=NULL && isNumberString(_widthString) && isNumberString(_heightString)){
			actualBackgroundWidth = atoi(_widthString);
			actualBackgroundHeight = atoi(_heightString);
			actualBackgroundSizesConfirmedForSmashFive=1;
			updateGraphicsScale();
			updateTextPositions();
		}
		free(_widthString);
		free(_heightString);
		fclose(fp);
	}
	//
	if (!isDown(SCE_CTRL_LTRIGGER) && !isDown(SCE_CTRL_RTRIGGER)){
		_LazyMessage("Loading font",NULL,NULL,NULL,0);
		_possibleThunbnailPath[strlen(streamingAssets)]=0;
		strcat(_possibleThunbnailPath,"/default.ttf");
		if (checkFileExist(_possibleThunbnailPath)){
			_loadSpecificFont(_possibleThunbnailPath);
		}
	}
	//
	_possibleThunbnailPath[strlen(streamingAssets)]=0;
	strcat(_possibleThunbnailPath,"/SEArchive.legArchive");
	if (checkFileExist(_possibleThunbnailPath)){
		_LazyMessage("Loading sound archive",NULL,NULL,NULL,0);
		soundArchive = loadLegArchive(_possibleThunbnailPath);
		useSoundArchive=1;
	}
	_LazyMessage("Body is ready.",NULL,NULL,NULL,0);
	
	controlsEnd();
	while (currentGameStatus!=GAMESTATUS_QUIT){
		fpsCapStart();
		controlsStart();

		_choice = MenuControls(_choice,0,3);
		if (wasJustPressed(SCE_CTRL_CROSS)){

			if (_choice<=1){
				forceResettingsButton=0;
				forceFontSizeOption=0;
				if (_choice==0){
					char _vndsSaveFileConcat[strlen(streamingAssets)+strlen("sav255")+1];
					sprintf(_vndsSaveFileConcat,"%s%s%d",streamingAssets,"sav",_chosenSaveSlot);
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
						nathanscriptDoScript(_vndsMainScriptConcat,0,inBetweenVNDSLines);
						currentGameStatus = GAMESTATUS_NAVIGATIONMENU;
					}else{
						LazyMessage("Main script file",_vndsMainScriptConcat,"not exist.",NULL);
					}
				}
			}else if (_choice==2){
				SettingsMenu(0,1,0,0,0,0,1,1,0);
			}else if (_choice==3){
				currentGameStatus = GAMESTATUS_QUIT;
			}
		}
		if (wasJustPressed(SCE_CTRL_RIGHT)){
			_chosenSaveSlot++;
			itoa(_chosenSaveSlot,_chosenSlotAsString,10);
		}else if (wasJustPressed(SCE_CTRL_LEFT)){
			_chosenSaveSlot--;
			itoa(_chosenSaveSlot,_chosenSlotAsString,10);
		}
		controls_setDefaultGame();
		controlsEnd();
		startDrawing();

		if (_loadedThumbnail!=NULL){
			drawTexture(_loadedThumbnail,screenWidth-getTextureWidth(_loadedThumbnail),screenHeight-getTextureHeight(_loadedThumbnail));
		}

		goodDrawText(MENUOPTIONOFFSET,0,_loadedNovelName,fontSize);

		goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(0+2),"Load Save",fontSize);
			goodDrawText(MENUOPTIONOFFSET+textWidth(fontSize,"Load Save")+singleSpaceWidth,5+currentTextHeight*(0+2),_chosenSlotAsString,fontSize);
		goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(1+2),"New Game",fontSize);
		goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(2+2),"VNDS Settings",fontSize);
		goodDrawText(MENUOPTIONOFFSET,5+currentTextHeight*(3+2),"Exit",fontSize);

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
		if (_didLoadHappyLua==1){
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
		nathanscriptAddFunction(vndswrapper_sound,nathanscriptMakeConfigByte(0,1),"sound");
		nathanscriptAddFunction(vndswrapper_choice,nathanscriptMakeConfigByte(1,0),"choice");
		nathanscriptAddFunction(vndswrapper_delay,0,"delay");
		nathanscriptAddFunction(vndswrapper_cleartext,0,"cleartext");
		nathanscriptAddFunction(vndswrapper_bgload,0,"bgload");
		nathanscriptAddFunction(vndswrapper_setimg,0,"setimg");
			foundSetImgIndex = nathanCurrentRegisteredFunctions-1;
		nathanscriptAddFunction(vndswrapper_jump,0,"jump");
		nathanscriptAddFunction(vndswrapper_music,0,"music");
		nathanscriptAddFunction(vndswrapper_gsetvar,nathanscriptMakeConfigByte(0,1),"gsetvar");
		nathanscriptAddFunction(scriptImageChoice,0,"imagechoice");
		nathanscriptAddFunction(vndswrapper_ENDOF,0,"ENDSCRIPT");
		nathanscriptAddFunction(vndswrapper_ENDOF,0,"END_OF_FILE");

		// Load global variables
		char _globalsSaveFilePath[strlen(saveFolder)+strlen("vndsGlobals")+1];
		strcpy(_globalsSaveFilePath,saveFolder);
		strcat(_globalsSaveFilePath,"vndsGlobals");
		if (checkFileExist(_globalsSaveFilePath)){
			FILE* fp = fopen(_globalsSaveFilePath,"rb");
			unsigned char _loadedFileFormat;
			fread(&_loadedFileFormat,sizeof(unsigned char),1,fp);
			if (_loadedFileFormat>=1){
				loadVariableList(fp,&nathanscriptGlobalvarList,&nathanscriptTotalGlobalvar);
			}
			fclose(fp);
		}

		increaseVNDSBustInfoArraysSize(0,maxBusts);
	}
}
void testCode(){
	//#warning TEST CODE INCLUDED!
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

	// Guess the graphic sizes
	actualBackgroundWidth = screenWidth;
	actualBackgroundHeight = screenHeight;
	actualBackgroundSizesConfirmedForSmashFive=0;

	// Make buffers for busts
	increaseBustArraysSize(0,maxBusts);

	// Reset bust cache
	// I could memset everything to 0, but apparently NULL is not guaranteed to be represented by all 0.
	// https://stackoverflow.com/questions/9894013/is-null-always-zero-in-c
	for (i=0;i<MAXBUSTCACHE;++i){
		bustCache[i].filename=NULL;
		bustCache[i].image=NULL;
	}

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

		free(gamesFolder);
		fixPath("",globalTempConcat,TYPE_EMBEDDED);
		gamesFolder = strdup(globalTempConcat);

		currentGameFolderName = strdup("game");
		currentGameStatus = GAMESTATUS_LOADGAMEFOLDER;
	}else{
		controlsStart();
		if (!isDown(SCE_CTRL_RTRIGGER)){ // Hold R to skip default game check
			char _defaultGameSaveFilenameBuffer[strlen(saveFolder)+strlen("_defaultGame")+1];
			strcpy(_defaultGameSaveFilenameBuffer,saveFolder);
			strcat(_defaultGameSaveFilenameBuffer,"_defaultGame");
			if (checkFileExist(_defaultGameSaveFilenameBuffer)){
				FILE* fp;
				fp = fopen(_defaultGameSaveFilenameBuffer,"r");
				char _readGameFolderName[256];
				_readGameFolderName[0]='\0';
				fgets(_readGameFolderName,256,fp);
				fclose(fp);
				if (strcmp(_readGameFolderName,"NONE")!=0){
					defaultGameIsSet=1;
					currentGameFolderName = malloc(strlen(_readGameFolderName)+1);
					strcpy(currentGameFolderName,_readGameFolderName);
					currentGameStatus=GAMESTATUS_LOADGAMEFOLDER;
				}
			}
		}
		controlsEnd();
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
			isGameFolderMode=1;
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
	ClearMessageArray(0);
	if (initializeLua()==2){
		return 2;
	}

	for (i=0;i<maxBusts;i++){
		ResetBustStruct(&(Busts[i]),0);
	}

	#if PLATFORM == PLAT_VITA && SOUNDPLAYER != SND_VITA
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

	testCode();
	return 0;
}
int main(int argc, char *argv[]){
	/* code */
	if (init()==2){
		currentGameStatus = GAMESTATUS_QUIT;
	}

	#if PLATFORM == PLAT_VITA
		sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, 1);
		sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, 1);
		sceTouchEnableTouchForce(SCE_TOUCH_PORT_FRONT);
		sceTouchEnableTouchForce(SCE_TOUCH_PORT_BACK);
	#endif

	// Put stupid test stuff here
	while (currentGameStatus!=GAMESTATUS_QUIT){
		switch (currentGameStatus){
			case GAMESTATUS_TITLE:
				TitleScreen();
				break;
			case GAMESTATUS_LOADPRESET: // Still needed because presets are loaded internally
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
			case GAMESTATUS_GAMEFOLDERSELECTION: // Sets game folder selection folder name to currentGameFolderName
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
				currentGameFolderName = _chosenGameFolder; // Do not free _chosenGameFolder
				currentGameStatus = GAMESTATUS_LOADGAMEFOLDER;
				break;
			case GAMESTATUS_LOADGAMEFOLDER: // Can load both Higurashi and VNDS games
				;
				char _possibleVNDSStatusFile[strlen(gamesFolder)+strlen(currentGameFolderName)+strlen("/Scripts/main.scr")+1];
				strcpy(_possibleVNDSStatusFile,gamesFolder);
				strcat(_possibleVNDSStatusFile,currentGameFolderName);
				strcat(_possibleVNDSStatusFile,"/isvnds");
				if (checkFileExist(_possibleVNDSStatusFile)){
					initializeNathanScript();
					// Special settings for vnds
					activateVNDSSettings();
					// Setup StreamingAssets path
					_possibleVNDSStatusFile[strlen(gamesFolder)+strlen(currentGameFolderName)]=0;
					GenerateStreamingAssetsPaths(_possibleVNDSStatusFile,0);
					currentGameStatus = GAMESTATUS_NAVIGATIONMENU;
					// VNDS games also support game specific lua
					LoadGameSpecificStupidity();
					VNDSNavigationMenu();
				}else{
					if (strcmp(currentGameFolderName,"PLACEHOLDER.txt")==0){
						strcpy(_possibleVNDSStatusFile,gamesFolder);
						strcat(_possibleVNDSStatusFile,currentGameFolderName);
						remove(_possibleVNDSStatusFile);
						free(currentGameFolderName);
						break;
					}
					if (startLoadingGameFolder(currentGameFolderName)){
						currentGameStatus=GAMESTATUS_LOADPRESET;
					}else{
						currentGameStatus=GAMESTATUS_TITLE;
						if (defaultGameIsSet){
							setDefaultGame("NONE"); // Prevent this message every time on startup if it's because of default game setting
							LazyMessage("Reset default game","setting.",NULL,NULL);
						}
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
