/*
	(OPTIONAL TODO)
		TODO - (Optional) Italics
			OutputLine(NULL, "　……知レバ、…巻キ込マレテシマウ…。",
			   NULL, "...<i>If she found out... she would become involved</i>...", Line_Normal);

			(Here's the problem, It'll be hard to draw non italics text and italics in the same line)
			(A possible solution is to store x cordinates to start italics text)
				// Here's the plan.
				// Make another message array, but store text that is in italics in it
				// Can I combine color with this?
		TODO - Position markup
			At the very end of Onikakushi, I think that there's a markup that looks something like this <pos=36>Keechi</pos>
		TODO - Remove scriptFolder variable
		TODO - Inspect SetDrawingPointOfMessage
			It appears to just set the line to draw on, or that's at least what it's usually used for.
			Inspect what the max line I can use in it is.
			Think about how I could implement this command if the value given is bigger than the total number of lines
				Change the actual text box X and text box Y and use the input arg as a percentage of the screen?
			How does this work in ADV mode?
				Actually, the command is removed in ADV mode.
		TODO - Allow VNDS sound command to stop all sounds
		TODO - SetSpeedOfMessage
		TODO - Sort files in file browser
	TODO - Mod libvita2d to not inlcude characters with value 1 when getting text width. (This should be easy to do. There's a for loop)
	TODO - is entire font in memory nonsense still needed
	TODO - Fix this text speed setting nonsense
	TODO - Game specific settings files
	TODO - Draw text with color properties. to allow having colors for every individual character, make a duplicate array, type uint64_t, first three bytes for color, last byte for text property flags
	TODO - in manual mode, running _GameSpecific.lua first won't keep the settings from being reset by activeVNDSSettings called before manual script.s
	TODO - i removed the secret save file editor code
				if (_codeProgress==4){
					SaveGameEditor();
					_nextChapterExist=1;
					_codeProgress=0;
				}
	TODO - add veritcal das to showMenu
	TODO - textbox alpha should change with background alpha
	TODO - what is this LazyChoice nonsense?
	TODO - Fix old character art peeks out the edge of textbox
	TODO - is it possible to reused showmenu for the title screen by cachign all info in a struct and passing that to a draw function?

	Colored text example:
		text x1b[<colorID>;1m<restoftext>
		text x1b[0m
*/
// This is pretty long because foreign characters can take two bytes
#define SINGLELINEARRAYSIZE 300
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
	#include <psp2/display.h> // used with thumbnail creation
	#include <psp2/power.h> // overclock
	#include <psp2/ctrl.h> // sound protect thread
#endif
#include "legarchive.h"
#include "../stolenCode/customgetline.h"

#define LOCATION_UNDEFINED 0
#define LOCATION_CG 1
#define LOCATION_CGALT 2
/////////////////////////////////////
#define MAXBUSTCACHE 8
#define MAXIMAGECHAR 20
#define MAXFILES 50
#define MAXFILELENGTH 51
#define MAXMESSAGEHISTORY 40
#define VERSIONSTRING "forgotversionnumber" // This
#define VERSIONNUMBER 8 // This
#define VERSIONSTRINGSUFFIX ""
#define VERSIONCOLOR 255,135,53 // It's Rena colored!
// Specific constants
#if GBPLAT != GB_3DS
	#define SELECTBUTTONNAME "X"
	#define BACKBUTTONNAME "O"
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

#define DROPSHADOWOFFX 1
#define DROPSHADOWOFFY 1

// showMenu
// ratio of screen width that the text will scroll in one second
#define TEXTSCROLLPERSECOND .25
#define TEXTSCROLLDELAYTIME 300
// bitmap of option propterties for showMenuAdvanced
// use the optionProp type for these
#define OPTIONPROP_LEFTRIGHT 1
#define OPTIONPROP_GOODCOLOR 2
#define OPTIONPROP_BADCOLOR	 4
//
#define MENUPROP_CANQUIT 1
#define MENUPROP_CANPAGEUPDOWN 2
// return bitmap of _returnInfo from showMenuAdvanced
// if the user pressed right to select this option
#define MENURET_RIGHT 1
// if the user held L when pressing the button
#define MENURET_LBUTTON 2

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

#define STUPIDTEXTYOFF 12
#define totalTextYOff() (STUPIDTEXTYOFF+messageInBoxYOffset+textboxYOffset)
#define shouldShowADVNames() (gameTextDisplayMode==TEXTMODE_ADV && (advNamesSupported==2 || (advNamesSupported && prefersADVNames)))
#define ADVNAMEOFFSET (currentTextHeight*1.5) // Space between ADV name and rest of the text

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
// 6 adds vndsClearAtBottom
// 7 adds showVNDSWarnings
// 8 adds higurashiUsesDynamicScale
// 9 adds preferredTextDisplayMode
// 10 adds autoModeVoicedWait
// 11 adds vndsSpritesFade
// 12 adds touchProceed
// 13 adds dropshadowOn
// 14 adds fontSize
// 15 adds prefersADVNames
#define OPTIONSFILEFORMAT 15

// 1 is end
// 2 adds currentADVName
#define VNDSSAVEFORMAT 2
#define validVNDSSaveFormat(a) (a==1 || a==2)

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

#define drawText(_x,_y,_text) gbDrawText(normalFont,_x,_y,_text,DEFAULTFONTCOLOR);

////////////////////////////////////////
// GBPLAT specific variables
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
//#define BUST_STATUS_FADEOUT_MOVE 3 // var 1 is alpha per frame. var 2 is x per frame. var 3 is y per frame
#define BUST_STATUS_SPRITE_MOVE 4 // var 1 is x per frame, var 2 is y per frame
#define BUST_STATUS_TRANSFORM_FADEIN 5 // The bust is fading into an already used slot. image is what the new texture is going to be thats fading in, transformTexture is the old texture that is fading out. var 1 is alpha per frame. added to image, subtracted from transformTexture.

#if GBPLAT == GB_VITA
	#include <psp2/touch.h>
	SceTouchData touch_old[SCE_TOUCH_PORT_MAX_NUM];
	SceTouchData touch[SCE_TOUCH_PORT_MAX_NUM];
	NathanAudio* _mlgsnd_loadAudioFILE(legArchiveFile _passedFile, char _passedFileFormat, char _passedShouldLoop, char _passedShouldStream);
#endif
signed char touchProceed=1;

