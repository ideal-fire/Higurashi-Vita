#ifndef GOOD_MAIN_HEADER_HAS_DOES_BEEN_INCLUDED
#define GOOD_MAIN_HEADER_HAS_DOES_BEEN_INCLUDED

void startDrawing();
void Draw(char _shouldDrawMessageBox);
void RecalculateBustOrder();
void PlayBGM(const char* filename, int _volume, int _slot);
void LazyMessage(const char* stra, const char* strb, const char* strc, const char* strd);
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
char* easyVNDSSaveSlot(signed char _slot);
typedef struct{
	char** theArray;
	unsigned char length;
}goodStringMallocArray;
typedef struct{
	unsigned char* theArray;
	unsigned char length;
}goodu8MallocArray;

#endif
