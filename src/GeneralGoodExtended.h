#ifndef GENERALGOODSTUFFEXTENDED
#define GENERALGOODSTUFFEXTENDED
	#define ISUSINGEXTENDED 1
	
	char tempPathFixBuffer[256];
	char* DATAFOLDER=NULL;
	
	// For FixPath argument
	#define TYPE_UNDEFINED 0
	#define TYPE_DATA 1
	#define TYPE_EMBEDDED 2

	#if DOFIXCOORDS == 1
		int FixX(int x);
		int FixY(int y);
	#endif

	#if DOFIXCOORDS == 1
		void FixCoords(int* _x, int* _y){
			*_x = FixX(*_x);
			*_y = FixY(*_y);
		}
		#define EASYFIXCOORDS(x, y) FixCoords(x,y)
	#else
		#define EASYFIXCOORDS(x,y)
	#endif


	unsigned char isSkipping=0;
	signed char InputValidity = 1;

	int screenHeight = 544;
	int screenWidth = 960;

	#include <stdio.h>
	#include <math.h>
	#include <string.h>
	#include <stdlib.h>
	#include <unistd.h>

	int SCE_TOUCH = 19;
	int SCE_ANDROID_BACK = 20;
	int touchX=-1;
	int touchY=-1;

	// Platform stuff
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

		// If we're not using SDL, we'll need these
		#if RENDERER != REND_SDL
			// Controls at start of frame.
			SceCtrlData pad;
			// Controls from start of last frame.
			SceCtrlData lastPad;
		#endif
	#endif
	#if PLATFORM == PLAT_WINDOWS
		// Header for directory functions
		#include <dirent.h>
	#endif
	#if PLATFORM != PLAT_VITA
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
	#if PLATFORM == PLAT_3DS
		u32 pad;
		u32 wasJustPad;
	#endif

	// Subplatform Stuff
	#if SUBPLATFORM == SUB_ANDROID
		// For mkdir
		#include <sys/stat.h>
		// So we can see console output with adb logcat
		#define printf SDL_Log

		//#include <android/asset_manager.h>
		//#include <android/asset_manager_jni.h>
	#endif

	// Renderer stuff
	#if RENDERER == REND_SDL
		// Stores control data
		char pad[21]={0};
		char lastPad[21]={0};
	#endif
	

	//////////////////////////////////////////////////////////
	// Need dis
	#include "GeneralGood.h"
	//////////////////////////////////////////////////////////

	// itoa replacement for android which only supports base 10
	#if SUBPLATFORM == SUB_ANDROID
		void itoa(int _num, char* _buffer, int _uselessBase){
			sprintf(_buffer, "%d", _num);
		}
	#endif

	signed char WasJustReleased(int value){
		if (InputValidity==1 || isSkipping==1){
			#if PLATFORM == PLAT_VITA
				if (lastPad.buttons & value && !(pad.buttons & value)){
					return 1;
				}
			#elif PLATFORM == PLAT_WINDOWS
				if (lastPad[value]==1 && pad[value]==0){
					return 1;
				}
			#endif
		}
		return 0;
	}

	signed char WasJustPressedRegardless(int value){
		
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
		
		return 0;
	}

	signed char WasJustPressed(int value){
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

	void ControlsStart(){
		#if PLATFORM == PLAT_VITA
			sceCtrlPeekBufferPositive(0, &pad, 1);
			//sceTouchPeek(SCE_TOUCH_PORT_FRONT, &currentTouch, 1);
		#endif
		#if RENDERER == REND_SDL
			SDL_Event e;
			while( SDL_PollEvent( &e ) != 0 ){
				if( e.type == SDL_QUIT ){
					//XOutFunction();
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
						}else if (e.key.keysym.sym==SDLK_ESCAPE || e.key.keysym.sym==SDLK_RETURN){ /* Start */
							pad[SCE_CTRL_START]=1;
						}else if (e.key.keysym.sym==SDLK_e){ /* Select */
							pad[SCE_CTRL_SELECT]=1;
						}else if (e.key.keysym.sym==SDLK_b || e.key.keysym.sym==SDLK_AC_BACK){ /* Back button on android */
							pad[SCE_ANDROID_BACK]=1;
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
						}else if (e.key.keysym.sym==SDLK_ESCAPE || e.key.keysym.sym==SDLK_RETURN){ /* Start */
							pad[SCE_CTRL_START]=0;
						}else if (e.key.keysym.sym==SDLK_e){ /* Select */
							pad[SCE_CTRL_SELECT]=0;
						}else if (e.key.keysym.sym==SDLK_b || e.key.keysym.sym==SDLK_AC_BACK){ /* Back button on android */
							pad[SCE_ANDROID_BACK]=0;
						}
					}
				#endif
				
				#if PLATFORM == PLAT_WINDOWS
					if( e.type == SDL_FINGERDOWN || (pad[SCE_TOUCH]==1 && e.type == SDL_FINGERMOTION)){
						touchX = e.tfinger.x * screenWidth;
						touchY = e.tfinger.y * screenHeight;
						pad[SCE_TOUCH]=1;
					}else if (e.type == SDL_MOUSEBUTTONDOWN || (pad[SCE_TOUCH]==1 && e.type == SDL_MOUSEMOTION) ){
						SDL_GetMouseState(&touchX,&touchY);
						pad[SCE_TOUCH] = 1;
					}
					if (e.type == SDL_FINGERUP){
						pad[SCE_TOUCH] = 0;
					}else if (e.type == SDL_MOUSEBUTTONUP){
						pad[SCE_TOUCH] = 0;
					}
				#endif

			}
		#endif

		// Platform for ANDROID is also PLAT_WINDOWS
		//#if PLATFORM == PLAT_WINDOWS
		//	CheckTouchControls();
		//#endif
	}

	void ControlsEnd(){
		#if PLATFORM == PLAT_VITA
			lastPad=pad;
		#elif PLATFORM == PLAT_WINDOWS
			memcpy(lastPad,pad,sizeof(pad));
		#endif
	}

	void ControlsResetFull(){
		#if PLATFORM != PLAT_VITA
			memset(&pad,0xFF,sizeof(pad));
			memset(&lastPad,0xFF,sizeof(lastPad));
		#elif PLATFORM == PLAT_VITA
			memset(&pad.buttons,0xFF,sizeof(pad.buttons));
			memset(&lastPad.buttons,0xFF,sizeof(pad.buttons));
		#endif
	}

	void ControlsReset(){
		ControlsStart();
		ControlsEnd();
	}

	void ControlsResetEmpty(){
		#if PLATFORM != PLAT_VITA
			memset(&pad,0,sizeof(pad));
			memset(&lastPad,0,sizeof(lastPad));
		#elif PLATFORM == PLAT_VITA
			memset(&pad.buttons,0,sizeof(pad.buttons));
			memset(&lastPad.buttons,0,sizeof(pad.buttons));
		#endif
	}

	signed char IsDown(int value){
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

	// Passed string should be freed already
	void GenerateDefaultDataDirectory(char** _dataDirPointer, char _useUma0){
		#if SUBPLATFORM == SUB_ANDROID
			*_dataDirPointer = malloc(strlen("/data/data/"ANDROIDPACKAGENAME"/")+1);
			strcpy(*_dataDirPointer,"/data/data/"ANDROIDPACKAGENAME"/"+1);
		#elif PLATFORM == PLAT_WINDOWS
			*_dataDirPointer = malloc(strlen("./")+1);
			strcpy(*_dataDirPointer,"./");
		#elif PLATFORM == PLAT_VITA
			if (_useUma0){
				*_dataDirPointer = malloc(strlen("uma0:data/"VITAAPPID"/")+1);
				strcpy(*_dataDirPointer,"uma0:data/"VITAAPPID"/");
			}else{
				*_dataDirPointer = malloc(strlen("ux0:data/"VITAAPPID"/")+1);
				strcpy(*_dataDirPointer,"ux0:data/"VITAAPPID"/");
			}
		#endif
	}

	void FixPath(char* filename,char _buffer[], char type){
		if (DATAFOLDER==NULL){
			GenerateDefaultDataDirectory(&DATAFOLDER,USEUMA0);
		}
		#if SUBPLATFORM == SUB_ANDROID
			if (type==TYPE_DATA){
				strcpy((char*)_buffer,DATAFOLDER);
			}else if (type==TYPE_EMBEDDED){
				strcpy((char*)_buffer,"");
			}
			strcat((char*)_buffer,filename);
		#elif PLATFORM == PLAT_WINDOWS
			if (type==TYPE_DATA){
				strcpy((char*)_buffer,DATAFOLDER);
			}else if (type==TYPE_EMBEDDED){
				strcpy((char*)_buffer,"./");
			}
			strcat((char*)_buffer,filename);
		#elif PLATFORM == PLAT_VITA
			if (type==TYPE_DATA){
				strcpy((char*)_buffer,DATAFOLDER);
			}else if (type==TYPE_EMBEDDED){
				strcpy((char*)_buffer,"app0:");
			}
			strcat((char*)_buffer,filename);
		#endif
	}

	void MakeDataDirectory(){
		char tempPathFixBuffer[256];
		FixPath("",tempPathFixBuffer,TYPE_DATA);
		if (directoryExists((const char*)tempPathFixBuffer)==0){
			createDirectory((const char*)tempPathFixBuffer);
		}
	}

#endif