void invertImage(crossTexture _passedImage, signed char _doInvertAlpha);
typedef struct{
	crossTexture image;
	signed int xOffset;
	signed int yOffset;
	char isActive;
	char isInvisible;
	int layer;
	signed short alpha;
	unsigned char bustStatus;
	char* relativeFilename; // Filename passed by the script
	unsigned int lineCreatedOn;
	double cacheXOffsetScale;
	double cacheYOffsetScale;
	// status variables. ignore most time.
	u64 fadeStartTime; // for BUST_STATUS_FADEIN
	u64 fadeEndTime;
	int startXMove;
	int startYMove;
	int diffXMove;
	int diffYMove;
	u64 startMoveTime;
	int diffMoveTime;
	crossTexture transformTexture; // See BUST_STATUS_TRANSFORM_FADEIN. This is the texture that is transforming
}bust;
typedef struct{
	crossTexture image;
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
int currentLine=0;
int place=0;

int lastBGMVolume = 128;

crossTexture currentBackground = NULL;
crossMusic currentMusic[MAXMUSICARRAY] = {NULL};
crossPlayHandle currentMusicHandle[MAXMUSICARRAY] = {0};
char* currentMusicFilepath[MAXMUSICARRAY]={NULL};
short currentMusicUnfixedVolume[MAXMUSICARRAY] = {0};
//crossMusic currentMusic = NULL;
crossSE soundEffects[MAXSOUNDEFFECTARRAY] = {NULL};

crossSE menuSound=NULL;
signed char menuSoundLoaded=0;

// Alpha of black rectangle over screen
unsigned char currentBoxAlpha=100;
unsigned char preferredBoxAlpha=100;
signed char MessageBoxEnabled=1;
signed char isSkipping=0;
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
// This may not be set because the user can choose to use the legacy preset folder mode.
char* currentGameFolderName=NULL;

#define GAMESTATUS_TITLE 0
#define GAMESTATUS_LOADPRESET 1
#define GAMESTATUS_PRESETSELECTION 2
#define GAMESTATUS_MAINGAME 3
#define GAMESTATUS_NAVIGATIONMENU 4
#define GAMESTATUS_GAMEFOLDERSELECTION 6
#define GAMESTATUS_LOADGAMEFOLDER 7
#define GAMESTATUS_QUIT 99
signed char currentGameStatus=GAMESTATUS_TITLE;

signed char tipNamesLoaded=0;
signed char chapterNamesLoaded=0;
unsigned char lastSelectionAnswer=0;

// The x position on screen of this image character
signed short imageCharX[MAXIMAGECHAR] = {0};
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
crossTexture imageCharImages[3]; // PLEASE DON'T FORGET TO CHANGE THIS IF ANOTHER IMAGE CHAR IS ADDED
int numImageCharSpaceEquivalent;
int imageCharSlotCenter;

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

unsigned char messageHistory[MAXMESSAGEHISTORY][SINGLELINEARRAYSIZE];
unsigned char oldestMessage=0;

char presetsAreInStreamingAssets=1;

float bgmVolume = 0.75;
float seVolume = 1.0;
float voiceVolume = 1.0;

crossFont normalFont=NULL;
double fontSize=-10; // default value < 0
int currentTextHeight;
int singleSpaceWidth;
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
crossTexture currentCustomTextbox=NULL;
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
char* currentFontFilename=NULL;

char currentlyVNDSGame=0;
char nextVndsBustshotSlot=0;
// If all the text should be cleared when the text reached the bottom of the screen when playing a VNDS game
signed char vndsClearAtBottom=0;
signed char showVNDSWarnings=1;
signed char dynamicAdvBoxHeight=0;
char* currentADVName=NULL;
unsigned char advNameR=255;
unsigned char advNameG=255;
unsigned char advNameB=255;
signed char prefersADVNames=1;
// 1 if supported
// 2 if they're forced
char advNamesSupported=0;
char advNamesPersist=0;
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
#if SUBPLATFORM == SUB_UNIX
	char* itoa(int value, char* _buffer, int _uselessBase){
		sprintf(_buffer,"%d",value);
		return _buffer;
	}
#endif
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
double partMoveFills(u64 _curTicks, u64 _startTime, int _totalDifference, double _max){
	return ((_totalDifference-(_startTime+_totalDifference-_curTicks))*_max)/(double)_totalDifference;
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
	int fixX(int _passed){
		return _passed;
	}
	int fixY(int _passed){
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
void drawTextureScaleAlphaGood(const crossTexture texture, float x, float y, double x_scale, double y_scale, unsigned char alpha){
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
crossTexture safeLoadImage(const char* path){
	crossTexture _tempTex = loadImage((char*)path);
	if (_tempTex==NULL){
		showErrorIfNull(_tempTex);
		easyMessagef(1,"Failed to load image %s, what will happen now?!",path);
	}
	return _tempTex;
}
crossTexture LoadEmbeddedPNG(const char* path){
	char* _fixedPath = fixPathAlloc(path,TYPE_EMBEDDED);
	crossTexture _tempTex = loadImage(_fixedPath);
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
		vita2d_font_draw_text_dropshadow(_realFont->data,_x,_y+textHeight(normalFont),RGBA8(_r,_g,_b,_a),_realFont->size,_message,DROPSHADOWOFFX,DROPSHADOWOFFY,RGBA8(_dropshadowR,_dropshadowG,_dropshadowB,_a));
	#else
		gbDrawTextAlpha(normalFont,_x+DROPSHADOWOFFX,_y+DROPSHADOWOFFY,_message,_dropshadowR,_dropshadowG,_dropshadowB,_a);
		gbDrawTextAlpha(normalFont,_x,_y,_message,_r,_g,_b,_a);
	#endif
}
void drawDropshadowText(int _x, int _y, char* _message, int _a){
	drawDropshadowTextSpecific(_x,_y,_message,DEFAULTFONTCOLOR,0,0,0,_a);
}
// Draw text intended to be used for the game, respects dropshadow setting
void drawTextGame(int _x, int _y, char* _message, unsigned char _alpha){
	if (dropshadowOn){
		drawDropshadowTextSpecific(_x,_y,_message,DEFAULTFONTCOLOR,0,0,0,_alpha);
	}else{
		gbDrawTextAlpha(normalFont,_x,_y,_message,DEFAULTFONTCOLOR,_alpha);
	}
}
void drawImageChars(unsigned char _alpha, int _maxDrawLine, int _maxDrawLineChar){
	int i;
	for (i=0;i<MAXIMAGECHAR;++i){
		if (imageCharType[i]!=-1){
			if ((imageCharLines[i]<_maxDrawLine) || (imageCharLines[i]==_maxDrawLine && imageCharCharPositions[i]<=_maxDrawLineChar)){
				drawTextureSizedAlpha(imageCharImages[imageCharType[i]],imageCharX[i],imageCharY[i],currentTextHeight,currentTextHeight,_alpha);
			}
		}
	}
}
void changeMaxLines(int _newMax){
	if (_newMax==maxLines || _newMax<=0){
		return;
	}
	char** _newCurrentMessages = malloc(sizeof(char*)*_newMax);
	if (_newMax<maxLines){
		int _diff = maxLines-_newMax;
		// Copy the latest lines that can fit
		memcpy(_newCurrentMessages,&(currentMessages[_diff]),_newMax*sizeof(char*));
		// free the lines that are gone
		int i;
		for (i=0;i<_diff;++i){
			free(currentMessages[i]);
		}
	}else{
		if (maxLines!=0 && currentMessages){
			// Copy all the old lines
			memcpy(_newCurrentMessages,currentMessages,sizeof(char*)*maxLines);
		}
		// alloc new ones
		int i;
		for (i=maxLines;i<_newMax;++i){
			_newCurrentMessages[i]=malloc(SINGLELINEARRAYSIZE);
			_newCurrentMessages[i][0]='\0';
		}
	}
	free(currentMessages);
	maxLines=_newMax;
	currentMessages=_newCurrentMessages;
}
void recalculateMaxLines(){
	changeMaxLines((outputLineScreenHeight-totalTextYOff())/currentTextHeight);
}
// Number of lines to draw is not zero based
// _finalLineMaxChar is the last char on the last line to draw. Must be a position inside the string, 
void DrawMessageText(unsigned char _alpha, int _maxDrawLine, int _finalLineMaxChar){
	if (_maxDrawLine==-1){
		_maxDrawLine=maxLines;
	}
	char _oldFinalChar;
	if (_finalLineMaxChar!=-1){
		if (_finalLineMaxChar<strlen(currentMessages[_maxDrawLine-1])){ // Bounds check
			// Temporarily trim the string
			_oldFinalChar = currentMessages[_maxDrawLine-1][_finalLineMaxChar+1];
			currentMessages[_maxDrawLine-1][_finalLineMaxChar+1]='\0';
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
	if (shouldShowADVNames() && currentADVName!=NULL){
		drawDropshadowTextSpecific(textboxXOffset+messageInBoxXOffset,totalTextYOff()-ADVNAMEOFFSET,currentADVName,advNameR,advNameG,advNameB,0,0,0,255);
	}
	for (i=0;i<_maxDrawLine;i++){
		drawTextGame(textboxXOffset+messageInBoxXOffset,totalTextYOff()+i*currentTextHeight,(char*)currentMessages[i],_alpha);
	}
	drawImageChars(_alpha,_maxDrawLine-1,_finalLineMaxChar!=-1 ? _finalLineMaxChar : INT_MAX);
	// Fix string if we trimmed it for _finalLineMaxChar
	if (_finalLineMaxChar!=-1){
		currentMessages[_maxDrawLine-1][_finalLineMaxChar+1]=_oldFinalChar;
	}
}
void DrawMessageBox(char _textmodeToDraw, unsigned char _targetAlpha){
	#if GBPLAT == GB_3DS
		if (textIsBottomScreen==1){
			return;
		}
	#endif
	if (_textmodeToDraw == TEXTMODE_NVL || currentCustomTextbox==NULL){
		drawRectangle(textboxXOffset,0,outputLineScreenWidth,outputLineScreenHeight,0,0,0,_targetAlpha);
	}else{
		drawTextureSizedAlpha(currentCustomTextbox,textboxXOffset,textboxYOffset,outputLineScreenWidth,advboxHeight,_targetAlpha);
	}
}
void DrawCurrentFilter(){
	if (currentFilterType==FILTERTYPE_EFFECTCOLORMIX){
		drawRectangle(textboxXOffset,0,outputLineScreenWidth,outputLineScreenHeight,filterR,filterG,filterB,filterA);
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
void reloadFont(double _passedSize){
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
	currentTextHeight = textHeight(normalFont);
	singleSpaceWidth = textWidth(normalFont," ");
	numImageCharSpaceEquivalent=currentTextHeight/singleSpaceWidth;

	char _tempBuffer[numImageCharSpaceEquivalent+1];
	memset(_tempBuffer,' ',numImageCharSpaceEquivalent);
	_tempBuffer[numImageCharSpaceEquivalent]='\0';
	imageCharSlotCenter=(textWidth(normalFont,_tempBuffer)-currentTextHeight)/2;

	recalculateMaxLines();
}
void globalLoadFont(const char* _filename){
	changeMallocString(&currentFontFilename,_filename);
	reloadFont(fontSize);
}
char menuControlsLow(int* _choice, char _canWrapUpDown, int _upDownChange, char _canWrapLeftRight, int _leftRightChange, int _menuMin, int _menuMax){
	int _oldValue = *_choice;
	if (_leftRightChange!=0){
		if (wasJustPressed(BUTTON_LEFT)){
			*_choice-=_leftRightChange;
		}else if (wasJustPressed(BUTTON_RIGHT)){
			*_choice+=_leftRightChange;
		}
		*_choice = _canWrapLeftRight ? wrapNum(*_choice,_menuMin,_menuMax) : limitNum(*_choice,_menuMin,_menuMax);
	}
	if (wasJustPressed(BUTTON_UP)){
		*_choice-=_upDownChange;
	}
	if (wasJustPressed(BUTTON_DOWN)){
		*_choice+=_upDownChange;
	}
	*_choice = _canWrapUpDown ? wrapNum(*_choice,_menuMin,_menuMax) : limitNum(*_choice,_menuMin,_menuMax);
	return _oldValue!=*_choice;
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
char SafeLuaDoFile(lua_State* passedState, char* passedPath, char showMessage){
	if (checkFileExist(passedPath)==0){
		if (showMessage==1){
			easyMessagef(1,"The LUA file %s does not exist!",passedPath);
		}
		return 0;
	}
	return lazyLuaError(luaL_dofile(passedState,passedPath));
}
void WriteToDebugFile(const char* stuff){
	#if GBPLAT == GB_LINUX
		printf("%s\n",stuff);
		return;
	#endif
	char* _tempDebugFileLocationBuffer = malloc(strlen(gbDataFolder)+strlen("log.txt")+1);
	strcpy(_tempDebugFileLocationBuffer,gbDataFolder);
	strcat(_tempDebugFileLocationBuffer,"log.txt");
	FILE *fp;
	fp = fopen(_tempDebugFileLocationBuffer, "a");
	if (!fp){
		easyMessagef(1,"Failed to open debug file, %s",_tempDebugFileLocationBuffer);
	}else{
		fprintf(fp,"%s\n",stuff);
		fclose(fp);
	}
	free(_tempDebugFileLocationBuffer);
}
// Returns one if they chose yes
// Returns zero if they chose no
int LazyChoice(const char* stra, const char* strb, const char* strc, const char* strd){
	int _choice=0;
	controlsStart();
	controlsEnd();
	while (currentGameStatus!=GAMESTATUS_QUIT){
		controlsStart();
		if (wasJustPressed(BUTTON_A)){
			PlayMenuSound();
			controlsReset();
			return _choice;
		}
		_choice = menuControls(_choice,0,1);
		controlsEnd();
		startDrawing();
		if (stra!=NULL){
			drawText(MENUOPTIONOFFSET,5+currentTextHeight*(0+2),stra);
		}
		if (strb!=NULL){
			drawText(MENUOPTIONOFFSET,5+currentTextHeight*(1+2),strb);
		}
		if (strc!=NULL){
			drawText(MENUOPTIONOFFSET,5+currentTextHeight*(2+2),strc);
		}
		if (strd!=NULL){
			drawText(MENUOPTIONOFFSET,5+currentTextHeight*(3+2),strd);
		}
		drawText(0,screenHeight-32-currentTextHeight*(_choice+1),MENUCURSOR);
		drawText(MENUOPTIONOFFSET,screenHeight-32-currentTextHeight*2,"Yes");
		drawText(MENUOPTIONOFFSET,screenHeight-32-currentTextHeight,"No");
		endDrawing();
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
void ClearMessageArray(char _doFadeTransition){
	if (textSpeed==TEXTSPEED_INSTANT){
		_doFadeTransition=0;
	}
	currentLine=0;
	int i;
	int _totalAddedToHistory=0;
	for (i=0;i<maxLines;i++){
		if (currentMessages[i][0]!='\0'){
			addToMessageHistory(currentMessages[i]);
			_totalAddedToHistory++;
		}
	}
	if (_totalAddedToHistory!=0 && MessageBoxEnabled && !isSkipping && _doFadeTransition){ // If we actually added stuff
		u64 _startTime=getMilli();
		u64 _currentTime;
		while ((_currentTime=getMilli())<_startTime+CLEARMESSAGEFADEOUTTIME){
			startDrawing();
			drawAdvanced(1,1,1,MessageBoxEnabled,1,0);
			DrawMessageText(255-partMoveFillsCapped(_currentTime,_startTime,CLEARMESSAGEFADEOUTTIME,255),-1,-1);
			endDrawing();
		}
	}
	for (i=0;i<maxLines;++i){
		currentMessages[i][0]='\0';
	}
	for (i=0;i<MAXIMAGECHAR;i++){
		imageCharType[i]=-1;
	}
	if (advNamesPersist!=2){
		changeMallocString(&currentADVName,NULL);
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
	return (const size_t)strlen((const char*)currentMessages[_linenum]);
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
// If the passed line index is too far down (>= maxLines), shift everything up to make room for a new line
void LastLineLazyFix(int* _line){
	if (*_line>=maxLines){
		if (*_line>maxLines){
			*_line=maxLines;
		}
		int i;
		addToMessageHistory(currentMessages[0]);
		for (i=1;i<maxLines;i++){
			strcpy(currentMessages[i-1],currentMessages[i]);
		}
		currentMessages[maxLines-1][0]=0;
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
	u64 _curTime=getMilli();
	int i;
	for (i = 0; i < maxBusts; i++){
		switch(Busts[i].bustStatus){
			case BUST_STATUS_FADEIN:
			case BUST_STATUS_TRANSFORM_FADEIN:
				if (_curTime>Busts[i].fadeStartTime){
					if (_curTime>=Busts[i].fadeEndTime){
						if (Busts[i].bustStatus == BUST_STATUS_TRANSFORM_FADEIN){
							freeTexture(Busts[i].transformTexture);
							Busts[i].transformTexture=NULL;
						}
						Busts[i].alpha=255;
						Busts[i].bustStatus = BUST_STATUS_NORMAL;
					}else{
						Busts[i].alpha=partMoveFills(_curTime,Busts[i].fadeStartTime,Busts[i].fadeEndTime-Busts[i].fadeStartTime,255);
					}
				}
				break;
			case BUST_STATUS_FADEOUT:
				if (_curTime>Busts[i].fadeEndTime){
					ResetBustStruct(&(Busts[i]),1);
					RecalculateBustOrder();
				}else{
					Busts[i].alpha=partMoveEmptys(_curTime, Busts[i].fadeStartTime, Busts[i].fadeEndTime-Busts[i].fadeStartTime, 255);
				}
				break;
			case BUST_STATUS_SPRITE_MOVE:
				if (_curTime>Busts[i].startMoveTime+Busts[i].diffMoveTime){
					Busts[i].bustStatus = BUST_STATUS_NORMAL;
					Busts[i].xOffset=Busts[i].startXMove+Busts[i].diffXMove;
					Busts[i].yOffset=Busts[i].startYMove+Busts[i].diffYMove;
				}else{
					Busts[i].xOffset=Busts[i].startXMove+partMoveFills(_curTime,Busts[i].startMoveTime,Busts[i].diffMoveTime,Busts[i].diffXMove);
					Busts[i].yOffset=Busts[i].startYMove+partMoveFills(_curTime,Busts[i].startMoveTime,Busts[i].diffMoveTime,Busts[i].diffYMove);
				}
				break;
		}
	}
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
void updateControlsGeneral(){
	if (wasJustPressed(BUTTON_Y)){
		isSkipping=1;
		endType=Line_ContinueAfterTyping;
	}
	if (isSkipping==1 && !isDown(BUTTON_Y)){
		isSkipping=0;
	}
	if (wasJustPressed(BUTTON_X)){
		SettingsMenu(1,currentlyVNDSGame,currentlyVNDSGame,!isActuallyUsingUma0 && GBPLAT != GB_VITA,!currentlyVNDSGame,0,currentlyVNDSGame,currentlyVNDSGame,(strcmp(VERSIONSTRING,"forgotversionnumber")==0));
	}
	if (wasJustPressed(BUTTON_SELECT)){
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
	#if GBSND == GBSND_VITA
		if (lastVoiceSlot!=-1 && soundEffects[lastVoiceSlot]!=NULL && mlgsndIsPlaying(soundEffects[lastVoiceSlot])){
			_inBetweenLinesMilisecondsStart=0;
			_chosenAutoWait = autoModeVoicedWait;
		}else{
			_inBetweenLinesMilisecondsStart = getMilli();
			if (_inBetweenLinesMilisecondsStart==0){
				_inBetweenLinesMilisecondsStart=1;
			}
			_chosenAutoWait = autoModeWait;
		}
	#else
		_inBetweenLinesMilisecondsStart = getMilli();
		_chosenAutoWait = autoModeWait;
		if (_inBetweenLinesMilisecondsStart==0){
			_inBetweenLinesMilisecondsStart=1;
		}
	#endif
	// On PS Vita, prevent sleep mode if using auto mode
	#if GBPLAT == GB_VITA
		if (autoModeOn){
			sceKernelPowerTick(0);
		}
	#endif
	char _didPressCircle=0;
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
				if (!vndsNormalSave(_foundPath,1,1)){
					PlayMenuSound();
					drawRectangle(0,0,screenWidth,screenHeight,0,255,0,255);
				}
				free(_foundPath);
			}
		}
		endDrawing();

		int touch_bool = 0;
		#if GBPLAT == GB_VITA
			memcpy(touch_old, touch, sizeof(touch_old));
			if (touchProceed){
				int port;
				for (port = 0; port < SCE_TOUCH_PORT_MAX_NUM; port++){
					sceTouchPeek(port, &touch[port], 1);
				}
				touch_bool = touchProceed && ((touch[SCE_TOUCH_PORT_FRONT].reportNum == 1) && (touch_old[SCE_TOUCH_PORT_FRONT].reportNum == 0));
			}
		#endif

		if (wasJustPressed(BUTTON_A) || touch_bool){
			if (_didPressCircle==1){
				showTextbox();
			}
			endType = LINE_RESERVED;
		}
		if (wasJustPressed(BUTTON_B)){
			if (_didPressCircle==1){
				if (_toggledTextboxTime!=0){
					_inBetweenLinesMilisecondsStart+=getMilli()-_toggledTextboxTime;
					_toggledTextboxTime=0;
				}
				MessageBoxEnabled = !MessageBoxEnabled;
			}else if (MessageBoxEnabled==1){
				MessageBoxEnabled=0;
				_didPressCircle=1;
				_toggledTextboxTime=getMilli();
			}
		}
		updateControlsGeneral();
		if (wasJustPressed(BUTTON_START)){
			if (currentlyVNDSGame){
				safeVNDSSaveMenu();
			}else{
				DrawHistory(messageHistory);
			}
		}
		if (wasJustPressed(BUTTON_UP)){
			DrawHistory(messageHistory);
		}
		controlsEnd();
		if (autoModeOn==1 && _toggledTextboxTime==0){
			if (_inBetweenLinesMilisecondsStart!=0){ // If we're not waiting for audio to end
				if (getMilli()>=(_inBetweenLinesMilisecondsStart+_chosenAutoWait)){
					endType = LINE_RESERVED;
				}
			}else{
				#if GBPLAT == GB_VITA
					// Check if audio has ended yet.
					if (mlgsndIsPlaying(soundEffects[lastVoiceSlot])==0){
						_inBetweenLinesMilisecondsStart = getMilli();
					}
				#endif
			}
		}
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
void GetXAndYOffset(crossTexture _tempImg, signed int* _tempXOffset, signed int* _tempYOffset){
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
double GetXOffsetScale(crossTexture _tempImg){
	if (dynamicScaleEnabled){
		return applyGraphicsScale(actualBackgroundWidth)/(double)scriptScreenWidth;
	}else{
		if (getTextureWidth(_tempImg)>screenWidth){
			return (screenWidth/scriptScreenWidth);
		}
		return (getTextureWidth(_tempImg)/(float)scriptScreenWidth);
	}
}
double GetYOffsetScale(crossTexture _tempImg){
	if (dynamicScaleEnabled){
		return applyGraphicsScale(actualBackgroundHeight)/(double)scriptScreenHeight;
	}else{
		if (getTextureHeight(_tempImg)>screenHeight){
			return (screenHeight/scriptScreenHeight);
		}
		return ( getTextureHeight(_tempImg)/(float)scriptScreenHeight);
	}
}
void drawHallowRect(int _x, int _y, int _w, int _h, int _thick, int _r, int _g, int _b, int _a){
	drawRectangle(_x,_y,_thick,_h,_r,_g,_b,_a);
	drawRectangle(_x+_w-_thick,_y,_thick,_h,_r,_g,_b,_a);
	drawRectangle(_x,_y,_w,_thick,_r,_g,_b,_a);
	drawRectangle(_x,_y+_h-_thick,_w,_thick,_r,_g,_b,_a);
}
void DrawBackgroundAlpha(crossTexture passedBackground, unsigned char passedAlpha){
	if (passedBackground!=NULL){
		signed int _tempXOffset;
		signed int _tempYOffset;
		GetXAndYOffset(passedBackground,&_tempXOffset,&_tempYOffset);
		drawTextureSizedAlpha(passedBackground,_tempXOffset,_tempYOffset, getTextureWidth(passedBackground)*graphicsScale, getTextureHeight(passedBackground)*graphicsScale, passedAlpha);
	}
}
void DrawBackground(crossTexture passedBackground){
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
// seek past a windows newline or a unix newline
void moveFilePointerPastNewline(crossFile fp){
	unsigned char _temp;
	crossfread(&_temp,1,1,fp);
	if (_temp==13){
		crossfseek(fp,1,SEEK_CUR);
	}
}
// getline but without a newline at the end
char* easygetline(crossFile fp){
	char* _tempReadLine=NULL;
	size_t _readLength=0;
	custom_getline(&_tempReadLine,&_readLength,fp);
	removeNewline(_tempReadLine);
	return _tempReadLine;
}
int readIntLine(crossFile fp){
	char* _tempReadLine=easygetline(fp);
	int _ret=atoi(_tempReadLine);
	free(_tempReadLine);
	return _ret;
}
unsigned char* ReadNumberStringList(crossFile fp, unsigned char* _outArraySize){
	*_outArraySize = readIntLine(fp);
	unsigned char* _retList = malloc(sizeof(unsigned char)*(*_outArraySize));
	int i;
	for (i=0;i<*_outArraySize;++i){
		_retList[i] = readIntLine(fp);
	}
	return _retList;
}
char** ReadFileStringList(crossFile fp, unsigned char* _outArraySize){
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
	crossFile fp;
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
void freeWrappedText(int _numLines, char** _passedLines){
	int i;
	for (i=0;i<_numLines;++i){
		free(_passedLines[i]);
	}
	free(_passedLines);
}
void wrapText(const char* _passedMessage, int* _numLines, char*** _realLines, int _maxWidth){
	*_numLines=-1;
	char* _workable = strdup(_passedMessage);

	int _cachedStrlen = strlen(_workable);
	if (_cachedStrlen==0){
		*_numLines=0;
		*_realLines=NULL;
		free(_workable);
		return;
	}
	int _lastNewline = -1; // Index
	int i;
	for (i=0;i<_cachedStrlen;++i){
		if (_workable[i]=='\n'){
			_workable[i]='\0';
			_lastNewline=i;
		}else if (_workable[i]==' ' || i==_cachedStrlen-1){
			if (_workable[i]==' '){ // Because alt condition
				_workable[i]='\0'; // Chop the string for textWidth function
			}
			if (textWidth(normalFont,&(_workable[_lastNewline+1]))>_maxWidth){ // If at this spot the string is too long for the screen
				// Find last word before we went off screen
				int j;
				for (j=i-1;j>_lastNewline;--j){
					if (_workable[j]==' '){
						break;
					}
				}
				if (j==_lastNewline){ // Didn't find a stopping point, the line is likely one giant word
					// Force a stopping point
					for (j=i-1;j>_lastNewline;--j){
						char _cacheChar = _workable[j];
						_workable[j]='\0';
						
						char _canSplit = (textWidth(normalFont,&(_workable[_lastNewline+1]))<=_maxWidth);
						_workable[j]=_cacheChar;
						if (_canSplit){
							// The character we're at right now, that's where the split needs to be because it's acting as the null terminator right now
							// In this code, j will represent the last real character from the string
							j-=2; // Minus one to make room for the dash, minus another because we're making the break at original j value minus one
							char* _newBuffer = malloc(_cachedStrlen+2+1);
							memcpy(_newBuffer,_workable,j+1);
							_newBuffer[j+1]='-';
							_newBuffer[j+2]='\0';
							_lastNewline=j+2;
							memcpy(&(_newBuffer[j+3]),&(_workable[j+1]),_cachedStrlen-(j)); // Should also copy null
							free(_workable);
							_workable = _newBuffer;

							// Account for new 2 bytes
							_cachedStrlen+=2;
							i+=2;
							if (_workable[i]=='\0'){ // Fix chop if happened
								_workable[i]=' ';
							}
							break;
						}
					}
					// Odd, no part between last new line and here less than _maxWidth. This should not happen, but just in case, put some code to at least do something.
					if (j==_lastNewline){
						_workable[i]='\0';
					}
				}else{ // Normal, found what we're looking for, just chop at the end of the last word
					_workable[j]='\0';
					if (_workable[i]=='\0'){ // Fix chop if happened
						_workable[i]=' ';
					}
					_lastNewline=j;
				}
				// No matter what we did, we'll still need to start our check from the last new line
				i=_lastNewline;
			}else{
				if (_workable[i]=='\0'){ // Fix chop if happened
					_workable[i]=' ';
				}
			}
		}
	}

	// fheuwfhuew (\0) ffhuehfu (\0) fheuhfueiwf (\0)
	*_numLines=1;
	for (i=0;i<_cachedStrlen;++i){
		if (_workable[i]=='\0'){
			*_numLines=(*_numLines)+1;
		}
	}
	*_realLines = malloc(sizeof(char*)*(*_numLines));
	int _nextCopyIndex=0;
	for (i=0;i<*_numLines;++i){
		(*_realLines)[i] = strdup(&(_workable[_nextCopyIndex]));
		_nextCopyIndex += 1+strlen(&(_workable[_nextCopyIndex]));
	}

	free(_workable);
}
void easyMessage(const char** _passedMessage, int _numLines, char _doWait){
	controlsStart();
	controlsEnd();
	do{
		controlsStart();
		if (wasJustPressed(BUTTON_A)){
			controlsStart();
			controlsEnd();
			break;
		}
		controlsEnd();
		startDrawing();

		int i;
		for (i=0;i<_numLines;++i){
			drawText(32,5+currentTextHeight*(i+2),_passedMessage[i]);
		}

		if (_doWait){
			drawText(32,screenHeight-32-currentTextHeight,SELECTBUTTONNAME" to continue.");
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
char* getSavefileName(const char* _passedPreset){
	return easyCombineStrings(2,saveFolder,_passedPreset);
}
void LoadGame(){
	char* _savefileLocation = getSavefileName(currentPresetFilename);
	currentPresetChapter=-1;
	if (checkFileExist((char*)_savefileLocation)==1){
		FILE *fp;
		fp = fopen((const char*)_savefileLocation, "rb");
		fread(&currentPresetChapter,2,1,fp);
		fclose(fp);
	}
	free(_savefileLocation);
}
void SaveGame(){
	char* _savefileLocation = getSavefileName(currentPresetFilename);
	FILE *fp;
	fp = fopen((const char*)_savefileLocation, "wb");
	fwrite(&currentPresetChapter,2,1,fp);
	fclose(fp);
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
		outputLineScreenWidth = screenWidth - textboxXOffset*2;
	}
	#if GBPLAT == GB_3DS
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
void applyTextboxChanges(){
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
	if (shouldShowADVNames()){
		messageInBoxYOffset = currentTextHeight*1.5;
	}else{
		messageInBoxYOffset=0;
	}
	// Apply textOnlyOverBackground setting
	if (textOnlyOverBackground==0){
		textboxXOffset=0;
		outputLineScreenWidth = screenWidth;
	}else{
		updateTextPositions();
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
	_returnFoundString = easyCombineStrings(4,streamingAssets, getUserPreferredImageDirectory(_folderPreference),filename,_fileFormat);
	
	if (checkFileExist(_returnFoundString)){
		return _returnFoundString;
	}

	// If not exist, try the other folder.
	free(_returnFoundString);
	_returnFoundString = easyCombineStrings(4,streamingAssets, getUserPreferredImageDirectoryFallback(_folderPreference),filename,_fileFormat);
	
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
crossTexture safeLoadGameImage(const char* filename, char _folderPreference, char _extensionIncluded){
	char* _tempFoundFilename;
	_tempFoundFilename = LocationStringFallback(filename,_folderPreference,_extensionIncluded,scriptForceResourceUppercase);
	if (_tempFoundFilename==NULL){
		if (shouldShowWarnings()){
			easyMessagef(1,"Image not found, %s",filename);
		}
		return NULL;
	}
	crossTexture _returnLoadedPNG = loadImage(_tempFoundFilename);
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
			applyTextboxChanges();
			// Draw the changes
			startDrawing();
			drawAdvanced(1,1,1,1,1,0); // Don't draw message text during transitions
			if (_maxDrawLine!=0){
				DrawMessageText(255,_maxDrawLine,-1);
			}
			endDrawing();
		}
	}
	advboxHeight=_newHeight;
	applyTextboxChanges();
}
// _overrideNewHeight is in lines
void updateDynamicADVBox(int _maxDrawLine, int _overrideNewHeight){
	if (_maxDrawLine==-1){
		_maxDrawLine=maxLines;
	}
	int _newAdvBoxHeight;
	if (_overrideNewHeight==-1){
		_newAdvBoxHeight=1; // One extra line to be safe
		short i;
		for (i=0;i<maxLines;++i){
			if (currentMessages[i][0]!='\0'){
				_newAdvBoxHeight=i+2; // Last non-empty line. Adding 1 is for the free line, adding another 1 is because line index is 0 based.
			}
		}		
		_newAdvBoxHeight*=currentTextHeight;
	}else{
		_newAdvBoxHeight = _overrideNewHeight*currentTextHeight;
	}
	if (shouldShowADVNames()){
		_newAdvBoxHeight+=ADVNAMEOFFSET;
	}
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
	recalculateMaxLines();
}
void disableVNDSADVMode(){
	gameTextDisplayMode=TEXTMODE_NVL;
	dynamicAdvBoxHeight=0;
	applyTextboxChanges();
	recalculateMaxLines();
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
		Busts[passedSlot].alpha=255;
		Busts[passedSlot].fadeEndTime=getMilli()+_time;
		if (_wait==1){
			while (Busts[passedSlot].isActive==1){
				controlsStart();
				Update();
				startDrawing();
				Draw(MessageBoxEnabled);
				endDrawing();
				if (wasJustPressed(BUTTON_A)){
					Busts[passedSlot].alpha = 1;
				}
				controlsEnd();
			}
		}
	}else{
		// The free will happen on the next update
		Busts[passedSlot].alpha=0;
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
				if (Busts[i].isActive==1){
					if (Busts[i].alpha>0){
						_isDone=0;
						break;
					}
				}
			}
			controlsStart();
			Update();
			startDrawing();
			Draw(MessageBoxEnabled);
			endDrawing();
			if (wasJustPressed(BUTTON_A)){
				for (i=0;i<maxBusts;i++){
					if (Busts[i].isActive==1){
						Busts[i].alpha=1;
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
		crossTexture newBackground = safeLoadGameImage(_filename,graphicsLocation,scriptUsesFileExtensions);
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
				for (i=maxBusts-1;i!=-1;i--){
					if (bustOrder[i]!=255 && Busts[bustOrder[i]].isActive==1 && Busts[bustOrder[i]].lineCreatedOn == currentScriptLine-1){
						Busts[bustOrder[i]].alpha = _backgroundAlpha;
					}
				}
				startDrawing();
				drawAdvanced(1,1,0,0,1,0); // Draws the old background
				DrawBackgroundAlpha(newBackground,_backgroundAlpha); // Draws the new background on top
				// Draw busts created on the last line at the same alpha as the new background
				for (i = maxBusts-1; i != -1; i--){
					if (bustOrder[i]!=255 && Busts[bustOrder[i]].isActive==1 && Busts[bustOrder[i]].lineCreatedOn == currentScriptLine-1){
						DrawBust(&(Busts[bustOrder[i]]));
					}
				}
				drawAdvanced(0,0,1,MessageBoxEnabled,0,MessageBoxEnabled);
				endDrawing();
	
				controlsStart();
				if (wasJustPressed(BUTTON_A)){
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
	// Fix alpha for busts created on the last line
	int i;
	for (i=maxBusts-1;i!=-1;i--){
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
			crossTexture _cachedOldTexture = Busts[passedSlot].image;
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
		Busts[passedSlot].relativeFilename=strdup(_filename);
		if (Busts[passedSlot].image==NULL){
			free(Busts[passedSlot].relativeFilename);
			Busts[passedSlot].relativeFilename=NULL;
			ResetBustStruct(&(Busts[passedSlot]),0);
			return 0;
		}
	}
	if (currentFilterType==FILTERTYPE_NEGATIVE){
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
	}
	if (_waitforfadein==1){
		while (Busts[passedSlot].alpha<255){
			controlsStart();
			Update();
			if (Busts[passedSlot].alpha>255){
				Busts[passedSlot].alpha=255;
			}
			startDrawing();
			Draw(MessageBoxEnabled);
			endDrawing();
			if (wasJustPressed(BUTTON_A) || skippedInitialWait==1){
				Busts[passedSlot].alpha = 255;
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
	#if GBSND != GBSND_VITA
		removeFileExtension(tempstringconcat);
		strcat(tempstringconcat,".wav");
		if (checkFileExist(tempstringconcat)==1){
			return tempstringconcat;
		}
	#endif
	//
	#if GBSND == GBSND_VITA
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
			if (textWidth(normalFont,&(message[lastNewlinePosition+1]))>=outputLineScreenWidth-MESSAGEEDGEOFFSET-messageInBoxXOffset || i-lastNewlinePosition>=SINGLELINEARRAYSIZE-1){
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
					// TODO - Won't work if only a less than sign and not a tag
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
						message[i]='\0'; // So we can use textWidth
						for (j=0;j<MAXIMAGECHAR;j++){
							if (imageCharType[j]==-1){
								imageCharX[j] = textWidth(normalFont,&(message[lastNewlinePosition+1]))+textboxXOffset+messageInBoxXOffset+imageCharSlotCenter;
								imageCharY[j] = totalTextYOff()+currentLine*currentTextHeight;
								imageCharLines[j] = currentLine;
								imageCharCharPositions[j] = strlenNO1(&(message[lastNewlinePosition+1]));
								imageCharType[j] = _imagechartype;
								break;
							}
						}
						memset(&(message[i]),32,numImageCharSpaceEquivalent);
						i+=(numImageCharSpaceEquivalent-1);
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
				}
			}
		}

		changeIfLazyLastLineFix(&currentLine, &_currentDrawLine);
	}
	// This code will make a new line if there needs to be one because of the last word
	if (textWidth(normalFont,&(message[lastNewlinePosition+1]))>=outputLineScreenWidth-MESSAGEEDGEOFFSET-messageInBoxXOffset){
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
				if (textWidth(normalFont,&(message[lastNewlinePosition+1]))>outputLineScreenWidth-MESSAGEEDGEOFFSET-messageInBoxXOffset){
					// What this means is that when only the string UP TO the last character was small enough. Now we have to replicate the behavior of the previous loop to get the shorter string.
					char _tempCharCache2 = message[i-1];
					message[i-1]='\0';
					strcpyNO1(currentMessages[currentLine],&(message[lastNewlinePosition+1]));
					message[i-1]=_tempCharCache2;
					currentLine++;
					changeIfLazyLastLineFix(&currentLine, &_currentDrawLine);
					lastNewlinePosition=i-2;
				}else{
					//printf("%d;%s\n",textWidth(normalFont,&(message[lastNewlinePosition+1])));
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
			// If not VNDS, there is a chance _currentDrawLine won't represent the next line, so we need to make sure we're actually drawing the _currentDrawLine and that it doesn't draw newly added text.
			char _archivedCharacter = currentMessages[_currentDrawLine][_currentDrawChar];
			currentMessages[_currentDrawLine][_currentDrawChar] = '\0';
			updateDynamicADVBox(_currentDrawLine+1,currentLine+2);
			currentMessages[_currentDrawLine][_currentDrawChar] = _archivedCharacter;
		}
	}
	#if GBPLAT == GB_3DS
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
		// The first one that takes two bytes is U+0080, or 0xC2 0x80
		// If it is a two byte character, we don't want to try and draw when there's only one byte. Skip to include the next one.
		if (currentMessages[_currentDrawLine][_currentDrawChar]>=0xC2){
			_currentDrawChar++;
		}

		controlsStart();
		if (wasJustPressed(BUTTON_A)){
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
		if (MessageBoxEnabled==1){
			DrawMessageText(255,_currentDrawLine+1,_currentDrawChar);
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
	}
	#if GBPLAT == GB_3DS
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
		crossMusic _tempHoldSlot=loadGameAudio(filename,PREFER_DIR_BGM,0);
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
// vndsClearAtBottom, 1 byte
// showVNDSWarnings, 1 byte
// higurashiUsesDynamicScale, 1 byte
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
	fwrite(&vndsClearAtBottom,sizeof(signed char),1,fp);
	fwrite(&showVNDSWarnings,sizeof(signed char),1,fp);
	fwrite(&higurashiUsesDynamicScale,sizeof(signed char),1,fp);
	fwrite(&preferredTextDisplayMode,sizeof(signed char),1,fp);
	fwrite(&autoModeVoicedWait,sizeof(int),1,fp);
	fwrite(&vndsSpritesFade,sizeof(signed char),1,fp);
	fwrite(&touchProceed,sizeof(signed char),1,fp);
	fwrite(&dropshadowOn,sizeof(signed char),1,fp);
	fwrite(&fontSize,sizeof(double),1,fp);
	fwrite(&prefersADVNames,sizeof(signed char),1,fp);

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
		fclose(fp);

		if (cpuOverclocked==1){
			#if GBPLAT == GB_VITA
				scePowerSetArmClockFrequency(444);
			#endif
			#if GBPLAT == GB_3DS
				outputLineScreenWidth = 320;
				outputLineScreenHeight = 240;
			#endif
		}
		applyTextboxChanges();
		printf("Loaded settings file.\n");
	}
	free(_fixedFilename);
}
#define HISTORYSCROLLBARHEIGHT (((double)HISTORYONONESCREEN/(double)MAXMESSAGEHISTORY)*screenHeight)
//#define HISTORYSCROLLRATE (floor((double)MAXMESSAGEHISTORY/15))
#define HISTORYSCROLLRATE 1
void DrawHistory(unsigned char _textStuffToDraw[][SINGLELINEARRAYSIZE]){
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
		drawRectangle(textboxXOffset,0,outputLineScreenWidth,screenHeight,0,0,0,150);
		int i;
		for (i = 0; i < HISTORYONONESCREEN; i++){
			gbDrawText(normalFont,textboxXOffset,textHeight(normalFont)+i*currentTextHeight,(const char*)_textStuffToDraw[FixHistoryOldSub(i+_scrollOffset,oldestMessage)],255,255,255);
		}
		if (outputLineScreenWidth == screenWidth){
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
	free(presetFolder);

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

	presetFolder = malloc(strlen(gbDataFolder)+strlen(_streamingAssetsFolderName)+strlen("/Presets/")+1);
	strcpy(presetFolder,gbDataFolder);
	strcat(presetFolder,"Presets/");
	if (!directoryExists(presetFolder)){ // If the normal preset folder doesn't exist
		presetFolder[0]='\0';
		if (_isRelativeToData){
			strcat(presetFolder,gbDataFolder);
		}
		strcat(presetFolder,_streamingAssetsFolderName); // Look 4 lines above for why this is okay
		strcat(presetFolder,"/Presets/");
		presetsAreInStreamingAssets=1;
	}else{
		presetsAreInStreamingAssets=0;
	}
}
void UpdatePresetStreamingAssetsDir(char* filename){
	char _tempNewStreamingAssetsPathbuffer[strlen(gbDataFolder)+strlen("StreamingAssets_")+strlen(filename)+1];
	strcpy(_tempNewStreamingAssetsPathbuffer,gbDataFolder);
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
			easyMessagef(1,"Error in Lua file, %s, %s",_completedSpecificLuaPath,lua_tostring(L,-1));
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
		currentCustomTextbox = loadImage(_tempFilepathBuffer);
	}else{
		#if GBPLAT != GB_3DS
			currentCustomTextbox = LoadEmbeddedPNG("assets/DefaultAdvBox.png");
		#else
			currentCustomTextbox = LoadEmbeddedPNG("assets/DefaultAdvBoxLowRes.png");
		#endif
	}
	applyTextboxChanges();
	recalculateMaxLines();
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
		easyMessagef(1,"Invalid game folder. I know this because the includedPreset.txt does not exist. Did you remember to convert this folder before moving it?");
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
	scriptForceResourceUppercase=0;
	dynamicScaleEnabled=1;
	advNamesPersist=0;
	//shouldUseBustCache=1;
	recalculateMaxLines();
}
void activateHigurashiSettings(){
	currentlyVNDSGame=0;
	scriptUsesFileExtensions=0;
	bustsStartInMiddle=1;
	scriptScreenWidth=640;
	scriptScreenHeight=480;
	scriptForceResourceUppercase=0;
	dynamicScaleEnabled=higurashiUsesDynamicScale;
	advNamesPersist=1;
	if (!advNamesSupported){
		advNamesSupported=1;
	}
	//shouldUseBustCache=0;
	applyTextboxChanges();
	recalculateMaxLines();
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
		char* _tempAsString = nathanscriptVariableAsString(&(_listToSave[i].variable));
		fwrite(&(_correctVariableType),sizeof(char),1,fp);
		writeLengthStringToFile(fp,_listToSave[i].name);
		writeLengthStringToFile(fp,_tempAsString);
		free(_tempAsString);
	}
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
	// Enough spaces for all valid paths
	char* _tempSavefilePath = malloc(strlen(streamingAssets)+strlen(saveFolder)+strlen("embeddedGameSave")+10+1);
	if (isEmbedMode){
		sprintf(_tempSavefilePath,"%s%s%d",saveFolder,"embeddedGameSave",_slot);
	}else{
		sprintf(_tempSavefilePath,"%s%s%d",streamingAssets,"sav",_slot);
	}
	return _tempSavefilePath;
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
			startDrawing();
			Draw(0);
			endDrawing();
			SDL_Surface* _saveSurface;
			#if SDL_BYTEORDER == SDL_BIG_ENDIAN
				_saveSurface=SDL_CreateRGBSurface(0,screenWidth,screenHeight,32,0xff000000,0x00ff0000,0x0000ff00,0x000000ff);
			#else
				_saveSurface=SDL_CreateRGBSurface(0,screenWidth,screenHeight,32,0x000000ff,0x0000ff00,0x00ff0000,0xff000000);
			#endif
			if (_saveSurface!=NULL){
				SDL_ClearError();
				SDL_LockSurface(_saveSurface);
				if (SDL_RenderReadPixels(mainWindowRenderer,NULL,_saveSurface->format->format,_saveSurface->pixels,_saveSurface->pitch)==0){
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
			currentMessages[i][0]='\0';
		}
	}
	// Read the lines
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

	controlsStart();
	controlsEnd();

	if (gameTextDisplayMode==TEXTMODE_ADV && dynamicAdvBoxHeight){
		updateDynamicADVBox(0,-1);
	}

	// Open
	nathanscriptCurrentOpenFile = crossfopen(_tempLoadedFilename,"rb");
	crossfseek(nathanscriptCurrentOpenFile,_readFilePosition,CROSSFILE_START);
	if (_startLoadedGame){
		// Don't instantly proceed
		endType=Line_Normal;
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
			endDrawing();
		}
		currentBoxAlpha = _oldMessageBoxAlpha;
	}
	MessageBoxEnabled=_isOn;
}
void hideTextbox(){
	_textboxTransition(0,TEXTBOXFADEOUTTIME);
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
	void invertImage(crossTexture _passedImage, signed char _doInvertAlpha){
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
void addGamePresetToLegacyFolder(char* _streamingAssetsRoot, char* _presetFilenameRelative){
	if (!directoryExists(_streamingAssetsRoot)){
		easyMessagef(1,"%s does not exist. This means you probably don't have to worry about %s",_streamingAssetsRoot,_presetFilenameRelative);
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
			case 1:
				easyMessagef(1,"Lua error.");
			break;
			default:
				easyMessagef(1,"UNKNOWN ERROR!");
			break;
		}
		return 1;
	}
	return 0;
}
void safeVNDSSaveMenu(){
	int _chosenSlot = vndsSaveSelector();
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
#define COLORMARKUPSTART "<color=#"
#define COLORMARKUPEND "</color>"
void setADVName(char* _newName){
	resetADVNameColor();
	if (_newName!=NULL){
		if (!advNamesSupported){
			advNamesSupported=1;
			applyTextboxChanges();
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
					if (i+8+strlen(COLORMARKUPEND)<_cachedStrlen){
						// Load RGB
						char _tempNumBuffer[3];
						_tempNumBuffer[2]='\0';
						char j;
						for (j=0;j<3;++j){
							_tempNumBuffer[0]=_newName[i++];
							_tempNumBuffer[1]=_newName[i++];
							int _parsedValue = strtol(_tempNumBuffer,NULL,16);
							switch(j){
								case 0:
									advNameR=_parsedValue;
									break;
								case 1:
									advNameG=_parsedValue;
									break;
								case 2:
									advNameB=_parsedValue;
									break;
							}
						}
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
		changeMallocString(&currentADVName,NULL);
	}
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
	if (_passedArguments[2].variableType==NATHAN_TYPE_STRING){ // If an English adv name was passed
		setADVName(nathanvariableToString(&_passedArguments[2]));
	}else if (shouldShowADVNames() && advNamesPersist==0){
		setADVName(NULL);
	}
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
	if (isSkipping!=1){
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
	DrawBustshot(nathanvariableToInt(&_passedArguments[0]), nathanvariableToString(&_passedArguments[1]), nathanvariableToInt(&_passedArguments[2]), nathanvariableToInt(&_passedArguments[3]), nathanvariableToInt(&_passedArguments[13]), nathanvariableToInt(&_passedArguments[14]), nathanvariableToBool(&_passedArguments[15]), nathanvariableToInt(&_passedArguments[12]));
	return;
}
void scriptSetValidityOfInput(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	inputValidity=(nathanvariableToBool(&_passedArguments[0])==1);	
	return;
}
// Fadeout time
// Wait for completely fadeout
void scriptFadeAllBustshots(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	FadeAllBustshots(nathanvariableToInt(&_passedArguments[0]),nathanvariableToBool(&_passedArguments[1]));
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
	if (_totalTime!=0){
		int _xTengoQue = nathanvariableToInt(&_passedArguments[1])-(Busts[_passedSlot].xOffset-320);
		int _yTengoQue = nathanvariableToInt(&_passedArguments[2])-(Busts[_passedSlot].yOffset-240);
		Busts[_passedSlot].bustStatus = BUST_STATUS_SPRITE_MOVE;
		Busts[_passedSlot].diffMoveTime=_totalTime;
		Busts[_passedSlot].startMoveTime=getMilli();
		Busts[_passedSlot].diffXMove = _xTengoQue;
		Busts[_passedSlot].diffYMove = _yTengoQue;
	}else{
		Busts[_passedSlot].xOffset=nathanvariableToInt(&_passedArguments[1])+320;
		Busts[_passedSlot].yOffset=nathanvariableToInt(&_passedArguments[2])+240;
	}
	if (nathanvariableToBool(&_passedArguments[9])){
		while(Busts[_passedSlot].bustStatus!=BUST_STATUS_NORMAL){
			controlsStart();
			if (wasJustPressed(BUTTON_A)){
				Busts[_passedSlot].diffMoveTime=0;
			}
			controlsEnd();
			Update();
			startDrawing();
			Draw(MessageBoxEnabled);
			endDrawing();
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
		easyMessagef(1,"Unknown LoadValueFromLocalWork, %s, please report.",_wordWant);
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
	currentBoxAlpha=255;
	return;
}

// Settings menu override
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
EASYLUAINTSETFUNCTION(oMenuFontSize,forceFontSizeOption)
// Manually set the options if you've chosen to disable the menu option
EASYLUAINTSETFUNCTION(textOnlyOverBackground,textOnlyOverBackground);
EASYLUAINTSETFUNCTION(dynamicAdvBoxHeight,dynamicAdvBoxHeight);
EASYLUAINTSETFUNCTION(advboxHeight,advboxHeight)
EASYLUAINTSETFUNCTION(setADVNameSupport,advNamesSupported)
EASYLUAINTSETFUNCTION(advNamesPersist,advNamesPersist)

// normal image 1, hover image 1, select image 1, normal image 2, hover image 2, select image 2
void scriptImageChoice(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	int i;
	// When you are not interacting with that choice
	crossTexture* _passedNormalImages;
	// When your cursor is over that choice
	crossTexture* _passedHoverImages;
	// When you've selected that choice
	crossTexture* _passedSelectImages;
	int _numberOfChoices = _numArguments/3;
	printf("Found %d choices\n",_numberOfChoices);
	_passedNormalImages = malloc(sizeof(crossTexture)*_numberOfChoices);
	_passedHoverImages = malloc(sizeof(crossTexture)*_numberOfChoices);
	_passedSelectImages = malloc(sizeof(crossTexture)*_numberOfChoices);

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
		controlsStart();
		_userChoice = menuControls(_userChoice,0,_numberOfChoices-1);
		if (wasJustPressed(BUTTON_A)){
			_isHoldSelect=1;
		}
		if (wasJustPressed(BUTTON_UP) || wasJustPressed(BUTTON_DOWN)){
			_isHoldSelect=0;
		}		
		if (wasJustPressed(BUTTON_B)){
			break;
		}
		if (wasJustReleased(BUTTON_A)){
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
void scriptSetFontSize(nathanscriptVariable* _passedArguments, int _numArguments, nathanscriptVariable** _returnedReturnArray, int* _returnArraySize){
	reloadFont(nathanvariableToInt(&_passedArguments[0]));
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
	if (_shouldDrawFilter){
		DrawCurrentFilter();
	}
	if (_shouldDrawMessageBox==1){
		DrawMessageBox(gameTextDisplayMode,currentBoxAlpha);
	}
	if (_shouldDrawHighBusts){
		for (i = maxBusts-1; i != -1; i--){
			if (bustOrderOverBox[i]!=255 && Busts[bustOrderOverBox[i]].isActive==1){
				DrawBust(&(Busts[bustOrderOverBox[i]]));
			}
		}
	}
	if (_shouldDrawMessageText==1){
		DrawMessageText(255,-1,-1);
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
			easyMessagef(1,"What? No preset files? %s is empty.",presetFolder);
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
		char* _bigMessageBuffer = easySprintf("Now that you've upgraded one or more of your StreamingAssets folders to include the preset file, you need to move all your StreamingAssets folder(s) using VitaShell or MolecularShell to\n%s\nYou will need to create that games folder first. After you create that game folder, you won't be able to use preset mode anymore, so make sure you've upgraded all of your StreamingAssets folders before.",gamesFolder);
		OutputLine(_bigMessageBuffer,Line_WaitForInput,0);
		while (!wasJustPressed(BUTTON_A)){
			controlsEnd();
			startDrawing();
			DrawMessageText(255,-1,-1);
			endDrawing();
			controlsStart();
		}
		free(_bigMessageBuffer);
	}else{
		easyMessagef(1,"You did not upgrade any folders.");
	}
	return _didUpgradeOne;
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
void FontSizeSetup(){
	char _choice=0;
	while (1){
		controlsStart();
		_choice = menuControls(_choice,0,2);
		fontSize=retMenuControlsLow(fontSize,0,0,0,1,8,70);;
		if (wasJustPressed(BUTTON_A)){
			if (_choice==1){
				reloadFont(fontSize);
			}else if (_choice==2){
				reloadFont(fontSize);
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
	SETTING_DYNAMICSCAL,
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
	signed char _showADVNamesOption=(gameTextDisplayMode==TEXTMODE_ADV && advNamesSupported==1);

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
		"Dynamic Scaling:",
		"Vita Touch:", // controls
		"Overclock CPU", // misc
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
	_values[SETTING_AUTOSPEED]=&(_tempAutoModeString[0]); // auto stuff
	_values[SETTING_AUTOVOICEDSPEED]=&(_tempAutoModeVoiceString[0]);
	_settingsOn[SETTING_BUSTLOC]=_showArtLocationSlot;
	_settingsOn[SETTING_VNDSWAR]=_shouldShowVNDSSettings;
	_settingsOn[SETTING_VNDSFADE]=_showVNDSFadeOption;
	_settingsOn[SETTING_DYNAMICSCAL]=_showScalingOption;
	#if GBPLAT != GB_VITA
		_settingsOn[SETTING_OVERCLOCK]=0;
	#endif
	_settingsOn[SETTING_DEBUG]=_showDebugButton;
	_settingsOn[SETTING_DEFAULT]=forceResettingsButton;
	//////////////////////////
	int _choice=0;
	char _shouldExit=0;
	while(!_shouldExit){
		// text
		#if GBPLAT == GB_3DS
			_values[SETTING_TEXTSCREEN] = textIsBottomScreen ? "Bottom Screen" : "Top Screen";
		#else
			_settingsOn[SETTING_TEXTSCREEN]=0;
		#endif
		if (_settingsOn[SETTING_DROPSHADOW]){
			_values[SETTING_DROPSHADOW] = charToSwitch(dropshadowOn);
		}
		_settingsOn[SETTING_TEXTBOXW]=(forceTextOverBGOption && preferredTextDisplayMode==TEXTMODE_NVL);
		if (_settingsOn[SETTING_TEXTBOXW]){
			_values[SETTING_TEXTBOXW] = textOnlyOverBackground ? "Small" : "Full";
		}
		if (_settingsOn[SETTING_TEXTMODE]){
			_values[SETTING_TEXTMODE]=(preferredTextDisplayMode==TEXTMODE_ADV ? "ADV" : "NVL");
		}
		_settingsOn[SETTING_ADVNAMES]=(_showADVNamesOption && preferredTextDisplayMode==TEXTMODE_ADV);
		if (_settingsOn[SETTING_ADVNAMES]){
			_values[SETTING_ADVNAMES]=charToSwitch(prefersADVNames);
		}
		_settingsOn[SETTING_HITBOTTOMCLEAR]=(_shouldShowVNDSSettings && preferredTextDisplayMode==TEXTMODE_NVL);
		if (_settingsOn[SETTING_HITBOTTOMCLEAR]){
			_values[SETTING_HITBOTTOMCLEAR] = charToBoolString(vndsClearAtBottom);
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
		if (_settingsOn[SETTING_DYNAMICSCAL]){
			_values[SETTING_DYNAMICSCAL]=charToSwitch(higurashiUsesDynamicScale);
		}
		// controls
		#if GBPLAT == GB_VITA
			_values[SETTING_TOUCH] = charToSwitch(touchProceed);
		#else
			_settingsOn[SETTING_TOUCH]=0;
		#endif
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
		_choice=showMenuAdvanced(_choice,"Settings",SETTINGS_MAX,_settings,_values,_settingsOn,_settingsProp,&_selectionInfo,0);
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
				if (LazyChoice("This will reset your settings.","Is this okay?",NULL,NULL)==1){
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
				vndsClearAtBottom = !vndsClearAtBottom;
				break;
			case SETTING_VNDSWAR:
				showVNDSWarnings = !showVNDSWarnings;
				break;
			case SETTING_DYNAMICSCAL:
				higurashiUsesDynamicScale=!higurashiUsesDynamicScale;
				dynamicScaleEnabled=higurashiUsesDynamicScale;
				updateGraphicsScale();
				updateTextPositions();
				for (i=0;i<maxBusts;++i){
					if (Busts[i].isActive){
						Busts[i].cacheXOffsetScale = GetXOffsetScale(Busts[i].image);
						Busts[i].cacheYOffsetScale = GetYOffsetScale(Busts[i].image);
					}
				}
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
	applyTextboxChanges();
	controlsEnd();
	SaveSettings();
	if (currentGameStatus!=GAMESTATUS_TITLE){
		// If we changed art location, reload busts
		if (_artBefore != graphicsLocation){
			for (i=0;i<maxBusts;++i){
				if (Busts[i].isActive){
					char* _cacheFilename = strdup(Busts[i].relativeFilename);
					DrawBustshot(i,_cacheFilename,Busts[i].xOffset,Busts[i].yOffset,Busts[i].layer,0,0,Busts[i].isInvisible);
					free(_cacheFilename);
				}
			}
		}
	}
	#if GBPLAT == GB_3DS
		if (textIsBottomScreen==1){
			outputLineScreenWidth = 320;
			outputLineScreenHeight = 240;
		}else{
			outputLineScreenWidth = 400;
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
char* newShowMap(int _numElements){
	char* _ret = malloc(sizeof(char)*_numElements);
	memset(_ret,1,sizeof(char)*_numElements);
	return _ret;
}
// returns -1 if user quit, otherwise returns chosen index
// pass the real _choice index
// does not support horizontal scrolling for options with _optionValues
int showMenuAdvanced(int _choice, const char* _title, int _mapSize, char** _options, char** _optionValues, char* _showMap, optionProp* _optionProp, char* _returnInfo, char _menuProp){
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
	int _ret=-1;
	int _numOptions;
	if (_showMap){
		_numOptions=0;
		for (i=0;i<_mapSize;++i){
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
	while(currentGameStatus!=GAMESTATUS_QUIT){
		controlsStart();
		if (menuControlsLow(&_choice,1,1,0,(_menuProp & MENUPROP_CANPAGEUPDOWN) ? _optionsOnScreen : 0,0,_numOptions-1)){
			_scrollOffset=-1;
		}
		if (_scrollOffset==-1){ // queued menu info update
			// Find horizontal scroll info
			_curRealIndex = -1;
			for (i=0;i<=_choice;++i){
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
			// Get the real index and set _ret to it
			_ret=-1;
			for (i=0;i<=_choice;++i){
				_ret=getNextEnabled(_showMap,_ret+1);
			}
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
		for (i=0;i<_scrollOffset;++i){
			_lastDrawn = getNextEnabled(_showMap,_lastDrawn+1);
		}
		// Draw the entries currently on screen
		for (i=0;i<_optionsOnScreen;++i){
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
					_r=0;
					_g=255;
					_b=0;
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
		endDrawing();
	}
	controlsEnd();
	return _ret;
}
int showMenu(int _defaultChoice, const char* _title, int _numOptions, char** _options, char _canQuit){
	char _menuProps=MENUPROP_CANPAGEUPDOWN;
	if (_canQuit){
		_menuProps|=MENUPROP_CANQUIT;
	}
	return showMenuAdvanced(_defaultChoice,_title,_numOptions,_options,NULL,NULL,NULL,NULL,_menuProps);
}
void TitleScreen(){
	signed char _choice=0;
	int _versionStringWidth = textWidth(normalFont,VERSIONSTRING VERSIONSTRINGSUFFIX);
	char _bottomConfigurationString[13+strlen(SYSTEMSTRING)];
	strcpy(_bottomConfigurationString,SYSTEMSTRING);
	if (isGameFolderMode){
		strcat(_bottomConfigurationString,";Games");
	}else{
		strcat(_bottomConfigurationString,";Presets");
	}
	#if GBPLAT != GB_3DS
		if (isActuallyUsingUma0){
			strcat(_bottomConfigurationString,";uma0");
		}else{
			strcat(_bottomConfigurationString,";ux0");
		}
	#endif
	while (currentGameStatus!=GAMESTATUS_QUIT){
		controlsStart();
		_choice = menuControls(_choice, 0, isGameFolderMode ? 3 : 4);
		if (wasJustPressed(BUTTON_A)){
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
						easyMessagef(1,"%s does not exist and no files in %s. Do you have any files?",scriptFolder,presetFolder);
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
				controlsStart();
				controlsEnd();
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
						RunScript(scriptFolder,_tempManualFileSelectionResult,0);
						free(_tempManualFileSelectionResult);
						currentGameStatus=GAMESTATUS_TITLE;
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
					easyMessagef(1,"You really shouldn't be here. You haven't escaped, you know? You're not even going the right way.");
				}else{
					ClearMessageArray(0);
					controlsStart();
					controlsEnd();
					OutputLine("This process will convert your legacy preset & StreamingAssets setup to the new game folder setup. It makes everything easier, so you should do it.\n\nHere's how this will work:\n1) Select a preset file\n2) That preset file will be put in the SteamingAssets folder for you. If you already upgraded the StreamingAssets folder, the preset file just overwrite the old one.\n3) Repeat for all of your games.\n4) You must manually move the StreamingAssets folder(s) using VitaShell or MolecularShell to the games folder.\n\nIf it sounds too hard for you, there's also a video tutorial on the Wololo thread.",Line_WaitForInput,0);
					while (!wasJustPressed(BUTTON_A)){
						controlsEnd();
						startDrawing();
						DrawMessageText(255,-1,-1);
						endDrawing();
						controlsStart();
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

		drawText(MENUOPTIONOFFSET,5,"Main Menu");

		// Menu options
		drawText(MENUOPTIONOFFSET,5+currentTextHeight*(0+2),"Load game");
		drawText(MENUOPTIONOFFSET,5+currentTextHeight*(1+2),"Manual mode");
		drawText(MENUOPTIONOFFSET,5+currentTextHeight*(2+2),"Basic Settings");
		drawText(MENUOPTIONOFFSET,5+currentTextHeight*(3+2),"Exit");
		if (!isGameFolderMode){
			gbDrawText(normalFont,MENUOPTIONOFFSET,5+currentTextHeight*(4+2),"Upgrade to game folder mode",0,255,0);
		}

		// Extra bottom data
		gbDrawText(normalFont,(screenWidth-5)-_versionStringWidth,screenHeight-5-currentTextHeight,VERSIONSTRING VERSIONSTRINGSUFFIX,VERSIONCOLOR);
		drawText(5,screenHeight-5-currentTextHeight,_bottomConfigurationString);

		// Cursor
		drawText(5,5+currentTextHeight*(_choice+2),MENUCURSOR);

		#if GBPLAT == GB_3DS
			startDrawingBottom();
		#endif
		endDrawing();
		controlsEnd();
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
	int i;
	for (i=0;i<=currentPresetChapter;++i){
		_options[i]=chapterNamesLoaded ? currentPresetFileFriendlyList.theArray[i] : currentPresetFileList.theArray[i];
	}
	int _chosenIndex=0;
	while(1){
		_chosenIndex = showMenu(_chosenIndex,"Chapter Jump",currentPresetChapter+1,_options,1);
		if (_chosenIndex==-1){
			break;
		}else{
			currentGameStatus=GAMESTATUS_MAINGAME;
			RunScript(scriptFolder,currentPresetFileList.theArray[_chosenIndex],1);
			continue;
		}
	}
	free(_options);
}
void SaveGameEditor(){
	controlsEnd();
	while (1){
		controlsStart();
		currentPresetChapter = retMenuControlsLow(currentPresetChapter,0,0,1,1,0,currentPresetFileList.length-1);
		if (wasJustPressed(BUTTON_A)){
			SaveGame();
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
		if (isGameFolderMode && !isEmbedMode && LazyChoice(defaultGameIsSet ? "Unset this game as the default?" : "Set this game as the default game?",NULL,NULL,NULL)){
			defaultGameIsSet = !defaultGameIsSet;
			setDefaultGame(defaultGameIsSet ? currentGameFolderName : "NONE");
		}
	}
}
void NavigationMenu(){
	char* _menuTitle;
	if (chapterNamesLoaded){
		_menuTitle=easySprintf("End of script: %s",currentPresetFileFriendlyList.theArray[currentPresetChapter]);
	}else{
		_menuTitle=easySprintf("End of script: %d",currentPresetChapter);
	}
	char* _menuOptions[] = {
		"Next",
		"Chapter Jump",
		"View TIPS",
		"Exit",
	};
	char* _optionOn = newShowMap(4);
	// if tips disabled or no tips unlocked
	if (!gameHasTips || currentPresetTipUnlockList.theArray[currentPresetChapter]==0){
		_optionOn[2]=0;
	}
	if (currentPresetChapter+1>=currentPresetFileList.length){
		_optionOn[0]=0;
	}
	int _choice=0;
	while(currentGameStatus!=GAMESTATUS_QUIT){
		_choice = showMenuAdvanced(_choice,_menuTitle,4,_menuOptions,NULL,_optionOn,NULL,NULL,0);
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
			currentGameStatus=GAMESTATUS_QUIT;
		}
	}
	free(_optionOn);
	free(_menuTitle);
	controlsEnd();
}
void NewGameMenu(){
	char* _options[]={"Start from beginning","Savegame Editor"};
	char _choice=showMenu(0,"NEW GAME",2,_options,0);
	if (_choice==1){
		currentPresetChapter=0;
		SaveGameEditor();
	}
}
// Returns selected slot or -1
int vndsSaveSelector(){
	#ifdef OVERRIDE_VNDSSAVEMENU
		return customVNDSSaveSelector();
	#endif
	controlsStart();
	controlsEnd();
	// screenWidth/3/2 free space for each text
	int _slotWidth = screenWidth/SAVEMENUPAGEW;
	int _slotHeight = screenHeight/SAVEMENUPAGEH;

	crossTexture _loadedThumbnail[SAVEMENUPAGESIZE]={NULL};
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
	// If the ADV box height won't change make sure the user doesn't make the font size huge
	if (dynamicAdvBoxHeight==0){
		forceFontSizeOption=0;
	}
	controlsStart();
	signed char _choice=0;
	char* _loadedNovelName=NULL;

	crossTexture _loadedThumbnail=NULL;

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
	if (!isDown(BUTTON_L) && !isDown(BUTTON_R)){
		easyMessagef(0,"Loading font");
		_possibleThunbnailPath[strlen(streamingAssets)]=0;
		strcat(_possibleThunbnailPath,"/default.ttf");
		if (checkFileExist(_possibleThunbnailPath)){
			globalLoadFont(_possibleThunbnailPath);
		}
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
				forceFontSizeOption=0;
				if (_choice==0){
					int _chosenSlot = vndsSaveSelector();
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
			if (LazyChoice("Make thumbnails from old saves?",NULL,NULL,NULL)){
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
		char _didLoadHappyLua = SafeLuaDoFile(L,_fixedPath,0);
		free(_fixedPath);
		lua_sethook(L, incrementScriptLineVariable, LUA_MASKLINE, 5);
		if (_didLoadHappyLua==1){
			#if GBPLAT == GB_VITA
				easyMessagef(1,"happy.lua is missing for some reason. Redownload the VPK. If that doesn't fix it, report the problem to MyLegGuy.");
			#else
				easyMessagef(1,"happy.lua missing.");
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
void hVitaCrutialInit(){
	srand(time(NULL));
	generalGoodInit();
	initGraphics(960,544,0);
	screenWidth = getScreenWidth();
	screenHeight = getScreenHeight();
	initImages();
	setClearColor(0,0,0);
	isActuallyUsingUma0=initGoodBrewDataDir();
	#if GBPLAT == GB_VITA
		sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, 1);
		sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, 1);
		sceTouchEnableTouchForce(SCE_TOUCH_PORT_FRONT);
		sceTouchEnableTouchForce(SCE_TOUCH_PORT_BACK);
	#elif GBPLAT == GB_3DS
		osSetSpeedupEnable(1);
	#endif
}
// verify install
void hVitaCheckVpk(){
	char* _embeddedCheckPath = fixPathAlloc("assets/star.png",TYPE_EMBEDDED);
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
	// Check if games folder mode or present files mode
	_fixedPath = fixPathAlloc("Games/",TYPE_DATA);
	if (directoryExists(_fixedPath)){
		isGameFolderMode=1;
	}else{ // Only allow present mode if games folder doesn't exist
		free(_fixedPath);
		_fixedPath=fixPathAlloc("Presets/",TYPE_DATA);
		isGameFolderMode = !(directoryExists(_fixedPath));
	}
	free(_fixedPath);
}
void hVitaInitFont(){
	// Load default font
	if (fontSize<0){
		fontSize = getResonableFontSize(GBTXT);
	}
	#if GBTXT==GBTXT_BITMAP
		currentFontFilename = fixPathAlloc("assets/Bitmap-LiberationSans-Regular",TYPE_EMBEDDED);
	#else
		currentFontFilename = fixPathAlloc("assets/LiberationSans-Regular.ttf",TYPE_EMBEDDED);
	#endif //loadFont("sa0:data/font/pvf/ltn4.pvf");
	globalLoadFont(currentFontFilename);
	menuCursorSpaceWidth = textWidth(normalFont,MENUCURSOR" ");
	// Needed for any advanced message display
	imageCharImages[IMAGECHARUNKNOWN] = LoadEmbeddedPNG("assets/unknown.png");
	imageCharImages[IMAGECHARNOTE] = LoadEmbeddedPNG("assets/note.png");
	imageCharImages[IMAGECHARSTAR] = LoadEmbeddedPNG("assets/star.png");
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
	for (i=0;i<MAXIMAGECHAR;i++){
		imageCharType[i]=-1;
	}
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
	outputLineScreenWidth = screenWidth;
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
signed char init(){
	#ifdef OVERRIDE_INIT
		return customInit();
	#endif
	hVitaCrutialInit();
	hVitaCheckVpk();
	hVitaInitMisc();
	hVitaInitSettings();
	hVitaInitFont();
	hVitaInitSound();
	return initializeLua();
}
#ifdef SPECIALEDITION
	#include "specialEditionFooter.h"
#endif
int main(int argc, char *argv[]){
	/* code */
	if (init()==2){
		currentGameStatus = GAMESTATUS_QUIT;
	}
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
				char* _presentFullPath = easyCombineStrings(2,presetFolder,currentPresetFilename);
				LoadPreset(_presentFullPath);
				free(_presentFullPath);
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
					easyMessagef(1,"No presets found. If you ran the converter, you should've gotten some. You can manually put presets in: %s",presetFolder);
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
			case GAMESTATUS_GAMEFOLDERSELECTION: // Sets game folder selection folder name to currentGameFolderName
				;
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
							easyMessagef(1,"Reset default game setting.");
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
