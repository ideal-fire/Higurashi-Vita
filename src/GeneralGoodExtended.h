#ifndef GENERALGOODSTUFFEXTENDED
#define GENERALGOODSTUFFEXTENDED

	unsigned char isSkipping=0;
	signed char InputValidity = 1;



	#include <stdio.h>

	// Platform stuff
	#if PLATFORM == PLAT_VITA
		#include <psp2/ctrl.h>
		#include <psp2/kernel/processmgr.h>
		#include <psp2/rtc.h>
		#include <psp2/types.h>
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
		};
	#endif

	// Subplatform Stuff
	#if SUBPLATFORM == SUB_ANDROID
		// For mkdir
		#include <sys/stat.h>
	#endif

	// Renderer stuff
	#if RENDERER == REND_SDL

		#define CrossTexture SDL_Texture
		#include <SDL2/SDL.h>
		#include <SDL2/SDL_image.h>

		// Stores control data
		char pad[19]={0};
		char lastPad[19]={0};

		//The window we'll be rendering to
		SDL_Window* mainWindow;
		
		//The window renderer
		SDL_Renderer* mainWindowRenderer;
	#endif
	#if RENDERER == REND_VITA2D
		#include <vita2d.h>
		// CROSS TYPES
		#define CrossTexture vita2d_texture
	#endif

	// Sound stuff
	#if SOUNDPLAYER == SND_SDL
		#include <SDL2/SDL.h>
		#include <SDL2/SDL_mixer.h>

		#define CROSSSFX Mix_Chunk
		#define CROSSMUSIC Mix_Music
	#endif
	#if SOUNDPLAYER == SND_NONE
		#define CROSSSFX int
		#define CROSSMUSIC int
	#endif

	//////////////////////////////////////////////////////////
	// Need dis
	#include "GeneralGood.h"
	//////////////////////////////////////////////////////////


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

	void ControlsStart(){
		#if PLATFORM == PLAT_VITA
			sceCtrlPeekBufferPositive(0, &pad, 1);
			//sceTouchPeek(SCE_TOUCH_PORT_FRONT, &currentTouch, 1);
		#endif
		#if RENDERER == REND_SDL
			SDL_Event e;
			while( SDL_PollEvent( &e ) != 0 ){
				if( e.type == SDL_QUIT ){
					XOutFunction();
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
						}else if (e.key.keysym.sym==SDLK_e){ /* Select */
							pad[SCE_CTRL_SELECT]=1;
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
						}else if (e.key.keysym.sym==SDLK_e){ /* Select */
							pad[SCE_CTRL_SELECT]=0;
						}
					}
				#endif
			}
		#endif
	}

	void ControlsEnd(){
		#if PLATFORM == PLAT_VITA
			lastPad=pad;
		#elif PLATFORM == PLAT_WINDOWS
			memcpy(lastPad,pad,19);
		#endif
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

	// Checks if the byte is the one for a newline
	// If it's 0D, it seeks past 0A
	signed char IsNewLine(FILE* fp, unsigned char _temp){
		if (_temp==13){
			fseek(fp,1,SEEK_CUR);
			return 1;
		}
		if (_temp=='\n' || _temp==10){
			// It seems like the other newline char is skipped for me?
			return 1;
		}
		return 0;
	}

#endif
