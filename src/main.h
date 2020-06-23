#ifndef GOOD_MAIN_HEADER_HAS_DOES_BEEN_INCLUDED
#define GOOD_MAIN_HEADER_HAS_DOES_BEEN_INCLUDED

#define wasJustPressed(x) ((currentGameStatus!=GAMESTATUS_MAINGAME || inputValidity || isSkipping) && wasJustPressed(x))
#define isDown(x) ((currentGameStatus!=GAMESTATUS_MAINGAME || inputValidity || isSkipping) && isDown(x))

typedef unsigned char optionProp;

void startDrawing();
void Draw(char _shouldDrawMessageBox);
void RecalculateBustOrder();
void PlayBGM(const char* filename, int _volume, int _slot);
void easyMessagef(char _doWait, const char* _formatString, ...);
void SaveSettings();
void XOutFunction();
void historyMenu();
void SaveGameEditor();
void SettingsMenu(signed char _shouldShowQuit, signed char _shouldShowVNDSSettings, signed char _shouldShowVNDSSave, signed char _shouldShowRestartBGM, signed char _showArtLocationSlot, signed char _showScalingOption, signed char _showTextBoxModeOption, signed char _showVNDSFadeOption, signed char _showDebugButton);
char FileSelector(char* directorylocation, char** _chosenfile, char* promptMessage);
void initializeNathanScript();
void activateVNDSSettings();
void activateHigurashiSettings();
void showTextbox();
void hideTextbox();
void testCode();
signed int atLeastOne(signed int _input);
void drawAdvanced(char _shouldDrawBackground, char _shouldDrawLowBusts, char _shouldDrawFilter, char _shouldDrawMessageBox, char _shouldDrawHighBusts, char _shouldDrawMessageText);
void loadADVBox();
int inBetweenVNDSLines(int _aboutToCommandIndex);
char lazyLuaError(int _loadResult);
char* easyVNDSSaveName(int _slot);
char vndsNormalSave(char* _filename, char _saveSpot, char _saveThumb);
int vndsSaveSelector(char _isSave);
void drawHallowRect(int _x, int _y, int _w, int _h, int _thick, int _r, int _g, int _b, int _a);
void safeVNDSSaveMenu();
int showMenu(int _defaultChoice, const char* _title, int _numOptions, char** _options, char _canQuit);
int showMenuAdvanced(int _choice, const char* _title, int _mapSize, char** _options, char** _optionValues, char* _showMap, optionProp* _optionProp, char* _returnInfo, char _menuProp);
char* newShowMap(int _numElements);
void setADVName(char* _newName);
void saveGlobalVNDSVars();
void drawWrappedText(int _x, int _y, char** _passedLines, int _numLines, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void freeWrappedText(int _numLines, char** _passedLines);
void wrapText(const char* _passedMessage, int* _numLines, char*** _realLines, int _maxWidth);
char isSpaceOrEmptyStr(const char* _check);
void enlargeScreenManual(int _destOffX, int _destOffY, double _destScaleX, double _destScaleY, int _time, char _waitForCompletion);
typedef struct{
	char** theArray;
	unsigned char length;
}goodStringMallocArray;
typedef struct{
	unsigned char* theArray;
	unsigned char length;
}goodu8MallocArray;

#endif
