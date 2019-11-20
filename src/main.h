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
void DrawHistory(unsigned char _textStuffToDraw[][SINGLELINEARRAYSIZE]);
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
int vndsSaveSelector();
void drawHallowRect(int _x, int _y, int _w, int _h, int _thick, int _r, int _g, int _b, int _a);
void safeVNDSSaveMenu();
int showMenu(int _defaultChoice, const char* _title, int _numOptions, char** _options, char _canQuit);
int showMenuAdvanced(int _choice, const char* _title, int _mapSize, char** _options, char** _optionValues, char* _showMap, optionProp* _optionProp, char* _returnInfo, char _menuProp);
char* newShowMap(int _numElements);
typedef struct{
	char** theArray;
	unsigned char length;
}goodStringMallocArray;
typedef struct{
	unsigned char* theArray;
	unsigned char length;
}goodu8MallocArray;

#endif
