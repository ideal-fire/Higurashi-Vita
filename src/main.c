/*
	(OPTIONAL TODO)
		TODO - text property - italics
			OutputLine(NULL, "　……知レバ、…巻キ込マレテシマウ…。",
			   NULL, "...<i>If she found out... she would become involved</i>...", Line_Normal);
		TODO - Position markup
			At the very end of Onikakushi, I think that there's a markup that looks something like this <pos=36>Keechi</pos>
		TODO - Remove scriptFolder variable
		TODO - Inspect SetDrawingPointOfMessage
			It appears to just set the line to draw on, or that's at least what it's usually used for.
			Inspect what the max line I can use in it is.
			Think about how I could implement this command if the value given is bigger than the total number of lines
				Change the actual text box X and text box Y and use the input arg as a percentage of the screen?
			Actually, the command is removed in ADV mode.
		TODO - Allow VNDS sound command to stop all sounds
		TODO - SetSpeedOfMessage
	TODO - Mod libvita2d to not inlcude characters with value 1 when getting text width. (This should be easy to do. There's a for loop)
	TODO - is entire font in memory nonsense still needed
	TODO - Fix this text speed setting nonsense
	TODO - Game specific settings files
	TODO - in manual mode, running _GameSpecific.lua first won't keep the settings from being reset before the next manual script you run.
	TODO - i removed the secret save file editor code
				if (_codeProgress==4){
					SaveGameEditor();
					_nextChapterExist=1;
					_codeProgress=0;
				}
	TODO - textbox alpha should change with background alpha
	TODO - outputLineScreenHeight variable name is a lie. it is just screenHeight
	TODO - don't show "save game" option in toucb bar if save not supported. oops. looks like the function should just be passed a map of which ones to enable
	TODO - textboxWidth bug
	TODO - restore default game functionality
	TODO - use showmenu for scriptselect (this is tough because we can't open the in-game menu from showmenu)

	Colored text example:
		text x1b[<colorID>;1m<restoftext>
		text x1b[0m
*/
#define PLAYTIPMUSIC 0

// Libraries all need
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h> // toupper
#include <stdarg.h>
#include <limits.h>
//
#include <Lua/lua.h>
#include <Lua/lualib.h>
#include <Lua/lauxlib.h>
//
#include <goodbrew/config.h>
#if GBVERSION < 7
	#error update libgoodbrew
#endif
#include <goodbrew/platform.h>
#include <goodbrew/base.h>
#include <goodbrew/graphics.h>
#include <goodbrew/controls.h>
#include <goodbrew/images.h>
#include <goodbrew/sound.h>
#include <goodbrew/paths.h>
#include <goodbrew/text.h>
#include <goodbrew/useful.h>
#include "main.h"
//
#if GBPLAT == GB_VITA
	#include <libvita2dplusbloat/vita2d.h>
	#include <psp2/kernel/processmgr.h> 
	#include <psp2/display.h> // used with thumbnail creation
	#include <psp2/power.h> // overclock
	#include <psp2/ctrl.h> // sound protect thread
#endif
#if GBPLAT == GB_ANDROID
	#include "SDLLuaDoFile.h"
#endif
#include "insensitiveFileFinder.h"
#include "legarchive.h"
#include "fragmentMenu.h"

#define LOCATION_UNDEFINED 0
#define LOCATION_CG 1
#define LOCATION_CGALT 2
/////////////////////////////////////
#define MAXBUSTCACHE 8
#define MAXFILES 50
#define MAXFILELENGTH 51
#define MAXMESSAGEHISTORY 40
#define DEFAULTVERSION "forgotversionnumber"
#define VERSIONSTRING DEFAULTVERSION // This
#define VERSIONNUMBER 8 // This
#define VERSIONSTRINGSUFFIX ""
#define VERSIONCOLOR 255,135,53 // It's Rena colored!
// Specific constants
#if GBPLAT != GB_3DS
	#if GBPLAT == GB_ANDROID
		#define SELECTBUTTONNAME "TOUCH"
		#define BACKBUTTONNAME "BACK"
	#else
		#define SELECTBUTTONNAME "X"
		#define BACKBUTTONNAME "O"
	#endif
	int advboxHeight = 181;
#else
	#define SELECTBUTTONNAME "A"
	#define BACKBUTTONNAME "B"
	int advboxHeight = 75;
	char textIsBottomScreen=0;
#endif
char* vitaAppId="HIGURASHI";
#if GBPLAT == GB_VITA
	#define CANINVERT 1
#else
	#define CANINVERT 0
#endif
#define HISTORYONONESCREEN ((int)((screenHeight-currentTextHeight*2-5)/currentTextHeight))
#define MENUCURSOR ">"
#define MENUCURSOROFFSET 5
#define MENUOPTIONOFFSET menuCursorSpaceWidth+5
#define CLEARMESSAGEFADEOUTTIME 100
#define DEFAULTFONTCOLORR 255
#define DEFAULTFONTCOLORG 255
#define DEFAULTFONTCOLORB 255
#define DEFAULTFONTCOLOR DEFAULTFONTCOLORR,DEFAULTFONTCOLORG,DEFAULTFONTCOLORB
////////////////////////////////////
#define TEXTBOXFADEOUTTIME 200 // In milliseconds
#define TEXTBOXFADEINTIME 150
////////////////////////////////////
#define COLORMARKUPSTART "<color=#"
#define COLORMARKUPEND "</color>"
////////////////////////////////////
#if GBTXT==GBTXT_BITMAP
	#define DEFAULTEMBEDDEDFONT "assets/Bitmap-LiberationSans-Regular"
#else
	#define DEFAULTEMBEDDEDFONT "assets/ume-pgo4.ttf"
#endif //loadFont("sa0:data/font/pvf/ltn4.pvf");

#define MAXMUSICARRAY 10
#define MAXSOUNDEFFECTARRAY 10
#define MESSAGEEDGEOFFSET 10
#define MENUSFXON 1

#define THUMBWIDTH screenWidth/3
#define THUMBHEIGHT screenHeight/3
#define SAVESELECTORRECTTHICK 5
#define SAVEMENUPAGEW 2
#define SAVEMENUPAGEH 3
#define SAVEMENUPAGESIZE (SAVEMENUPAGEW*SAVEMENUPAGEH)
#define MAXSAVESLOT 258 // Divisible by 6
#define VNDSSAVESELSLOTPREFIX "Slot "

#define FAKELUAERRORMSG "higuvitafakeerr"
#define HIDDENCHAPTERTITLE ".\\/hidden"

// showMenu
// ratio of screen width that the text will scroll in one second
#define TEXTSCROLLPERSECOND .25
#define TEXTSCROLLDELAYTIME 300

#define PREFER_DIR_BGM 0
#define PREFER_DIR_SE 1
#define PREFER_DIR_VOICE 2
#define PREFER_DIR_NOMEIMPORTA 3

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

#define PREFERREDMINMAXLINES 10 // if we have fewer than this maxLines and in NVL mode then discard NVL bottom padding
#define totalTextYOff() (textboxTopPad+messageInBoxYOffset+textboxYOffset)
#define totalTextXOff() (textboxXOffset+messageInBoxXOffset)
#define shouldShowADVNames() (gameTextDisplayMode==TEXTMODE_ADV && (advNamesSupported==2 || (advNamesSupported && prefersADVNames)))
#define shouldClearHitBottom() (currentlyVNDSGame && clearAtBottom)
#define getOutputLineScreenWidth() (textboxWidth-textboxXOffset*2-messageInBoxXOffset*2)
#define ADVNAMEOFFSET (currentTextHeight*1.5) // Space between top of ADV name and rest of the text. does not apply if adv name is an image
#define IMADVNAMEPOSTPAD (textboxTopPad)

// TODO - Proper libGeneralGood support for this
#if GBSND == GBSND_VITA
	char mlgsndIsPlaying(NathanAudio* _passedAudio);
#endif

#if GBPLAT == GB_VITA
	#include "../stolenCode/qdbmp.h"
#endif

// 1 is start
// 2 adds BGM and SE volume
// 3 adds voice volume
// 4 adds preferredBoxAlpha and textOnlyOverBackground
// 5 adds textSpeed
// 6 adds clearAtBottom
// 7 adds showVNDSWarnings
// 8 adds higurashiUsesDynamicScale (now ignored)
// 9 adds preferredTextDisplayMode
// 10 adds autoModeVoicedWait
// 11 adds vndsSpritesFade
// 12 adds touchProceed
// 13 adds dropshadowOn
// 14 adds fontSize
// 15 adds prefersADVNames
// 16 adds playerLanguage
#define OPTIONSFILEFORMAT 16

// 1 is end
// 2 adds currentADVName
#define VNDSSAVEFORMAT 2
#define validVNDSSaveFormat(a) (a==1 || a==2)

#define VNDSGLOBALSSAVEFORMAT 1

// 2 is start
// 3 adds localFlags
// 4 adds fragment info
#define HIGUSAVEFORMAT 4

//#define LUAREGISTER(x,y) DebugLuaReg(y);
#define LUAREGISTER(x,y) lua_pushcfunction(L,x);\
	lua_setglobal(L,y);

// Make a lua function and all it does is set a number variable to what you give to it
#define EASYLUAINTSETFUNCTION(scriptFunctionName,varname) \
	int L_##scriptFunctionName(lua_State* passedState){ \
		varname = lua_tonumber(passedState,1); \
		return 0; \
	}
#define EASYLUAINTSETFUNCTIONPOSTCALL(scriptFunctionName,varname,postfunc)	\
	int L_##scriptFunctionName(lua_State* passedState){ \
		varname = lua_tonumber(passedState,1); \
		postfunc \
		return 0; \
	}
// Make a lua function and all it does it return a variable
#define EASYLUAINTGETFUNCTION(scriptFunctionName,varname) \
	int L_##scriptFunctionName(lua_State* passedState){ \
		lua_pushnumber(passedState,varname);			\
		return 1; \
	}
