#ifndef GENERALGOODEXTENDED_H
#define GENERALGOODEXTENDED_H

#if PLATFORM != PLAT_VITA && PLATFORM != PLAT_3DS && PLATFORM != PLAT_VITA
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
		//int SCE_TOUCH = 19;
	};
#endif
#if PLATFORM == PLAT_VITA
	#include <psp2/ctrl.h>
	#include <psp2/kernel/processmgr.h>
	#include <psp2/rtc.h>
	#include <psp2/types.h>
	#include <psp2/display.h>
	#include <psp2/touch.h>
	#include <psp2/io/fcntl.h>
	#include <psp2/io/dirent.h>
	#include <psp2/power.h>
#endif

#if PLATFORM == PLAT_3DS
	#define SCE_CTRL_CROSS KEY_A
	#define SCE_CTRL_CIRCLE KEY_B
	#define SCE_CTRL_UP KEY_UP
	#define SCE_CTRL_DOWN KEY_DOWN
	#define SCE_CTRL_LEFT KEY_LEFT
	#define SCE_CTRL_RIGHT KEY_RIGHT
	#define SCE_CTRL_TRIANGLE KEY_X
	#define SCE_CTRL_SQUARE KEY_Y
	#define SCE_CTRL_START KEY_START
	#define SCE_CTRL_SELECT KEY_SELECT
#endif

#if PLATFORM == PLAT_SWITCH
	#include <switch.h>

	#define SCE_CTRL_CROSS KEY_A
	#define SCE_CTRL_CIRCLE KEY_B
	#define SCE_CTRL_TRIANGLE KEY_X
	#define SCE_CTRL_SQUARE KEY_Y
	#define SCE_CTRL_START KEY_PLUS
	#define SCE_CTRL_SELECT KEY_MINUS
	//
	#define SCE_CTRL_LTRIGGER KEY_ZL
	#define SCE_CTRL_RTRIGGER KEY_ZR
	//
	#define SCE_CTRL_UP KEY_UP
	#define SCE_CTRL_DOWN KEY_DOWN
	#define SCE_CTRL_LEFT KEY_LEFT
	#define SCE_CTRL_RIGHT KEY_RIGHT

	#define possibleControlNumberType u64
#else
	#define possibleControlNumberType int
#endif

extern char tempPathFixBuffer[256];
extern char* DATAFOLDER;

extern unsigned char isSkipping;
extern signed char InputValidity;

extern int screenHeight;
extern int screenWidth;

// For FixPath argument
#define TYPE_UNDEFINED 0
#define TYPE_DATA 1
#define TYPE_EMBEDDED 2

void controlsEnd();
void controlsResetEmpty();
void controlsStart();
void fixCoords(int* _x, int* _y);
void fixPath(char* filename,char _buffer[], char type);
void generateDefaultDataDirectory(char** _dataDirPointer, char _useUma0);
#if SUBPLATFORM == SUB_ANDROID
	void itoa(int _num, char* _buffer, int _uselessBase);
#endif
void makeDataDirectory();
possibleControlNumberType isDown(int value);
possibleControlNumberType wasJustPressedRegardless(possibleControlNumberType value);
possibleControlNumberType wasJustPressed(possibleControlNumberType value);
possibleControlNumberType wasJustReleased(possibleControlNumberType value);
 
#endif /* GENERALGOODEXTENDED_H */