// Easily push stuff made with EASYLUAINTSETFUNCTION
#define PUSHEASYLUAINTSETFUNCTION(scriptFunctionName) \
	LUAREGISTER(L_##scriptFunctionName,#scriptFunctionName)

#define drawText(_x,_y,_text) gbDrawText(normalFont,_x,_y,_text,DEFAULTFONTCOLOR);

////////////////////////////////////////
// GBPLAT specific variables
///////////////////////////////////////

//////////////
// The streamingAssets variable is only used for images and sound
char* streamingAssets;
char* scriptFolder;
char* saveFolder;
char* gamesFolder;

#define BUST_STATUS_NORMAL 0
#define BUST_STATUS_FADEIN 1 // var 1 is alpha per frame. var 2 is the time until we're actually going to start the fadein. Until var 2 is at 0, this bust will have 0 alpha. For some reason, the Higurashi engine wastes half of the specified time with the bust just not there.
#define BUST_STATUS_FADEOUT 2 // var 1 is alpha per frame
//#define BUST_STATUS_FADEOUT_MOVE 3 // var 1 is alpha per frame. var 2 is x per frame. var 3 is y per frame
#define BUST_STATUS_SPRITE_MOVE 4 // var 1 is x per frame, var 2 is y per frame
#define BUST_STATUS_TRANSFORM_FADEIN 5 // The bust is fading into an already used slot. image is what the new texture is going to be thats fading in, transformTexture is the old texture that is fading out. var 1 is alpha per frame. added to image, subtracted from transformTexture.

#if GBPLAT == GB_VITA
	NathanAudio* _mlgsnd_loadAudioFILE(legArchiveFile _passedFile, char _passedFileFormat, char _passedShouldLoop, char _passedShouldStream);
#endif
signed char touchProceed=1;

void invertImage(crossTexture* _passedImage, signed char _doInvertAlpha);
struct shakeInfo{
	double timePerPeak;
	double dragMultiplier;
	int range;
	char direction; // changed to be a bitmap. bit 1 is left and right, bit 2 is up and down.
	u64 endTime;
	u64 startTime;
};
typedef struct{
	crossTexture* image;
	signed int xOffset;
	signed int yOffset;
	double scaleX;
	double scaleY;
	char isActive;
	int layer;
	signed short curAlpha;
	unsigned char bustStatus;
	char* relativeFilename; // Filename passed by the script
	unsigned int lineCreatedOn;
	double cacheXOffsetScale;
	double cacheYOffsetScale;
	int originXForAdjust; // originX and originY sets a custom point to be the center of the screen. your position is relative to that.
	int originYForAdjust;
	//int angle;
	struct shakeInfo* curShake;
	// status variables. ignore most time.
	u64 fadeStartTime; // for BUST_STATUS_FADEIN, BUST_STATUS_TRANSFORM_FADEIN, BUST_STATUS_FADEOUT,
	u64 fadeEndTime;
	int startXMove;
	int startYMove;
	int diffXMove;
	int diffYMove;
	u64 startMoveTime;
	int diffMoveTime;
	crossTexture* transformTexture; // See BUST_STATUS_TRANSFORM_FADEIN. This is the texture that is transforming
	unsigned char destAlpha; // how much we want it to have after the fadein
}bust;
typedef struct{
	crossTexture* image;
	char* filename;
}cachedImage;

// text properties is stored in int32_t with four bytes
// first three bytes are color
// lsat byte is a bitmap of the following properties:
#define TEXTPROP_COLORED 1

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

char playerLanguage=1;

// used in the enlargeScreen function
double extraGameScaleX=1;
double extraGameScaleY=1;
int extraGameOffX=0;
int extraGameOffY=0;
struct enlargeAnimInfo* workingEnlarge=NULL; // not NULL if we're enlarging
struct enlargeAnimInfo{
	u64 startTime;
	int totalTime;
	double destScaleX;
	double destScaleY;
	int destOffX;
	int destOffY;
	double startScaleX;
	double startScaleY;
	int startOffX;
	int startOffY;
};
struct shakeInfo* curBackgroundShake;
struct shakeInfo* curUIShake;

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
int maxLines=0;
char** currentMessages;
int32_t** messageProps; // a line in messageProps can only be valid if its corresponding currentMessages line isn't NULL
int currentLine=0;
unsigned char nextTextR=DEFAULTFONTCOLORR;
unsigned char nextTextG=DEFAULTFONTCOLORG;
unsigned char nextTextB=DEFAULTFONTCOLORB;
int place=0;

int lastBGMVolume = 128;

crossTexture* currentBackground = NULL;
crossMusic* currentMusic[MAXMUSICARRAY] = {NULL};
crossPlayHandle currentMusicHandle[MAXMUSICARRAY] = {0};
char* currentMusicFilepath[MAXMUSICARRAY]={NULL};
short currentMusicUnfixedVolume[MAXMUSICARRAY] = {0};
//crossMusic* currentMusic = NULL;
crossSE* soundEffects[MAXSOUNDEFFECTARRAY] = {NULL};

crossSE* menuSound=NULL;
signed char menuSoundLoaded=0;

// Alpha of black rectangle over screen
unsigned char currentBoxAlpha=100;
unsigned char preferredBoxAlpha=100;
signed char MessageBoxEnabled=1;
signed char isSkipping=0;
signed char isTouchSkipHold=0;
signed char inputValidity=1;

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
char* currentGameFolderName=NULL;

signed char currentGameStatus=GAMESTATUS_TITLE;

signed char tipNamesLoaded=0;
signed char chapterNamesLoaded=0;
unsigned char lastSelectionAnswer=0;

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
signed char currentFilterType=FILTERTYPE_INACTIVE;

signed char autoModeOn=0;
int autoModeWait=500;
int autoModeVoicedWait=500;

signed char cpuOverclocked=0;

#define TEXTMODE_NVL 0
#define TEXTMODE_ADV 1
#define TEXTMODE_AVD TEXTMODE_ADV // Wrong spelling
char gameTextDisplayMode=TEXTMODE_NVL;
signed char dropshadowOn=1;
char hasOwnVoiceSetting=0;

unsigned char graphicsLocation = LOCATION_CGALT;

char* messageHistory[MAXMESSAGEHISTORY] = {NULL};
unsigned char oldestMessage=0;

char presetsAreInStreamingAssets=1;

float bgmVolume = 0.75;
float seVolume = 1.0;
float voiceVolume = 1.0;

crossFont* normalFont=NULL;
double fontSize=-10; // default value < 0
int currentTextHeight;
int singleSpaceWidth;
int dropshadowOffX; // calculated by reloadFont
int dropshadowOffY;
unsigned char dropshadowR=0;
unsigned char dropshadowG=0;
unsigned char dropshadowB=0;
#if GBPLAT == GB_VITA
	pthread_t soundProtectThreadId;
	void* _loadedFontBuffer=NULL;
#elif GBPLAT == GB_3DS
	char _3dsSoundProtectThreadIsAlive=1;
	Thread _3dsSoundUpdateThread;
	char _bgmIsLock=0;
#endif
char isActuallyUsingUma0=0;
int maxBusts = 9;
short textboxYOffset=0;
short textboxXOffset=0;
crossTexture* currentCustomTextbox=NULL;
int textboxWidth;
int outputLineScreenHeight;
int messageInBoxXOffset=10;
int messageInBoxYOffset=0;
int textboxTopPad=12; // formerly STUPIDTEXTYOFF
int textboxBottomPad=0;
// 1 by default to retain compatibility with games converted before game specific Lua 
char gameHasTips=1;
char textOnlyOverBackground=1;
// This is a constant value between 0 and 127 that means that the text should be instantly displayed
#define TEXTSPEED_INSTANT 100
signed char textSpeed=1;
char isEmbedMode;
int menuCursorSpaceWidth;
char canChangeBoxAlpha=1;
// When this variable is 1, we can assume that the current game is the default game because the user can't chose a different game when a default is set.
char defaultGameIsSet;
char nathanscriptIsInit=0;
char scriptUsesFileExtensions=0;
char bustsStartInMiddle=1;

// What scripts think the screen width and height is for sprite positions
// For Higurashi, this is 640x480
// For vnds, this is the DS' screen resolution
int scriptScreenWidth=640;
int scriptScreenHeight=480;

// X and Y scale applied to graphics size
double graphicsScale=1.0;
signed char dynamicScaleEnabled=1;
char overrideOffsetVals;
double overrideXOffScale;
double overrideYOffScale;
int overrideBustOffX;
int overrideBustOffY;

// This assumes the background is the biggest image. These values decide how much to scale all other elements, such as characters.
int actualBackgroundWidth;
int actualBackgroundHeight;
// If these values should not change because we're sure they're right. We can be sure these are right if they're specified in vnds ini file.
char actualBackgroundSizesConfirmedForSmashFive=0;

char* lastBackgroundFilename=NULL;
char* currentScriptFilename=NULL;
char* lastBGMFilename=NULL;
char* currentFontFilename=NULL;

char currentlyVNDSGame=0;
char nextVndsBustshotSlot=0;
// If all the text should be cleared when the text reached the bottom of the screen when playing a VNDS game
signed char clearAtBottom=0;
signed char showVNDSWarnings=1;
signed char dynamicAdvBoxHeight=0;
crossTexture* advNameImSheet=NULL;
char* currentADVName=NULL;
int currentADVNameIm=-1;
int advNameImHeight=-1;
int* advImageNamePos;
unsigned char advNameR=255;
unsigned char advNameG=255;
unsigned char advNameB=255;
signed char prefersADVNames=1;
// 1 if supported
// 2 if they're forced
char advNamesSupported=0;
char advNamesPersist=0;
int advImageNameCount=0;
// Will only be used in games it can be used in
signed char preferredTextDisplayMode=TEXTMODE_NVL;
signed char useSoundArchive=0;
legArchive soundArchive;
signed char lastVoiceSlot=-1;
// only valid of lastVoiceSlot is not -1
crossPlayHandle lastVoiceHandle;
int foundSetImgIndex = -1;
signed char vndsSpritesFade=1;
char textDisplayModeOverriden=0; // If the text display mode has been changed manually by the script
//
// showMenuAdvancedTouch info - there's no way UI functions need to be thread safe
//
char** lastTouchMenuOptions=NULL;
int lastTouchMenuRetYPos=-1;
//
signed char forceShowQuit=-1;
signed char forceShowVNDSSettings=-1;
signed char forceShowVNDSSave=-1;
signed char forceShowRestartBGM=-1;
signed char forceArtLocationSlot=-1;
signed char forceTextBoxModeOption=-1;
signed char forceVNDSFadeOption=-1;
signed char forceDebugButton=-1;
signed char forceResettingsButton=1;
signed char forceTextOverBGOption=1;
signed char forceFontSizeOption=1;
signed char forceDropshadowOption=1;

#ifdef SPECIALEDITION
	#include "specialEditionHeader.h"
#endif

/*
====================================================
*/
#if GBPLAT == GB_LINUX
	char* itoa(int value, char* _buffer, int _uselessBase){
		sprintf(_buffer,"%d",value);
		return _buffer;
	}
#endif
char isSpaceOrEmptyStr(const char* _check){
	if (!_check){
		return 1;
	}
	for (;;++_check){
		unsigned char _c = *_check;
		if (_c=='\0'){
			break;
		}
		if (!isspace(_c)){
			return 0;
		}
	}
	return 1;
}
void getInverseBGCol(unsigned char* r, unsigned char* g, unsigned char* b){
	getClearColor(r,g,b);
	*r=255-*r;
	*g=255-*g;
	*b=255-*b;
}
char proceedPressed(){
	return wasJustPressed(BUTTON_A) || (touchProceed && wasJustPressed(BUTTON_TOUCH));
}
// strcpy, but it won't copy from src to dest if the value is 1.
// You can use this to exclude certian spots
void strcpyNO1(char* dest, const char* src){
	int i;
	int _destCopyOffset=0;
	int _srcStrlen = strlen(src);
	for (i=0;i<_srcStrlen;i++){
		if (src[i]!=1){
			dest[_destCopyOffset++]=src[i];
		}
	}
	dest[_destCopyOffset]=0;
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
void setPropBit(int32_t* _dest, unsigned char _flags){
	*(((char*)_dest)+3)|=_flags;
}
// the list itself must also be allocated
void freeAllocdStrList(char** _passedList, int _passedLen){
	int i;
	for (i=0;i<_passedLen;++i){
		free(_passedList[i]);
	}
	free(_passedList);
}
int getOtherScaled(int _orig, int _scaled, int _altDim){
	return _altDim*(_scaled/(double)_orig);
}
char pointInBox(int _x, int _y, int _boxX, int _boxY, int _boxW, int _boxH){
	return (_x>_boxX && _x<_boxX+_boxW && _y>_boxY && _y<_boxY+_boxH);
}
void fitInBox(int _imgW, int _imgH, int _boxW, int _boxH, int* _retW, int* _retH){
	if ((_boxW/(double)_imgW) < (_boxH/(double)_imgH)){
		*_retW=_boxW;
		*_retH=getOtherScaled(_imgW,_boxW,_imgH);
	}else{
		*_retW=getOtherScaled(_imgH,_boxH,_imgW);
		*_retH=_boxH;
	}
}
int easyCenter(int _smallSize, int _bigSize){
	return (_bigSize-_smallSize)/2;
}
double partMoveFillsEndTime(u64 _curTicks, u64 _endTime, int _totalDifference, double _max){
	return ((_totalDifference-(_endTime-_curTicks))*_max)/(double)_totalDifference;
}
double partMoveValToVal(u64 _curTicks, u64 _endTime, int _totalDifference, double _startRet, double _endRet){
	return _startRet+partMoveFillsEndTime(_curTicks,_endTime,_totalDifference,_endRet-_startRet);
}
double partMoveFills(u64 _curTicks, u64 _startTime, int _totalDifference, double _max){
	return partMoveFillsEndTime(_curTicks,_startTime+_totalDifference,_totalDifference,_max);
}
double partMoveFillsCapped(u64 _curTicks, u64 _startTime, int _totalDifference, double _max){
	if (_curTicks>=_startTime+_totalDifference){
		return _max;
	}else{
		return partMoveFills(_curTicks,_startTime,_totalDifference,_max);
	}
}
double partMoveEmptys(u64 _curTicks, u64 _startTime, int _totalDifference, double _max){
	return _max-partMoveFills(_curTicks,_startTime,_totalDifference,_max);
}
#ifndef SPECIALEDITION
	float fixX(float _passed){
		return _passed;
	}
	float fixY(float _passed){
		return _passed;
	}
	int fixTouchX(int _passed){
		return _passed;
	}
	int fixTouchY(int _passed){
		return _passed;
	}
#endif
int limitNum(int _passed, int _min, int _max){
	if (_passed<_min){
		return _min;
	}if (_passed>_max){
		return _max;
	}
	return _passed;
}
int wrapNum(int _passed, int _min, int _max){
	if (_passed<_min){
		return _max-(_min-_passed-1);
	}
	if (_passed>_max){
		return _min+(_passed-_max-1);
	}
	return _passed;
}
void showErrorIfNull(void* _passedImage){
	if (!_passedImage){
		printf("Error, showErrorIfNull.\n");
	}
}
void drawTextureScaleAlphaGood(crossTexture* texture, float x, float y, double x_scale, double y_scale, unsigned char alpha){
	drawTextureSizedAlpha(texture,x,y,getTextureWidth(texture)*x_scale,getTextureHeight(texture)*y_scale,alpha);
}
char shouldShowWarnings(){
	return !(currentlyVNDSGame && !showVNDSWarnings);
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
char* charToBoolString(char _boolValue){
	return _boolValue ? "True" : "False";
}
char* charToSwitch(char _boolValue){
	return _boolValue ? "On" : "Off";
}
double applyGraphicsScale(double _valueToScale){
	return _valueToScale*graphicsScale;
}
void changeMallocString(char** _stringToChange, const char* _newValue){
	char* _newBuffer = _newValue!=NULL ? strdup(_newValue) : NULL; // Make new buffer before free allows us to set a malloc string to itself
	if (*_stringToChange!=NULL){
		free(*_stringToChange);
	}
	*_stringToChange = _newBuffer;
}
void XOutFunction(){
	#if GBPLAT == GB_3DS
		_3dsSoundProtectThreadIsAlive=0;
		threadJoin(_3dsSoundUpdateThread,30000000000);
	#endif
	quitGraphics();
	quitAudio();
	generalGoodQuit();
	exit(0);
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
		playSound(menuSound,10);
	}
}
void safeFreeShakeInfo(struct shakeInfo* s){
	// if this shakeInfo is used anywhere, don't free it.
	if (curBackgroundShake==s || curUIShake==s){
		return;
	}
	for (int i=0;i<maxBusts;i++){
		if (Busts[i].curShake==s){
			return;
		}
	}
	free(s);
}
void freeBustShakeInfo(int _slot){
	if (Busts[_slot].isActive && Busts[_slot].curShake){
		struct shakeInfo* s=Busts[_slot].curShake;
		Busts[_slot].curShake=NULL;
		safeFreeShakeInfo(s);
	}
}
// uses new speed formula
struct shakeInfo* makeShakeInfo(int speed, int range, int drag, int direction, int loops, u64 _startTime){
	struct shakeInfo* _ret = malloc(sizeof(struct shakeInfo));
	_ret->timePerPeak=((256-speed)*5)/(double)1000;
	_ret->dragMultiplier=1-drag/(double)100;
	if (direction==0){
		_ret->direction=1;
	}else if (direction==1){
		_ret->direction=3;
	}else if (direction==2){
		_ret->direction=2;
	}
	_ret->range=range;
	_ret->startTime=_startTime;
	_ret->endTime=_ret->startTime+loops*_ret->timePerPeak*1000;
	return _ret;
}
void waitForShakeEnd(struct shakeInfo** s){
	while(*s){
		controlsStart();
		if(proceedPressed()){
			(*s)->endTime=(*s)->startTime+1;
		}
		Update();
		controlsEnd();
		startDrawing();
		Draw(MessageBoxEnabled);
		endDrawing();
	}
}
void offsetForShake(struct shakeInfo* s, int* x, int* y){
	if (s){
		double _functionX = (((getMilli()-s->startTime)/(double)1000)/s->timePerPeak)*M_PI;
		int _amount=(sin(_functionX)*pow(s->dragMultiplier,(_functionX-M_PI_2)/M_PI))*s->range;
		if (s->direction & 1){
			*x+=_amount;
		}
		if (s->direction & 2){
			*y+=_amount;
		}
	}
}
void freeDoneShake(struct shakeInfo** s, u64 _sTime){
	if (*s && _sTime>=(*s)->endTime){
		struct shakeInfo* _hold=*s;
		*s=NULL;
		safeFreeShakeInfo(_hold);
	}
}
char getLocalFlag(const char* _varName, int* _retVal){
	char _did=0;
	lua_getglobal(L,"localFlags");
	if (lua_getfield(L,-1,_varName)==LUA_TNUMBER){
		_did=1;
		*_retVal=lua_tonumber(L,-1);
	}
	lua_pop(L,2);
	return _did;
}
void setLocalFlag(const char* _varName, int _val){
	lua_getglobal(L,"localFlags");
	lua_pushnumber(L,_val);
	lua_setfield(L,-2,_varName);
}
crossTexture* safeLoadImage(const char* path){
	crossTexture* _tempTex = loadImage((char*)path);
	if (_tempTex==NULL){
		showErrorIfNull(_tempTex);
		easyMessagef(1,"Failed to load image %s, what will happen now?!",path);
	}
	return _tempTex;
}
crossTexture* LoadEmbeddedPNG(const char* path){
	char* _fixedPath = fixPathAlloc(path,TYPE_EMBEDDED);
	crossTexture* _tempTex = loadImage(_fixedPath);
	if (_tempTex==NULL){
		showErrorIfNull(_tempTex);
		easyMessagef(1,"Failed to load image %s.\n%s",path,GBPLAT != GB_3DS ? "This is supposed to be embedded..." : "Check you set up everything correctly.");
	}
	free(_fixedPath);
	return _tempTex;
}
void drawDropshadowTextSpecific(int _x, int _y, const char* _message, int _r, int _g, int _b, int _dropshadowR, int _dropshadowG, int _dropshadowB, int _a){
	#if GBPLAT == GB_VITA
		struct goodbrewfont* _realFont = (struct goodbrewfont*)normalFont;
		vita2d_font_draw_text_dropshadow(_realFont->data,_x,_y+textHeight(normalFont),RGBA8(_r,_g,_b,_a),_realFont->size,_message,dropshadowOffX,dropshadowOffY,RGBA8(_dropshadowR,_dropshadowG,_dropshadowB,_a));
	#else
		gbDrawTextAlpha(normalFont,_x+dropshadowOffX,_y+dropshadowOffY,_message,_dropshadowR,_dropshadowG,_dropshadowB,_a);
		gbDrawTextAlpha(normalFont,_x,_y,_message,_r,_g,_b,_a);
	#endif
}
void drawDropshadowText(int _x, int _y, char* _message, int _a){
	drawDropshadowTextSpecific(_x,_y,_message,DEFAULTFONTCOLOR,0,0,0,_a);
}
// Draw text intended to be used for the game, respects dropshadow setting
void drawTextGame(int _x, int _y, char* _message, unsigned char r, unsigned char g, unsigned char b, unsigned char _alpha){
	if (dropshadowOn){
		drawDropshadowTextSpecific(_x,_y,_message,r,g,b,dropshadowR,dropshadowG,dropshadowB,_alpha);
	}else{
		gbDrawTextAlpha(normalFont,_x,_y,_message,r,g,b,_alpha);
	}
}
void drawPropertyStreakText(int _x, int _y, char* _message, int32_t _prop, unsigned char _alpha){
	unsigned char _propBitmap = (unsigned char)(((char*)&_prop)[3]);
	unsigned char r;
	unsigned char g;
	unsigned char b;
	if (_propBitmap & TEXTPROP_COLORED){
		r = (unsigned char)(((char*)&_prop)[0]);
		g = (unsigned char)(((char*)&_prop)[1]);
		b = (unsigned char)(((char*)&_prop)[2]);
	}else{
		r=DEFAULTFONTCOLORR;
		g=DEFAULTFONTCOLORG;
		b=DEFAULTFONTCOLORB;
	}
	drawTextGame(_x,_y,_message,r,g,b,_alpha);	
}
// _message must be writeable
void drawPropertyGameText(int _x, int _y, char* _message, int32_t* _props, unsigned char _alpha){
	int _cachedStrlen = strlen(_message);
	if (_cachedStrlen==0){
		return;
	}
	int32_t _curProps = _props[0];
	int _lastStrEnd=0;
	int i;
	for (i=0;i<_cachedStrlen;++i){
		if (_props[i]!=_curProps){
			char _cachedChar=_message[i];
			_message[i]='\0';
			drawPropertyStreakText(_x,_y,&_message[_lastStrEnd],_curProps,_alpha);
			_x+=textWidth(normalFont,&_message[_lastStrEnd]);
			_lastStrEnd+=strlen(&_message[_lastStrEnd]);
			_message[i]=_cachedChar;
			_curProps=_props[i];
		}
	}
	drawPropertyStreakText(_x,_y,&_message[_lastStrEnd],_curProps,_alpha);
}
double GetXOffsetScale(){
	if (overrideXOffScale!=0){
		return overrideXOffScale;
	}
	return applyGraphicsScale(actualBackgroundWidth)/(double)scriptScreenWidth;
}
double GetYOffsetScale(){
	if (overrideYOffScale){
		return overrideYOffScale;
	}
	return applyGraphicsScale(actualBackgroundHeight)/(double)scriptScreenHeight;
}
void centerSize(int _w, int _h, int* _retX, int* _retY){
	*_retX=easyCenter(_w,screenWidth);
	*_retY=easyCenter(_h,screenHeight);
}
// convert it to a position that refers to the left edge of the sprite from the background
// if the x or y is anything but 0, the left of the image is aligned at middle of the screen, +x
// and the top of the image is aligned at middle of the screen, +y
// for higurashi only
void fixScriptSpritePos(int* x, int* y){
	*x+=scriptScreenWidth/2;
	*y+=scriptScreenHeight/2;
}
void getBackgroundOff(int* _retX, int* _retY){
	centerSize(actualBackgroundWidth*graphicsScale,actualBackgroundHeight*graphicsScale,_retX,_retY);
}
void getBustOff(int* _retX, int* _retY){
	if (overrideOffsetVals){
		*_retX=overrideBustOffX;
		*_retY=overrideBustOffY;
	}else{
		getBackgroundOff(_retX,_retY);
	}
}
// higurashi positions are relative to the image centered in the middle of the screen
// we fix that before storing the position
void convertPosition(char _inDoesReferToMiddle, int* x, int* y, crossTexture* _texture){
	// get the absolute starting position of the bust in screen units once it's upscaled.
	int _startX;
	int _startY;
	centerSize(getTextureWidth(_texture)*graphicsScale,getTextureHeight(_texture)*graphicsScale,&_startX,&_startY);
	// get how far that is from the initial bust draw position
	int _usualStartX;
	int _usualStartY;
	getBustOff(&_usualStartX,&_usualStartY);

	int _freeX = (_startX-_usualStartX)/GetXOffsetScale();
	int _freeY = (_startY-_usualStartY)/GetYOffsetScale();
	
	if (!_inDoesReferToMiddle){ // convert from coords we can draw at to coords that refer to the middle
		_freeX*=-1;
		_freeY*=-1;
	}
	*x=*x+_freeX;
	*y=*y+_freeY;
}
// originX and originY sets a custom point to be the center of the screen. your position is relative to that.
void adjustForOriginPos(int* x, int* y, int _originX, int _originY){
	*x=*x-_originX;
	*y=*y-_originY;
}
void gameObjectClipOn(){
	int _startX;
	int _startY;
	getBackgroundOff(&_startX,&_startY);
	enableClipping(_startX,_startY,actualBackgroundWidth*graphicsScale,actualBackgroundHeight*graphicsScale);
}
void gameObjectClipOff(){
	disableClipping();
}
void endWorkingEnlarge(){
	if (workingEnlarge){
		extraGameScaleX=workingEnlarge->destScaleX;
		extraGameScaleY=workingEnlarge->destScaleY;
		extraGameOffX=workingEnlarge->destOffX;
		extraGameOffY=workingEnlarge->destOffY;
		free(workingEnlarge);
		workingEnlarge=NULL;
	}
}
char enlargeActive(){
	return workingEnlarge || extraGameScaleX!=1 || extraGameScaleY!=1 || extraGameOffX!=0 || extraGameOffY!=0;
}
void turnOffEnlarge(){
	extraGameScaleX=1;
	extraGameScaleY=1;
	extraGameOffX=0;
	extraGameOffY=0;
}
void changeMaxLines(int _newMax){
	if (_newMax==maxLines || _newMax<=0){
		return;
	}
	char** _newCurrentMessages = malloc(sizeof(char*)*_newMax);
	int32_t** _newMessageProps = malloc(sizeof(int32_t*)*_newMax);
	if (_newMax<maxLines){
		int _diff = maxLines-_newMax;
		// Copy the latest lines that can fit
		memcpy(_newCurrentMessages,&currentMessages[_diff],_newMax*sizeof(char*));
		memcpy(_newMessageProps,&messageProps[_diff],_newMax*sizeof(int32_t*));
		// free the lines that are gone
		int i;
		for (i=0;i<_diff;++i){
			if (currentMessages[i]){
				free(currentMessages[i]);
				free(messageProps[i]);
			}
		}
	}else{
		if (maxLines!=0 && currentMessages){
			// Copy all the old lines
			memcpy(_newCurrentMessages,currentMessages,sizeof(char*)*maxLines);
			memcpy(_newMessageProps,messageProps,sizeof(int32_t*)*maxLines);
		}
		// intialize new ones
		int i;
		for (i=maxLines;i<_newMax;++i){
			_newCurrentMessages[i]=NULL;
			_newMessageProps[i]=NULL;
		}
	}
	free(currentMessages);
	free(messageProps);
	maxLines=_newMax;
	currentMessages=_newCurrentMessages;
	messageProps=_newMessageProps;
}
int getADVNameYSpace(){
	if (shouldShowADVNames()){
		if (advNameImHeight!=-1){
			return advNameImHeight+IMADVNAMEPOSTPAD;
		}else{
			return ADVNAMEOFFSET;
		}
	}
	return 0;
}
void recalculateMaxLines(){
	int _usableHeight = outputLineScreenHeight-totalTextYOff()-textboxBottomPad;
	changeMaxLines(_usableHeight/currentTextHeight);
}
// Number of lines to draw is not zero based
// _finalLineMaxChar is the last char on the last line to draw. Must be a position inside the string, 
void DrawMessageText(unsigned char _alpha, int _maxDrawLine, int _finalLineMaxChar){
	if (_maxDrawLine==-1){
		_maxDrawLine=maxLines-1;
	}
	char _oldFinalChar;
	if (_finalLineMaxChar!=-1){
		if (_finalLineMaxChar<=strlen(currentMessages[_maxDrawLine])){ // Bounds check
			// Temporarily trim the string
			_oldFinalChar = currentMessages[_maxDrawLine][_finalLineMaxChar];
			currentMessages[_maxDrawLine][_finalLineMaxChar]='\0';
		}else{
			_finalLineMaxChar=-1;
		}
	}
	int i;
	/*
	#if GBPLAT == GB_3DS
		if (textIsBottomScreen==1){
			startDrawingBottom();
			if (strlen(currentMessages[i])==0){
				drawText(0,0,"."); // Hotfix to fix crash when no text on bottom screen.
			}
			for (i=0;i<_maxDrawLine;i++){
				drawText(0,STUPIDTEXTYOFF+i*currentTextHeight,(char*)currentMessages[i]);
			}
			drawImageChars(_alpha,INT_MAX,0);
			return;
		}
	#endif
	*/
	int _totalTextXOff=totalTextXOff();
	int _totalTextYOff=totalTextYOff();
	offsetForShake(curUIShake,&_totalTextXOff,&_totalTextYOff);
	if (shouldShowADVNames()){
		if (currentADVName!=NULL){
			drawDropshadowTextSpecific(_totalTextXOff,_totalTextYOff-ADVNAMEOFFSET,currentADVName,advNameR,advNameG,advNameB,0,0,0,255);
		}else if (currentADVNameIm!=-1){
			int* _nameImageInfo = advImageNamePos+currentADVNameIm*4;
			drawTexturePartSized(advNameImSheet,_totalTextXOff,_totalTextYOff-advNameImHeight-IMADVNAMEPOSTPAD,getOtherScaled(_nameImageInfo[3],advNameImHeight,_nameImageInfo[2]),advNameImHeight,_nameImageInfo[0],_nameImageInfo[1],_nameImageInfo[2],_nameImageInfo[3]);
		}
	}
	for (i=0;i<=_maxDrawLine;i++){
		if (currentMessages[i]){
			drawPropertyGameText(_totalTextXOff,_totalTextYOff+i*currentTextHeight,currentMessages[i],messageProps[i],_alpha);
		}
	}
	// Fix string if we trimmed it for _finalLineMaxChar
	if (_finalLineMaxChar!=-1){
		currentMessages[_maxDrawLine][_finalLineMaxChar]=_oldFinalChar;
	}
}
void DrawMessageBox(char _textmodeToDraw, unsigned char _targetAlpha){
	#if GBPLAT == GB_3DS
		if (textIsBottomScreen==1){
			return;
		}
	#endif
	if (_textmodeToDraw == TEXTMODE_NVL || currentCustomTextbox==NULL){
		drawRectangle(textboxXOffset,0,textboxWidth,outputLineScreenHeight,0,0,0,_targetAlpha);
	}else{
		drawTextureSizedAlpha(currentCustomTextbox,textboxXOffset,textboxYOffset,textboxWidth,advboxHeight,_targetAlpha);
	}
}
void DrawCurrentFilter(){
	if (currentFilterType==FILTERTYPE_EFFECTCOLORMIX){
		drawRectangle(textboxXOffset,0,textboxWidth-textboxXOffset*2,outputLineScreenHeight,filterR,filterG,filterB,filterA);
	}	
}
u64 waitwithCodeTarget;
void WaitWithCodeStart(int amount){
	waitwithCodeTarget = getMilli()+amount;
}
void WaitWithCodeEnd(int amount){
	if (getMilli()<waitwithCodeTarget){
		wait(waitwithCodeTarget-getMilli());
	}
}
void _postFontReload(double _passedSize){
	currentTextHeight = textHeight(normalFont);
	singleSpaceWidth = textWidth(normalFont," ");
	menuCursorSpaceWidth = textWidth(normalFont,MENUCURSOR" ");
	if (singleSpaceWidth==0){
		singleSpaceWidth=1;
	}
	if (_passedSize>=33){
		dropshadowOffX=2;
		dropshadowOffY=2;
	}else{
		dropshadowOffX=1;
		dropshadowOffY=1;
	}
}
void reloadFont(double _passedSize, char _recalcMaxLines){
	#ifdef OVERRIDE_LOADFONT
	customReloadFont(_passedSize,_recalcMaxLines);
	return;
	#endif
	#if GBPLAT != GB_VITA
		if (normalFont!=NULL){
			freeFont(normalFont);
		}
		normalFont = loadFont(currentFontFilename,_passedSize);
	#else
		// Here I put custom code for loading fonts on the Vita. I need this for fonts with a lot of characters. Why? Well, if the font has a lot of characters, FreeType won't load all of them at once. It'll stream the characters from disk. At first that sounds good, but remember that the Vita breaks its file handles after sleep mode. So new text wouldn't work after sleep mode. I could fix this by modding libvita2d and making it use my custom IO commands, but I just don't feel like doing that right now.
		struct goodbrewfont* _realFont = normalFont;
		if (_realFont!=NULL){
			vita2d_free_font(_realFont->data);
			free(_realFont);
		}
		normalFont = malloc(sizeof(struct goodbrewfont));
		_realFont = normalFont;
		_realFont->type=GBTXT_VITA2D;
		_realFont->size=_passedSize;
		if (_loadedFontBuffer!=NULL){
			free(_loadedFontBuffer);
		}
		FILE* fp = fopen(currentFontFilename, "rb");
		// Get file size
		fseek(fp, 0, SEEK_END);
		long _foundFilesize = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		// Read file into memory
		_loadedFontBuffer = malloc(_foundFilesize);
		fread(_loadedFontBuffer, _foundFilesize, 1, fp);
		fclose(fp);
		_realFont->data = vita2d_load_font_mem(_loadedFontBuffer,_foundFilesize);
	#endif
	_postFontReload(_passedSize);
	if (_recalcMaxLines){
		recalculateMaxLines();
	}
}
void globalLoadFont(const char* _filename){
	changeMallocString(&currentFontFilename,_filename);
	reloadFont(fontSize,1);
}
char menuControlsLow(int* _choice, char _canWrapUpDown, int _upDownChange, char _canWrapLeftRight, int _leftRightChange, int _menuMin, int _menuMax){
	int _play = *_choice;
	if (_leftRightChange!=0){
		if (wasJustPressed(BUTTON_LEFT)){
			_play-=_leftRightChange;
		}else if (wasJustPressed(BUTTON_RIGHT)){
			_play+=_leftRightChange;
		}
		_play = _canWrapLeftRight ? wrapNum(_play,_menuMin,_menuMax) : limitNum(_play,_menuMin,_menuMax);
	}
	if (wasJustPressed(BUTTON_UP)){
		_play-=_upDownChange;
	}
	if (wasJustPressed(BUTTON_DOWN)){
		_play+=_upDownChange;
	}
	if (_play!=*_choice){
		*_choice = _canWrapUpDown ? wrapNum(_play,_menuMin,_menuMax) : limitNum(_play,_menuMin,_menuMax);
		return 1;
	}
	return 0;
}
int retMenuControlsLow(int _choice, char _canWrapUpDown, int _upDownChange, char _canWrapLeftRight, int _leftRightChange, int _menuMin, int _menuMax){
	int _fakeRet=_choice;
	menuControlsLow(&_fakeRet,_canWrapUpDown,_upDownChange,_canWrapLeftRight,_leftRightChange,_menuMin,_menuMax);
	return _fakeRet;
}
int menuControls(int _choice,int _menuMin,int _menuMax){
	menuControlsLow(&_choice,1,1,0,0,_menuMin,_menuMax);
	return _choice;
}
// returns 1 on error. shows messages.
char SafeLuaDoFile(lua_State* passedState, char* passedPath){
	if (!checkFileExist(passedPath)){
		easyMessagef(1,"The LUA file %s does not exist!",passedPath);
		return 1;
	}
	return lazyLuaError(luaL_dofile(passedState,passedPath));
}
void WriteToDebugFile(const char* _formatString, ...){
	va_list _tempArgs;
	va_start(_tempArgs, _formatString);
	char* _completeString = formatf(_tempArgs,_formatString);
	#if GBPLAT == GB_LINUX
		printf("%s",_completeString);
	#else
		char* _tempDebugFileLocationBuffer = malloc(strlen(gbDataFolder)+strlen("log.txt")+1);
		strcpy(_tempDebugFileLocationBuffer,gbDataFolder);
		strcat(_tempDebugFileLocationBuffer,"log.txt");
		FILE *fp;
		fp = fopen(_tempDebugFileLocationBuffer, "a");
		if (!fp){
			easyMessagef(1,"Failed to open debug file, %s",_tempDebugFileLocationBuffer);
		}else{
			fwrite(_completeString,1,strlen(_completeString),fp);
			fclose(fp);
		}
		free(_tempDebugFileLocationBuffer);
	#endif
	free(_completeString);
}
#define LAZYCHOICEFYESNOHEIGHTRATIO (1/(double)10)
// Returns one if they chose yes
// Returns zero if they chose no
int LazyChoicef(const char* _formatString, ...){
	char** _wrappedLines;
	int _numLines;
	//
	va_list _tempArgs;
	va_start(_tempArgs, _formatString);
	char* _completeString = formatf(_tempArgs,_formatString);
	wrapText(_completeString,&_numLines,&_wrappedLines,screenWidth);
	free(_completeString);
	//
	int _choice=0;
	int _buttonH = screenHeight*LAZYCHOICEFYESNOHEIGHTRATIO;
	int _yesX=easyCenter(textWidth(normalFont,"Yes"),screenWidth/2);
	int _noX=screenWidth/2+easyCenter(textWidth(normalFont,"No"),screenWidth/2);
	int _labelY = screenHeight-_buttonH+easyCenter(currentTextHeight,_buttonH);
	int _buttonY=screenHeight-_buttonH;
	unsigned char _tR, _tG, _tB;
	getInverseBGCol(&_tR,&_tG,&_tB);
	controlsReset();
	while (currentGameStatus!=GAMESTATUS_QUIT){
		controlsStart();
		if (wasJustPressed(BUTTON_A)){
			break;
		}
		if (wasJustPressed(BUTTON_TOUCH)){
			int _ty = fixTouchY(touchY);
			int _tx = fixTouchX(touchX);
			if (pointInBox(_tx,_ty,0,_buttonY,screenWidth,_buttonH)){
				_choice = (_tx>=screenWidth/2);
				break;
			}
		}
		if (wasJustPressed(BUTTON_BACK)){ // auto "no" if press back
			_choice=1;
			break;
		}
		_choice = retMenuControlsLow(_choice,0,0,1,1,0,1);
		controlsEnd();
		startDrawing();
		drawWrappedText(0,0,_wrappedLines,_numLines,_tR,_tG,_tB,255);
		if (gbHasButtons() & GBYES){
			gbDrawText(normalFont,(_choice==0 ? _yesX : _noX)-MENUOPTIONOFFSET,_labelY,MENUCURSOR,_tR,_tG,_tB);
		}
		gbDrawText(normalFont,_yesX,_labelY,"Yes",_tR,_tG,_tB);
		gbDrawText(normalFont,_noX,_labelY,"No",_tR,_tG,_tB);
		drawHallowRect(0,_buttonY,screenWidth/2,_buttonH,5,_tR,_tG,_tB,255);
		drawHallowRect(screenWidth/2,_buttonY,screenWidth/2,_buttonH,5,_tR,_tG,_tB,255);
		endDrawing();
	}
	controlsEnd();
	freeWrappedText(_numLines,_wrappedLines);
	return _choice==0;
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
// must be malloc'd memory
void addToMessageHistoryOwned(char* _newWords){
	if (_newWords==NULL){
		return;
	}
	free(messageHistory[oldestMessage]);
	messageHistory[oldestMessage] = _newWords;
	oldestMessage++;
	if (oldestMessage==MAXMESSAGEHISTORY){
		oldestMessage=0;
	}
}
void clearHistory(){
	int i;
	for (i=0;i<MAXMESSAGEHISTORY;i++){
		if (messageHistory[i]){
			messageHistory[i][0]='\0';
		}
	}
}
void ClearMessageArray(char _doFadeTransition){
	if (textSpeed==TEXTSPEED_INSTANT){
		_doFadeTransition=0;
	}
	// check if there are lines to clear
	int i;
	for (i=0;i<maxLines;i++){
		if (currentMessages[i]!=NULL){
			break;
		}
	}
	// fadeout transition if we are going to remove text
	if (i!=maxLines && MessageBoxEnabled && !isSkipping && _doFadeTransition){
		u64 _startTime=getMilli();
		u64 _currentTime;
		while ((_currentTime=getMilli())<_startTime+CLEARMESSAGEFADEOUTTIME){
			startDrawing();
			drawAdvanced(1,1,1,MessageBoxEnabled,1,0);
			DrawMessageText(255-partMoveFillsCapped(_currentTime,_startTime,CLEARMESSAGEFADEOUTTIME,255),-1,-1);
			endDrawing();
		}
	}
	currentLine=0;
	// add to history. the history will free this memory later
	for (i=0;i<maxLines;i++){
		if (currentMessages[i]!=NULL){
			addToMessageHistoryOwned(currentMessages[i]);
			currentMessages[i]=NULL;
			free(messageProps[i]);
		}
	}
	//
	if (advNamesPersist!=2){
		setADVName(NULL);
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
void WriteIntToDebugFile(int a){
	char _tempCompleteNumberBuffer[11];
	sprintf(_tempCompleteNumberBuffer,"%d",a);
	WriteToDebugFile(_tempCompleteNumberBuffer);
}
// Does not clear the debug file at ux0:data/HIGURASHI/log.txt  , I promise.
void ClearDebugFile(){
	char* _tempDebugFileLocationBuffer = malloc(strlen(gbDataFolder)+strlen("log.txt")+1);
	strcpy(_tempDebugFileLocationBuffer,gbDataFolder);
	strcat(_tempDebugFileLocationBuffer,"log.txt");
	FILE *fp;
	fp = fopen(_tempDebugFileLocationBuffer, "w");
	if (!fp){
		easyMessagef(1,"Failed to open debug file, %s",_tempDebugFileLocationBuffer);
		return;
	}
	fclose(fp);
	free(_tempDebugFileLocationBuffer);
}
void _freeBustImage(bust* passedBust){
	if (passedBust->image!=NULL){
		freeTexture(passedBust->image);
	}
	free(passedBust->relativeFilename);
	if (passedBust->transformTexture!=NULL){
		freeTexture(passedBust->transformTexture);
	}
	free(passedBust->curShake);
}
void ResetBustStruct(bust* passedBust, int canfree){
	if (canfree==1){
		_freeBustImage(passedBust);
	}
	passedBust->image=NULL;
	passedBust->transformTexture=NULL;
	passedBust->xOffset=0;
	passedBust->yOffset=0;
	passedBust->originXForAdjust=0;
	passedBust->originYForAdjust=0;
	passedBust->scaleX=1;
	passedBust->scaleY=1;
	passedBust->isActive=0;
	passedBust->curAlpha=255;
	passedBust->destAlpha=255;
	passedBust->bustStatus = BUST_STATUS_NORMAL;
	passedBust->lineCreatedOn=0;
	passedBust->relativeFilename=NULL;
	passedBust->curShake=NULL;
}
void DisposeOldScript(){
	// Frees the script main
	lua_getglobal(L,"FreeTrash");
	lua_call(L, 0, 0);
}
char StringStartWith(const char *a, const char *b){
	return (strncmp(a, b, strlen(b)) == 0);
}
// Give it a full script file path and it will return 1 if the file was converted beforehand
int DidActuallyConvert(char* filepath){
	if (checkFileExist(filepath)==0){
		easyMessagef(1,"I was going to check if you converted this file, but I can't find it! %s",filepath);
		return 1;
	}
	FILE* file = fopen(filepath, "r");
	char line[256];

	int _isConverted=0;

	startDrawing();
	drawText(32,50,"Checking if you actually converted the script...");
	drawText(32,200,filepath);
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
void DisplaypcallError(int val, const char* fourthMessage){
	char* _specificError;
	switch (val){
		case LUA_ERRRUN:
			_specificError="LUA_ERRRUN, a runtime error";
		break;
		case LUA_ERRMEM:
			_specificError="LUA_ERRMEM, an out of memory error";
		break;
		case LUA_ERRERR:
			_specificError="ERRERR, a message handler error";
		break;
		case LUA_ERRGCMM:
			_specificError="LUA_ERRGCMM, an __gc metamethod error";
		break;
		default:
			_specificError="UNKNOWN ERROR, something that shouldn't happen";
		break;
	}
	easyMessagef(1,"lua_pcall failed with error %s, please report the bug on the thread.\n%s",_specificError,fourthMessage);
	easyMessagef(1,"%s",lua_tostring(L,-1));
}
void refreshGameState(){
	nextTextR=DEFAULTFONTCOLORR;
	nextTextG=DEFAULTFONTCOLORG;
	nextTextB=DEFAULTFONTCOLORB;
}
// Returns 1 if it worked
char RunScript(const char* _scriptfolderlocation,char* filename, char addTxt){
	// Hopefully, nobody tries to call a script from a script and wants to keep the current message display.
	ClearMessageArray(0);
	currentScriptLine=0;
	refreshGameState();
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
					easyMessagef(1,"luaL_loadfile failed with error LUA_ERRSYNTAX You seem to have converted the files correctly... Please report the bug on the thread.");
				}else{
					easyMessagef(1,"luaL_loadfile failed with error LUA_ERRSYNTAX You probably forgot to convert the files with the converter. Please make sure you did.");
				}
			break;
			case LUA_ERRMEM:
				easyMessagef(1,"luaL_loadfile failed with error LUA_ERRMEM This is an out of memory error. Please report the bug on the thread.");
			break;
			case LUA_ERRGCMM:
				easyMessagef(1,"luaL_loadfile failed with error LUA_ERRGCMM This is a __gc metamethod error. Please report the bug on the thread.");
			break;
			case LUA_ERRFILE:
				easyMessagef(1,"luaL_loadfile failed with error LUA_ERRFILE, this means the file failed to load. Make sure the file exists. %s",tempstringconcat);
			break;
			default:
				easyMessagef(1,"luaL_loadfile failed with error UNKNOWN ERROR! This is weird and should NEVER HAPPEN! Please report the bug on the thread.");
			break;
		}
		easyMessagef(1,lua_tostring(L,-1));
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
	// Adds function to stack
	lua_getglobal(L,"main");
	// Call funciton. Removes function from stack.
	_pcallResult = lua_pcall(L, 0, 0, 0);
	if (_pcallResult!=LUA_OK){
		const char* _message = lua_tostring(L,-1);
		int _len = strlen(_message);
		int _checkLen = strlen(FAKELUAERRORMSG);
		if (!(_len>=_checkLen && strcmp(_message+(_len-_checkLen),FAKELUAERRORMSG)==0)){
			DisplaypcallError(_pcallResult,"This is the second lua_pcall in RunScript.");
		}
		lua_pop(L,1);
	}
	return 1;
}
// _desiredLines is some 0 based index for the line you want to draw on
// returns 0 based line index for where you can use
void upshiftText(int _numDelLines){
	int i;
	int _numLeftLines = maxLines-_numDelLines;
	// delete old lines to make room
	for(i=0;i<_numDelLines;++i){
		addToMessageHistoryOwned(currentMessages[i]);
		free(messageProps[i]);
	}
	memmove(currentMessages,&currentMessages[_numDelLines],sizeof(char*)*_numLeftLines);
	memmove(messageProps,&messageProps[_numDelLines],sizeof(int32_t*)*_numLeftLines);
	// initialize the freed spaces
	for (i=_numLeftLines;i<maxLines;++i){
		currentMessages[i]=NULL;		
	}
}
void updateBust(bust* _target, u64 _curTime){
	freeDoneShake(&_target->curShake,_curTime);
	switch(_target->bustStatus){
		case BUST_STATUS_FADEIN:
		case BUST_STATUS_TRANSFORM_FADEIN:
			if (_curTime>=_target->fadeStartTime){
				if (_curTime>=_target->fadeEndTime){
					if (_target->bustStatus == BUST_STATUS_TRANSFORM_FADEIN){
						freeTexture(_target->transformTexture);
						_target->transformTexture=NULL;
					}
					_target->curAlpha=_target->destAlpha;
					_target->bustStatus = BUST_STATUS_NORMAL;
				}else{
					_target->curAlpha=partMoveFills(_curTime,_target->fadeStartTime,_target->fadeEndTime-_target->fadeStartTime,_target->destAlpha);
				}
			}
			break;
		case BUST_STATUS_FADEOUT:
			if (_curTime>_target->fadeEndTime){
				ResetBustStruct(_target,1);
				RecalculateBustOrder();
			}else{
				_target->curAlpha=partMoveEmptys(_curTime,_target->fadeStartTime,_target->fadeEndTime-_target->fadeStartTime,_target->destAlpha);
			}
			break;
		case BUST_STATUS_SPRITE_MOVE:
			if (_curTime>_target->startMoveTime+_target->diffMoveTime){
				_target->bustStatus = BUST_STATUS_NORMAL;
				_target->xOffset=_target->startXMove+_target->diffXMove;
				_target->yOffset=_target->startYMove+_target->diffYMove;
			}else{
				_target->xOffset=_target->startXMove+partMoveFills(_curTime,_target->startMoveTime,_target->diffMoveTime,_target->diffXMove);
				_target->yOffset=_target->startYMove+partMoveFills(_curTime,_target->startMoveTime,_target->diffMoveTime,_target->diffYMove);
			}
			break;
	}
}
// Update what bustshots are doing depending on their bustStatus
void Update(){
	u64 _curTime=getMilli();	
	int i;
	for (i=0;i<maxBusts;i++){
		updateBust(&(Busts[i]),_curTime);
	}
	if (workingEnlarge){
		u64 _endTime = workingEnlarge->startTime+workingEnlarge->totalTime;
		if (_curTime>=_endTime){
			endWorkingEnlarge();
		}else{
			extraGameScaleX=partMoveValToVal(_curTime,_endTime,workingEnlarge->totalTime,workingEnlarge->startScaleX,workingEnlarge->destScaleX);
			extraGameScaleY=partMoveValToVal(_curTime,_endTime,workingEnlarge->totalTime,workingEnlarge->startScaleY,workingEnlarge->destScaleY);
			extraGameOffX=partMoveValToVal(_curTime,_endTime,workingEnlarge->totalTime,workingEnlarge->startOffX,workingEnlarge->destOffX);
			extraGameOffY=partMoveValToVal(_curTime,_endTime,workingEnlarge->totalTime,workingEnlarge->startOffY,workingEnlarge->destOffY);
		}
	}
	freeDoneShake(&curBackgroundShake,_curTime);
	freeDoneShake(&curUIShake,_curTime);
}
// prepare a bust to be settled upon the next update
void settleBust(bust* _target){
	_target->fadeStartTime=0;
	_target->fadeEndTime=0;
	_target->startMoveTime=0;
	_target->diffMoveTime=0;
	updateBust(_target,1);
}
// the history array wraps. This fixes the array index.
int FixHistoryOldSub(int _val, int _scroll){
	if (_val+_scroll>=MAXMESSAGEHISTORY){
		return (_val+_scroll)-MAXMESSAGEHISTORY;
	}else{
		return _val+_scroll;
	}
}
void incrementScriptLineVariable(lua_State* L, lua_Debug* ar){
	currentScriptLine++;
}
#define OUTPUTLINETOPBARH (1/(double)5)
#define OUTPUTLINETOPBARNUM 4
// used from inside a control loop. returns the index of the currently held button in the top UI bar for touch controls
signed char getTouchedBarOption(){
	if (isDown(BUTTON_TOUCH)){
		int _tx = fixTouchX(touchX);
		int _ty = fixTouchY(touchY);
		if (pointInBox(_tx,_ty,0,0,screenWidth,screenHeight*OUTPUTLINETOPBARH)){
			return _tx/(screenWidth/(double)OUTPUTLINETOPBARNUM);
		}
	}
	return -1;
}
void openInGameSettingsMenu(){
	SettingsMenu(1,currentlyVNDSGame,currentlyVNDSGame,!isActuallyUsingUma0 && GBPLAT != GB_VITA,!currentlyVNDSGame,0,currentlyVNDSGame,currentlyVNDSGame,(strcmp(VERSIONSTRING,DEFAULTVERSION)==0));
}
void updateControlsGeneral(){
	if (wasJustPressed(BUTTON_Y)){
		isSkipping=1;
		endType=Line_ContinueAfterTyping;
	}
	if (isSkipping){		
		if (!((isTouchSkipHold && getTouchedBarOption()==3) || isDown(BUTTON_Y))){
			isTouchSkipHold=0;
			isSkipping=0;
		}
	}
	if (wasJustPressed(BUTTON_X)){
		openInGameSettingsMenu();
	}
	if (wasJustPressed(BUTTON_SELECT) || (wasJustPressed(BUTTON_BACK) && autoModeOn)){
		PlayMenuSound();
		autoModeOn = !autoModeOn;
	}
}
char curVoicePlaying(){
	if (lastVoiceSlot!=-1){
		#if GBSND == GBSND_VITA
			return (soundEffects[lastVoiceSlot]!=NULL && mlgsndIsPlaying(soundEffects[lastVoiceSlot]));
		#elif GBSND == GBSND_SDL
			return Mix_Playing(lastVoiceHandle);
		#endif
	}
	return 0;
}
void drawTouchBar(){
	int _barH=screenHeight*OUTPUTLINETOPBARH;
	drawRectangle(0,0,screenWidth,_barH,0,0,0,255);
	int _buttonW=screenWidth/OUTPUTLINETOPBARNUM;
	int _buttonY=easyCenter(currentTextHeight,_barH);
	char* _autoState = (autoModeOn ? "Auto off" : "Auto on");
	drawText(easyCenter(textWidth(normalFont,"Settings"),_buttonW),_buttonY,"Settings");
	drawText(_buttonW+easyCenter(textWidth(normalFont,_autoState),_buttonW),_buttonY,_autoState);
	drawText(_buttonW*2+easyCenter(textWidth(normalFont,"Save"),_buttonW),_buttonY,"Save");
	drawText(_buttonW*3+easyCenter(textWidth(normalFont,"Skip (hold)"),_buttonW),_buttonY,"Skip (hold)");
}
void outputLineWait(){
	// 1 - show some touch buttons
	// 2 - textbox hide
	char _backPressLevel=0;
	char _clearBeforeWeGo=(endType==Line_Normal);
	//
	if (isSkipping==1){
		controlsStart();
		updateControlsGeneral();
		controlsEnd();
		if (isSkipping){
			if (isTouchSkipHold){
				_backPressLevel=1;
			}
			endType=LINE_RESERVED;
		}
	}
	u64 _toggledTextboxTime=0;
	// 0 if we need to wait for sound to end.
	u64 _inBetweenLinesMilisecondsStart;
	int _chosenAutoWait;
	// Initial check for auto mode
	if (curVoicePlaying()){
		_inBetweenLinesMilisecondsStart=0;
		_chosenAutoWait = autoModeVoicedWait;
	}else{
		_inBetweenLinesMilisecondsStart = getMilli();
		_chosenAutoWait = autoModeWait;
		if (_inBetweenLinesMilisecondsStart==0){
			_inBetweenLinesMilisecondsStart=1;
		}
	}
	// On PS Vita, prevent sleep mode if using auto mode
	#if GBPLAT == GB_VITA
	if (autoModeOn){
		sceKernelPowerTick(0);
	}
	#endif
	do{
		controlsStart();
		Update();
		startDrawing();
		Draw(MessageBoxEnabled);
		// Easy save menu
		if (currentlyVNDSGame && isDown(BUTTON_R)){
			drawRectangle(0,0,screenWidth,currentTextHeight*5,0,0,0,255);
			drawText(6,currentTextHeight*0+6,"UP: Save slot 1");
			drawText(6,currentTextHeight*1+6,"DOWN: Save slot 2");
			drawText(6,currentTextHeight*2+6,"LEFT: Save slot 3");
			drawText(6,currentTextHeight*3+6,"RIGHT: Save slot 4");
			drawHallowRect(0,0,screenWidth,currentTextHeight*5,5,255,255,255,255);
			if (wasJustPressed(BUTTON_UP) || wasJustPressed(BUTTON_DOWN) || wasJustPressed(BUTTON_LEFT) || wasJustPressed(BUTTON_RIGHT)){
				unsigned char _selectedSlot;
				if (wasJustPressed(BUTTON_UP)){
					_selectedSlot=1;
				}else if (wasJustPressed(BUTTON_DOWN)){
					_selectedSlot=2;
				}else if (wasJustPressed(BUTTON_LEFT)){
					_selectedSlot=3;
				}else if (wasJustPressed(BUTTON_RIGHT)){
					_selectedSlot=4;
				}
				char* _foundPath = easyVNDSSaveName(_selectedSlot);
				endDrawing();
				char _ret = !vndsNormalSave(_foundPath,1,1);
				free(_foundPath);
				controlsReset();
				startDrawing();
				if (_ret){
					PlayMenuSound();
					drawRectangle(0,0,screenWidth,screenHeight,0,255,0,255);
				}
			}
		}
		
		// draw the touch options
		if (_backPressLevel==1){
			#ifdef CUSTOM_TOUCH_BAR_DRAW
			customDrawTouchBar();
			#else
			drawTouchBar();
			#endif
		}
		endDrawing();
		if (proceedPressed()){
			switch(_backPressLevel){
				case 0:
					endType=LINE_RESERVED;
					break;
				case 1:
					if (wasJustPressed(BUTTON_TOUCH)){
						signed char _selectedChoice = getTouchedBarOption();
						switch(_selectedChoice){
							case 0:
								openInGameSettingsMenu();
								break;
							case 1:
								autoModeOn = !autoModeOn;
								break;
							case 2:
								safeVNDSSaveMenu();
								break;
							case 3:
								isTouchSkipHold=1;
								isSkipping=1;
								endType=Line_ContinueAfterTyping;
								break;
						}
					}
					break;
				case 2:
					showTextbox();
					break;
			}
			_backPressLevel=0;
		}
		if (wasJustPressed(BUTTON_B)){
			if (_backPressLevel==0){
				if (MessageBoxEnabled){
					MessageBoxEnabled=0;
					_backPressLevel=2;
					_toggledTextboxTime=getMilli();
				}
			}else if (_backPressLevel==2){
				if (_toggledTextboxTime!=0){
					_inBetweenLinesMilisecondsStart+=getMilli()-_toggledTextboxTime;
					_toggledTextboxTime=0;
				}
				MessageBoxEnabled = !MessageBoxEnabled;
			}
		}
		if (wasJustPressed(BUTTON_BACK)){
			if (autoModeOn){
				autoModeOn=0;
			}else{
				if (_backPressLevel==0){
					_backPressLevel=1;
				}else if (_backPressLevel==1){
					_backPressLevel=2;
					hideTextbox();
				}else if (_backPressLevel==2){
					showTextbox();
					_backPressLevel=0;
				}
			}
		}
		updateControlsGeneral();
		if (wasJustPressed(BUTTON_START)){
			if (currentlyVNDSGame){
				safeVNDSSaveMenu();
			}else{
				historyMenu();
			}
		}
		if (wasJustPressed(BUTTON_UP)){
			historyMenu();
		}
		controlsEnd();
		if (autoModeOn==1 && _toggledTextboxTime==0){
			if (_inBetweenLinesMilisecondsStart!=0){ // If we're not waiting for audio to end
				if (getMilli()>=(_inBetweenLinesMilisecondsStart+_chosenAutoWait)){
					endType = LINE_RESERVED;
				}
			}else{
				// Check if audio has ended yet. if so, start the waiting now
				if (!curVoicePlaying()){
					_inBetweenLinesMilisecondsStart = getMilli();
				}
			}
		}
	}while(endType==Line_Normal || endType == Line_WaitForInput);
	// If we pressed a button to continue the text and we're doing VNDS ADV mode
	if (_clearBeforeWeGo || (currentlyVNDSGame && gameTextDisplayMode==TEXTMODE_ADV && endType==LINE_RESERVED)){
		ClearMessageArray(1);
	}
	endType=Line_ContinueAfterTyping;
	lastVoiceSlot=-1;
}
void drawHallowRect(int _x, int _y, int _w, int _h, int _thick, int _r, int _g, int _b, int _a){
	drawRectangle(_x,_y,_thick,_h,_r,_g,_b,_a);
	drawRectangle(_x+_w-_thick,_y,_thick,_h,_r,_g,_b,_a);
	drawRectangle(_x,_y,_w,_thick,_r,_g,_b,_a);
	drawRectangle(_x,_y+_h-_thick,_w,_thick,_r,_g,_b,_a);
}
void DrawBackgroundAlpha(crossTexture* passedBackground, unsigned char passedAlpha){
	if (passedBackground!=NULL){
		int _xoff;
		int _yoff;
		getBackgroundOff(&_xoff,&_yoff);
		_xoff+=extraGameOffX;
		_yoff+=extraGameOffY;
		offsetForShake(curBackgroundShake,&_xoff,&_yoff);
		drawTextureSizedAlpha(passedBackground,_xoff,_yoff,getTextureWidth(passedBackground)*graphicsScale*extraGameScaleX,getTextureHeight(passedBackground)*graphicsScale*extraGameScaleY,passedAlpha);
	}
}
void DrawBackground(crossTexture* passedBackground){
	DrawBackgroundAlpha(passedBackground,255);
}
void DrawBust(bust* passedBust){
	int _startXOffset=0;
	int _startYOffset=0;
	getBustOff(&_startXOffset,&_startYOffset);
	offsetForShake(passedBust->curShake,&_startXOffset,&_startYOffset);
	// If the busts end one pixel off again, it may be because these are now int instead of float.
	float _drawBustX = ceil(_startXOffset+passedBust->xOffset*passedBust->cacheXOffsetScale*extraGameScaleX)+extraGameOffX;
	float _drawBustY = ceil(_startYOffset+passedBust->yOffset*passedBust->cacheYOffsetScale*extraGameScaleY)+extraGameOffY;
	double _scaleX=graphicsScale*extraGameScaleX*passedBust->scaleX;
	double _scaleY=graphicsScale*extraGameScaleY*passedBust->scaleY;
	//printf("%d\n",passedBust->curAlpha);
	if (passedBust->curAlpha==255){
		//printf("%f;%f;%f;%f\n",_drawBustX,_drawBustY,_scaleX,_scaleY);
		drawTextureScaleAlphaGood(passedBust->image,_drawBustX,_drawBustY,_scaleX,_scaleY,255);
	}else{
		if (passedBust->bustStatus==BUST_STATUS_TRANSFORM_FADEIN){
			drawTextureScaleAlphaGood(passedBust->transformTexture,_drawBustX,_drawBustY, _scaleX, _scaleY, 255-passedBust->curAlpha);
		}
		drawTextureScaleAlphaGood(passedBust->image,_drawBustX,_drawBustY, _scaleX, _scaleY, passedBust->curAlpha);
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
// seek past a windows newline or a unix newline
void moveFilePointerPastNewline(crossFile* fp){
	unsigned char _temp;
	crossfread(&_temp,1,1,fp);
	if (_temp==13){
		crossfseek(fp,1,SEEK_CUR);
	}
}
// getline but without a newline at the end
char* easygetline(crossFile* fp){
	char* _tempReadLine=NULL;
	size_t _readLength=0;
	if (crossgetline(&_tempReadLine,&_readLength,fp)==-1){
		free(_tempReadLine);
		return NULL;
	}
	removeNewline(_tempReadLine);
	return _tempReadLine;
}
int readIntLine(crossFile* fp){
	char* _tempReadLine=easygetline(fp);
	int _ret=atoi(_tempReadLine);
	free(_tempReadLine);
	return _ret;
}
unsigned char* ReadNumberStringList(crossFile* fp, unsigned char* _outArraySize){
	*_outArraySize = readIntLine(fp);
	unsigned char* _retList = malloc(sizeof(unsigned char)*(*_outArraySize));
	int i;
	for (i=0;i<*_outArraySize;++i){
		_retList[i] = readIntLine(fp);
	}
	return _retList;
}
char** ReadFileStringList(crossFile* fp, unsigned char* _outArraySize){
	*_outArraySize = readIntLine(fp);
	char** _retList = malloc(sizeof(char*)*(*_outArraySize));
	int i;
	for (i=0;i<*_outArraySize;++i){
		_retList[i] = easygetline(fp);
	}
	return _retList;
}
void LoadPreset(char* filename){
	tipNamesLoaded=0;
	chapterNamesLoaded=0;
	crossFile* fp;
	fp = crossfopen(filename, "r");
	currentPresetFileList.theArray = ReadFileStringList(fp,&currentPresetFileList.length);
	if (gameHasTips==1){
		currentPresetTipList.theArray = ReadFileStringList(fp,&currentPresetTipList.length);
		currentPresetTipUnlockList.theArray = ReadNumberStringList(fp,&(currentPresetTipUnlockList.length));
	}
	char* _lastReadLine = easygetline(fp);
	if (gameHasTips){
		if (_lastReadLine!=NULL && !strcmp(_lastReadLine,"tipnames")){
			currentPresetTipNameList.theArray = ReadFileStringList(fp,&currentPresetTipNameList.length);
			tipNamesLoaded=1;
			free(_lastReadLine);
			_lastReadLine = easygetline(fp);
		}
	}
	if (_lastReadLine!=NULL && !strcmp(_lastReadLine,"chapternames")){
		currentPresetFileFriendlyList.theArray = ReadFileStringList(fp,&currentPresetFileFriendlyList.length);
		chapterNamesLoaded=1;
	}
	free(_lastReadLine);
	crossfclose(fp);
}
void readRGBString(char* _message, unsigned char* r, unsigned char* g, unsigned char* b){
	char _tempNumBuffer[3];
	_tempNumBuffer[2]='\0';
	int i=0;
	char j;
	for (j=0;j<3;++j){
		_tempNumBuffer[0]=_message[i++];
		_tempNumBuffer[1]=_message[i++];
		int _parsedValue = strtol(_tempNumBuffer,NULL,16);
		switch(j){
			case 0:
				*r=_parsedValue;
				break;
			case 1:
				*g=_parsedValue;
				break;
			case 2:
				*b=_parsedValue;
				break;
		}
	}
}
void drawWrappedText(int _x, int _y, char** _passedLines, int _numLines, unsigned char r, unsigned char g, unsigned char b, unsigned char a){
	int i;
	for (i=0;i<_numLines;++i){
		gbDrawTextAlpha(normalFont,_x,_y+currentTextHeight*i,_passedLines[i],r,g,b,a);
	}
}
void drawWrappedTextCentered(int _x, int _y, char** _passedLines, int _numLines, int _boxWidth, unsigned char r, unsigned char g, unsigned char b, unsigned char a){
	int i;
	for (i=0;i<_numLines;++i){
		gbDrawTextAlpha(normalFont,_x+easyCenter(textWidth(normalFont,_passedLines[i]),_boxWidth),_y+currentTextHeight*i,_passedLines[i],r,g,b,a);
	}
}
char issecondaryutf8(unsigned char c){
	return (c & 192)==128;
}
// _c starts out on the primary utf8 char
int nextutf8(char* _stringStart, int i){
	while(1){
		if (!issecondaryutf8(_stringStart[++i])){
			return i;
		}
	}
}
int prevutf8(char* _stringStart, int i){
	while(i>0){
		if (!issecondaryutf8(_stringStart[--i])){
			return i;
		}
	}
	return -1;
}
void freeWrappedText(int _numLines, char** _passedLines){
	int i;
	for (i=0;i<_numLines;++i){
		free(_passedLines[i]);
	}
	free(_passedLines);
}
// _workable must be made with malloc. its buffer may be changed
void wrapTextAdvanced(char** _passedMessage, int* _numLines, char*** _realLines, int _maxWidth, int32_t** _propRet){
	int32_t* _propBuff = (_propRet!=NULL ? *_propRet : NULL);
	unsigned char* _workable = (unsigned char*)(*_passedMessage);
	int _cachedStrlen = strlen(_workable);
	if (_cachedStrlen==0){
		*_numLines=0;
		if (_realLines!=NULL){
			*_realLines=NULL;
		}
		return;
	}
	int _lastPropSet=-1; // only set properties if at index greater than this one
	int _lastNewline = -1; // Index
	int32_t _curProp=0;
	int _lastCharLen = _cachedStrlen-prevutf8(_workable,_cachedStrlen);
	int i;
	for (i=0;i<=_cachedStrlen-_lastCharLen;){
		char _isBreakChar = (_workable[i]=='\n' || _workable[i]==' ');
		if (_isBreakChar || i>=_cachedStrlen-_lastCharLen){
			char _didChop=0;
			char _oldChar;
			if (_isBreakChar){
				_didChop=1;
				_oldChar = _workable[i];
				_workable[i]='\0'; // Chop the string for textWidth function
			}
			if (textWidth(normalFont,&(_workable[_lastNewline+1]))>_maxWidth){ // If at this spot the string is too long for the screen
				// Find last word before we went off screen
				int j;
				for (j=prevutf8(_workable,i);j>_lastNewline;j=prevutf8(_workable,j)){
					if (_workable[j]==' '){
						break;
					}
				}
				if (j==_lastNewline){ // Didn't find a stopping point, the line is likely one giant word
					char _didBreak=0;
					// Force a stopping point
					for (j=prevutf8(_workable,i);j>_lastNewline;j=prevutf8(_workable,j)){
						char _cacheChar = _workable[j];
						_workable[j]='\0';
						char _canSplit = (textWidth(normalFont,&(_workable[_lastNewline+1]))<=_maxWidth);
						_workable[j]=_cacheChar;
						if (_canSplit){
							// The character we're at right now, that's where the split needs to be because it's acting as the null terminator right now
							char* _newBuffer = malloc(_cachedStrlen+2);
							memcpy(_newBuffer,_workable,j);
							_newBuffer[j]='\0';
							_lastNewline=j;
							memcpy(&(_newBuffer[j+1]),&(_workable[j]),_cachedStrlen-(j)+1); // Should also copy null
							free(_workable);
							_workable = _newBuffer;
							// also realloc properties
							if (_propBuff!=NULL){
								int32_t* _newProps = malloc((_cachedStrlen+1)*sizeof(int32_t));
								memcpy(_newProps,_propBuff,j*sizeof(int32_t));
								memcpy(&(_newProps[j+1]),&(_propBuff[j]),(_cachedStrlen-j)*sizeof(int32_t));
								free(_propBuff);
								_propBuff=_newProps;
							}
							// Account for new byte
							_cachedStrlen++;
							i++;
							_didBreak=1;
							break;
						}
					}
					// Odd, no part between last new line and here less than _maxWidth. This should not happen, but just in case, put some code to at least do something.
					if (!_didBreak){
						printf("odd\n");
						_workable[i]='\0';
					}
				}else{ // Normal, found what we're looking for, just chop at the end of the last word
					_workable[j]='\0';
					_lastNewline=j;
				}
				if (_didChop && _lastNewline!=i){ // Fix chop if happened
					_workable[i]=_oldChar;
				}
				// No matter what we did, we'll still need to start our check from the last new line
				// _lastPropSet keeps properties from being overwritten
				i=_lastNewline;
			}else{
				if (_didChop){
					if (_oldChar=='\n'){ // we're okay to put the line break here. also it's already chopped.
						_lastNewline=i;
					}else{ // Fix chop if happened
						_workable[i]=_oldChar;
					}
				}
			}
		}else if (_propBuff!=NULL){
			if (_workable[i]=='<'){
				char _foundMarkup=0;
				if (strncmp(&_workable[i],COLORMARKUPSTART,strlen(COLORMARKUPSTART))==0){
					int _startI=i;
					i+=strlen(COLORMARKUPSTART);
					if (i+6+strlen(COLORMARKUPEND)<_cachedStrlen){
						_curProp=0;
						readRGBString(&_workable[i],(char*)&_curProp,((char*)&_curProp)+1,((char*)&_curProp)+2);
						i+=6;
						if (_workable[i]!='>'){
							puts("parsing color markup failed");
							i-=6;
							_curProp=0;
						}else{
							setPropBit(&_curProp,TEXTPROP_COLORED);
							_foundMarkup=1;
							memset(&_workable[_startI],1,strlen(COLORMARKUPSTART)+6+1);
							i=_startI;
						}
					}
				}else if (strncmp(&_workable[i],COLORMARKUPEND,strlen(COLORMARKUPEND))==0){
					memset(&_workable[i],1,strlen(COLORMARKUPEND));
					_curProp=0;
					_foundMarkup=1;
				}
				//_propBuff
				if (!_foundMarkup){
					char* _endPos = strchr(&(_workable[i]),'>');
					if (_endPos!=NULL){
						int _deltaChars = (_endPos-(char*)&(_workable[i]));
						memset(&(_workable[i]),1,_deltaChars+1); // Because this starts at i, k being 11 with i as 10 would just write 1 byte, therefor missing the end '>'. THe fix is to add one.
					}
				}
			}else if (_workable[i]=='\\' || _workable[i]=='x'){ // I saw that Umineko VNDS doesn't use a backslash before
				//http://jafrog.com/2013/11/23/colors-in-terminal.html
				if (_cachedStrlen-i>=strlen("x1b[0m")+(_workable[i]=='\\')){
					if (strncmp(&(_workable[i+(_workable[i]=='\\')]),"x1b[",strlen("x1b["))==0){
						int _oldIndex=i;
						// Advance to the x character if we chose to use backslash
						if (_workable[i]=='\\'){
							i++;
						}
						i+=4; // We're now in the parameters
						int _mSearchIndex;
						for (_mSearchIndex=i;_mSearchIndex<_cachedStrlen;++_mSearchIndex){
							if (_workable[_mSearchIndex]=='m'){
								break;
							}
						}
						// If found the ending
						if (_workable[_mSearchIndex]=='m'){
							// TODO - Do stuff with the found color code
							if (_workable[i]=='0'){ // If we're resetting the color
	
							}else{
								int _semiColonSearchIndex;
								for (_semiColonSearchIndex=i;_semiColonSearchIndex<_mSearchIndex;++_semiColonSearchIndex){
									if (_workable[_semiColonSearchIndex]==';'){
										break;
									}
								}
								_workable[_semiColonSearchIndex]=0;
								printf("the number is %s\n",&(_workable[i]));
								_workable[_semiColonSearchIndex]=';';
							}
							i=_oldIndex;
							memset(&(_workable[i]),1,_mSearchIndex-i+1);
						}else{
							printf("Failed to parse color markup");
							i=_oldIndex; // Must be invalid otherwise
						}
					}
				}
				
			}
		}
		/////
		if (_propBuff && i>_lastPropSet){
			int _destIndex = nextutf8(_workable,i);
			for (;i<_destIndex;++i){
				_propBuff[i]=_curProp;
			}
			_lastPropSet=i-1;
		}else{
			i=nextutf8(_workable,i);
		}
	}
	*_passedMessage = _workable;
	// fheuwfhuew (\0) ffhuehfu (\0) fheuhfueiwf (\0)
	*_numLines=1;
	for (i=0;i<_cachedStrlen;++i){
		if (_workable[i]=='\0'){
			*_numLines=(*_numLines)+1;
		}
	}
	if (_realLines!=NULL){
		*_realLines = malloc(sizeof(char*)*(*_numLines));
		int _nextCopyIndex=0;
		for (i=0;i<*_numLines;++i){
			(*_realLines)[i] = strdup(&(_workable[_nextCopyIndex]));
			_nextCopyIndex += 1+strlen(&(_workable[_nextCopyIndex]));
		}
	}
	if (_propRet){
		*_propRet=_propBuff;
	}
}
void wrapText(const char* _passedMessage, int* _numLines, char*** _realLines, int _maxWidth){
	char* _workable = strdup(_passedMessage);
	wrapTextAdvanced(&_workable,_numLines,_realLines,_maxWidth,NULL);
	free(_workable);
}
void easyMessage(const char** _passedMessage, int _numLines, char _doWait){
	unsigned char _tR, _tG, _tB;
	getInverseBGCol(&_tR,&_tG,&_tB);
	controlsReset();
	do{
		controlsStart();
		if (proceedPressed()){
			controlsStart();
			controlsEnd();
			break;
		}
		controlsEnd();
		startDrawing();

		int i;
		for (i=0;i<_numLines;++i){
			gbDrawText(normalFont,32,5+currentTextHeight*(i+2),_passedMessage[i],_tR,_tG,_tB);
		}
		if (_doWait){
			gbDrawText(normalFont,32,screenHeight-32-currentTextHeight,SELECTBUTTONNAME" to continue.",_tR,_tG,_tB);
		}
		endDrawing();
	}while (currentGameStatus!=GAMESTATUS_QUIT && _doWait);
}
void easyMessagef(char _doWait, const char* _formatString, ...){
	va_list _tempArgs;
	va_start(_tempArgs, _formatString);
	char* _completeString = formatf(_tempArgs,_formatString);
	char** _wrappedLines;
	int _numLines;
	wrapText(_completeString,&_numLines,&_wrappedLines,screenWidth-32);
	easyMessage((const char**)_wrappedLines,_numLines,_doWait);
	freeWrappedText(_numLines,_wrappedLines);
	free(_completeString);
}
void writeStringNumTable(FILE* fp, const char* _name){
	lua_getglobal(L,_name);
	lua_pushnil(L); // first key
	while (lua_next(L,-2)!=0){
		// key is at -2, value at -1
		if (lua_type(L,-2)!=LUA_TSTRING){
			fprintf(stderr,"error, is not string type index.\n");
		}
		if (lua_type(L,-1)!=LUA_TNUMBER){
			fprintf(stderr,"error, is not number type value.\n");
		}
		writeLengthStringToFile(fp,lua_tostring(L,-2));
		int _value = lua_tonumber(L,-1);
		fwrite(&_value,1,sizeof(int),fp);
		// pop this, but we need to keep the key
		lua_pop(L, 1);
	}
	lua_pop(L,1);
	writeLengthStringToFile(fp,NULL);
}
void loadStringNumTable(FILE* fp, const char* _name){
	lua_getglobal(L,_name);
	while (1){
		char* _curName = readLengthStringFromFile(fp);
		if (_curName[0]=='\0'){
			free(_curName);
			break;
		}
		int _readVal;
		fread(&_readVal,1,sizeof(int),fp);
		lua_pushnumber(L,_readVal);
		lua_setfield(L,-2,_curName);
		free(_curName);
	}
	lua_pop(L,1);
}
void loadHiguGame(){
	currentPresetChapter=-1;
	char* _specificName = getHiguSavePath();
	if (checkFileExist(_specificName)){
		FILE* fp = fopen(_specificName,"rb");
		int _v = fgetc(fp);
		if (!(_v>=2 && _v<=HIGUSAVEFORMAT)){
			easyMessagef(1,"expected version %d but got %d\n",HIGUSAVEFORMAT,_v);
			goto err;
		}
		fread(&currentPresetChapter,2,1,fp);
		if (_v>=3){
			loadStringNumTable(fp,"localFlags");
		}
		if (_v>=4){
			// load fragment progress
			if (fragStatus){
				free(fragStatus);
				fragStatus=NULL;
			}
			int _readFrags;
			fread(&_readFrags,1,sizeof(int),fp);
			if (_readFrags!=0){
				fragStatus=malloc(_readFrags);
				for (int i=0;i<_readFrags;++i){
					fragStatus[i]=fgetc(fp);
				}
			}
		}
	err:
		fclose(fp);
	}else{
		char* _savefileLocation = oldHiguSavePath(currentPresetFilename);
		if (checkFileExist(_savefileLocation)){
			FILE* fp = fopen(_savefileLocation, "rb");
			fread(&currentPresetChapter,2,1,fp);
			fclose(fp);
		}
		free(_savefileLocation);
	}
	free(_specificName);
}
void saveHiguGame(){
	char* _specificPath = getHiguSavePath();
	FILE *fp;
	fp = fopen(_specificPath, "wb");
	fputc(HIGUSAVEFORMAT,fp);
	fwrite(&currentPresetChapter,2,1,fp);
	writeStringNumTable(fp,"localFlags");
	{
		// write fragment progress
		fwrite(&numFragments,1,sizeof(int),fp);
		for (int i=0;i<numFragments;++i){
			fputc(fragStatus[i],fp);
		}
	}
	fclose(fp);
	free(_specificPath);
}
void TryLoadMenuSoundEffect(const char* _passedPathIdea){
	#if MENUSFXON == 1
		if (menuSound!=NULL){
			return;
		}
		char* tempstringconcat;
		if (_passedPathIdea==NULL){
			tempstringconcat = easyCombineStrings(4,streamingAssets, "SE/","wa_038",".ogg");
		}else{
			tempstringconcat = (char*)_passedPathIdea;
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
	#else
		menuSound=NULL;
	#endif
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
// Called often, so it's separate from applyTextboxChanges
void updateTextPositions(){
	if (textOnlyOverBackground){
		textboxXOffset = floor((float)(screenWidth-applyGraphicsScale(actualBackgroundWidth))/2);
	}else{
		textboxXOffset=0;
	}
	#if GBPLAT == GB_3DS
		if (textIsBottomScreen==1){
			textboxWidth=320;
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
void applyTextboxChanges(char _doRecalcMaxLines){
	if (gameTextDisplayMode==TEXTMODE_ADV){
		textboxYOffset=screenHeight-advboxHeight;
	}else{
		textboxYOffset=0;
	}
	if (!canChangeBoxAlpha && gameTextDisplayMode==TEXTMODE_ADV){
		currentBoxAlpha=255;
	}else{
		currentBoxAlpha=preferredBoxAlpha;
	}
	messageInBoxYOffset=getADVNameYSpace();
	// Apply textOnlyOverBackground setting
	updateTextPositions();
	if (_doRecalcMaxLines){
		recalculateMaxLines();
	}
}
// Returns the folder for CG or CGAlt depending on the user's settings
char* getUserPreferredImageDirectory(char _folderPreference){
	return _folderPreference==LOCATION_CGALT ? locationStrings[LOCATION_CGALT] : locationStrings[LOCATION_CG];
}
// Returns the image folder the user didn't choose.
char* getUserPreferredImageDirectoryFallback(char _folderPreference){
	return _folderPreference==LOCATION_CGALT ? locationStrings[LOCATION_CG] : locationStrings[LOCATION_CGALT];
}
// Location string fallback with a specific image format
char* _locationStringFallbackFormat(const char* filename, char _folderPreference, char* _fileFormat){
	if (!_fileFormat){
		_fileFormat="";
	}
	char* _sensitive;
	// Try the user's first choice
	_sensitive = easyCombineStrings(4,streamingAssets, getUserPreferredImageDirectory(_folderPreference),filename,_fileFormat);
	char* _potentialPath;
	_potentialPath=insensitiveFileExists(_sensitive);
	free(_sensitive);
	if (_potentialPath){
		return _potentialPath;
	}
	// If not exist, try the other folder.
	_sensitive = easyCombineStrings(4,streamingAssets, getUserPreferredImageDirectoryFallback(_folderPreference),filename,_fileFormat);
	_potentialPath=insensitiveFileExists(_sensitive);
	free(_sensitive);
	if (_potentialPath){
		return _potentialPath;
	}
	return NULL;
}
char* LocationStringFallback(const char* filename, char _folderPreference, char _extensionIncluded){
	char* _foundFileExtension=NULL;
	char* _workableFilename = malloc(strlen(filename)+1);
	strcpy(_workableFilename,filename);
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
			// If not, try the extensions anyway.
			goto trywithextension;
		}
	}else{
	trywithextension:
		_returnFoundString = _locationStringFallbackFormat(_workableFilename,_folderPreference, ".png");
		if (_returnFoundString==NULL){
			_returnFoundString = _locationStringFallbackFormat(_workableFilename,_folderPreference, ".jpg");
		}
	}
	if (_foundFileExtension!=NULL){
		free(_foundFileExtension);
	}
	free(_workableFilename);
	return _returnFoundString;
}
// Will load a PNG from CG or CGAlt
crossTexture* safeLoadGameImage(const char* filename, char _folderPreference, char _extensionIncluded){
	char* _tempFoundFilename;
	_tempFoundFilename = LocationStringFallback(filename,_folderPreference,_extensionIncluded);
	if (_tempFoundFilename==NULL){
		if (shouldShowWarnings()){
			easyMessagef(1,"Image not found, %s",filename);
		}
		return NULL;
	}
	crossTexture* _returnLoadedPNG = loadImage(_tempFoundFilename);
	free(_tempFoundFilename);
	#if GBPLAT == GB_VITA
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
crossTexture* loadImageOrCache(const char* _filename, char** _relativeFilenameDest){
	crossTexture* _ret=NULL;
	cachedImage* _possibleCachedImage = searchBustCache(_filename);
	if (_possibleCachedImage!=NULL){
		_ret=_possibleCachedImage->image;
		*_relativeFilenameDest=_possibleCachedImage->filename;
		// Remove from cache so we don't free it early
		_possibleCachedImage->image = NULL;
		_possibleCachedImage->filename = NULL;
	}else{
		_ret=safeLoadGameImage(_filename,graphicsLocation,scriptUsesFileExtensions);
		*_relativeFilenameDest=strdup(_filename);
	}
	if (_ret){
		if (currentFilterType==FILTERTYPE_NEGATIVE){
			invertImage(_ret,0);
		}
	}else{
		*_relativeFilenameDest=NULL;
	}
	return _ret;
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
			applyTextboxChanges(0);
			// Draw the changes
			startDrawing();
			drawAdvanced(1,1,1,1,1,0); // Don't draw message text during transitions
			if (_maxDrawLine>=0){
				DrawMessageText(255,_maxDrawLine,-1);
			}
			endDrawing();
		}
	}
	advboxHeight=_newHeight;
	applyTextboxChanges(1);
}
// _overrideNewHeight is in lines
// pass a negative number other than -1 to not do fancy transition
void updateDynamicADVBox(int _maxDrawLine, int _overrideNewHeight){
	if (_maxDrawLine==-1){
		_maxDrawLine=maxLines-1;
	}
	int _newAdvBoxHeight;
	if (_overrideNewHeight==-1){
		// find the number of used lines
		_newAdvBoxHeight=1; // By default one
		int i;
		for (i=0;i<maxLines;++i){
			if (currentMessages[i]){
				_newAdvBoxHeight=i+2; // Last non-empty line. Adding 1 is for one-based number, adding the other 1 is for safety line
			}
		}		
		_newAdvBoxHeight*=currentTextHeight;
	}else{
		_newAdvBoxHeight = _overrideNewHeight*currentTextHeight;
	}
	_newAdvBoxHeight+=getADVNameYSpace();
	_newAdvBoxHeight+=textboxTopPad;
	_newAdvBoxHeight+=textboxBottomPad;
	if (_maxDrawLine>=0){
		smoothADVBoxHeightTransition(advboxHeight,_newAdvBoxHeight,_maxDrawLine);
	}else{
		advboxHeight=_newAdvBoxHeight;
		applyTextboxChanges(1);
	}
}
void enableVNDSADVMode(){
	gameTextDisplayMode=TEXTMODE_ADV;
	dynamicAdvBoxHeight=1;
	loadADVBox();
	updateDynamicADVBox(-2,-1);
	applyTextboxChanges(1);
}
void disableVNDSADVMode(){
	gameTextDisplayMode=TEXTMODE_NVL;
	dynamicAdvBoxHeight=0;
	applyTextboxChanges(1);
}
char* getFileExtension(char* _passedFilename){
	return &(_passedFilename[strlen(_passedFilename)-3]);
}
//===================
void FadeBustshot(int passedSlot,int _time,char _wait){
	if (isSkipping){
		_time=0;
	}
	//int passedSlot = nathanvariableToInt(&_passedArguments[1)-1;
	//Busts[passedSlot].bustStatus = BUST_STATUS_FADEOUT;
	//Busts[passedSlot].statusVariable = 
	Busts[passedSlot].bustStatus = BUST_STATUS_FADEOUT;
	if (_time!=0){
		Busts[passedSlot].curAlpha=Busts[passedSlot].destAlpha;
		Busts[passedSlot].fadeStartTime=getMilli();
		Busts[passedSlot].fadeEndTime=getMilli()+_time;
		if (_wait==1){
			while (Busts[passedSlot].isActive==1){
				controlsStart();
				Update();
				startDrawing();
				Draw(MessageBoxEnabled);
				endDrawing();
				if (proceedPressed()){
					Busts[passedSlot].fadeEndTime=1;
				}
				controlsEnd();
			}
		}
	}else{
		// The free will happen on the next update
		Busts[passedSlot].fadeEndTime=1;
	}
}
void FadeAllBustshots(int _time, char _wait){
	if (isSkipping){
		_time=0;
	}
	int i;
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
				if (Busts[i].isActive==1 && Busts[i].bustStatus==BUST_STATUS_FADEOUT){
					_isDone=0;
					break;
				}
			}
			controlsStart();
			Update();
			startDrawing();
			Draw(MessageBoxEnabled);
			endDrawing();
			if (proceedPressed()){
				for (i=0;i<maxBusts;i++){
					if (Busts[i].isActive==1){
						Busts[i].fadeEndTime=1;
					}
				}
			}
			controlsEnd();
		}
	}
}
void waitForBustSettle(){
	int i;
	while(1){
		char _didBreak=0;
		for (i=0;i<maxBusts;i++){
			//printf("%d is not done.",i);
			if (Busts[i].isActive && Busts[i].bustStatus!=BUST_STATUS_NORMAL){
				_didBreak=1;
				break;
			}
		}
		if (!_didBreak){
			break;
		}
		Update();
		startDrawing();
		Draw(MessageBoxEnabled);
		endDrawing();
	}
}
void DrawScene(const char* _filename, int time){
	if (isSkipping==1){
		time=0;
	}
	// If we're NOT doing the VNDS easy bust reset trick
	if (!(lastBackgroundFilename!=NULL && strcmp(lastBackgroundFilename,_filename)==0)){
		changeMallocString(&lastBackgroundFilename,_filename);
		crossTexture* newBackground = safeLoadGameImage(_filename,graphicsLocation,scriptUsesFileExtensions);
		if (newBackground==NULL){
			freeTexture(currentBackground);
			currentBackground=NULL;
			return;
		}
		if (currentFilterType==FILTERTYPE_NEGATIVE){
			invertImage(newBackground,0);
		}
		if (actualBackgroundSizesConfirmedForSmashFive==0){
			actualBackgroundWidth = getTextureWidth(newBackground);
			actualBackgroundHeight = getTextureHeight(newBackground);
			updateGraphicsScale();
			updateTextPositions();
		}
		if (time!=0){
			u64 _startTime=getMilli();
			u64 _curTime;
			while ((_curTime = getMilli())<_startTime+time){
				Update();
				int _backgroundAlpha=partMoveFillsCapped(_curTime,_startTime,time,255);
				// Change alpha of busts made on the last line
				int i;
				{
					double _ratio = _backgroundAlpha/(double)255;
					for (i=maxBusts-1;i!=-1;i--){
						if (bustOrder[i]!=255 && Busts[bustOrder[i]].isActive==1 && Busts[bustOrder[i]].lineCreatedOn == currentScriptLine-1){
							Busts[bustOrder[i]].curAlpha = Busts[bustOrder[i]].destAlpha*_ratio;
						}
					}
				}
				startDrawing();
				drawAdvanced(1,1,0,0,1,0); // Draws the old background
				gameObjectClipOn();
				double _oldScaleX=extraGameScaleX; // temporarily disable any enlargeScreen if active. the new scene won't have it.
				double _oldScaleY=extraGameScaleY;
				int _oldOffX=extraGameOffX;
				int _oldOffY=extraGameOffY;
				extraGameScaleX=1;
				extraGameScaleY=1;
				extraGameOffX=0;
				extraGameOffY=0;
				// draw new stuff
				DrawBackgroundAlpha(newBackground,_backgroundAlpha); // Draws the new background on top
				// Draw busts created on the last line at the same alpha as the new background
				for (i = maxBusts-1; i != -1; i--){
					if (bustOrder[i]!=255 && Busts[bustOrder[i]].isActive==1 && Busts[bustOrder[i]].lineCreatedOn == currentScriptLine-1){
						DrawBust(&(Busts[bustOrder[i]]));
					}
				}
				gameObjectClipOff();
				extraGameScaleX=_oldScaleX;
				extraGameScaleY=_oldScaleY;
				extraGameOffX=_oldOffX;
				extraGameOffY=_oldOffY;
				drawAdvanced(0,0,1,MessageBoxEnabled,0,MessageBoxEnabled);
				endDrawing();
	
				controlsStart();
				if (proceedPressed()){
					controlsEnd();
					break;
				}
				controlsEnd();
			}
		}
		if (currentBackground!=NULL){
			freeTexture(currentBackground);
		}
		currentBackground=newBackground;
	}
	// A scene change removes any enlargeScreen active
	extraGameScaleX=1;
	extraGameScaleY=1;
	extraGameOffX=0;
	extraGameOffY=0;
	// Fix alpha for busts created on the last line
	int i;
	for (i=maxBusts-1;i!=-1;i--){
		if (bustOrder[i]!=255 && Busts[bustOrder[i]].isActive==1 && Busts[bustOrder[i]].lineCreatedOn == currentScriptLine-1){
			Busts[bustOrder[i]].curAlpha = Busts[bustOrder[i]].destAlpha;
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

			// Fix the bust cache if cached images are inverted.
			if (currentFilterType==FILTERTYPE_NEGATIVE){
				invertImage(_slotToUse->image,0);
			}
		}
	}
	for (i=0;i<maxBusts;i++){
		if (Busts[i].isActive==1 && Busts[i].lineCreatedOn != currentScriptLine-1){
			ResetBustStruct(&Busts[i], 1); // Won't double free the busts in the cache because we set the references in the busts to NULL
		}
	}
}
void enlargeScreenManual(int _destOffX, int _destOffY, double _destScaleX, double _destScaleY, int _time, char _waitForCompletion){
	endWorkingEnlarge(); // previous one
	workingEnlarge = malloc(sizeof(struct enlargeAnimInfo));
	workingEnlarge->startScaleX=extraGameScaleX;
	workingEnlarge->startScaleY=extraGameScaleY;
	workingEnlarge->startOffX=extraGameOffX;
	workingEnlarge->startOffY=extraGameOffY;
	workingEnlarge->totalTime=_time;
	workingEnlarge->startTime=getMilli();
	workingEnlarge->destScaleX=_destScaleX;
	workingEnlarge->destScaleY=_destScaleY;
	workingEnlarge->destOffX=_destOffX;
	workingEnlarge->destOffY=_destOffY;
	if (workingEnlarge->destOffX>0){
		workingEnlarge->destOffX=0;
	}if (workingEnlarge->destOffY>0){
		workingEnlarge->destOffY=0;
	}
	if (_waitForCompletion){
		while(workingEnlarge){
			controlsStart();
			if (proceedPressed()){
				endWorkingEnlarge();
			}
			controlsEnd();
			Update();
			startDrawing();
			Draw(MessageBoxEnabled);
			endDrawing();
		}
	}
}
void enlargeScreen(int _x, int _y, int _w, int _h, int _time, char _waitForCompletion){
	double _destScaleX=scriptScreenWidth/(double)_w;
	double _destScaleY=scriptScreenHeight/(double)_h;
	
	int _destOffX=(_x/(double)scriptScreenWidth)*actualBackgroundWidth*graphicsScale*((scriptScreenWidth-_w)/(double)scriptScreenWidth)*_destScaleX*-1;
	int _destOffY=(_y/(double)scriptScreenHeight)*actualBackgroundHeight*graphicsScale*((scriptScreenHeight-_h)/(double)scriptScreenHeight)*_destScaleY*-1;

	enlargeScreenManual(_destOffX,_destOffY,_destScaleX,_destScaleY,_time,_waitForCompletion);
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
// _scriptForcedWidth and _scriptForcedHeight not used if _scriptForcedWidth is -1
int drawBustshotAdvanced(unsigned char passedSlot, const char* _filename, int _xoffset, int _yoffset, int _layer, int _fadeintime, int _waitforfadein, int _destAlpha, int _scriptForcedWidth, int _scriptForcedHeight, char _coordsReferToSprMiddle){
	if (passedSlot>=maxBusts){
		int _oldMaxBusts = maxBusts;
		maxBusts = passedSlot+1;
		increaseBustArraysSize(_oldMaxBusts,maxBusts);
		if (drawBustshotAdvanced(passedSlot,_filename,_xoffset,_yoffset,_layer,_fadeintime,_waitforfadein,_destAlpha,_scriptForcedWidth,_scriptForcedHeight,_coordsReferToSprMiddle)!=0){
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
			crossTexture* _cachedOldTexture = Busts[passedSlot].image;
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

	Busts[passedSlot].image = loadImageOrCache(_filename,&(Busts[passedSlot].relativeFilename));
	if (Busts[passedSlot].image==NULL){
		free(Busts[passedSlot].relativeFilename);
		Busts[passedSlot].relativeFilename=NULL;
		ResetBustStruct(&(Busts[passedSlot]),0);
		return 0;
	}

	Busts[passedSlot].cacheXOffsetScale = GetXOffsetScale();
	Busts[passedSlot].cacheYOffsetScale = GetYOffsetScale();
	// apply the forced draw size
	if (_scriptForcedWidth!=-1){
		Busts[passedSlot].scaleX=_scriptForcedWidth/(double)getTextureWidth(Busts[passedSlot].image)*GetXOffsetScale();
		Busts[passedSlot].scaleY=_scriptForcedHeight/(double)getTextureHeight(Busts[passedSlot].image)*GetYOffsetScale();
	}
	// adjust for _coordsReferToSprMiddle
	if (_coordsReferToSprMiddle){
		convertPosition(1,&_xoffset,&_yoffset,Busts[passedSlot].image);
	}

	Busts[passedSlot].xOffset = _xoffset;
	Busts[passedSlot].yOffset = _yoffset;
	Busts[passedSlot].layer = _layer;
	// The lineCreatedOn variable is used to know if the bustshot should stay after a scene change. The bustshot can only stay after a scene change if it's created the line before the scene change AND it doesn't wait for fadein completion.
	if (_waitforfadein==0){
		Busts[passedSlot].lineCreatedOn = currentScriptLine;
	}else{
		Busts[passedSlot].lineCreatedOn = 0;
	}
	Busts[passedSlot].isActive=1;
	RecalculateBustOrder();
	Busts[passedSlot].destAlpha=_destAlpha;
	if (_fadeintime!=0){
		Busts[passedSlot].curAlpha=0;
		u64 _curTime = getMilli();
		if (Busts[passedSlot].bustStatus == BUST_STATUS_TRANSFORM_FADEIN){
			Busts[passedSlot].fadeStartTime=_curTime; // Transform fadein doesn't waste any time
		}else{
			Busts[passedSlot].bustStatus = BUST_STATUS_FADEIN;
			Busts[passedSlot].fadeStartTime=_curTime+_fadeintime/2; // Normal fadein wastes half the time for no reason
		}
		Busts[passedSlot].fadeEndTime=_curTime+_fadeintime;		
	}else{
		Busts[passedSlot].bustStatus = BUST_STATUS_NORMAL;
		Busts[passedSlot].curAlpha=_destAlpha;
	}
	if (_waitforfadein==1){
		while (Busts[passedSlot].bustStatus!=BUST_STATUS_NORMAL){
			controlsStart();
			Update();
			if (Busts[passedSlot].curAlpha>Busts[passedSlot].destAlpha){
				Busts[passedSlot].curAlpha=Busts[passedSlot].destAlpha;
			}
			startDrawing();
			Draw(MessageBoxEnabled);
			endDrawing();
			if (proceedPressed() || skippedInitialWait==1){
				Busts[passedSlot].curAlpha = Busts[passedSlot].destAlpha;
				Busts[passedSlot].bustStatus = BUST_STATUS_NORMAL;
				startDrawing();
				Draw(MessageBoxEnabled);
				endDrawing(); // Draw once more with the bust gone.
				controlsEnd();
				break;
			}
			controlsEnd();
		}
	}
	return 0;
}
int DrawBustshot(unsigned char passedSlot, const char* _filename, int _xoffset, int _yoffset, int _layer, int _fadeintime, int _waitforfadein, int _destAlpha){
	return drawBustshotAdvanced(passedSlot, _filename, _xoffset, _yoffset, _layer, _fadeintime, _waitforfadein, _destAlpha, -1, -1, bustsStartInMiddle);
}
char* getSpecificPossibleSoundFilename(const char* _filename, char* _folderName){
	char* _ret=NULL;
	char* tempstringconcat = malloc(strlen(streamingAssets)+strlen(_folderName)+strlen(_filename)+1+4);
	strcpy(tempstringconcat,streamingAssets);
	strcat(tempstringconcat,_folderName);
	strcat(tempstringconcat,_filename);
	//
	char* _insensitive;
	//
	if (scriptUsesFileExtensions){
		_insensitive=insensitiveFileExists(tempstringconcat);
		if (_insensitive){
			_ret=_insensitive;
			goto foundret;
		}
		removeFileExtension(tempstringconcat);
	}
	//
	strcat(tempstringconcat,".ogg");
	_insensitive=insensitiveFileExists(tempstringconcat);
	if (_insensitive){
		_ret=_insensitive;
		goto foundret;
	}
	//
	#if GBSND != GBSND_VITA
		removeFileExtension(tempstringconcat);
		strcat(tempstringconcat,".wav");
		_insensitive=insensitiveFileExists(tempstringconcat);
		if (_insensitive){
			_ret=_insensitive;
			goto foundret;
		}
	#endif
	//
	#if GBSND == GBSND_VITA
		removeFileExtension(tempstringconcat);
		strcat(tempstringconcat,".mp3");
		_insensitive=insensitiveFileExists(tempstringconcat);
		if (_insensitive){
			_ret=_insensitive;
			goto foundret;
		}
	#endif
	//
foundret:
	free(tempstringconcat);
	return _ret;
}
#if GBSND == GBSND_VITA
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
		easyMessagef(1,"File format not found, %s",_passedFilename);
		return FILE_FORMAT_NONE;
	}
#else
	char getProbableSoundFormat(const char* _passedFilename){
		easyMessagef(1,"Error with getProbableSoundFormat being the wrong version. Contact MyLegGuy.\n%s",_passedFilename);
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
	#if GBSND != GBSND_VITA
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
	#if GBSND == GBSND_VITA
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
		#if GBSND == GBSND_VITA
			char _foundFormat=0;
			legArchiveFile _foundArchiveFile = soundArchiveGetFilename(_filename,&_foundFormat);
			if (_foundArchiveFile.fp==NULL){
				//LazyMessage(filename,"not found in archive",NULL,NULL);
			}else{
				_tempHoldSlot = _mlgsnd_loadAudioFILE(_foundArchiveFile, _foundFormat, !_isSE, 1);
			}
		#else
			#if GBPLAT != GB_LINUX
				easyMessagef(1,"sound archive not supported.");
			#endif
		#endif
	}
	if (_tempHoldSlot==NULL){
		if (shouldShowWarnings()){
			easyMessagef(1,"Sound file %s not found in folders. %s",_filename,useSoundArchive ? "Not found in archive either." : "");
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
		easyMessagef(1,"Sound effect slot too high. No action will be taken.");
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
		crossPlayHandle _tempHandle = playSound(soundEffects[passedSlot],passedSlot+10);
		setSFXVolume(_tempHandle,GenericFixSpecificVolume(unfixedVolume,_passedVolumeFixScale));
		// Used for auto mode
		lastVoiceSlot=passedSlot;
		lastVoiceHandle=_tempHandle;
	}
}
void strcpyNO1WithProps(char* dest, const char* src, int32_t* _destmap, int32_t* _srcmap){
	int i;
	int _destCopyOffset=0;
	int _srcStrlen = strlen(src);
	for (i=0;i<_srcStrlen;i++){
		if (src[i]!=1){
			_destmap[_destCopyOffset]=_srcmap[i];
			dest[_destCopyOffset++]=src[i];
		}
	}
	dest[_destCopyOffset]=0;
}
void OutputLine(const unsigned char* _tempMsg, char _endtypetemp, char _autoskip){
	if (strlen(_tempMsg)==0){
		return;
	}
	if (isSkipping || textSpeed==TEXTSPEED_INSTANT){
		_autoskip=1;
	}
	if (!MessageBoxEnabled){
		showTextbox();
	}
	int _currentDrawChar; // exclusive
	char* message;
	if (currentLine<maxLines && currentMessages[currentLine]!=NULL){ // if required, prepend what was already on the line
		_currentDrawChar=strlen(currentMessages[currentLine]);
		message = malloc(strlen(_tempMsg)+strlen(currentMessages[currentLine])+1);
		strcpy(message,currentMessages[currentLine]);
		strcat(message,_tempMsg);
	}else{
		_currentDrawChar=0;
		message = strdup(_tempMsg);
	}
	int _numLines;
	int32_t* _wrappedProps = malloc(strlen(message)*sizeof(int32_t));
	wrapTextAdvanced(&message,&_numLines,NULL,getOutputLineScreenWidth(),&_wrappedProps);
	if (_numLines==0){
		goto cleanup;
	}
	// apply default color
	if (nextTextR!=DEFAULTFONTCOLORR || nextTextG!=DEFAULTFONTCOLORG || nextTextB!=DEFAULTFONTCOLORB){
		char _colorBuff[3];
		_colorBuff[0]=nextTextR;
		_colorBuff[1]=nextTextG;
		_colorBuff[2]=nextTextB;
		char _linesPassed=0;
		int i;
		for (i=0;;++i){
			if (message[i]=='\0' && (++_linesPassed)==_numLines){
				break;
			}
			if (!(_wrappedProps[i] & TEXTPROP_COLORED)){
				memcpy(&_wrappedProps[i],_colorBuff,3);
				setPropBit(&_wrappedProps[i],TEXTPROP_COLORED);
			}
		}
	}
	int _linesCopied;
	// if we're not displaying interactively, skip the lines that overflow.
	if (_autoskip && _numLines>maxLines){
		_linesCopied=_numLines-maxLines;
	}else{
		_linesCopied=0;
	}
	int _nextCopyIndex=0;
transferMoreLines:
	;
	// the dynamic ADV box will become as big as required for all the lines. pray there isn't a message that takes the entire screen at once.
	int _destCurLine = currentLine+(_numLines-_linesCopied-1);
	if (dynamicAdvBoxHeight){ // we should never use the goto if we're in dynamic adv box mode btw
		updateDynamicADVBox(-1,_destCurLine+1);
	}
	// fix if we're going to overflow maxLines
	if (_destCurLine>=maxLines){
		if (clearAtBottom || (_numLines-_linesCopied)>=maxLines){ // clearAtBottom is a condition, but also if we have no hope of upshifting enough then clear entire message. more code to handle this situation down below.
			ClearMessageArray(1);
			currentLine=0;
		}else{ // because of above condition, upshifting means that we won't need to copy text twice. this allows the assumption that, if we need to copy more lines later, we last copied `maxLines` lines.
			int _delLines = _destCurLine-maxLines+1;
			upshiftText(_delLines);
			currentLine-=_delLines;
		}
	}
	// free the first line first because we're redoing it
	if (currentMessages[currentLine]){
		free(currentMessages[currentLine]);
		free(messageProps[currentLine]);
	}
	int _currentDrawLine=currentLine;
	int i;
	for (i=0;_linesCopied<_numLines;++_linesCopied,++i){
		if (i!=0){
			currentLine++;
		}
		if (currentLine==maxLines){ // don't overflow if we have too many lines.
			break;
		}
		int _lineLen = strlenNO1(&(message[_nextCopyIndex]));
		messageProps[currentLine] = malloc(sizeof(int32_t)*_lineLen);
		currentMessages[currentLine] = malloc(_lineLen+1);
		strcpyNO1WithProps(currentMessages[currentLine],&(message[_nextCopyIndex]),messageProps[currentLine],&(_wrappedProps[_nextCopyIndex]));
		_nextCopyIndex += 1+strlen(&(message[_nextCopyIndex]));
	}
	char _isDone=_autoskip;
	char _slowTextSpeed=0;
	while(!_isDone){
		controlsStart();
		Update();
		if (proceedPressed()){
			_isDone=1;
		}
		updateControlsGeneral();
		controlsEnd();
		
		startDrawing();
		drawAdvanced(1,1,1,MessageBoxEnabled,1,0);
		#if GBPLAT == GB_3DS
			if (textIsBottomScreen==1){
				startDrawingBottom();
			}
		#endif
		if (MessageBoxEnabled){
			DrawMessageText(255,_currentDrawLine,_currentDrawChar);
		}
		endDrawing();
		if (_isDone==0 && ( (textSpeed>0) || (_slowTextSpeed++ == abs(textSpeed)) )){
			_slowTextSpeed=0;
			int i;
			for (i=0;i<(textSpeed>0 ? textSpeed : 1);i++){
				if (currentMessages[_currentDrawLine][_currentDrawChar]=='\0'){ // If the next char we're about to display is the end of the line
					_currentDrawLine++;
					// If we just passed the line we'll be writing to next time then we're done
					if (_currentDrawLine==currentLine+1 || _currentDrawLine==maxLines){
						_isDone=1;
						break;
					}else{ // Otherwise, start displaying at the start of the next line
						_currentDrawChar=0;
					}
				}else{
					// if it's not ASCII, skip to end of UTF-8 character
					if ((unsigned char)(currentMessages[_currentDrawLine][_currentDrawChar])>0x7F){ // this is same as checking if last bit is on
						// https://tools.ietf.org/html/rfc3629
						// the number of bits on the left set to 1 determintes number of bytes
						unsigned char _firstByte = currentMessages[_currentDrawLine][_currentDrawChar];
						if ((_firstByte & 0xF0) == 0xF0){
							_currentDrawChar+=3;
						}else if ((_firstByte & 0xE0) == 0xE0){
							_currentDrawChar+=2;
						}else if ((_firstByte & 0xC0) == 0xC0){
							_currentDrawChar++;
						}
					}
					_currentDrawChar++;
				}
			}
		}
	}
	if (_linesCopied!=_numLines){
		_currentDrawChar=0; // if we're putting up brand new text, we're definetly starting at the start of the line again.
		endType = Line_WaitForInput;
		outputLineWait();
		goto transferMoreLines;
	}
cleanup:
	free(message);
	free(_wrappedProps);
	endType = _endtypetemp;
}
#if GBPLAT == GB_3DS
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
	#if GBPLAT == GB_3DS
		lockBGM();
	#endif
	__freeBGMNoLock(_slot);
	#if GBPLAT == GB_3DS
		_bgmIsLock = 0;
	#endif
}
void StopBGM(int _slot){
	#if GBPLAT == GB_3DS
		lockBGM();
	#endif
	changeMallocString(&lastBGMFilename,NULL);
	if (currentMusic[_slot]!=NULL){
		stopMusic(currentMusicHandle[_slot]);
	}
	#if GBPLAT == GB_3DS
		_bgmIsLock = 0;
	#endif
}
// Unfixed bgm
void PlayBGM(const char* filename, int _volume, int _slot){
	#if GBPLAT == GB_3DS
		lockBGM();
	#endif
	if (filename!=lastBGMFilename){ // HACK
		changeMallocString(&lastBGMFilename,filename);
	}
	if (bgmVolume==0){
		printf("BGM volume is 0, ignore music change.\n");
	}else if (_slot>=MAXMUSICARRAY){
		easyMessagef(1,"Music slot too high. No action will be taken.");
	}else{
		crossMusic* _tempHoldSlot=loadGameAudio(filename,PREFER_DIR_BGM,0);
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
	#if GBPLAT == GB_3DS
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
// preferredBoxAlpha, 1 byte
// textOnlyOverBackground, 1 byte
// textSpeed, 1 byte
// clearAtBottom, 1 byte
// showVNDSWarnings, 1 byte
// higurashiUsesDynamicScale, 1 byte. (00 written now.)
// preferredTextDisplayMode, 1 byte
// autoModeVoicedWait, 4 bytes
void SaveSettings(){
	char* _fixedFilename = fixPathAlloc("settings.noob",TYPE_DATA);
	FILE* fp=fopen(_fixedFilename, "wb");
	free(_fixedFilename);

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
	fwrite(&preferredBoxAlpha,1,1,fp);
	fwrite(&textOnlyOverBackground,1,1,fp);
	fwrite(&textSpeed,1,1,fp);
	fwrite(&clearAtBottom,sizeof(signed char),1,fp);
	fwrite(&showVNDSWarnings,sizeof(signed char),1,fp);
	fputc(0x00,fp);
	fwrite(&preferredTextDisplayMode,sizeof(signed char),1,fp);
	fwrite(&autoModeVoicedWait,sizeof(int),1,fp);
	fwrite(&vndsSpritesFade,sizeof(signed char),1,fp);
	fwrite(&touchProceed,sizeof(signed char),1,fp);
	fwrite(&dropshadowOn,sizeof(signed char),1,fp);
	fwrite(&fontSize,sizeof(double),1,fp);
	fwrite(&prefersADVNames,sizeof(signed char),1,fp);
	fwrite(&playerLanguage,sizeof(signed char),1,fp);

	#ifdef CUSTOM_SAVED_SETTINGS
		customSettingsSave(fp);
	#endif
	
	fclose(fp);
	printf("SAved settings file.\n");
}
void LoadSettings(){
	char* _fixedFilename = fixPathAlloc("settings.noob",TYPE_DATA);
	if (checkFileExist(_fixedFilename)){
		FILE* fp = fopen (_fixedFilename, "rb");
		// This is the version of the format of the options file.
		unsigned char _tempOptionsFormat;
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
			fread(&preferredBoxAlpha,1,1,fp);
			fread(&textOnlyOverBackground,1,1,fp);
		}
		if (_tempOptionsFormat>=5){
			fread(&textSpeed,1,1,fp);
		}
		if (_tempOptionsFormat>=6){
			fread(&clearAtBottom,sizeof(signed char),1,fp);
		}
		if (_tempOptionsFormat>=7){
			fread(&showVNDSWarnings,sizeof(signed char),1,fp);
		}
		if (_tempOptionsFormat>=8){
			fgetc(fp); // ignore higurashiUsesDynamicScale
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
			fread(&touchProceed,sizeof(signed char),1,fp);
		}
		if (_tempOptionsFormat>=13){
			fread(&dropshadowOn,sizeof(signed char),1,fp);
		}
		if (_tempOptionsFormat>=14){
			fread(&fontSize,sizeof(double),1,fp);
		}
		if (_tempOptionsFormat>=15){
			fread(&prefersADVNames,sizeof(signed char),1,fp);
		}
		if (_tempOptionsFormat>=16){
			fread(&playerLanguage,1,1,fp);
		}
		#ifdef CUSTOM_SAVED_SETTINGS
			customSettingsLoad(fp);
		#endif
		fclose(fp);

		if (cpuOverclocked==1){
			#if GBPLAT == GB_VITA
				scePowerSetArmClockFrequency(444);
			#endif
			#if GBPLAT == GB_3DS
				textboxWidth = 320;
				outputLineScreenHeight = 240;
			#endif
		}
		applyTextboxChanges(0); // max lines will be calculated later by the font loading
		printf("Loaded settings file.\n");
	}
	free(_fixedFilename);
}
#define HISTORYSCROLLBARHEIGHT (((double)HISTORYONONESCREEN/(double)MAXMESSAGEHISTORY)*screenHeight)
//#define HISTORYSCROLLRATE (floor((double)MAXMESSAGEHISTORY/15))
#define HISTORYSCROLLRATE 1
void historyMenu(){
	controlsReset();
	int _maxScroll = MAXMESSAGEHISTORY-HISTORYONONESCREEN;
	int _scrollOffset=_maxScroll;
	while (1){
		controlsStart();
		menuControlsLow(&_scrollOffset,0,HISTORYSCROLLRATE,0,HISTORYSCROLLRATE*2,0,_maxScroll);
		if (wasJustPressed(BUTTON_B) || wasJustPressed(BUTTON_START)){
			controlsReset();
			break;
		}
		controlsEnd();
		startDrawing();
		Draw(0);
		drawRectangle(textboxXOffset,0,textboxWidth,screenHeight,0,0,0,150);
		int i;
		for (i=0;i<HISTORYONONESCREEN;i++){
			int _arrIndex = FixHistoryOldSub(i+_scrollOffset,oldestMessage);
			if (messageHistory[_arrIndex]){
				gbDrawText(normalFont,textboxXOffset,textHeight(normalFont)+i*currentTextHeight,messageHistory[_arrIndex],255,255,255);
			}
		}
		if (textboxWidth == screenWidth){
			gbDrawText(normalFont,3,screenHeight-currentTextHeight-5,"TEXTLOG",0,0,0);
		}
		drawRectangle((screenWidth-5),0,5,screenHeight,0,0,0,255);
		drawRectangle((screenWidth-5),floor((screenHeight-HISTORYSCROLLBARHEIGHT)*((double)_scrollOffset/(MAXMESSAGEHISTORY-HISTORYONONESCREEN))),5,HISTORYSCROLLBARHEIGHT,255,0,0,255);
		endDrawing();
	}
}
// FOLDER NAME SHOULD NOT END WITH SLASH
void GenerateStreamingAssetsPaths(char* _streamingAssetsFolderName, char _isRelativeToData){
	free(streamingAssets);
	free(scriptFolder);

	streamingAssets = malloc(strlen(gbDataFolder)+strlen(_streamingAssetsFolderName)+2);
	scriptFolder = malloc(strlen(gbDataFolder)+strlen(_streamingAssetsFolderName)+strlen("/Scripts/")+1);
	streamingAssets[0]='\0';
	scriptFolder[0]='\0';
	if (_isRelativeToData){
		strcat(streamingAssets,gbDataFolder); 
		strcat(scriptFolder,gbDataFolder); 
	}
	//
	strcat(streamingAssets,_streamingAssetsFolderName);
	strcat(streamingAssets,"/");
	//
	strcat(scriptFolder,_streamingAssetsFolderName);
	strcat(scriptFolder,"/Scripts/");
}
void RunGameSpecificLua(){
	char _completedSpecificLuaPath[strlen(scriptFolder)+strlen("_GameSpecific.lua")+1];
	strcpy(_completedSpecificLuaPath,scriptFolder);
	strcat(_completedSpecificLuaPath,"_GameSpecific.lua");
	if (checkFileExist(_completedSpecificLuaPath)){
		printf("Game specific LUA found.\n");
		if(luaL_dofile(L,_completedSpecificLuaPath)==1){
			easyMessagef(1,"Error in Lua file, %s, %s",_completedSpecificLuaPath,lua_tostring(L,-1));
			printf("%s\n",lua_tostring(L,-1));
		}
	}
	// Just in case
	applyTextboxChanges(1);
}
void generateADVBoxPath(char* _passedStringBuffer, char* _passedSystemString){
	strcpy(_passedStringBuffer,streamingAssets);
	strcat(_passedStringBuffer,"GameSpecificAdvBox");
	strcat(_passedStringBuffer,_passedSystemString);
	strcat(_passedStringBuffer,".png");
}
void _loadDefaultADVBox(){
	#if GBPLAT != GB_3DS
		currentCustomTextbox = LoadEmbeddedPNG("assets/DefaultAdvBox.png");
	#else
		currentCustomTextbox = LoadEmbeddedPNG("assets/DefaultAdvBoxLowRes.png");
	#endif
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
		currentCustomTextbox = loadImage(_tempFilepathBuffer);
	}else{
		_loadDefaultADVBox();
	}
	applyTextboxChanges(1);
}
void LoadGameSpecificStupidity(){
	TryLoadMenuSoundEffect(NULL);
	RunGameSpecificLua();
}
void deleteIfExist(const char* _passedPath){
	if (checkFileExist(_passedPath)){
		remove(_passedPath);
	}
}
void resetSettings(){
	char* _fixedPath = fixPathAlloc("settings.noob",TYPE_DATA);
	deleteIfExist(_fixedPath);
	free(_fixedPath);
}
// This will assume that trying to create a directory that already exists is okay.
// Must call this function after paths are set up.
void createRequiredDirectories(){
	// These directories need to be made if it's CIA version
	#if GBPLAT == GB_3DS
		createDirectory("/3ds/");
		createDirectory("/3ds/data/");
		createDirectory("/3ds/data/HIGURASHI/");
	#endif
	createDirectory(gbDataFolder);
	createDirectory(saveFolder);
}
char loadHiguGameFolder(char* _chosenGameFolder){
	char* _fileWithPresetFilenamePath = easyCombineStrings(3,gamesFolder,_chosenGameFolder,"/includedPreset.txt");
	if (!checkFileExist(_fileWithPresetFilenamePath)){
		easyMessagef(1,"Invalid game folder. I know this because the includedPreset.txt does not exist. Did you remember to convert this folder before moving it?");
		free(_fileWithPresetFilenamePath);
		return 0;
	}
	activateHigurashiSettings();
	free(streamingAssets);
	streamingAssets = easyCombineStrings(3,gamesFolder,_chosenGameFolder,"/");
	free(scriptFolder);
	scriptFolder = easyCombineStrings(2,streamingAssets,"Scripts/");
	{ // read the preset filename
		crossFile* fp = crossfopen(_fileWithPresetFilenamePath,"rb");
		currentPresetFilename = easygetline(fp);
		crossfclose(fp);
		free(_fileWithPresetFilenamePath);
	}
	LoadGameSpecificStupidity();
	{ // load the preset
		char* _presentFullPath = easyCombineStrings(2,streamingAssets,currentPresetFilename);
		LoadPreset(_presentFullPath);
		free(_presentFullPath);
		// Does not load the savefile, I promise.
		loadHiguGame();
		// If there is no save game, start a new one at chapter 0
		// Otherwise, go to the navigation menu
		if (currentPresetChapter==-1){
			char* _options[]={"Start from beginning","Savegame Editor"};
			char _choice=showMenu(0,"NEW GAME",2,_options,0);
			if (_choice==1){
				currentPresetChapter=0;
				SaveGameEditor();
			}
			if (currentPresetChapter==-1){
				controlsEnd();
				currentPresetChapter=0;
				currentGameStatus=GAMESTATUS_MAINGAME;
			}else{
				currentGameStatus=GAMESTATUS_NAVIGATIONMENU;
			}
		}else{
			currentGameStatus=GAMESTATUS_NAVIGATIONMENU;
		}
	}
	return 1;
}

void setDefaultGame(char* _defaultGameFolderName){
	char _defaultGameSaveFilenameBuffer[strlen(saveFolder)+strlen("_defaultGame")+1];
	strcpy(_defaultGameSaveFilenameBuffer,saveFolder);
	strcat(_defaultGameSaveFilenameBuffer,"_defaultGame");

	FILE* fp;
	fp = fopen(_defaultGameSaveFilenameBuffer, "w");
	if (!fp){
		easyMessagef(1,"Failed to open default game save file, %s",_defaultGameSaveFilenameBuffer);
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
	dynamicScaleEnabled=1;
	advNamesPersist=0;
	recalculateMaxLines();
}
void activateHigurashiSettings(){
	currentlyVNDSGame=0;
	scriptUsesFileExtensions=0;
	bustsStartInMiddle=1;
	scriptScreenWidth=640;
	scriptScreenHeight=480;
	dynamicScaleEnabled=1;
	advNamesPersist=1;
	if (!advNamesSupported){
		advNamesSupported=1;
	}
	// regardless of the background, the busts act as if it's a 640x480 box chilling in the center
	{
		int _positionBoxW;
		int _positionBoxH;
		fitInBox(640,480,screenWidth,screenHeight,&_positionBoxW,&_positionBoxH);
		overrideOffsetVals=1;
		overrideXOffScale=_positionBoxW/(double)scriptScreenWidth;
		overrideYOffScale=_positionBoxH/(double)scriptScreenHeight;
		overrideBustOffX=easyCenter(_positionBoxW,screenWidth);
		overrideBustOffY=easyCenter(_positionBoxH,screenHeight);
	}
	applyTextboxChanges(1);
}
#if GBPLAT == GB_VITA
	char wasJustPressedSpecific(SceCtrlData _currentPad, SceCtrlData _lastPad, int _button){
		if (_currentPad.buttons & _button){
			if (!(_lastPad.buttons & _button)){
				return 1;
			}
		}
		return 0;
	}
	#if GBSND != GBSND_VITA
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
#if GBPLAT == GB_3DS
	char getIsCiaBuild(){
		return checkFileExist("romfs:/assets/star.png");
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
void writeLengthStringToFile(FILE* fp, const char* _stringToWrite){
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
		char* _tempAsString = nathanscriptVariableAsString(&(_listToSave[i].variable));
		fwrite(&(_correctVariableType),sizeof(char),1,fp);
		writeLengthStringToFile(fp,_listToSave[i].name);
		writeLengthStringToFile(fp,_tempAsString);
		free(_tempAsString);
	}
}
void saveGlobalVNDSVars(){
	// Resave the global variable file
	char* _globalsSaveFilePath = easyCombineStrings(2,saveFolder,"vndsGlobals");
	FILE* fp = fopen(_globalsSaveFilePath,"wb");
	unsigned char _tempFileFormat = VNDSGLOBALSSAVEFORMAT;
	fwrite(&_tempFileFormat,sizeof(unsigned char),1,fp);
	saveVariableList(fp,nathanscriptGlobalvarList,nathanscriptTotalGlobalvar);
	fclose(fp);
	free(_globalsSaveFilePath);
}
void skipLengthStringInFile(FILE* fp){
	short _tempFoundStrlen;
	fread(&_tempFoundStrlen,sizeof(short),1,fp);
	fseek(fp,_tempFoundStrlen,SEEK_CUR);
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
char* easyVNDSSaveName(int _slot){
	if (isEmbedMode){
		return easySprintf("%s%s%d",saveFolder,"embeddedGameSave",_slot);
	}else{
		return easySprintf("%s%s%d",streamingAssets,"sav",_slot);
	}
}
char* getHiguSavePath(){
	return easyVNDSSaveName(999);
}
char* oldHiguSavePath(const char* _passedPreset){
	return easyCombineStrings(2,saveFolder,_passedPreset);
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
char vndsNormalSave(char* _filename, char _saveSpot, char _saveThumb){
	if (nathanscriptCurrentOpenFile==NULL){
		return 1;
	}
	if (_saveSpot){
		FILE* fp = fopen(_filename,"wb");
		if (fp==NULL){
			return 1;
		}
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
		fwrite(&maxLines,sizeof(int),1,fp); //
		// Save our current line
		fwrite(&currentLine,sizeof(int),1,fp); //
		// Save the current messages
		for (i=0;i<maxLines;i++){
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
		//
		writeLengthStringToFile(fp,lastBGMFilename);
		fwrite(&lastBGMVolume,sizeof(int),1,fp);
		// format v2
		writeLengthStringToFile(fp,currentADVName);
		// Write game specific var list
		saveVariableList(fp,nathanscriptGamevarList,nathanscriptTotalGamevar); //
		fclose(fp);
	}
	// thumbnail saving can fail and return here. will not count as a return 1 error
	if (_saveThumb){
		char _thumbFilename[strlen(_filename)+7];
		strcpy(_thumbFilename,_filename);
		strcat(_thumbFilename,".thumb");
		// Renderer specific code for saving thumbnails
		#if GBPLAT == GB_VITA	
			int _destWidth = THUMBWIDTH;
			int _destHeight = THUMBHEIGHT;
			vita2d_texture* _smallTexture = vita2d_create_empty_texture_rendertarget(_destWidth,_destHeight,SCE_GXM_TEXTURE_FORMAT_A8B8G8R8);
	
			vita2d_pool_reset();
			vita2d_start_drawing_advanced(_smallTexture,0);
			Draw(0);
			vita2d_end_drawing();
	
			vita2d_wait_rendering_done();
			sceDisplayWaitVblankStart();
	
			// 
			BMP* testimg = BMP_Create(_destWidth,_destHeight,24);
			// Pixels stored in uint32_t
			void* _currentImageData = vita2d_texture_get_datap(_smallTexture);
			uint32_t y;
			for (y=0;y<_destHeight;++y) {
				uint32_t x;
				for (x=0;x< _destWidth;++x) {
					int _baseIndex = (x + _destWidth * y)*4;
					BMP_SetPixelRGB(testimg,x,y,((uint8_t*)_currentImageData)[_baseIndex],((uint8_t*)_currentImageData)[_baseIndex+1],((uint8_t*)_currentImageData)[_baseIndex+2]);
				}
			}
			BMP_WriteFile(testimg,_thumbFilename);
			BMP_Free(testimg);
			
			freeTexture(_smallTexture);
		#elif GBREND==GBREND_SDL
			// Draw the game a few times to ensure that SDL_RenderReadPixels gets the right screen
			int k;
			for (k=0;k<3;++k){
				startDrawing();
				Draw(0);
				endDrawing();
			}
			SDL_Surface* _saveSurface;
			#if SDL_BYTEORDER == SDL_BIG_ENDIAN
				_saveSurface=SDL_CreateRGBSurface(0,screenWidth,screenHeight,32,0xff000000,0x00ff0000,0x0000ff00,0x000000ff);
			#else
				_saveSurface=SDL_CreateRGBSurface(0,screenWidth,screenHeight,32,0x000000ff,0x0000ff00,0x00ff0000,0xff000000);
			#endif
			if (_saveSurface!=NULL){
				SDL_ClearError();
				SDL_LockSurface(_saveSurface);
				SDL_Rect _readRect;
				_readRect.x=fixX(0);
				_readRect.y=fixY(0);
				_readRect.w=screenWidth;
				_readRect.h=screenHeight;
				if (SDL_RenderReadPixels(mainWindowRenderer,&_readRect,_saveSurface->format->format,_saveSurface->pixels,_saveSurface->pitch)==0){
					if (SDL_SaveBMP(_saveSurface,_thumbFilename)!=0){
						printf("Error saving surface\n");
					}
					SDL_UnlockSurface(_saveSurface);
				}else{
					printf("error reading pixels: %s\n",SDL_GetError());
				}
				SDL_FreeSurface(_saveSurface);
			}else{
				printf("error making surface\n");
			}
		#endif
	}
	return 0;
}
void vndsNormalLoad(char* _filename, char _startLoadedGame){
	FILE* fp = fopen(_filename,"rb");
	unsigned char _readFileFormat;
	fread(&_readFileFormat,sizeof(unsigned char),1,fp); //
	if (!validVNDSSaveFormat(_readFileFormat)){
		easyMessagef(1,"Bad file format version. %d",_readFileFormat);
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
	if (_readMaxLines>maxLines){ // Skip the lines we can't hold
		for (i=0;i<_readMaxLines-maxLines;++i){
			free(readLengthStringFromFile(fp));
		}
		_readMaxLines=maxLines;
	}else{ // Zero the extra space lines
		for (i=_readMaxLines;i<maxLines;++i){
			if (currentMessages[i]){
				free(currentMessages[i]);
				currentMessages[i]=NULL;
				free(messageProps[i]);
			}
		}
	}
	// Read the lines
	for (i=0;i<_readMaxLines;i++){
		currentMessages[i] = readLengthStringFromFile(fp);
		if (currentMessages[i][0]=='\0'){
			free(currentMessages[i]);
			currentMessages[i]=NULL;
		}else{
			messageProps[i] = calloc(1,strlen(currentMessages[i])*sizeof(int32_t));
		}
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
			DrawBustshot(i,_tempReadFilename,_tempReadX,_tempReadY,i,0,0,255);
		}
		free(_tempReadFilename);
	}

	char* _foundBGMFilename = readLengthStringFromFile(fp);
	fread(&lastBGMVolume,sizeof(int),1,fp);
	if (strlen(_foundBGMFilename)!=0){
		PlayBGM(_foundBGMFilename,lastBGMVolume,0);
	}
	free(_foundBGMFilename);

	if (_readFileFormat>=2){
		char* _foundADVName = readLengthStringFromFile(fp);
		if (strlen(_foundADVName)!=0){
			currentADVName=_foundADVName;
			if (!advNamesSupported){
				advNamesSupported=1;
			}
		}else{
			free(_foundADVName);
		}
	}

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

	controlsReset();

	if (gameTextDisplayMode==TEXTMODE_ADV && dynamicAdvBoxHeight){
		updateDynamicADVBox(-2,-1);
	}

	// Open
	nathanscriptCurrentOpenFile = crossfopen(_tempLoadedFilename,"rb");
	crossfseek(nathanscriptCurrentOpenFile,_readFilePosition,CROSSFILE_START);
	if (_startLoadedGame){
		// Don't instantly proceed
		endType=Line_WaitForInput;
		outputLineWait();
		// Now we can
		nathanscriptLowDoFile(nathanscriptCurrentOpenFile,inBetweenVNDSLines);
		// Must be done manually because low do file. We must use the global variable here because the script may have changed
		crossfclose(nathanscriptCurrentOpenFile);
	}
}
void _textboxTransition(char _isOn, int _totalTime){
	if (MessageBoxEnabled!=_isOn && !isSkipping){
		unsigned char _oldMessageBoxAlpha = currentBoxAlpha;
		u64 _startTime = getMilli();
		u64 _curTime;
		while ((_curTime = getMilli())<_startTime+_totalTime){
			currentBoxAlpha=partMoveFills(_curTime,_startTime,_totalTime,_oldMessageBoxAlpha);
			if (!_isOn){
				currentBoxAlpha=_oldMessageBoxAlpha-currentBoxAlpha;
			}
			startDrawing();
			drawAdvanced(1,1,1,1,1,0); // Don't draw text
			unsigned char _curTextAlpha = partMoveFills(_curTime,_startTime,_totalTime,255);
			DrawMessageText(_isOn ? _curTextAlpha : 255-_curTextAlpha,-1,-1);
			endDrawing();
		}
		currentBoxAlpha = _oldMessageBoxAlpha;
	}
	MessageBoxEnabled=_isOn;
}
void hideTextboxTimed(int _milli){
	_textboxTransition(0,_milli);
}
void hideTextbox(){
	hideTextboxTimed(TEXTBOXFADEOUTTIME);
}
void showTextbox(){
	_textboxTransition(1,TEXTBOXFADEINTIME);
}
#if GBPLAT == GB_VITA
	void invertImage(vita2d_texture* _passedImage, signed char _doInvertAlpha){
		if (_passedImage==NULL){
			easyMessagef(1,"Null image passed to invertImage");
			return;
		}
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
	void invertImage(crossTexture* _passedImage, signed char _doInvertAlpha){
		printf("Invert image at %p. Alpha change: %d\n",_passedImage,_doInvertAlpha);
	}
#endif
void applyNegative(int _actionTime, signed char _waitforcompletion){
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
	currentFilterType=FILTERTYPE_INACTIVE;
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
				easyMessagef(1,"LUA_ERRSYNTAX");
			break;
			case LUA_ERRMEM:
				easyMessagef(1,"LUA_ERRMEM");
			break;
			case LUA_ERRGCMM:
				easyMessagef(1,"LUA_ERRGCMM");
			break;
			case LUA_ERRFILE:
				easyMessagef(1,"LUA_ERRFILE");
			break;
			case LUA_ERRRUN:
				easyMessagef(1,"LUA_ERRRUN");
			break;
			case LUA_ERRERR:
				easyMessagef(1,"LUA_ERRERR");
			break;
			default:
				easyMessagef(1,"lua error %d",_loadResult);
			break;
		}
		return 1;
	}
	return 0;
}
void safeVNDSSaveMenu(){
	int _chosenSlot = vndsSaveSelector(1);
	if (_chosenSlot!=-1){
		char* _savedPath = easyVNDSSaveName(_chosenSlot);
		if (vndsNormalSave(_savedPath,1,1)){
			easyMessagef(1,"Failed to save to %s",_savedPath);
		}
		free(_savedPath);
	}
}
void resetADVNameColor(){
	advNameR=255;
	advNameG=255;
	advNameB=255;
}
// pass NULL to clear
void setADVName(char* _newName){
	resetADVNameColor();
	if (_newName!=NULL){
		if (!advNamesSupported){
			advNamesSupported=1;
			applyTextboxChanges(1);
		}
		int _cachedStrlen=strlen(_newName);
		char _actualNameBuff[strlen(_newName)+1];
		_actualNameBuff[0]='\0';
		int i;
		// Search for color markup
		// //<color=#5ec69a>Mion</color>
		for (i=0;i<_cachedStrlen;++i){
			if (_newName[i]=='<'){
				if (strncmp(&(_newName[i]),COLORMARKUPSTART,strlen(COLORMARKUPSTART))==0){
					i+=strlen(COLORMARKUPSTART);
					if (i+6+strlen(COLORMARKUPEND)<_cachedStrlen){
						readRGBString(&_newName[i],&advNameR,&advNameG,&advNameB);
						i+=6;
						char* _endPos = strstr(_newName,COLORMARKUPEND);
						if (_newName[i]=='>' && _endPos!=NULL){ 
							_endPos[0]='\0';
							strcat(_actualNameBuff,&(_newName[++i]));
							for (;_newName[i];++i); // Bring loop variable to the null char that we just set
							i+=(strlen(COLORMARKUPEND)-1);
							_endPos[0]=COLORMARKUPEND[0];
						}else{ // If the tag end isn't in the correct spot, bail
							resetADVNameColor();
							i-=(6+strlen(COLORMARKUPSTART));
						}
					}
				}
			}
		}
		changeMallocString(&currentADVName,strlen(_actualNameBuff)!=0 ? _actualNameBuff : _newName);
	}else{
		currentADVNameIm=-1;
		changeMallocString(&currentADVName,NULL);
	}
}
/*
=================================================
*/
void scriptDisplayWindow(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	showTextbox();
}
void scriptClearMessage(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	currentLine=0;
	ClearMessageArray(1);
}
void scriptOutputLine(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	int _nameIndex=playerLanguage ? 2 : 0;
	if (_passedArguments[_nameIndex].variableType==NATHAN_TYPE_STRING){ // If an English adv name was passed
		setADVName(nathanvariableToString(&_passedArguments[_nameIndex]));
	}else if (_passedArguments[_nameIndex].variableType==NATHAN_TYPE_FLOAT){
		int _desireIndex = nathanvariableToInt(&_passedArguments[_nameIndex]);
		if (_desireIndex<advImageNameCount){
			currentADVNameIm=_desireIndex;
		}else if (shouldShowWarnings()){
			easyMessagef(1,"image adv name index bad. %d/%d",_desireIndex,advImageNameCount-1);
		}
	}else if (shouldShowADVNames() && advNamesPersist==0){
		setADVName(NULL);
	}
	int _textIndex=playerLanguage ? 3 : 1;
	if (_passedArguments[_textIndex].variableType!=NATHAN_TYPE_NULL){
		if (strcmp(nathanvariableToString(&_passedArguments[_textIndex]),"0")==0){
			return;
		}
		OutputLine((unsigned const char*)nathanvariableToString(&_passedArguments[_textIndex]),nathanvariableToInt(&_passedArguments[4]),0);
		outputLineWait();
	}
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
}
//
void scriptWait(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	if (isSkipping){
		startDrawing();
		Draw(MessageBoxEnabled);
		endDrawing();
	}else{
		u64 _endTime=getMilli()+nathanvariableToInt(&_passedArguments[0]);
		while(_endTime>getMilli()){
			controlsStart();
			if(proceedPressed()){
				_endTime=getMilli();
			}
			Update();
			controlsEnd();
			startDrawing();
			Draw(MessageBoxEnabled);
			endDrawing();
		}
	}
}
// filename, filter, unknown, unknown, time
void scriptDrawSceneWithMask(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	DrawScene(nathanvariableToString(&_passedArguments[0]),nathanvariableToInt(&_passedArguments[4]));
}
// filename
// fadein
void scriptDrawScene(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	DrawScene(nathanvariableToString(&_passedArguments[0]),nathanvariableToInt(&_passedArguments[1]));
}
// Placeholder for unimplemented function
void scriptNotYet(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	printf("An unimplemented Lua function was just executed.\n");
}
void scriptNotYetFlash(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	scriptNotYet(_passedArguments,_numArguments,_returnedReturnArray,_returnArraySize);
	startDrawing();
	drawRectangle(0,0,screenWidth,screenHeight,221,80,225,255);
	endDrawing();
	wait(30);
}
// Fist arg seems to be a channel arg.
	// Usually 1 for msys
	// Usually 2 for lsys
// Second arg is path in BGM folder without extension
// Third arg is volume. 128 seems to be average. I can hardly hear 8 with computer volume on 10.
// Fourth arg is unknown
void scriptPlayBGM(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	PlayBGM(nathanvariableToString(&_passedArguments[1]),nathanvariableToInt(&_passedArguments[2]),nathanvariableToInt(&_passedArguments[0]));
}
// Some int argument
// Maybe music slot
void scriptStopBGM(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	char _slot = nathanvariableToInt(&_passedArguments[0]);
	if (currentMusic[_slot]!=NULL){
		StopBGM(_slot);
		FreeBGM(_slot);
	}
}
#if GBSND == GBSND_3DS
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
			#if GBSND == GBSND_SOLOUD
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
	DrawBustshot(nathanvariableToInt(&_passedArguments[0]), nathanvariableToString(&_passedArguments[1]), nathanvariableToInt(&_passedArguments[4]), nathanvariableToInt(&_passedArguments[5]), nathanvariableToInt(&_passedArguments[12]), nathanvariableToInt(&_passedArguments[13]), nathanvariableToBool(&_passedArguments[14]), nathanvariableToInt(&_passedArguments[11]) ? 0 : 255);
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
	DrawBustshot(nathanvariableToInt(&_passedArguments[0]), nathanvariableToString(&_passedArguments[1]), nathanvariableToInt(&_passedArguments[2]), nathanvariableToInt(&_passedArguments[3]), nathanvariableToInt(&_passedArguments[13]), nathanvariableToInt(&_passedArguments[14]), nathanvariableToBool(&_passedArguments[15]), nathanvariableToInt(&_passedArguments[12]) ? 0 : 255);
}
// slot, filename, time, waitforcompletion
void scriptChangeBustshot(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){	
	int _slot = nathanvariableToInt(&_passedArguments[0]);
	if (_slot>=maxBusts){
		fprintf(stderr,"%d is greater than max busts. calling ChangeBustshot on nonexistent bustshot?",_slot);
		return;
	}
	// activate a transform fadein
	crossTexture* _oldTexture = Busts[_slot].image;
	Busts[_slot].image=NULL;
	_freeBustImage(&Busts[_slot]);
	Busts[_slot].image=loadImageOrCache(nathanvariableToString(&_passedArguments[1]),&Busts[_slot].relativeFilename);
	Busts[_slot].transformTexture = _oldTexture;
	Busts[_slot].bustStatus = BUST_STATUS_TRANSFORM_FADEIN;
	Busts[_slot].fadeStartTime=getMilli();
	Busts[_slot].fadeEndTime=Busts[_slot].fadeStartTime+nathanvariableToInt(&_passedArguments[2]);
	if (nathanvariableToBool(&_passedArguments[3])){
		while (Busts[_slot].bustStatus!=BUST_STATUS_NORMAL){
			controlsStart();
			Update();
			startDrawing();
			Draw(MessageBoxEnabled);
			endDrawing();
			if (proceedPressed()){
				Busts[_slot].fadeEndTime=Busts[_slot].fadeStartTime+1;
			}
			controlsEnd();
		}
	}
}
void scriptSetValidityOfInput(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	inputValidity=(nathanvariableToBool(&_passedArguments[0])==1);	
}
// Fadeout time
// Wait for completely fadeout
void scriptFadeAllBustshots(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	FadeAllBustshots(nathanvariableToInt(&_passedArguments[0]),nathanvariableToBool(&_passedArguments[1]));
}
void scriptDisableWindow(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	hideTextbox();
}
void scriptFadeBustshotWithFiltering(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	FadeBustshot(nathanvariableToInt(&_passedArguments[0]),nathanvariableToInt(&_passedArguments[6]),nathanvariableToBool(&_passedArguments[7]));
}
//FadeBustshot( 2, FALSE, 0, 0, 0, 0, 0, TRUE );
//FadeBustshot( SLOT, MOVE, X, Y, UNKNOWNA, UNKNOWNB, FADETIME, WAIT );
void scriptFadeBustshot(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	FadeBustshot(nathanvariableToInt(&_passedArguments[0]),nathanvariableToInt(&_passedArguments[6]),nathanvariableToBool(&_passedArguments[7]));
}
// Slot, file, volume
void scriptPlaySE(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	if (isSkipping==0 && seVolume>0){
		GenericPlayGameSound(nathanvariableToInt(&_passedArguments[0]),nathanvariableToString(&_passedArguments[1]),nathanvariableToInt(&_passedArguments[2]),PREFER_DIR_SE,seVolume);
	}
}
// PlayVoice(channel, filename, volume)
void scriptPlayVoice(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	if (isSkipping==0 && (hasOwnVoiceSetting==1 ? voiceVolume : seVolume)>0){
		GenericPlayGameSound(nathanvariableToInt(&_passedArguments[0]),nathanvariableToString(&_passedArguments[1]),nathanvariableToInt(&_passedArguments[2]),PREFER_DIR_VOICE, hasOwnVoiceSetting==1 ? voiceVolume : seVolume);
	}
}
// Loads a script file
void scriptCallScript(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	const char* filename = nathanvariableToString(&_passedArguments[0]);

	char* tempstringconcat = easyCombineStrings(3,scriptFolder,filename,".txt");
	char tempstring2[strlen(tempstringconcat)+1];
	strcpy(tempstring2,tempstringconcat);
	free(tempstringconcat);

	if (checkFileExist(tempstring2)==1){
		printf("Do script %s\n",tempstring2);
		RunScript("",tempstring2,0);
	}else{
		printf("Failed to find script\n");
	}
}
// "bg_166", 7, 200, 0
void scriptChangeScene(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	DrawScene(nathanvariableToString(&_passedArguments[0]),0);
}
// DrawSprite(slot, filename, ?, x, y, ?, ?, ?, ?, ?, ?, ?, ?, LAYER, FADEINTIME, WAITFORFADEIN)
// x is relative to -320
	// y is relative to -240???
	// DrawSprite(slot, filename, ?, x, y, ?, ?, ?, ?, ?, ?, ?, ?, LAYER, FADEINTIME, WAITFORFADEIN)
// DrawSprite(slot, filename, ?, x, y, z, originx, originy, angle, ignored, ignored, style(???,ignored), alpha, layer, wait, waitForcompletion)
void scriptDrawSprite(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	//void DrawBustshot(unsigned char passedSlot, char* _filename, int _xoffset, int _yoffset, int _layer, int _fadeintime, int _waitforfadein, int _isinvisible){
	int _x=nathanvariableToInt(&_passedArguments[3]);
	int _y=nathanvariableToInt(&_passedArguments[4]);
	int _originX=nathanvariableToInt(&_passedArguments[6]);
	int _originY=nathanvariableToInt(&_passedArguments[7]);
	adjustForOriginPos(&_x,&_y,_originX,_originY);
	fixScriptSpritePos(&_x,&_y);
	int _slot=nathanvariableToInt(&_passedArguments[0]);
	drawBustshotAdvanced(_slot,nathanvariableToString(&_passedArguments[1]),_x,_y,nathanvariableToInt(&_passedArguments[13]), nathanvariableToInt(&_passedArguments[14]),nathanvariableToBool(&_passedArguments[15]),255,-1,-1,0);
	Busts[_slot].originXForAdjust=_originX;
	Busts[_slot].originYForAdjust=_originY;
}
//MoveSprite(slot, destinationx, destinationy, ?, ?, ?, ?, ?, timeittakes, waitforcompletion)
	// MoveSprite(5,-320,-4500,0,0,0,0,0,101400, TRUE)
// MoveSprite(slot,x,y,z,angle,alpha(?),type(?),time,waitforcompeltion)
void scriptMoveSprite(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	int _totalTime = nathanvariableToInt(&_passedArguments[8]);
	int _passedSlot = nathanvariableToInt(&_passedArguments[0]);
	int _xDest=nathanvariableToInt(&_passedArguments[1]);
	int _yDest=nathanvariableToInt(&_passedArguments[2]);

	adjustForOriginPos(&_xDest,&_yDest,Busts[_passedSlot].originXForAdjust,Busts[_passedSlot].originYForAdjust);
	fixScriptSpritePos(&_xDest,&_yDest);
	
	if (_totalTime!=0){
		int _xTengoQue = _xDest-Busts[_passedSlot].xOffset;
		int _yTengoQue = _yDest-Busts[_passedSlot].yOffset;
		Busts[_passedSlot].bustStatus = BUST_STATUS_SPRITE_MOVE;
		Busts[_passedSlot].diffMoveTime=_totalTime;
		Busts[_passedSlot].startMoveTime=getMilli();
		Busts[_passedSlot].diffXMove = _xTengoQue;
		Busts[_passedSlot].diffYMove = _yTengoQue;
		Busts[_passedSlot].startXMove = Busts[_passedSlot].xOffset;
		Busts[_passedSlot].startYMove = Busts[_passedSlot].yOffset;
	}else{
		Busts[_passedSlot].xOffset=_xDest;
		Busts[_passedSlot].yOffset=_yDest;
	}
	if (Busts[_passedSlot].curAlpha!=255){
		Busts[_passedSlot].curAlpha=255;
		Busts[_passedSlot].destAlpha=255;
	}
	if (nathanvariableToBool(&_passedArguments[9])){
		while(Busts[_passedSlot].bustStatus!=BUST_STATUS_NORMAL){
			controlsStart();
			if (proceedPressed()){
				Busts[_passedSlot].diffMoveTime=0;
			}
			controlsEnd();
			Update();
			startDrawing();
			Draw(MessageBoxEnabled);
			endDrawing();
		}
	}
}
//FadeSprite(slot, time, waitfrocompletion)
		// FadeSprite(5,700,FALSE)
void scriptFadeSprite(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	FadeBustshot(nathanvariableToInt(&_passedArguments[0]),nathanvariableToInt(&_passedArguments[1]),nathanvariableToBool(&_passedArguments[2]));	
	return;
}
// DrawSpriteFixedSize(slot,filename,???,x,y,z(bigger->smaller)(ignoredbyme),originx,originy,destw,desth,angle(ignoredbyme),ignored,ignored,style(???.ignored),alpha(0isopaque),layer,time,waitForCompletion)
void scriptDrawSpriteFixedSize(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	int _slot= nathanvariableToInt(&_passedArguments[0]);
	char* _filename = nathanvariableToString(&_passedArguments[1]);

	int _destX = nathanvariableToInt(&_passedArguments[3]);
	int _destY = nathanvariableToInt(&_passedArguments[4]);
	int _originX = nathanvariableToInt(&_passedArguments[6]);
	int _originY = nathanvariableToInt(&_passedArguments[7]);

	int _w = nathanvariableToInt(&_passedArguments[8]);
	int _h = nathanvariableToInt(&_passedArguments[9]);

	//int _angle = nathanvariableToInt(&_passedArguments[10]);
	
	// the higher the alpha, he more invisible. also it's on a 256 scale.
	int _alpha = (256-nathanvariableToInt(&_passedArguments[14]));
	if (_alpha!=256){
		_alpha=(_alpha/(double)256)*255;
	}
	_alpha=limitNum(_alpha,0,255);
	
	int _layer = nathanvariableToInt(&_passedArguments[15]);
	int _time = nathanvariableToInt(&_passedArguments[16]);
	char _waitForCompletion = nathanvariableToBool(&_passedArguments[17]);
	
	adjustForOriginPos(&_destX,&_destY,_originX,_originY);
	fixScriptSpritePos(&_destX,&_destY);
	
	drawBustshotAdvanced(_slot,_filename,_destX,_destY,_layer,_time,_waitForCompletion,_alpha,_w,_h,0);
	Busts[_slot].originXForAdjust=_originX;
	Busts[_slot].originYForAdjust=_originY;
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
	controlsReset();
	int _totalOptions = nathanvariableToInt(&_passedArguments[0]);
	char* noobOptions[_totalOptions];
	int i;
	for (i=0;i<_totalOptions;i++){
		noobOptions[i] = strdup(nathanvariableGetArray(&_passedArguments[1],i));
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
		controlsStart();
		char _oldIndex = _choice;
		_choice = menuControls(_choice,0,_totalOptions-1);
		// Init scrolling if we changed menu index or just started the loop
		if (_needScrolling==-1 || _oldIndex!=_choice){
			_scrollOffset=0;
			if (textWidth(normalFont,noobOptions[_choice])>screenWidth-MENUOPTIONOFFSET-MENUCURSOROFFSET){
				_needScrolling=1;
				_lastScrollTime = getMilli();
				_isScrollingText=0;
				_scrollRight=1;
			}else{
				_needScrolling=0;
			}
		}

		// Process scrolling
		if (_needScrolling){
			if (_isScrollingText){
				if (getMilli()>_lastScrollTime+CHOICESCROLLTIMEINTERVAL){
					char _oldDirection = _scrollRight;
					if (_scrollRight){
						if (textWidth(normalFont,&(noobOptions[_choice][_scrollOffset]))>screenWidth-MENUOPTIONOFFSET-MENUCURSOROFFSET){
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
						_lastScrollTime = getMilli()+CHOICESCROLLTIMEOFFSET-CHOICESCROLLTIMEINTERVAL; // Trick the program into waiting CHOICESCROLLTIMEOFFSET before it goes back the other way
					}else{
						_lastScrollTime = getMilli();
					}
				}
			}else{
				if (getMilli()>_lastScrollTime+CHOICESCROLLTIMEOFFSET){
					_isScrollingText=1;
					_lastScrollTime = 0; // So we'll scroll next frame
				}
			}
		}
		updateControlsGeneral();
		if (wasJustPressed(BUTTON_A)){
			lastSelectionAnswer = _choice;
			break;
		}
		controlsEnd();
		startDrawing();
		Draw(0);
		DrawMessageBox(TEXTMODE_NVL,preferredBoxAlpha);
		for (i=0;i<_totalOptions;i++){
			if (_choice!=i){
				drawText(MENUOPTIONOFFSET,i*currentTextHeight,noobOptions[i]);
			}
		}
		drawText(MENUOPTIONOFFSET+MENUCURSOROFFSET,_choice*currentTextHeight,&(noobOptions[_choice][_scrollOffset]));
		drawText(MENUCURSOROFFSET*2,_choice*currentTextHeight,MENUCURSOR);

		endDrawing();
	}

	if (currentlyVNDSGame){
		nathanscriptAdvanceLine(); // Fix what we did at the start of the function
	}

	// Free strings that were made with calloc earlier
	for (i=0;i<_totalOptions;i++){
		free(noobOptions[i]);
	}
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
		easyMessagef(1,"Unknown LoadValueFromLocalWork, %s, please report.",_wordWant);
	}
}
// Calls a function that was made in a script
void scriptCallSection(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	char* buf = easyCombineStrings(2,nathanvariableToString(&_passedArguments[0]),"()");
	if (luaL_loadstring(L,buf)){
		easyMessagef(1,"luaL_loadstring failed: %s",buf);
	}else{
		int _res = lua_pcall(L, 0, LUA_MULTRET, 0);
		if (_res!=LUA_OK){
			DisplaypcallError(_res,buf);
		}
	}
	free(buf);
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
		currentFilterType = FILTERTYPE_EFFECTCOLORMIX;
	}
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
	currentFilterType = FILTERTYPE_INACTIVE;
}
// This command is used so unoften that I didn't bother to make it look good.
// FadeBG( 3000, TRUE );
void scriptFadeBG(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	if (currentBackground!=NULL){
		freeTexture(currentBackground);
		currentBackground=NULL;
	}
}
void scriptSetAllTextColor(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	char _colorBuff[3];
	_colorBuff[0]=nathanvariableToInt(&_passedArguments[1]);
	_colorBuff[1]=nathanvariableToInt(&_passedArguments[2]);
	_colorBuff[2]=nathanvariableToInt(&_passedArguments[3]);
	// change color of all text on screen
	int i;
	for (i=0;i<maxLines;i++){
		if (currentMessages[i]!=NULL){
			int _len=strlen(currentMessages[i]);
			int j;
			for (j=0;j<_len;++j){
				memcpy(&messageProps[i][j],_colorBuff,3);
				setPropBit(&messageProps[i][j],TEXTPROP_COLORED);
			}
		}
	}
	nextTextR=_colorBuff[0];
	nextTextG=_colorBuff[1];
	nextTextB=_colorBuff[2];
}
// intended behavior in the original game: generate a whole number from [0,<passed number>-1]
// actual behavior in the original game: generate a whole number from [0,<passedNumber>-2] with a near-zero chance of generating <passedNumber>-1
// my behavior: intended behavior plus additional skew because lazy.
void scriptHigurashiGetRandomNumber(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	int _passed = nathanvariableToInt(&_passedArguments[0]);
	int _ret;
	if (_passed<=1){ // invalid
		_ret=0;
	}else{
		_ret=(rand() % _passed);
	}
	makeNewReturnArray(_returnedReturnArray,_returnArraySize,1);
	nathanvariableArraySetFloat(*_returnedReturnArray,0,_ret);
}
void scriptHideTextboxAdvanced(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	hideTextboxTimed(nathanvariableToInt(&_passedArguments[0]));
}
// top left X, top left Y, width, height, ignored bool, int time, bool waitForCompletion
void scriptEnlargeScreen(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	enlargeScreen(nathanvariableToInt(&_passedArguments[0]),nathanvariableToInt(&_passedArguments[1]),nathanvariableToInt(&_passedArguments[2]),nathanvariableToInt(&_passedArguments[3]),nathanvariableToInt(&_passedArguments[5]),nathanvariableToBool(&_passedArguments[6]));
}
// layer,_ignored,_ignored
void scriptTerminateShakingOfBustshot(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	freeBustShakeInfo(nathanvariableToInt(&_passedArguments[0]));
}
// _ignored, _ignored
void scriptTerminateShakingOfWindow(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	struct shakeInfo* s=curUIShake;
	curUIShake=NULL;
	safeFreeShakeInfo(s);
}
// _ignored, _ignored
void scriptTerminateShakingOfAllObjects(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	struct shakeInfo* s=curBackgroundShake;
	curBackgroundShake=NULL;
	safeFreeShakeInfo(s);
	for (int i=0;i<maxBusts;++i){
		freeBustShakeInfo(i);
	}
}
// slot,speed(newformula),range,drag,direction,loops,waitforcompletion
void scriptStartShakingOfBustshot(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	int _slot = nathanvariableToInt(&_passedArguments[0]);
	freeBustShakeInfo(_slot);
	Busts[_slot].curShake=makeShakeInfo(nathanvariableToInt(&_passedArguments[1]),nathanvariableToInt(&_passedArguments[2]),nathanvariableToInt(&_passedArguments[3]),nathanvariableToInt(&_passedArguments[4]),nathanvariableToInt(&_passedArguments[5]),getMilli());
	if (nathanvariableToBool(&_passedArguments[6])){
		waitForShakeEnd(&Busts[_slot].curShake);
	}
}
// speed(newformula),range,drag,direction,loops,waitforcompletion
void scriptStartShakingOfWindow(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	scriptTerminateShakingOfWindow(NULL,0,NULL,NULL);
	curUIShake=makeShakeInfo(nathanvariableToInt(&_passedArguments[0]),nathanvariableToInt(&_passedArguments[1]),nathanvariableToInt(&_passedArguments[2]),nathanvariableToInt(&_passedArguments[3]),nathanvariableToInt(&_passedArguments[4]),getMilli());
	if (nathanvariableToBool(&_passedArguments[5])){
		waitForShakeEnd(&curUIShake);
	}
}
// speed(newformula),range,drag,direction,loops,waitforcompletion
void scriptStartShakingOfAllObjects(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	scriptTerminateShakingOfAllObjects(NULL,0,NULL,NULL);
	struct shakeInfo* s = makeShakeInfo(nathanvariableToInt(&_passedArguments[0]),nathanvariableToInt(&_passedArguments[1]),nathanvariableToInt(&_passedArguments[2]),nathanvariableToInt(&_passedArguments[3]),nathanvariableToInt(&_passedArguments[4]),getMilli());
	for (int i=0;i<maxBusts;++i){
		if (Busts[i].isActive){
			Busts[i].curShake=s;
		}
	}
	curBackgroundShake=s;
	if (nathanvariableToBool(&_passedArguments[5])){
		waitForShakeEnd(&curBackgroundShake);
	}
}
//
void scriptMoveBust(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	MoveBustSlot(nathanvariableToInt(&_passedArguments[0]),nathanvariableToInt(&_passedArguments[1]));
}
void scriptGetScriptLine(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	makeNewReturnArray(_returnedReturnArray,_returnArraySize,1);
	nathanvariableArraySetFloat(*_returnedReturnArray,0,currentScriptLine);
}
void scriptDebugFile(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	WriteToDebugFile("%s\n",nathanvariableToString(&_passedArguments[0]));
}
void scriptOptionsEnableVoiceSetting(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	if (_numArguments==0){
		hasOwnVoiceSetting=1;
	}else{
		hasOwnVoiceSetting = nathanvariableToBool(&_passedArguments[0]);
	}
}
void scriptOptionsSetTextMode(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	textDisplayModeOverriden=1;
	gameTextDisplayMode = nathanvariableToInt(&_passedArguments[0]);
	applyTextboxChanges(1);
}
void scriptLoadADVBox(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	loadADVBox();
}
void scriptOptionsSetTips(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	gameHasTips=nathanvariableToBool(&_passedArguments[0]);
}
void scriptOptionsCanChangeBoxAlpha(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	canChangeBoxAlpha = nathanvariableToBool(&_passedArguments[0]);
	currentBoxAlpha=255;
}

// Settings menu override
EASYLUAINTSETFUNCTION(oMenuQuit,forceShowQuit)
EASYLUAINTSETFUNCTION(oMenuVNDSSettings,forceShowVNDSSettings)
EASYLUAINTSETFUNCTION(oMenuVNDSSave,forceShowVNDSSave)
EASYLUAINTSETFUNCTION(oMenuRestartBGM,forceShowRestartBGM)
EASYLUAINTSETFUNCTION(oMenuArtLocations,forceArtLocationSlot)
EASYLUAINTSETFUNCTION(oMenuVNDSBustFade,forceVNDSFadeOption)
EASYLUAINTSETFUNCTION(oMenuDebugButton,forceDebugButton)
EASYLUAINTSETFUNCTION(oMenuTextboxMode,forceTextBoxModeOption) // ADV or NVL
EASYLUAINTSETFUNCTION(oMenuTextOverBG,forceTextOverBGOption) // text only over background
EASYLUAINTSETFUNCTION(oMenuFontSize,forceFontSizeOption)
// Manually set the options if you've chosen to disable the menu option
EASYLUAINTSETFUNCTION(textOnlyOverBackground,textOnlyOverBackground);
EASYLUAINTSETFUNCTION(dynamicAdvBoxHeight,dynamicAdvBoxHeight);
EASYLUAINTSETFUNCTIONPOSTCALL(advboxHeight,advboxHeight,applyTextboxChanges(1);)
EASYLUAINTSETFUNCTION(setADVNameSupport,advNamesSupported)
EASYLUAINTSETFUNCTION(advNamesPersist,advNamesPersist)
// Some properties
EASYLUAINTSETFUNCTIONPOSTCALL(setTextboxTopPad,textboxTopPad,applyTextboxChanges(1);)
EASYLUAINTSETFUNCTIONPOSTCALL(setTextboxBottomPad,textboxBottomPad,applyTextboxChanges(1);)
EASYLUAINTSETFUNCTIONPOSTCALL(setADVNameImageHeight,advNameImHeight,applyTextboxChanges(1);)
// get
EASYLUAINTGETFUNCTION(getTextDisplayMode,gameTextDisplayMode)
	
#define MAXIMAGECHOICEW (screenWidth*.85)
#define PREFERREDIMAGECHOICESONSCREEN 7
#define MAXIMAGECHOICEH (screenHeight*(1/(double)PREFERREDIMAGECHOICESONSCREEN))
#define MINDRAGRATIO ((1)/(double)16)
// normal image 1, hover image 1, select image 1, normal image 2, hover image 2, select image 2
void scriptImageChoice(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	int i;
	// Load images
	crossTexture** _normalImages;
	crossTexture** _hoverImages;
	crossTexture** _selectImages;
	int _numChoices = _numArguments/3;
	_normalImages = malloc(sizeof(crossTexture*)*_numChoices);
	_hoverImages = malloc(sizeof(crossTexture*)*_numChoices);
	_selectImages = malloc(sizeof(crossTexture*)*_numChoices);
	for (i=0;i<_numChoices;i++){
		_normalImages[i] = safeLoadGameImage(nathanvariableToString(&_passedArguments[i*3+1-1]),graphicsLocation,scriptUsesFileExtensions);
		_hoverImages[i] = safeLoadGameImage(nathanvariableToString(&_passedArguments[i*3+2-1]),graphicsLocation,scriptUsesFileExtensions);
		_selectImages[i] = safeLoadGameImage(nathanvariableToString(&_passedArguments[i*3+3-1]),graphicsLocation,scriptUsesFileExtensions);
	}
	// Get image size
	int _choiceW;
	int _choiceH;
	fitInBox(getTextureWidth(_normalImages[0]),getTextureHeight(_normalImages[0]),MAXIMAGECHOICEW,MAXIMAGECHOICEH,&_choiceW,&_choiceH);
	int _choicePad = _choiceH/2;
	// controls
	int _startTouchY;
	int _startTouchX;
	char _isDrag;
	char _canDrag;
	char _userChoice;
	int _isHoldSelect=-1;
	int _lastTouchX;
	int _lastTouchY;
	if (gbHasTouch()==GBPREFERRED){
		_lastTouchX=-1;
		_lastTouchY=-1;
	}else{
		_lastTouchX=touchX;
		_lastTouchY=touchY;
		_userChoice=0;
	}
	// display
	int _totalChoiceH=_choiceH*_numChoices+_choicePad*(_numChoices-1);
	int _startDrawX = easyCenter(_choiceW,screenWidth);
	int _startDrawY;
	int _maxStartDrawY;
	int _minStartDrawY;
	int _buttonScrollStartY = PREFERREDIMAGECHOICESONSCREEN/2; // what _userChoice to start scrolling at if using buttons
	if (_totalChoiceH<screenHeight){
		_startDrawY = easyCenter(_totalChoiceH,screenHeight);
		_minStartDrawY=_startDrawY;
		_maxStartDrawY=_startDrawY;
		_canDrag=0;
	}else{
		_startDrawY=0;
		_minStartDrawY=_totalChoiceH*-1+screenHeight;
		_maxStartDrawY=0;
		_canDrag=1;
	}
	while (1){
		controlsStart();
		int _tx = fixTouchX(touchX);
		int _ty = fixTouchY(touchY);
		char _touchIsInAChoice = pointInBox(_tx,_ty,_startDrawX,_startDrawY,_choiceW,_totalChoiceH) && (_ty-_startDrawY)%(_choiceH+_choicePad)<=_choiceH;
		int _potentialTouchChoice;
		if (_ty-_startDrawY>0){
			_potentialTouchChoice=(_ty-_startDrawY)/(_choiceH+_choicePad);
		}else{
			_potentialTouchChoice=-1;
		}
		if (_tx!=_lastTouchX || _ty!=_lastTouchY){
			_userChoice=_touchIsInAChoice ? _potentialTouchChoice : -1;
			_lastTouchX=_tx;
			_lastTouchY=_ty;
		}
		
		if (wasJustPressed(BUTTON_TOUCH)){
			_startTouchX=_tx;
			_startTouchY=_ty;
			if (_touchIsInAChoice && _potentialTouchChoice>=0 && _potentialTouchChoice<_numChoices){
				_isDrag=0;
				_userChoice=_potentialTouchChoice;
				_isHoldSelect=_userChoice;
			}else{
				_isDrag=_canDrag;
			}
		}else if (isDown(BUTTON_TOUCH)){
			if (!_isDrag && _canDrag){
				if (_userChoice==-1){
					if (abs(_tx-_startTouchX)>(screenWidth*MINDRAGRATIO) || abs(_ty-_startTouchY)>(screenHeight*MINDRAGRATIO)){
						_isDrag=1;
						_isHoldSelect=-1;
						_userChoice=-1;
						_startTouchY=touchY;
					}
				}else{
					if (_potentialTouchChoice!=_userChoice){
						_userChoice=-1;
					}
				}
			}else{
				_startDrawY=limitNum(_startDrawY+(_ty-_startTouchY),_minStartDrawY,_maxStartDrawY);
				_startTouchY=touchY;
			}
		}
		if (wasJustPressed(BUTTON_A)){
			_isHoldSelect=_userChoice;
		}
		if (wasJustPressed(BUTTON_UP) || wasJustPressed(BUTTON_DOWN)){
			_isHoldSelect=-1;
			signed char _dir = wasJustPressed(BUTTON_UP) ? -1 : 1;
			_userChoice = wrapNum(_userChoice+_dir,0,_numChoices-1);
			_startDrawY=limitNum((_userChoice-_buttonScrollStartY)*(_choiceH+_choicePad)*-1,_minStartDrawY,_maxStartDrawY);
		}
		if (wasJustReleased(BUTTON_A) || wasJustReleased(BUTTON_TOUCH)){
			if (_isHoldSelect!=-1 && _userChoice==_isHoldSelect){
				controlsEnd();
				break;
			}
			_isHoldSelect=-1;
		}
		controlsEnd();
		startDrawing();
		Draw(1);
		for (i=0;i<_numChoices;i++){
			int _curY=_startDrawY+(_choiceH+_choicePad)*i;
			if (_curY<_choiceH*-1){
				continue;
			}else if (_curY>screenHeight){
				break;
			}
			crossTexture* _curImg;
			if (i==_userChoice){
				_curImg = _isHoldSelect==i ? _selectImages[i] : _hoverImages[i];
			}else{
				_curImg = _normalImages[i];
			}
			drawTextureSized(_curImg,_startDrawX,_curY,_choiceW,_choiceH);
		}
		endDrawing();
	}
	//
	for (i=0;i<_numChoices;i++){
		freeTexture(_normalImages[i]);
		freeTexture(_hoverImages[i]);
		freeTexture(_selectImages[i]);
	}
	free(_normalImages);
	free(_hoverImages);
	free(_selectImages);
	//
	makeNewReturnArray(_returnedReturnArray,_returnArraySize,1);
	nathanvariableArraySetFloat(*_returnedReturnArray,0,_userChoice);
	if (currentlyVNDSGame && nathanscriptIsInit){
		char _numberToStringBuffer[5];
		sprintf(_numberToStringBuffer,"%d",_userChoice+1);
		genericSetVar("selected","=",_numberToStringBuffer,&nathanscriptGamevarList,&nathanscriptTotalGamevar);
	}
}
// Sets the size of the screen that positions are relative to. For example, this would be the DS' screen resolution for vnds games. It's 640x480 for Higurashi
// Sets scriptScreenWidth and scriptScreenHeight
void scriptSetPositionsSize(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	scriptScreenWidth = nathanvariableToInt(&_passedArguments[0]);
	scriptScreenHeight = nathanvariableToInt(&_passedArguments[1]);
}
void scriptSetIncludedFileExtensions(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	scriptUsesFileExtensions = nathanvariableToBool(&_passedArguments[0]);
}
void scriptSetFontSize(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	reloadFont(nathanvariableToInt(&_passedArguments[0]),1);
}
void scriptScalePixels(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	int _passed=nathanvariableToInt(&_passedArguments[0]);
	double _newVal;
	if (_numArguments==2 && nathanvariableToInt(&_passedArguments[1])==1){ // height
		_newVal=(_passed/(double)scriptScreenHeight)*screenHeight;
	}else{ // width
		_newVal=(_passed/(double)scriptScreenWidth)*screenWidth;
	}
	makeNewReturnArray(_returnedReturnArray,_returnArraySize,1);
	nathanvariableArraySetFloat(*_returnedReturnArray,0,_newVal);
}
// x, y, w, h
void scriptDefineImageName(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	int _slot=nathanvariableToInt(&_passedArguments[0]);
	if (_slot>=advImageNameCount){
		advImageNameCount=_slot+1;
		advImageNamePos = realloc(advImageNamePos,sizeof(int)*advImageNameCount*4);
	}
	int i;
	for (i=0;i<4;++i){
		advImageNamePos[_slot*4+i]=nathanvariableToInt(&_passedArguments[1+i]);
	}
	// initialize adv image name height if user forgot
	if (advNameImHeight==-1){
		advNameImHeight=currentTextHeight;
	}
}
void scriptLoadImageNameSheet(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	if (advNameImSheet!=NULL){
		freeTexture(advNameImSheet);
	}
	advNameImSheet = safeLoadGameImage(nathanvariableToString(&_passedArguments[0]),LOCATION_CG,scriptUsesFileExtensions);
}
// lua only functions
int L_setVNDSVar(lua_State* passedState){
	if (lua_toboolean(passedState,1)){
		genericSetVar((char*)lua_tostring(passedState,2),(char*)lua_tostring(passedState,3),(char*)lua_tostring(passedState,4),&nathanscriptGlobalvarList,&nathanscriptTotalGlobalvar);
	}else{
		genericSetVar((char*)lua_tostring(passedState,2),(char*)lua_tostring(passedState,3),(char*)lua_tostring(passedState,4),&nathanscriptGamevarList,&nathanscriptTotalGamevar);
	}
	return 0;
}
int L_settleBust(lua_State* passedState){
	int _slot = lua_tonumber(passedState,1);
	if (_slot<maxBusts){
		settleBust(&(Busts[_slot]));
	}
	return 0;
}
int L_setDropshadowColor(lua_State* passedState){
	dropshadowR=lua_tonumber(passedState,1);
	dropshadowG=lua_tonumber(passedState,1);
	dropshadowB=lua_tonumber(passedState,1);
	return 0;
}
#include "LuaWrapperDefinitions.h"
//======================================================
void drawAdvanced(char _shouldDrawBackground, char _shouldDrawLowBusts, char _shouldDrawFilter, char _shouldDrawMessageBox, char _shouldDrawHighBusts, char _shouldDrawMessageText){
	int i;
	gameObjectClipOn();
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
	gameObjectClipOff();
	if (_shouldDrawFilter){
		DrawCurrentFilter();
	}
	if (_shouldDrawMessageBox){
		DrawMessageBox(gameTextDisplayMode,currentBoxAlpha);
	}
	if (_shouldDrawHighBusts){
		char _clipOn=0;
		for (i = maxBusts-1; i != -1; i--){
			if (bustOrderOverBox[i]!=255 && Busts[bustOrderOverBox[i]].isActive==1){
				if (!_clipOn){
					gameObjectClipOn();
					_clipOn=1;
				}
				DrawBust(&(Busts[bustOrderOverBox[i]]));
			}
		}
		if (_clipOn){
			gameObjectClipOff();
		}
	}
	if (_shouldDrawMessageText){
		DrawMessageText(255,-1,-1);
	}
}
void Draw(char _shouldDrawMessageBox){
	drawAdvanced(1,1,1,_shouldDrawMessageBox,1,_shouldDrawMessageBox);
}
int qsortStringComparer(const void* a, const void* b){
	return strcmp(*(char**)a,*(char**)b);
}
char FileSelector(char* _dirPath, char** _retChosen, char* _promptMessage){
	*_retChosen=NULL;
	crossDir dir=openDirectory(_dirPath);
	if (!dirOpenWorked(dir)){
		easyMessagef(1,"Failed to open directory %s",_dirPath);
		return 2;
	}
	char _ret=0;
	int _maxFiles=20;
	char** _foundFiles = malloc(sizeof(char*)*_maxFiles);
	crossDirStorage lastStorage;
	int _nFiles;
	for (_nFiles=0;directoryRead(&dir,&lastStorage)!=0;_nFiles++){
		if (_nFiles==_maxFiles){
			_maxFiles*=2;
			_foundFiles = realloc(_foundFiles,sizeof(char*)*_maxFiles);
		}
		_foundFiles[_nFiles] = strdup(getDirectoryResultName(&lastStorage));
	}
	qsort(_foundFiles,_nFiles,sizeof(char*),qsortStringComparer);
	directoryClose (dir);
	if (_nFiles==0){
		easyMessagef(1,"No files found.");
		_ret=2;
	}else{
		int _menuRet = showMenu(0,_promptMessage,_nFiles,_foundFiles,1);
		if (_menuRet==-1){
			_ret=1;
		}else{
			*_retChosen=strdup(_foundFiles[_menuRet]);
		}
	}
	freeAllocdStrList(_foundFiles,_nFiles);
	return _ret;
}
void getFontSetupText(int* _numLines, char*** _realLines, int _maxWidth){
	if (*_realLines!=NULL){
		freeWrappedText(*_numLines,*_realLines);
	}
	wrapText("Tap the labeled regions to change the font size. When the red bar on the bottom of the screen runs out, the font size change is applied. This minimizes freezing.",_numLines,_realLines,_maxWidth);
}
void drawTextfCenterBG(crossFont* _passedFont, int _x, int _y, unsigned char r, unsigned char g, unsigned char b, unsigned char a, unsigned char _bgR, unsigned char _bgG, unsigned char _bgB, int _containerW, double _boxExtraPad, const char* _formatString, ...){
	va_list _tempArgs;
	va_start(_tempArgs, _formatString);
	char* _completeString = formatf(_tempArgs,_formatString);
	int _textW = textWidth(_passedFont,_completeString);
	_x+=easyCenter(_textW,_containerW);
	int _bgW=_textW*_boxExtraPad;
	int _bgH=currentTextHeight*_boxExtraPad;
	drawRectangle(_x+easyCenter(_bgW,_textW),_y+easyCenter(_bgH,currentTextHeight),_bgW,_bgH,_bgR,_bgG,_bgB,255);
	gbDrawTextAlpha(_passedFont,_x,_y,_completeString,r,g,b,a);
	free(_completeString);
}
#define FONTSIZETOUCHSETUPRATIO (1/(double)5)
#define FONTSIZERELOADTIME 400
#define FONTRELOADBARRATIOH (1/(double)20)
void fontSizeSetupTouch(){
	unsigned char _tR, _tG, _tB;
	getInverseBGCol(&_tR,&_tG,&_tB);
	unsigned char _bR, _bG, _bB;
	getClearColor(&_bR,&_bG,&_bB);
	int _buttonW = screenWidth*FONTSIZETOUCHSETUPRATIO;
	char _fontReloadQueued=0;
	u64 _nextFontReloadTime=0;
	int _numLines;
	char** _wrappedLines=NULL;
	getFontSetupText(&_numLines,&_wrappedLines,getOutputLineScreenWidth());
	char _isDone=0;
	while(!_isDone){
		u64 _sTime=getMilli();
		controlsStart();
		if (wasJustPressed(BUTTON_TOUCH)){
			int _tx = fixTouchX(touchX);
			if ((_tx>=0 && _tx<=_buttonW) || (_tx>screenWidth-_buttonW && _tx<=screenWidth)){
				fontSize+=(_tx<=_buttonW ? -2 : 2);
				_fontReloadQueued=1;
				_nextFontReloadTime=_sTime+FONTSIZERELOADTIME;
			}
		}
		if (wasJustPressed(BUTTON_BACK)){
			if (_fontReloadQueued){
				_sTime=_nextFontReloadTime;
			}
			_isDone=1;
			goto checkFontReload;
		}
		controlsEnd();
		startDrawing();
		drawAdvanced(1,1,1,1,1,0);
		if (_sTime<_nextFontReloadTime){
			int _reloadBarH = screenHeight*FONTRELOADBARRATIOH;
			drawRectangle(0,screenHeight-_reloadBarH,screenWidth-partMoveFillsEndTime(_sTime,_nextFontReloadTime,FONTSIZERELOADTIME,screenWidth),_reloadBarH,255,0,0,255);
		}
		drawTextfCenterBG(normalFont,0,0,_tR,_tG,_tB,255,_bR,_bG,_bB,screenWidth,1.2,"Size: %f",fontSize);
		drawTextfCenterBG(normalFont,0,easyCenter(currentTextHeight,screenHeight),_tR,_tG,_tB,255,_bR,_bG,_bB,_buttonW,1.2,"Size down",fontSize);
		drawTextfCenterBG(normalFont,screenWidth-_buttonW,easyCenter(currentTextHeight,screenHeight),_tR,_tG,_tB,255,_bR,_bG,_bB,_buttonW,1.2,"Size up",fontSize);
		drawWrappedText(totalTextXOff(),totalTextYOff(),_wrappedLines,_numLines,_tR,_tG,_tB,255);
		endDrawing();
	checkFontReload:
		if (_fontReloadQueued && _sTime>=_nextFontReloadTime){
			_fontReloadQueued=0;
			reloadFont(fontSize,0);
			getFontSetupText(&_numLines,&_wrappedLines,getOutputLineScreenWidth());
		}
	}
	controlsEnd();
	freeWrappedText(_numLines,_wrappedLines);
	recalculateMaxLines(); // need to do this manually because it's not done by the reloadFont
}
void fontSizeSetupButton(){
	char _choice=0;
	while (1){
		controlsStart();
		_choice = menuControls(_choice,0,2);
		fontSize=retMenuControlsLow(fontSize,0,0,0,1,8,70);
		if (wasJustPressed(BUTTON_A)){
			if (_choice==1){
				reloadFont(fontSize,0);
			}else if (_choice==2){
				reloadFont(fontSize,1);
				break;
			}
		}
		controlsEnd();
		startDrawing();
		
		gbDrawTextf(normalFont,MENUOPTIONOFFSET,currentTextHeight,255,255,255,255,"Font Size: %f",fontSize);
		drawText(MENUOPTIONOFFSET,currentTextHeight*2,"Test");
		drawText(MENUOPTIONOFFSET,currentTextHeight*3,"Done");
		drawText(5,currentTextHeight*(_choice+1),MENUCURSOR);

		drawText(MENUOPTIONOFFSET,currentTextHeight*5,"This is some test text for you to look at while changing the font size.");
		drawText(MENUOPTIONOFFSET,currentTextHeight*6,"The font must be reloaded for you to see the changes.");
		drawText(MENUOPTIONOFFSET,currentTextHeight*7,"Select the \"Test\" option to do so.");
		endDrawing();
	}
}
void FontSizeSetup(){
	if (gbHasTouch()==GBPREFERRED){
		fontSizeSetupTouch();
	}else{
		fontSizeSetupButton();
	}
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
		easyMessagef(1,"TODO");
	}
}
void _settingsChangeAuto(int* _storeValue, char* _storeString, signed char _direction, char _isHoldingL){
	*_storeValue+=(_direction*(_isHoldingL ? 50 : 100));
	if (*_storeValue<=0){
		*_storeValue=0;
	}
	_storeString[0]='\0';
}
void overrideIfSet(signed char* _possibleTarget, signed char _possibleOverride){
	if (_possibleOverride!=-1){
		*_possibleTarget = _possibleOverride;
	}
}
void incrementSoundSetting(float* _changeThis, signed char _directionMultiplier, char* _chopThisString){
	*_changeThis+=.25*_directionMultiplier;
	if (*_changeThis>1){
		*_changeThis=0;
	}else if (*_changeThis<0){
		*_changeThis=1;
	}
	_chopThisString[0]='\0';
}
void itoaIfEmpty(int _numVal, char* _destBuffer){
	if (_destBuffer[0]=='\0'){
		itoa(_numVal,_destBuffer,10);
	}
}
typedef enum{
	SETTING_BACK=0,
	SETTING_SAVE,
	SETTING_RESTARTBGM, //
	SETTING_BGMVOL,
	SETTING_SEVOL,
	SETTING_VOICEVOL,
	SETTING_TEXTSCREEN, //
	SETTING_TEXTSPEED,
	SETTING_DROPSHADOW,
	SETTING_FONTSIZE,
	SETTING_TEXTMODE, //
	SETTING_BOXALPHA,
	SETTING_TEXTBOXW,
	SETTING_ADVNAMES,
	SETTING_HITBOTTOMCLEAR,
	SETTING_AUTOSPEED, //
	SETTING_AUTOVOICEDSPEED,
	SETTING_BUSTLOC, //
	SETTING_VNDSWAR, //
	SETTING_VNDSFADE,
	SETTING_TOUCH, //
	SETTING_OVERCLOCK, //
	SETTING_DEFAULT,
	SETTING_DEBUG,
	SETTING_QUIT,

	SETTINGS_MAX,
}settingsSlotNum;
/*
  -- How to add a new option to the settings menu --
  Step 1 - Add a new enum to settingsSlotNum. make sure it's before SETTINGS_MAX
  Step 2 - Using your enum value as the array index, assign the _settings and (optional) _values strings.
  Step 3 - In the big switch statement, add a new case with your enum
*/
void SettingsMenu(signed char _shouldShowQuit, signed char _shouldShowVNDSSettings, signed char _shouldShowVNDSSave, signed char _shouldShowRestartBGM, signed char _showArtLocationSlot, signed char _showScalingOption, signed char _showTextBoxModeOption, signed char _showVNDSFadeOption, signed char _showDebugButton){
	#ifdef OVERRIDE_SETTINGSMENU
		customSettingsMenu(_shouldShowQuit,_shouldShowVNDSSettings,_shouldShowVNDSSave,_shouldShowRestartBGM,_showArtLocationSlot,_showScalingOption,_showTextBoxModeOption,_showVNDSFadeOption,_showDebugButton);
		return;
	#endif
	// Allow global overide for settings
	overrideIfSet(&_shouldShowQuit,forceShowQuit);
	overrideIfSet(&_shouldShowVNDSSettings,forceShowVNDSSettings);
	overrideIfSet(&_shouldShowVNDSSave,forceShowVNDSSave);
	overrideIfSet(&_shouldShowRestartBGM,forceShowRestartBGM);
	overrideIfSet(&_showArtLocationSlot,forceArtLocationSlot);
	overrideIfSet(&_showTextBoxModeOption,forceTextBoxModeOption);
	overrideIfSet(&_showVNDSFadeOption,forceVNDSFadeOption);
	overrideIfSet(&_showDebugButton,forceDebugButton);

	controlsReset();
	PlayMenuSound();
	int i;
	char _artBefore=graphicsLocation;
	// important that these all start with the first element as '\0' for itoaIfEmpty
	char _tempItoaHoldBGM[5] = {'\0'};
	char _tempItoaHoldSE[5] = {'\0'};
	char _tempItoaHoldVoice[5] = {'\0'};
	char _tempItoaHoldBoxAlpha[5] = {'\0'};
	char _tempItoaHoldTextSpeed[8] = {'\0'}; // Needs to be big enough to hold "instant"
	char _tempAutoModeString[10] = {'\0'};
	char _tempAutoModeVoiceString[10] = {'\0'};
	
	char* _settings[] = {
		NULL, // back
		"=Save Game=",
		"Restart BGM", // sound
		"BGM Volume:",
		"SE Volume:",
		"Voice Volume:",
		"Text Screen:", // text
		"Text Speed:",
		"Drop Shadow:",
		"Font Size",
		"Text Mode:", // textbox
		"Message Box Alpha:",
		"Textbox:",
		"ADV Names:",
		"Clear at bottom:",
		"Auto Speed:", // auto
		"Auto Voiced Speed:",
		"Bust Location:", // graphics
		"VNDS Warnings:", // VNDS specific
		"VNDS Image Fade:",
		"Touch:", // controls
		"Overclock CPU:", // misc
		"Defaults",
		"Debug",
		"Quit",
	};
	_settings[SETTING_BACK] = (currentGameStatus == GAMESTATUS_TITLE) ? "Back" : "Resume";
	optionProp _settingsProp[SETTINGS_MAX] = {
		0,
		0,
		0,
		OPTIONPROP_LEFTRIGHT,
		OPTIONPROP_LEFTRIGHT,
		OPTIONPROP_LEFTRIGHT,
		0,
		OPTIONPROP_LEFTRIGHT,
		0,
		0,
		0,
		OPTIONPROP_LEFTRIGHT,
		0,
		0,
		0,
		OPTIONPROP_LEFTRIGHT,
		OPTIONPROP_LEFTRIGHT,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
	};
	//
	char* _settingsOn = newShowMap(SETTINGS_MAX);
	char* _values[SETTINGS_MAX] = {NULL};
	// constant values or constant on/off state
	_settingsOn[SETTING_SAVE]=_shouldShowVNDSSave;
	_settingsOn[SETTING_RESTARTBGM]=_shouldShowRestartBGM;
	_values[SETTING_BGMVOL] = &(_tempItoaHoldBGM[0]);
	_values[SETTING_SEVOL] = &(_tempItoaHoldSE[0]);
	_values[SETTING_VOICEVOL] = &(_tempItoaHoldVoice[0]);
	_settingsOn[SETTING_VOICEVOL]=hasOwnVoiceSetting;
	_values[SETTING_TEXTSPEED] = &(_tempItoaHoldTextSpeed[0]);
	_settingsOn[SETTING_DROPSHADOW]=forceDropshadowOption;
	_settingsOn[SETTING_FONTSIZE]=forceFontSizeOption;
	_settingsOn[SETTING_BOXALPHA]=(gameTextDisplayMode==TEXTMODE_NVL || canChangeBoxAlpha);
	_values[SETTING_BOXALPHA] = &(_tempItoaHoldBoxAlpha[0]);
	_settingsOn[SETTING_TEXTMODE]=_showTextBoxModeOption;
	_settingsOn[SETTING_ADVNAMES]=(gameTextDisplayMode==TEXTMODE_ADV && advNamesSupported==1);
	_values[SETTING_AUTOSPEED]=&(_tempAutoModeString[0]); // auto stuff
	_values[SETTING_AUTOVOICEDSPEED]=&(_tempAutoModeVoiceString[0]);
	_settingsOn[SETTING_BUSTLOC]=_showArtLocationSlot;
	_settingsOn[SETTING_VNDSWAR]=_shouldShowVNDSSettings;
	_settingsOn[SETTING_VNDSFADE]=_showVNDSFadeOption;
	#if GBPLAT != GB_VITA
		_settingsOn[SETTING_OVERCLOCK]=0;
	#endif
	_settingsOn[SETTING_DEBUG]=_showDebugButton;
	_settingsOn[SETTING_DEFAULT]=forceResettingsButton;
	//////////////////////////
	int _choice=0;
	char _shouldExit=0;
	while(!_shouldExit){
		char _showNVLOptions = (gameTextDisplayMode==TEXTMODE_NVL);
		//char _showADVOptions = (gameTextDisplayMode==TEXTMODE_ADV);
		// text
		#if GBPLAT == GB_3DS
			_values[SETTING_TEXTSCREEN] = textIsBottomScreen ? "Bottom Screen" : "Top Screen";
		#else
			_settingsOn[SETTING_TEXTSCREEN]=0;
		#endif
		if (_settingsOn[SETTING_DROPSHADOW]){
			_values[SETTING_DROPSHADOW] = charToSwitch(dropshadowOn);
		}
		_settingsOn[SETTING_TEXTBOXW]=(forceTextOverBGOption && _showNVLOptions);
		if (_settingsOn[SETTING_TEXTBOXW]){
			_values[SETTING_TEXTBOXW] = textOnlyOverBackground ? "Small" : "Full";
		}
		if (_settingsOn[SETTING_TEXTMODE]){
			_values[SETTING_TEXTMODE]=(preferredTextDisplayMode==TEXTMODE_ADV ? "ADV" : "NVL");
		}
		if (_settingsOn[SETTING_ADVNAMES]){
			_values[SETTING_ADVNAMES]=charToSwitch(prefersADVNames);
		}
		_settingsOn[SETTING_HITBOTTOMCLEAR]=(_showNVLOptions);
		if (_settingsOn[SETTING_HITBOTTOMCLEAR]){
			_values[SETTING_HITBOTTOMCLEAR] = charToBoolString(clearAtBottom);
		}
		// graphics stuff
		if (_settingsOn[SETTING_BUSTLOC]){
			_values[SETTING_BUSTLOC]=(graphicsLocation == LOCATION_CG ? "CG" : (graphicsLocation==LOCATION_CGALT ? "CGAlt" : "Undefined"));
		}
		// vnds specific settings
		if (_settingsOn[SETTING_VNDSWAR]){
			_values[SETTING_VNDSWAR] = showVNDSWarnings ? "Show" : "Hide";
		}
		if (_settingsOn[SETTING_VNDSFADE]){
			_values[SETTING_VNDSFADE]=charToSwitch(vndsSpritesFade);
		}
		// controls
		if (gbHasTouch()==GBYES){ // do not allow turning off touch if it's the preferred method
			_values[SETTING_TOUCH] = charToSwitch(touchProceed);
		}else{
			_settingsOn[SETTING_TOUCH]=0;
		}
		// misc
		_values[SETTING_OVERCLOCK]=charToSwitch(cpuOverclocked);
		// update strings if needed
		itoaIfEmpty(autoModeWait,_tempAutoModeString);
		itoaIfEmpty(autoModeVoicedWait,_tempAutoModeVoiceString);
		itoaIfEmpty(bgmVolume*4,_tempItoaHoldBGM);
		itoaIfEmpty(seVolume*4,_tempItoaHoldSE);
		itoaIfEmpty(voiceVolume*4,_tempItoaHoldVoice);
		itoaIfEmpty(preferredBoxAlpha,_tempItoaHoldBoxAlpha);
		if (_tempItoaHoldTextSpeed[0]=='\0'){
			makeTextSpeedString(_tempItoaHoldTextSpeed,textSpeed);
		}
		// update colors if needed
		// If message box alpha is very high, make it red
		_settingsProp[SETTING_BOXALPHA]=OPTIONPROP_LEFTRIGHT;
		if (preferredBoxAlpha>=230){
			_settingsProp[SETTING_BOXALPHA]|=OPTIONPROP_BADCOLOR;
		}
		char _selectionInfo;
		_choice=showMenuAdvanced(_choice,"Settings",SETTINGS_MAX,_settings,_values,_settingsOn,_settingsProp,&_selectionInfo,0,NULL);
		signed char _directionMultiplier = (_selectionInfo & MENURET_RIGHT) ? 1 : -1;
		char _didHoldL = (_selectionInfo & MENURET_LBUTTON);
		switch(_choice){
			case SETTING_BACK:
				_shouldExit=1;
				break;
			case SETTING_OVERCLOCK:
				cpuOverclocked=!cpuOverclocked;
				#if GBPLAT == GB_VITA
					scePowerSetArmClockFrequency(cpuOverclocked ? 444 : 333);
				#endif
				break;
			case SETTING_RESTARTBGM:
				PlayBGM(lastBGMFilename,lastBGMVolume,1);
				break;
			case SETTING_BGMVOL:
				incrementSoundSetting(&bgmVolume,_directionMultiplier,_tempItoaHoldBGM);
				SetAllMusicVolume(FixBGMVolume(lastBGMVolume));
				break;
			case SETTING_SEVOL:
				incrementSoundSetting(&seVolume,_directionMultiplier,_tempItoaHoldSE);
				if (menuSoundLoaded==1){
					setSFXVolumeBefore(menuSound,FixSEVolume(256));
					PlayMenuSound();
				}
				break;
			case SETTING_VOICEVOL:
				incrementSoundSetting(&voiceVolume,_directionMultiplier,_tempItoaHoldVoice);
				break;
			case SETTING_FONTSIZE:
				FontSizeSetup();
				break;
			case SETTING_DEFAULT:
				if (LazyChoicef("This will reset your settings. Is this okay?")==1){
					resetSettings();
					easyMessagef(1,"Restart for the changes to take effect.");
				}
				break;
			case SETTING_TEXTBOXW:
				textOnlyOverBackground=!textOnlyOverBackground;
				break;
			case SETTING_TEXTSPEED:
				textSpeed+=_directionMultiplier;
				if (_directionMultiplier==1){
					if (textSpeed==11){
						textSpeed=TEXTSPEED_INSTANT;
					}else if (textSpeed==TEXTSPEED_INSTANT+1){
						textSpeed=TEXTSPEED_INSTANT;
					}else if (textSpeed==0){
						textSpeed=1;
					}
				}else{
					if (textSpeed==-11){
						textSpeed=-10;
					}else if (textSpeed==TEXTSPEED_INSTANT-1){
						textSpeed=10;
					}else if (textSpeed==0){
						textSpeed=-1;
					}
				}
				_tempItoaHoldTextSpeed[0]='\0';
				break;
			case SETTING_AUTOSPEED:
				_settingsChangeAuto(&autoModeWait,_tempAutoModeString,_directionMultiplier,_didHoldL);
				break;
			case SETTING_AUTOVOICEDSPEED:
				_settingsChangeAuto(&autoModeVoicedWait,_tempAutoModeVoiceString,_directionMultiplier,_didHoldL);
				break;
			case SETTING_BOXALPHA:
			{
				int _tempHoldChar = preferredBoxAlpha; // Prevent wrapping
				if (_didHoldL){
					_tempHoldChar+=15*_directionMultiplier;
				}else{
					_tempHoldChar+=25*_directionMultiplier;
				}
				if (_tempHoldChar>255){
					_tempHoldChar=255;
				}else if (_tempHoldChar<0){
					_tempHoldChar=0;
				}
				preferredBoxAlpha = _tempHoldChar;
				_tempItoaHoldBoxAlpha[0]='\0';
			}
				break;
			case SETTING_BUSTLOC:
				graphicsLocation = (graphicsLocation==LOCATION_CG) ? LOCATION_CGALT : LOCATION_CG;
				break;
			case SETTING_SAVE:
				PlayMenuSound();
				safeVNDSSaveMenu();
				break;
			case SETTING_HITBOTTOMCLEAR:
				clearAtBottom = !clearAtBottom;
				break;
			case SETTING_VNDSWAR:
				showVNDSWarnings = !showVNDSWarnings;
				break;
			case SETTING_TEXTMODE:
				preferredTextDisplayMode = (preferredTextDisplayMode==TEXTMODE_ADV ? TEXTMODE_NVL : TEXTMODE_ADV);
				switchTextDisplayMode(preferredTextDisplayMode);
				break;
			case SETTING_VNDSFADE:
				vndsSpritesFade=!vndsSpritesFade;
				break;
			case SETTING_DEBUG:
				debugMenuOption();
				break;
			case SETTING_TOUCH:
				touchProceed=!touchProceed;
				break;
			case SETTING_DROPSHADOW:
				dropshadowOn = !dropshadowOn;
				break;
			case SETTING_ADVNAMES:
				prefersADVNames=!prefersADVNames;
				break;
			case SETTING_QUIT:
				#if GBPLAT == GB_3DS
					lockBGM();
				#endif
				endType = Line_ContinueAfterTyping;
				currentGameStatus=GAMESTATUS_QUIT;
				exit(0);
				break;
		}
	}
	//////////////////////////
	free(_settingsOn);
	applyTextboxChanges(1);
	controlsEnd();
	SaveSettings();
	if (currentGameStatus!=GAMESTATUS_TITLE){
		// If we changed art location, reload busts
		if (_artBefore != graphicsLocation){
			for (i=0;i<maxBusts;++i){
				if (Busts[i].isActive){
					char* _cacheFilename = strdup(Busts[i].relativeFilename);
					DrawBustshot(i,_cacheFilename,Busts[i].xOffset,Busts[i].yOffset,Busts[i].layer,0,0,Busts[i].destAlpha);
					free(_cacheFilename);
				}
			}
		}
	}
	#if GBPLAT == GB_3DS
		if (textIsBottomScreen==1){
			textboxWidth = 320;
			outputLineScreenHeight = 240;
		}else{
			textboxWidth = 400;
			outputLineScreenHeight = 240;
		}
	#endif
}
// Starting at _startIndex, search _searchThis for the next slot that is 1
// No overflow protection
// If you pass null, it just returns _startIndex
int getNextEnabled(char* _searchThis, int _startIndex){
	if (_searchThis==NULL){
		return _startIndex;
	}
	int i;
	for (i=_startIndex;_searchThis[i]!=1;++i);
	return i;
}
int menuIndexToReal(int _fakeIndex, char* _showMap){
	int _ret=-1;
	int i;
	for (i=0;i<=_fakeIndex;++i){
		_ret=getNextEnabled(_showMap,_ret+1);
	}
	return _ret;
}
char* newShowMap(int _numElements){
	char* _ret = malloc(sizeof(char)*_numElements);
	memset(_ret,1,sizeof(char)*_numElements);
	return _ret;
}
int getNumShownOptions(char* _showMap, int _mapSize){
	if (_showMap){
		int _numOptions=0;
		int i;
		for (i=0;i<_mapSize;++i){
			if (_showMap[i]){
				++_numOptions;
			}
		}
		return _numOptions;
	}
	return _mapSize;
}
char* getFullMenuOptionStr(int _realIndex, char** _options, char** _optionValues){
	if (_optionValues && _optionValues[_realIndex]){
		return easyCombineStrings(2,_options[_realIndex],_optionValues[_realIndex]);
	}else{
		return strdup(_options[_realIndex]);
	}
}
void wrapOption(int _realIndex, char** _options, char** _optionValues, int _fakeIndex, char*** _wrappedOptions, int* _wrappedLen, int _wrapW, char _freeOld){
	if (_freeOld){
		if (_wrappedOptions[_fakeIndex]){
			freeWrappedText(_wrappedLen[_fakeIndex],_wrappedOptions[_fakeIndex]);
		}
	}
	char* _withValue = getFullMenuOptionStr(_realIndex,_options,_optionValues);
	wrapText(_withValue,&(_wrappedLen[_fakeIndex]),&(_wrappedOptions[_fakeIndex]),_wrapW);
	free(_withValue);
}
#define TOUCHOPTIONCOLORA 100,100,100
#define TOUCHOPTIONCOLORB 75,75,75
#define TOUCHARROWCOLOR 255,0,255
// will be bad if option number zero is a left right option
int showMenuAdvancedTouch(int _choice, const char* _title, int _mapSize, char** _options, char** _optionValues, char* _showMap, optionProp* _optionProp, char* _returnInfo, char _menuProp, int _passedOptionXOff, int _passedOptionYOff, int _menuW, int _menuH, inttakeretfunc _drawHook){
	int _ret=_choice;
	int _changeArrowW = _menuW*(1/(double)6);
	//
	int _optionPad=screenHeight/80;
	controlsReset();
	if (_returnInfo){
		*_returnInfo=0;
	}
	int i;
	// convert the passed _choice from real index in _options to fake index
	if (_showMap){
		int _realIndex=_choice;
		for (i=0;i<_realIndex;++i){
			if (!_showMap[i]){
				--_choice;
			}
		}
	}
	// calculate the number of shown options
	int _numOptions = getNumShownOptions(_showMap,_mapSize);
	if (_numOptions==0){
		return -1;
	}
	// This is it. Menu is confirmed.
	// control info
	char _isDrag=0;
	int _startTx=-1;
	int _startTy=-1;
	int _holdSelect=-1;
	char _isLeftRightMode=0;
	//
	char*** _wrappedOptions = malloc(sizeof(char**)*_numOptions);
	int* _wrappedLen = malloc(sizeof(int)*_numOptions);
	int* _optionY = malloc(sizeof(int)*_numOptions);
	// Wrap	
	int _loopRealIndex=-1;
	for (i=0;i<_numOptions;++i){
		_loopRealIndex=getNextEnabled(_showMap,_loopRealIndex+1);
		int _wrapW=_menuW;
		// do small wrap if this option needs space for the arrows
		if (_choice==i && _optionProp && (_optionProp[_loopRealIndex] & OPTIONPROP_LEFTRIGHT)){
			_isLeftRightMode=1;
			_wrapW-=_changeArrowW*2;
		}
		wrapOption(_loopRealIndex,_options,_optionValues,i,_wrappedOptions,_wrappedLen,_wrapW,0);
	}
	// draw info
	int _startDrawY=-1;
recalcPositions:
	;
	// calculate positions
	int _totalHeight=_optionPad*-1;
	for (i=0;i<_numOptions;++i){
		_totalHeight+=_optionPad;
		_optionY[i]=_totalHeight;
		_totalHeight+=_wrappedLen[i]*currentTextHeight;
	}
	int _minStartDrawY;
	int _maxStartDrawY;
	if (_totalHeight>=_menuH){
		_maxStartDrawY=_optionPad/2;
		_minStartDrawY=_totalHeight*-1+_menuH;
	}else{
		_maxStartDrawY=easyCenter(_totalHeight,_menuH);
		_minStartDrawY=_maxStartDrawY;
	}
	if (_startDrawY==-1){ // initialize if required
		// if this is the same menu again, restore the old scroll position
		if (lastTouchMenuOptions==_options){
			_startDrawY=limitNum(lastTouchMenuRetYPos-_optionY[_choice],_minStartDrawY,_maxStartDrawY);
		}else{
			_startDrawY=_maxStartDrawY;
		}
	}
	while(1){
		controlsStart();
		if ((_menuProp & MENUPROP_CANQUIT) && (wasJustPressed(BUTTON_B) || wasJustPressed(BUTTON_BACK))){
			_ret=-1;
			break;
		}
		if (!_isLeftRightMode){
			if (wasJustPressed(BUTTON_TOUCH)){
				int _fTouchX = fixTouchX(touchX);
				int _fTouchY = fixTouchY(touchY);
				_isDrag=!pointInBox(_fTouchX,_fTouchY,_passedOptionXOff,_passedOptionYOff,_menuW,_menuH);
				if (!_isDrag){
					int i;
					int _fakeTouchY=_fTouchY-_startDrawY-_passedOptionYOff;
					for (i=_numOptions-1;i>=0;--i){
						if (_fakeTouchY>=_optionY[i]){
							if (_fakeTouchY-_optionY[i]<=_wrappedLen[i]*currentTextHeight){
								_holdSelect=i;
							}
							break;
						}
					}
				}
				_startTx=touchX;
				_startTy=touchY;
			}else if (isDown(BUTTON_TOUCH)){
				if (!_isDrag){
					if (abs(touchX-_startTx)>(screenWidth*MINDRAGRATIO) || abs(touchY-_startTy)>(screenHeight*MINDRAGRATIO)){
						_isDrag=1;
						_holdSelect=-1;
						_startTy=touchY;
					}
				}else{
					_startDrawY=limitNum(_startDrawY+(touchY-_startTy),_minStartDrawY,_maxStartDrawY);
					_startTy=touchY;
				}
			}else if (wasJustReleased(BUTTON_TOUCH)){
				if (_holdSelect!=-1){
					_choice=_holdSelect;
					_ret = menuIndexToReal(_holdSelect,_showMap);
					if (_optionProp && (_optionProp[_ret] & OPTIONPROP_LEFTRIGHT)){
						_isLeftRightMode=1;;
						_holdSelect=-1;
						// rewrap this line because we now need button space
						wrapOption(_ret,_options,_optionValues,_choice,_wrappedOptions,_wrappedLen,_menuW-_changeArrowW*2,1);
						controlsEnd();
						goto recalcPositions;
					}else{
						break;
					}
				}
			}
		}else{
			if (wasJustPressed(BUTTON_TOUCH)){
				int _fTouchX = fixTouchX(touchX);
				int _fTouchY = fixTouchY(touchY);
				if (!pointInBox(_fTouchX,_fTouchY,_passedOptionXOff,_passedOptionYOff+_startDrawY+_optionY[_choice],_menuW,_wrappedLen[_choice]*currentTextHeight)){
					_isLeftRightMode=0;
					// rewrap because we may not need as much space anymore
					wrapOption(_ret,_options,_optionValues,_choice,_wrappedOptions,_wrappedLen,_menuW,1);
					controlsEnd();
					goto recalcPositions;
				}else{
					_fTouchX-=_passedOptionXOff;
					// at this point, _ret is already set to the real index of the choice
					if (_fTouchX>=0 && _fTouchX<=_changeArrowW){
						// by default, _returnInfo says that you pushed left. do not set anything
						break;
					}else if (_fTouchX>=_menuW-_changeArrowW && _fTouchX<=_menuW){
						if (_returnInfo){
							*_returnInfo|=MENURET_RIGHT;
						}
						break;
					}
				}
			}
		}
		controlsEnd();
		startDrawing();
		drawHallowRect(_passedOptionXOff-3,_passedOptionYOff-3,_menuW+3,_menuH+3,3,100,0,100,255);
		//enableClipping(_passedOptionXOff,_passedOptionYOff,_menuW,_menuH);
		gbSetDrawOffX(_passedOptionXOff);
		gbSetDrawOffY(_passedOptionYOff+_startDrawY);
		for (i=0;i<_numOptions;++i){
			if (i&1){
				drawRectangle(0,_optionY[i],_menuW,_wrappedLen[i]*currentTextHeight,TOUCHOPTIONCOLORA,255);
			}else{
				drawRectangle(0,_optionY[i],_menuW,_wrappedLen[i]*currentTextHeight,TOUCHOPTIONCOLORB,255);
			}
			if (_isLeftRightMode && i==_choice){
				drawRectangle(0,_optionY[i],_changeArrowW,_wrappedLen[i]*currentTextHeight,TOUCHARROWCOLOR,255);
				drawRectangle(_menuW-_changeArrowW,_optionY[i],_changeArrowW,_wrappedLen[i]*currentTextHeight,TOUCHARROWCOLOR,255);
				drawWrappedTextCentered(_changeArrowW,_optionY[i],_wrappedOptions[i],_wrappedLen[i],_menuW-_changeArrowW*2,255,255,255,255);
			}else{
				drawWrappedText(0,_optionY[i],_wrappedOptions[i],_wrappedLen[i],255,255,255,255);
				if (i==_holdSelect){
					drawRectangle(0,_optionY[i],_menuW,_wrappedLen[i]*currentTextHeight,255,0,0,255);
				}
			}
		}
		gbSetDrawOffX(0);
		gbSetDrawOffY(0);
		disableClipping();
		if (_drawHook && _drawHook(menuIndexToReal(_choice,_showMap))){
			break;
		}
		endDrawing();
	}
	if (_ret!=-1){
		lastTouchMenuRetYPos=_startDrawY+_optionY[_choice];
		lastTouchMenuOptions=_options;
	}else{
		lastTouchMenuOptions=NULL;
	}
	for (i=0;i<_numOptions;++i){
		freeWrappedText(_wrappedLen[i],_wrappedOptions[i]);
	}
	free(_wrappedLen);
	free(_wrappedOptions);
	free(_optionY);
	return _ret;
}
#define SHOWMENUBUTTONDELAY 300
#define SHOWMENUAUTOSHIFT 30
int showMenuAdvancedButton(int _choice, const char* _title, int _mapSize, char** _options, char** _optionValues, char* _showMap, optionProp* _optionProp, char* _returnInfo, char _menuProp, inttakeretfunc _drawHook){
	controlsReset();
	if (_returnInfo){
		*_returnInfo=0;
	}
	// convert the passed _choice from real index in _options to fake index
	if (_showMap){
		int _realIndex=_choice;
		for (int i=0;i<_realIndex;++i){
			if (!_showMap[i]){
				--_choice;
			}
		}
	}
	int _ret=-1;
	int _numOptions = getNumShownOptions(_showMap,_mapSize);
	if (_showMap){
		_numOptions=0;
		for (int i=0;i<_mapSize;++i){
			if (_showMap[i]){
				++_numOptions;
			}
		}
	}else{
		_numOptions=_mapSize;
	}
	if (_numOptions==0){
		return -1;
	}
	int _scrollOffset=-1;
	int _optionsOnScreen = (_title!=NULL ? screenHeight-currentTextHeight*1.5 : screenHeight)/currentTextHeight-1;
	if (_optionsOnScreen>_numOptions){
		_optionsOnScreen=_numOptions;
	}
	char _curScrollDirection;
	int _maxHScroll;
	u64 _startHScroll;
	int _scrollTime;
	int _curRealIndex;
	u64 _nextDASShift=0;
	while(currentGameStatus!=GAMESTATUS_QUIT){
		controlsStart();
		if (menuControlsLow(&_choice,1,1,0,(_menuProp & MENUPROP_CANPAGEUPDOWN) ? _optionsOnScreen : 0,0,_numOptions-1)){
			_scrollOffset=-1;
		}
		if (wasJustPressed(BUTTON_DOWN) || wasJustPressed(BUTTON_UP)){
			_nextDASShift=getMilli()+SHOWMENUBUTTONDELAY;
		}else if ((isDown(BUTTON_DOWN) || isDown(BUTTON_UP)) && _nextDASShift!=0){
			u64 _curTime = getMilli();
			if (_curTime>=_nextDASShift){
				_nextDASShift=getMilli()+SHOWMENUAUTOSHIFT;
				_choice=wrapNum(_choice+(isDown(BUTTON_DOWN) ? 1 : -1),0,_numOptions-1);
				_scrollOffset=-1;
			}
		}
		if (_scrollOffset==-1){ // queued menu info update
			// Find horizontal scroll info
			_curRealIndex = -1;
			for (int i=0;i<=_choice;++i){
				_curRealIndex = getNextEnabled(_showMap,_curRealIndex+1);
			}
			int _curWidth = textWidth(normalFont,_options[_curRealIndex]);
			int _maxTextWidth = screenWidth-MENUOPTIONOFFSET;
			if (_curWidth>_maxTextWidth){
				_maxHScroll=_curWidth-_maxTextWidth;
				_startHScroll=getMilli()+TEXTSCROLLDELAYTIME;
				_scrollTime = ((_maxHScroll/(double)screenWidth)/TEXTSCROLLPERSECOND)*1000;
			}else{
				_maxHScroll=0;
			}
			_curScrollDirection=0;
			// update vertical scroll info
			if (_choice>=_optionsOnScreen/2){
				_scrollOffset = _choice-_optionsOnScreen/2;
				if (_scrollOffset+_optionsOnScreen>_numOptions){
					_scrollOffset=_numOptions-_optionsOnScreen;
				}
			}else{
				_scrollOffset=0;
			}
		}
		if (wasJustPressed(BUTTON_B) && (_menuProp & MENUPROP_CANQUIT)){ // cancel
			_ret=-1;
			break;
		}
		// Check if user selected this one with A button or left or right (if enabled)
		signed char _didPress=0;
		if (_optionProp!=NULL && (_optionProp[_curRealIndex] & OPTIONPROP_LEFTRIGHT)){
			if (wasJustPressed(BUTTON_RIGHT)){
				_didPress=1;
			}else if (wasJustPressed(BUTTON_LEFT)){
				_didPress=-1;
			}
		}else{
			if (wasJustPressed(BUTTON_A)){
				_didPress=1;
			}
		}
		if (_didPress){
			_ret=menuIndexToReal(_choice,_showMap);
			// set the flag if the user pressed right to select this entry
			if (_returnInfo){
				if (_didPress==1){
					*_returnInfo|=MENURET_RIGHT;
				}
				if (isDown(BUTTON_L)){
					*_returnInfo|=MENURET_LBUTTON;
				}
			}
			break;
		}
		controlsEnd();
		startDrawing();
		if (_title!=NULL){
			drawText(easyCenter(textWidth(normalFont,_title),screenWidth),0,_title);
			gbSetDrawOffY(currentTextHeight*1.5);
		}
		// update horizontal scroll
		if (_maxHScroll!=0){
		startCalcOffset:
			;
			u64 _curTime = getMilli();
			int _xOff;
			if (_curTime>_startHScroll){
				int _diff = abs(_startHScroll-_curTime);
				if (_diff>_scrollTime){
					if (_diff>_scrollTime+TEXTSCROLLDELAYTIME){
						_startHScroll=_startHScroll+_scrollTime+TEXTSCROLLDELAYTIME;
						_curScrollDirection=!_curScrollDirection;
						goto startCalcOffset;
					}else{
						_xOff=_curScrollDirection ? 0 : _maxHScroll;
					}
				}else{
					_xOff = partMoveFills(_curTime,_startHScroll,_scrollTime,_maxHScroll);
					if (_curScrollDirection){
						_xOff=_maxHScroll-_xOff;
					}
				}
			}else{
				_xOff=0;
			}
			drawText(MENUOPTIONOFFSET-_xOff,(_choice-_scrollOffset)*currentTextHeight,_options[_curRealIndex]);
		}
		drawRectangle(0,0,MENUOPTIONOFFSET,screenHeight,0,0,0,255);
		drawText(MENUCURSOROFFSET,(_choice-_scrollOffset)*currentTextHeight,MENUCURSOR);
		// Draw the menu options
		int _lastDrawn=-1;
		// First, fast forward to the correct starting index according to scroll
		for (int i=0;i<_scrollOffset;++i){
			_lastDrawn = getNextEnabled(_showMap,_lastDrawn+1);
		}
		// Draw the entries currently on screen
		for (int i=0;i<_optionsOnScreen;++i){
			_lastDrawn  = getNextEnabled(_showMap,_lastDrawn+1);
			if (i==_choice-_scrollOffset && _maxHScroll!=0){
				continue;
			}
			// process properties for this option
			int _r = DEFAULTFONTCOLORR;
			int _g = DEFAULTFONTCOLORG;
			int _b = DEFAULTFONTCOLORB;
			if (_optionProp!=NULL){
				if (_optionProp[_lastDrawn] & OPTIONPROP_GOODCOLOR){
					if (_optionProp[_lastDrawn] & OPTIONPROP_BADCOLOR){
						_r=255;
						_g=127;
						_b=0;
					}else{
						_r=0;
						_g=255;
						_b=0;
					}
				}else if (_optionProp[_lastDrawn] & OPTIONPROP_BADCOLOR){
					_r=255;
					_g=0;
					_b=0;
				}
			}
			// draw the option
			gbDrawText(normalFont,MENUOPTIONOFFSET,currentTextHeight*i,_options[_lastDrawn],_r,_g,_b);
			if (_optionValues && _optionValues[_lastDrawn]){
				gbDrawText(normalFont,MENUOPTIONOFFSET+textWidth(normalFont,_options[_lastDrawn]),currentTextHeight*i,_optionValues[_lastDrawn],_r,_g,_b);
			}
		}
		if (_optionsOnScreen!=_numOptions && _scrollOffset!=_numOptions-_optionsOnScreen){
			drawText(MENUOPTIONOFFSET,currentTextHeight*_optionsOnScreen,"\\/\\/\\/\\/");
		}
		gbSetDrawOffY(0);
		if (_drawHook && _drawHook(menuIndexToReal(_choice,_showMap))){
			break;
		}
		endDrawing();
	}
	controlsEnd();
	return _ret;
}
// returns -1 if user quit, otherwise returns chosen index
// pass the real _choice index
// does not support horizontal scrolling for options with _optionValues
int showMenuAdvanced(int _choice, const char* _title, int _mapSize, char** _options, char** _optionValues, char* _showMap, optionProp* _optionProp, char* _returnInfo, char _menuProp, inttakeretfunc _drawHook){
	return gbHasButtons()==GBPREFERRED ? showMenuAdvancedButton(_choice,_title,_mapSize,_options,_optionValues,_showMap,_optionProp,_returnInfo,_menuProp,_drawHook) : showMenuAdvancedTouch(_choice,_title,_mapSize,_options,_optionValues,_showMap,_optionProp,_returnInfo,_menuProp,0,0,screenWidth,screenHeight,_drawHook);
}
int showMenu(int _defaultChoice, const char* _title, int _numOptions, char** _options, char _canQuit){
	char _menuProps=MENUPROP_CANPAGEUPDOWN;
	if (_canQuit){
		_menuProps|=MENUPROP_CANQUIT;
	}
	return showMenuAdvanced(_defaultChoice,_title,_numOptions,_options,NULL,NULL,NULL,NULL,_menuProps,NULL);
}
char* _bottomString;
int _titleScreenDraw(int _choice){
	int _y = screenHeight-5-currentTextHeight;
	drawText(5,_y,_bottomString);
	gbDrawText(normalFont,(screenWidth-5)-textWidth(normalFont,VERSIONSTRING VERSIONSTRINGSUFFIX),_y,VERSIONSTRING VERSIONSTRINGSUFFIX,VERSIONCOLOR);
	return 0;
}
void TitleScreen(){
	_bottomString=easyCombineStrings(2,SYSTEMSTRING,GBPLAT!=GB_3DS ? (isActuallyUsingUma0 ? ";uma0" : ";ux0") : (""));
	while (currentGameStatus!=GAMESTATUS_QUIT){
		char* _options[5]={"Load game","Manual mode","Basic settings",NULL,"Exit"};
		_options[3]=(playerLanguage ? "JP" : "EN");
		char _showMap[5];
		memset(_showMap,1,sizeof(_showMap));
		int _choice=showMenuAdvanced(0,"Main Menu",sizeof(_options)/sizeof(char*),_options,NULL,_showMap,NULL,NULL,MENUPROP_CANPAGEUPDOWN, _titleScreenDraw);
		if (_choice==0){
			PlayMenuSound(); 
			if (currentPresetFilename==NULL){
				currentPresetChapter=0;
				controlsEnd();
				currentGameStatus=GAMESTATUS_GAMEFOLDERSELECTION;
			}
			break;
		}else if (_choice==1){
			PlayMenuSound();
			controlsEnd();
			char* _chosenGameFolder;
			if (FileSelector(gamesFolder,&_chosenGameFolder,(char*)"Select a game")==2 || _chosenGameFolder==NULL){
				continue;
			}
			char _tempNewStreamingAssetsPathbuffer[strlen(gamesFolder)+strlen(_chosenGameFolder)+1];
			strcpy(_tempNewStreamingAssetsPathbuffer,gamesFolder);
			strcat(_tempNewStreamingAssetsPathbuffer,_chosenGameFolder);
			GenerateStreamingAssetsPaths(_tempNewStreamingAssetsPathbuffer,0);
			free(_chosenGameFolder);
			if (!directoryExists(scriptFolder)){
				controlsEnd();
				easyMessagef(1,"%s does not exist.",scriptFolder);
				continue;
			}
			controlsEnd();
			char* _tempManualFileSelectionResult;
			FileSelector(scriptFolder,&_tempManualFileSelectionResult,(char*)"Select a script");
			controlsReset();
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
				}else{
					currentGameStatus=GAMESTATUS_MAINGAME;
					activateHigurashiSettings();
					RunScript(scriptFolder,_tempManualFileSelectionResult,0);
					free(_tempManualFileSelectionResult);
					currentGameStatus=GAMESTATUS_TITLE;
				}
			}
		}else if (_choice==2){ // Go to setting menu
			controlsEnd();
			SettingsMenu(0,0,0,0,1,0,0,0,0);
			controlsEnd();
			break;
		}else if (_choice==3){
			playerLanguage=!playerLanguage;
			SaveSettings();
			currentGameStatus=GAMESTATUS_QUIT;
			break;
		}else if (_choice==4){ // Quit button
			currentGameStatus=GAMESTATUS_QUIT;
			break;
		}else{
			_choice=0;
		}
	}
}
void TipMenu(){
	if (currentPresetTipUnlockList.theArray[currentPresetChapter]==0){
		easyMessagef(1,"No TIPS unlocked.");
		currentGameStatus=GAMESTATUS_NAVIGATIONMENU;
		controlsEnd();
		return;
	}
	char** _options = malloc(sizeof(char*)*currentPresetTipUnlockList.theArray[currentPresetChapter]);
	int i;
	for (i=0;i<currentPresetTipUnlockList.theArray[currentPresetChapter];++i){
		_options[i] = tipNamesLoaded ? currentPresetTipNameList.theArray[i] : currentPresetTipList.theArray[i];
	}
	int _chosenIndex=0;
	while(1){
		_chosenIndex = showMenu(_chosenIndex,"TIP Menu",currentPresetTipUnlockList.theArray[currentPresetChapter],_options,1);		
		if (_chosenIndex==-1){
			#if PLAYTIPMUSIC == 1
				StopBGM();
			#endif
			break;
		}else{
			controlsReset();
			// This will trick the in between lines functions into thinking that we're in normal script execution mode and not quit
			currentGameStatus=GAMESTATUS_MAINGAME;
			RunScript(scriptFolder, currentPresetTipList.theArray[_chosenIndex],1);
			currentGameStatus=GAMESTATUS_NAVIGATIONMENU;
			controlsReset();
			continue;
		}
	}
	free(_options);		
}
void ChapterJump(){
	char** _options = malloc(sizeof(char*)*(currentPresetChapter+1));
	char* _showMap = NULL;
	int i;
	for (i=0;i<=currentPresetChapter;++i){
		if (chapterNamesLoaded){
			char* _goodName = currentPresetFileFriendlyList.theArray[i];
			_options[i]=_goodName;
			if (strcmp(_goodName,HIDDENCHAPTERTITLE)==0){
				if (!_showMap){
					_showMap = newShowMap(currentPresetChapter+1);
				}
				_showMap[i]=0;
			}
		}else{
			_options[i]=currentPresetFileList.theArray[i];
		}
	}
	int _chosenIndex=0;
	while(1){
		_chosenIndex=showMenuAdvanced(_chosenIndex,"Chapter Jump",currentPresetChapter+1,_options,NULL,_showMap,NULL,NULL,MENUPROP_CANPAGEUPDOWN|MENUPROP_CANQUIT,NULL);
		if (_chosenIndex==-1){
			break;
		}else{
			currentGameStatus=GAMESTATUS_MAINGAME;
			RunScript(scriptFolder,currentPresetFileList.theArray[_chosenIndex],1);
			continue;
		}
	}
	free(_options);
	free(_showMap);
}
void SaveGameEditor(){
	controlsEnd();
	while (1){
		controlsStart();
		currentPresetChapter = retMenuControlsLow(currentPresetChapter,0,0,1,1,0,currentPresetFileList.length-1);
		if (wasJustPressed(BUTTON_A)){
			saveHiguGame();
			controlsEnd();
			break;
		}
		controlsEnd();
		startDrawing();
		if (chapterNamesLoaded==0){
			gbDrawTextf(normalFont,MENUOPTIONOFFSET,currentTextHeight,DEFAULTFONTCOLOR,255,"%d",currentPresetChapter);
		}else{
			drawText(MENUOPTIONOFFSET, currentTextHeight, currentPresetFileFriendlyList.theArray[currentPresetChapter]);
		}
		drawText(MENUOPTIONOFFSET, screenHeight-currentTextHeight*3, "Welcome to the save file editor!");
		drawText(MENUOPTIONOFFSET, screenHeight-currentTextHeight*2, SELECTBUTTONNAME" - Finish and save");
		drawText(MENUOPTIONOFFSET, screenHeight-currentTextHeight, "Left and Right - Change last completed chapter");
		endDrawing();
	}
}
void controls_setDefaultGame(){
	if (wasJustPressed(BUTTON_X)){
		if (!isEmbedMode && LazyChoicef(defaultGameIsSet ? "Unset this game as the default?" : "Set this game as the default game?")){
			defaultGameIsSet = !defaultGameIsSet;
			setDefaultGame(defaultGameIsSet ? currentGameFolderName : "NONE");
		}
	}
}
void NavigationMenu(){
	char* _menuTitle;
	if (chapterNamesLoaded){
		char* _goodName=currentPresetFileFriendlyList.theArray[currentPresetChapter];
		if (strcmp(_goodName,HIDDENCHAPTERTITLE)!=0){
			_menuTitle=easySprintf("End of script: %s",_goodName);
		}else{
			_menuTitle=strdup("---"); // we still want a title because it'll keep the menu a familiar distance from the top of the screen
		}
	}else{
		_menuTitle=easySprintf("End of script: %d",currentPresetChapter);
	}
	char* _menuOptions[] = {
		"Next",
		"Chapter Jump",
		"View TIPS",
		"Fragments",
		"Exit",
	};
	char* _optionOn = newShowMap(5);
	int _choice=0;
	while(currentGameStatus!=GAMESTATUS_QUIT){
		//
		_optionOn[2]=(gameHasTips && currentPresetTipUnlockList.theArray[currentPresetChapter]>0);
		_optionOn[0]=(currentPresetChapter+1<currentPresetFileList.length);
		{
			_optionOn[3]=fragmentsModeOn();
			if (_optionOn[3]){
				_optionOn[0]=0;
				if (!fragmentInfo){ // initial fragment info load
					char* _fullPath = easyCombineStrings(2,streamingAssets,"Data/fragmentdata.txt");
					parseFragmentFile(_fullPath);
					free(_fullPath);
				}
			}
		}
		//
		_choice = showMenuAdvanced(_choice,_menuTitle,5,_menuOptions,NULL,_optionOn,NULL,NULL,0,NULL);
		if (_choice==0){
			printf("Go to next chapter\n");
			if (currentPresetChapter+1==currentPresetFileList.length){
				easyMessagef(1,"There is no next chapter.");
			}else{
				currentPresetChapter++;
				currentGameStatus=GAMESTATUS_MAINGAME;
				break;
			}
		}else if (_choice==1){
			ChapterJump();
		}else if (_choice==2){
			#if PLAYTIPMUSIC == 1
				PlayBGM("lsys14",256);
			#endif
			TipMenu();
		}else if (_choice==3){
			fragmentMenu();
		}else if (_choice==4){
			currentGameStatus=GAMESTATUS_QUIT;
		}
	}
	free(_optionOn);
	free(_menuTitle);
	controlsEnd();
}
// Returns selected slot or -1
int vndsSaveSelector(char _isSave){
	#ifdef OVERRIDE_VNDSSAVEMENU
		return customVNDSSaveSelector(_isSave);
	#endif
	controlsReset();
	// screenWidth/3/2 free space for each text
	int _slotWidth = screenWidth/SAVEMENUPAGEW;
	int _slotHeight = screenHeight/SAVEMENUPAGEH;

	crossTexture* _loadedThumbnail[SAVEMENUPAGESIZE]={NULL};
	char* _loadedTextThumb[SAVEMENUPAGESIZE]={NULL};

	int _ret=-2;
	char _reloadThumbs=1;
	// Preserve these between calls for easy slot selection
	static int _selected=0;
	static int _slotOffset=0;
	while(1){
		int i;
		controlsStart();
		if (wasJustPressed(BUTTON_RIGHT)){
			if (isDown(BUTTON_R)){
				_slotOffset+=SAVEMENUPAGESIZE*5;
				_reloadThumbs=1;
			}else{
				if ((_selected&1)==1){
					_slotOffset+=SAVEMENUPAGESIZE;
					--_selected;
					_reloadThumbs=1;
				}else{
					++_selected;
				}
			}
		}if (wasJustPressed(BUTTON_LEFT)){
			if (isDown(BUTTON_R)){
				_slotOffset-=SAVEMENUPAGESIZE*5;
				_reloadThumbs=1;
			}else{
				if ((_selected&1)==0){
					_slotOffset-=SAVEMENUPAGESIZE;
					++_selected;
					_reloadThumbs=1;
				}else{
					--_selected;
				}
			}
		}if (wasJustPressed(BUTTON_UP)){
			_selected = wrapNum(_selected-SAVEMENUPAGEW,0,SAVEMENUPAGESIZE-1);
		}if (wasJustPressed(BUTTON_DOWN)){
			_selected = wrapNum(_selected+SAVEMENUPAGEW,0,SAVEMENUPAGESIZE-1);
		}if (wasJustPressed(BUTTON_A)){
			_ret=_selected+_slotOffset;
		}if (wasJustPressed(BUTTON_B)){
			_ret=-1;
		}
		controlsEnd();
		// Free thumbs before leave or on page switch
		if (_ret!=-2 || _reloadThumbs){
			for (i=0;i<SAVEMENUPAGEH;++i){
				int j;
				for (j=0;j<SAVEMENUPAGEW;++j){
					int _tempIndex = j+i*SAVEMENUPAGEW;
					if (_loadedThumbnail[_tempIndex]!=NULL){
						freeTexture(_loadedThumbnail[_tempIndex]);
						_loadedThumbnail[_tempIndex]=NULL;
					}
					if (_loadedTextThumb[_tempIndex]!=NULL){
						free(_loadedTextThumb[_tempIndex]);
						_loadedTextThumb[_tempIndex]=NULL;
					}
				}
			}
			if (_ret!=-2){
				break;
			}
		}
		if (_reloadThumbs){
			easyMessagef(0,"Loading thumbnails...");
			if (_slotOffset<0){
				_slotOffset=MAXSAVESLOT-SAVEMENUPAGESIZE;
			}else if (_slotOffset>MAXSAVESLOT-SAVEMENUPAGESIZE){
				_slotOffset=0;
			}
			_reloadThumbs=0;
			for (i=0;i<SAVEMENUPAGEH;++i){
				int j;
				for (j=0;j<SAVEMENUPAGEW;++j){
					int _tempIndex = j+i*SAVEMENUPAGEW;
					int _trueIndex = _tempIndex+_slotOffset;
					char* _tempFilename = easyVNDSSaveName(_trueIndex);
					if (checkFileExist(_tempFilename)){
						FILE* fp = fopen(_tempFilename,"rb");
						unsigned char _readFileFormat;
						fread(&_readFileFormat,sizeof(unsigned char),1,fp);
						if (validVNDSSaveFormat(_readFileFormat)){
							skipLengthStringInFile(fp); // Skip script filename
							fseek(fp,sizeof(long int),SEEK_CUR); // Seek past position and maxLines
							int _readMaxLines;
							fread(&_readMaxLines,sizeof(int),1,fp);
							int _displayLine;
							fread(&_displayLine,sizeof(int),1,fp);
							// This next block of code loads the text thumbnail. Extra code is used to support both VNDS games (which have their currentLine variable set to the next line they'll use, which is empty) and OutputLine games (which have their currentLine variable anywhere, including on a line that has text). This loading code checks if the text on the currentLine line has text on it. If it does, it'll show that one. Otherwise it will go back a line and show that one.
							//
							// Skip all but two lines before where we are
							int k;
							for (k=0;k<_displayLine-1;++k){
								skipLengthStringInFile(fp);
							}
							// Read string right before where we are. If our primary line doesn't look good, this one will be used.
							char* _backupString = readLengthStringFromFile(fp);
							// If the display line is 0 then no lines were skipped and our backup string is actually our primary string
							if (_displayLine!=0 && _displayLine!=_readMaxLines){
								// If our current line isn't empty, we'll use that. Otherwise we'll use the backup line.
								char* _possiblePrimary = readLengthStringFromFile(fp);
								if (strlen(_possiblePrimary)!=0){
									free(_backupString);
									_backupString = _possiblePrimary;
								}else{
									free(_possiblePrimary);
								}
							}
							_loadedTextThumb[_tempIndex] = _backupString;

							char _thumbFilename[strlen(_tempFilename)+7];
							strcpy(_thumbFilename,_tempFilename);
							strcat(_thumbFilename,".thumb");
							if (checkFileExist(_thumbFilename)){
								_loadedThumbnail[_tempIndex]=loadImage(_thumbFilename);
							}
						}else{
							_loadedTextThumb[_tempIndex]=NULL;
						}
						fclose(fp);
					}else{
						_loadedTextThumb[_tempIndex]=NULL;
					}
					free(_tempFilename);
				}
			}
		}
		startDrawing();
		char _labelBuffer[12+strlen(VNDSSAVESELSLOTPREFIX)];
		for (i=0;i<SAVEMENUPAGEH;++i){
			int j;
			for (j=0;j<SAVEMENUPAGEW;++j){
				int _tempIndex = j+i*SAVEMENUPAGEW;
				unsigned char _r;
				unsigned char _g;
				unsigned char _b;
				if (_selected==_tempIndex){
					_r=0;
					_g=255;
					_b=0;
				}else{
					_r=255;
					_g=255;
					_b=255;
				}
				// Thumb goes behind everything else
				if (_loadedThumbnail[_tempIndex]!=NULL){
					int _destW;
					int _destH;
					fitInBox(getTextureWidth(_loadedThumbnail[_tempIndex]),getTextureHeight(_loadedThumbnail[_tempIndex]),_slotWidth,_slotHeight,&_destW,&_destH);
					drawTextureSized(_loadedThumbnail[_tempIndex],(j+1)*_slotWidth-_destW,i*_slotHeight+easyCenter(_destH,_slotHeight),_destW,_destH);
				}
				sprintf(_labelBuffer,VNDSSAVESELSLOTPREFIX"%d",_tempIndex+_slotOffset);
				if (_loadedTextThumb[_tempIndex]==NULL){
					strcat(_labelBuffer," (Empty)");
				}else{
					char** _wrappedLines;
					int _numLines;
					wrapText(_loadedTextThumb[_tempIndex],&_numLines,&_wrappedLines,_slotWidth-5);
					int k;
					for (k=0;k<_numLines;++k){
						drawText(j*_slotWidth+5,i*_slotHeight+5+currentTextHeight*(k+1),_wrappedLines[k]);
					}
					freeWrappedText(_numLines,_wrappedLines);
				}
				drawText(j*_slotWidth+5,i*_slotHeight+5,_labelBuffer);
				drawHallowRect(j*_slotWidth,i*_slotHeight,_slotWidth,_slotHeight,SAVESELECTORRECTTHICK,_r,_g,_b,255);
			}
		}
		
		endDrawing();
	}
	return _ret;
}
// Hold L to disable font loading
// Hold R to disable all optional loading
void VNDSNavigationMenu(){
	#ifdef OVERRIDE_VNDSNAVIGATION
		customVNDSNavigationMenu();
		return;
	#endif
	if (!textDisplayModeOverriden){
		switchTextDisplayMode(preferredTextDisplayMode);
	}
	controlsStart();
	signed char _choice=0;
	char* _loadedNovelName=NULL;

	crossTexture* _loadedThumbnail=NULL;

	//
	char _possibleThunbnailPath[strlen(streamingAssets)+strlen("/SEArchive.legArchive.legList")+1];
	strcpy(_possibleThunbnailPath,streamingAssets);
	strcat(_possibleThunbnailPath,"/vndsvitaproperties");
	if (checkFileExist(_possibleThunbnailPath)){
		easyMessagef(0,"Loading properties");
		FILE* fp = fopen(_possibleThunbnailPath,"rb");
		char _loadedVersionNumber;
		fread(&_loadedVersionNumber,1,1,fp);
		fclose(fp);
	}else{
		if (!defaultGameIsSet && !isEmbedMode){
			//LazyMessage("VNDSVita Game Converter < v1.1",NULL,"Game may crash.",NULL);
		}
		printf("Is old game converter.\n");
	}
	//
	strcpy(_possibleThunbnailPath,streamingAssets);
	strcat(_possibleThunbnailPath,"/thumbnail.png");
	if (checkFileExist(_possibleThunbnailPath) && !isDown(BUTTON_R)){
		easyMessagef(0,"Loading thumbnail");
		_loadedThumbnail = loadImage(_possibleThunbnailPath);
	}
	//
	_possibleThunbnailPath[strlen(streamingAssets)]=0;
	strcat(_possibleThunbnailPath,"/info.txt");
	if (checkFileExist(_possibleThunbnailPath) && !isDown(BUTTON_R)){
		easyMessagef(0,"Loading info.txt");
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
	if (checkFileExist(_possibleThunbnailPath) && !isDown(BUTTON_R)){
		easyMessagef(0,"Loading img.ini");
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
	_possibleThunbnailPath[strlen(streamingAssets)]=0;
	strcat(_possibleThunbnailPath,"/SEArchive.legArchive");
	if (checkFileExist(_possibleThunbnailPath)){
		easyMessagef(0,"Loading sound archive");
		soundArchive = loadLegArchive(_possibleThunbnailPath);
		useSoundArchive=1;
	}
	easyMessagef(0,"Body is ready.");
	
	controlsEnd();
	while (currentGameStatus!=GAMESTATUS_QUIT){
		controlsStart();

		_choice = menuControls(_choice,0,3);
		if (wasJustPressed(BUTTON_A)){
			if (_choice<=1){
				forceResettingsButton=0;
				if (_choice==0){
					int _chosenSlot = vndsSaveSelector(0);
					if (_chosenSlot!=-1){
						char* _loadPath = easyVNDSSaveName(_chosenSlot);
						if (checkFileExist(_loadPath)){
							vndsNormalLoad(_loadPath,1);
						}else{
							easyMessagef(1,"Save file %s does not exist",_loadPath);
						}
						free(_loadPath);
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
						easyMessagef(1,"Main script file %s does not exist.",_vndsMainScriptConcat);
					}
				}
			}else if (_choice==2){
				SettingsMenu(0,1,0,0,0,0,1,1,0);
			}else if (_choice==3){
				currentGameStatus = GAMESTATUS_QUIT;
			}
		}

		if (wasJustPressed(BUTTON_SELECT) && isDown(BUTTON_L) && isDown(BUTTON_R)){
			if (LazyChoicef("Make thumbnails from old saves?")){
				// Before thumbnails, 255 was the max slot
				int i;
				for (i=0;i<=255;++i){
					easyMessagef(0,"%d/255",i);
					char* _nextFilename = easyVNDSSaveName(i);
					if (checkFileExist(_nextFilename)){
						vndsNormalLoad(_nextFilename,0);
						vndsNormalSave(_nextFilename,0,1);
						crossfclose(nathanscriptCurrentOpenFile);
						nathanscriptCurrentOpenFile=NULL;
					}
					free(_nextFilename);
				}
			}
		}

		controls_setDefaultGame();
		controlsEnd();
		startDrawing();

		if (_loadedThumbnail!=NULL){
			drawTexture(_loadedThumbnail,screenWidth-getTextureWidth(_loadedThumbnail),screenHeight-getTextureHeight(_loadedThumbnail));
		}

		drawText(MENUOPTIONOFFSET,0,_loadedNovelName);

		drawText(MENUOPTIONOFFSET,5+currentTextHeight*(0+2),"Load Save");
		drawText(MENUOPTIONOFFSET,5+currentTextHeight*(1+2),"New Game");
		drawText(MENUOPTIONOFFSET,5+currentTextHeight*(2+2),"VNDS Settings");
		drawText(MENUOPTIONOFFSET,5+currentTextHeight*(3+2),"Exit");

		drawText(5,5+currentTextHeight*(_choice+2),MENUCURSOR);

		endDrawing();
	}
	free(_loadedNovelName);
}
// =====================================================
char initializeLua(){
	if (L==NULL){
		L = luaL_newstate();
		luaL_openlibs(L);
		initLuaWrappers();

		lua_pushnumber(L,GBPLAT);
		lua_setglobal(L,"GBPLAT");
		
		// happy.lua contains functions that both Higurashi script files use and my C code
		char* _fixedPath = fixPathAlloc("assets/happy.lua",TYPE_EMBEDDED);
		char _didFailLoad = SafeLuaDoFile(L,_fixedPath);
		free(_fixedPath);
		//
		lua_getglobal(L,"globalFlags");
		lua_pushnumber(L,playerLanguage);
		lua_setfield(L,-2,"GLanguage");
		//
		lua_sethook(L, incrementScriptLineVariable, LUA_MASKLINE, 5);
		return _didFailLoad ? 2 : 0;
	}
	return 0;
}
// These functions do some stuff with the data and usually eventually call the same functions that the Lua functions do.
#include "VNDSScriptWrappers.h"
void initializeNathanScript(){
	if (!nathanscriptIsInit){
		nathanscriptIsInit=1;
		nathanscriptInit();
		nathanscriptIncreaseMaxFunctions(nathanCurrentMaxFunctions+14);
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
		nathanscriptAddFunction(vndswrapper_advname,0,"advname");
		nathanscriptAddFunction(vndswrapper_advnameim,0,"advnameim");

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
// All init after this assumes this is avalible
void hVitaCrutialInit(int argc, char** argv){
	srand(time(NULL));
	generalGoodInit();
	{
		int _widthRequest=960;
		int _heightRequest=544;
		for (int i=0;i<argc;++i){
			if (strcmp(argv[i],"--size")==0){
				_widthRequest=atoi(argv[i+1]);
				_heightRequest=atoi(argv[i+2]);
				i+=2;
			}
		}
		initGraphics(_widthRequest,_heightRequest,WINDOWFLAG_EXTRAFEATURES);
	}
	screenWidth = getScreenWidth();
	screenHeight = getScreenHeight();
	initImages();
	setClearColor(0,0,0);
	isActuallyUsingUma0=initGoodBrewDataDir();
	controlsInit();
	#if GBPLAT == GB_3DS
		osSetSpeedupEnable(1);
	#endif
}
// verify install
void hVitaCheckVpk(){
	char* _embeddedCheckPath = fixPathAlloc("assets/happy.lua",TYPE_EMBEDDED);
	if (!checkFileExist(_embeddedCheckPath)){
		while(1){
			controlsReset();
			startDrawing();
			drawRectangle(0,0,20,100,255,0,0,255);
			drawRectangle(20,0,30,15,255,0,0,255);
			drawRectangle(20,35,30,15,255,0,0,255);
			endDrawing();
		}
	}
	free(_embeddedCheckPath);
}
// Must come before font loading because it loads font size
void hVitaInitSettings(){
	// This will also load the font size file and therefor must come before font loading
	// Will not crash if no settings found
	LoadSettings();
	// Check if the application came with a game embedded. If so, load it.
	char* _fixedPath = fixPathAlloc("isEmbedded.txt",TYPE_EMBEDDED);
	if (checkFileExist(_fixedPath)){
		isEmbedMode=1;
		free(gamesFolder);
		gamesFolder = fixPathAlloc("",TYPE_EMBEDDED);
		currentGameFolderName = strdup("game");
		currentGameStatus = GAMESTATUS_LOADGAMEFOLDER;
	}else{
		controlsStart();
		if (!isDown(BUTTON_R)){ // Hold R to skip default game check
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
	free(_fixedPath);
}
void hVitaInitFont(){
	// Load default font
	if (fontSize<0){
		fontSize = getResonableFontSize(GBTXT);
	}
	currentFontFilename = fixPathAlloc(DEFAULTEMBEDDEDFONT,TYPE_EMBEDDED);
	reloadFont(fontSize,1);
}
// relies on font for error messages
void hVitaInitSound(){
	if (initAudio()){
		#if GBPLAT == GB_3DS
			easyMessagef(1,"dsp init failed. Do you have dsp firm dumped and in /3ds/dspfirm.cdc ?");
		#else
			easyMessagef(1,"audio init failed. isn't supposed to be possible...");
		#endif
	}	
	// Load the menu sound effect if it's present
	char* _fixedPath = fixPathAlloc("assets/wa_038.ogg",TYPE_EMBEDDED);
	TryLoadMenuSoundEffect(_fixedPath);
	free(_fixedPath);
	#if GBPLAT == GB_VITA && GBSND != GBSND_VITA
		// Create the protection thread.
		if (pthread_create(&soundProtectThreadId, NULL, &soundProtectThread, NULL) != 0){
			return 2;
		}
	#endif
	#if GBPLAT == GB_3DS
		// Create the sound update thread
		s32 _foundMainThreadPriority = 0;
		svcGetThreadPriority(&_foundMainThreadPriority, CUR_THREAD_HANDLE);
		_3dsSoundUpdateThread = threadCreate(soundUpdateThread, NULL, 4 * 1024, _foundMainThreadPriority-1, -2, false);
	#endif
}
// no other init functions rely on this one, and this one relies only on crutial.
// but this will overwirte maxLines if set by font function
void hVitaInitMisc(){
	int i;
	changeMaxLines(15);
	increaseBustArraysSize(0,maxBusts);	
	ClearMessageArray(0);
	for (i=0;i<maxBusts;i++){
		ResetBustStruct(&(Busts[i]),0);
	}
	// Reset bust cache
	// I could memset everything to 0, but apparently NULL is not guaranteed to be represented by all 0.
	// https://stackoverflow.com/questions/9894013/is-null-always-zero-in-c
	for (i=0;i<MAXBUSTCACHE;++i){
		bustCache[i].filename=NULL;
		bustCache[i].image=NULL;
	}
	//
	textboxWidth = screenWidth;
	outputLineScreenHeight = screenHeight;
	// Guess the graphic sizes
	actualBackgroundWidth = screenWidth;
	actualBackgroundHeight = screenHeight;
	actualBackgroundSizesConfirmedForSmashFive=0;
	//
	saveFolder = malloc(strlen(gbDataFolder)+strlen("Saves/")+1);
	strcpy(saveFolder,gbDataFolder);
	strcat(saveFolder,"Saves/");
	gamesFolder = malloc(strlen(gbDataFolder)+strlen("Games/")+1);
	strcpy(gamesFolder,gbDataFolder);
	strcat(gamesFolder,"Games/");
	// Make file paths with default StreamingAssets folder
	GenerateStreamingAssetsPaths("StreamingAssets",1);
	// Save folder, data folder, and others
	createRequiredDirectories();
	//
	ClearDebugFile();
}
signed char init(int argc, char** argv){
	#ifdef OVERRIDE_INIT
		return customInit();
	#endif
	hVitaCrutialInit(argc, argv);
	hVitaCheckVpk();
	hVitaInitMisc();
	hVitaInitSettings();
	hVitaInitFont();
	hVitaInitSound();
	refreshGameState();
	return initializeLua();
}
#ifdef SPECIALEDITION
	#include "specialEditionFooter.h"
#endif
int main(int argc, char *argv[]){
	/* code */
	if (init(argc,argv)==2){
		currentGameStatus = GAMESTATUS_QUIT;
	}

	while (currentGameStatus!=GAMESTATUS_QUIT){
		switch (currentGameStatus){
			case GAMESTATUS_TITLE:
				TitleScreen();
				break;
			case GAMESTATUS_MAINGAME:
			{
				char _didWork = RunScript(scriptFolder, currentPresetFileList.theArray[currentPresetChapter], 1);
				if (currentPresetFileList.length!=0){
					if (_didWork==0){ // If the script didn't run, don't advance the game
						currentPresetChapter--; // Go back a script
						if (currentPresetChapter<0 || currentPresetChapter==255){ // o, no, we've gone back too far!
							easyMessagef(1,"So... the first script failed to launch. You now have the info on why, so go try and fix it. Pressing "SELECTBUTTONNAME" will close the application.");
							currentGameStatus=GAMESTATUS_QUIT;
						}else{
							currentGameStatus=GAMESTATUS_NAVIGATIONMENU;
						}
					}else{
						saveHiguGame();
					}
					if (currentGameStatus!=GAMESTATUS_QUIT){
						currentGameStatus=GAMESTATUS_NAVIGATIONMENU;
					}
				}else{
					currentGameStatus=GAMESTATUS_TITLE;
				}
				break;
			}
			case GAMESTATUS_NAVIGATIONMENU:
				// Menu for chapter jump, tip selection, and going to the next chapter
				NavigationMenu();
				break;
			case GAMESTATUS_GAMEFOLDERSELECTION: // Sets game folder selection folder name to currentGameFolderName
			{
				char* _chosenGameFolder;
				if (FileSelector(gamesFolder,&_chosenGameFolder,(char*)"Select a game")==2){
					easyMessagef(1,"No folders found. After running the script converter you should've put the converted files in %s",gamesFolder);
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
			}
			case GAMESTATUS_LOADGAMEFOLDER: // Can load both Higurashi and VNDS games
			{
				char _filePath[strlen(gamesFolder)+strlen(currentGameFolderName)+strlen("/Scripts/main.scr")+1];
				strcpy(_filePath,gamesFolder);
				strcat(_filePath,currentGameFolderName);
				char* _filenamePart=&(_filePath[0])+strlen(gamesFolder)+strlen(currentGameFolderName);
				if (!isDown(BUTTON_L) && !isDown(BUTTON_R)){
					strcpy(_filenamePart,"/default.ttf");
					if (checkFileExist(_filePath)){
						easyMessagef(0,"Loading font");
						globalLoadFont(_filePath);
					}
				}
				strcpy(_filenamePart,"/isvnds");
				if (checkFileExist(_filePath)){
					initializeNathanScript();
					// Special settings for vnds
					activateVNDSSettings();
					// Setup StreamingAssets path
					*_filenamePart='\0';
					GenerateStreamingAssetsPaths(_filePath,0);
					currentGameStatus = GAMESTATUS_NAVIGATIONMENU;
					// VNDS games also support game specific lua
					LoadGameSpecificStupidity();
					VNDSNavigationMenu();
				}else{
					if (!loadHiguGameFolder(currentGameFolderName)){
						currentGameStatus=GAMESTATUS_TITLE;
						if (defaultGameIsSet){
							setDefaultGame("NONE"); // Prevent this message every time on startup if it's because of default game setting
							easyMessagef(1,"Reset default game setting.");
						}
					}
				}
				break;
			}
		}
	}
	printf("ENDGAME\n");
	//QuitApplication(L);
	quitGraphics();
	quitAudio();
	generalGoodQuit();
	return 0;
}
