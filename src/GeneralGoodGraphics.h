#ifndef GENERALGOODGRAPHICSHEADER
#define GENERALGOODGRAPHICSHEADER

	#if ISUSINGEXTENDED == 0
		int FixX(int x);
		int FixY(int y);
	#endif

	#if ISUSINGEXTENDED == 0
		#if DOFIXCOORDS == 1
			void FixCoords(int* _x, int* _y){
				*_x = FixX(*_x);
				*_y = FixY(*_y);
			}
			#define EASYFIXCOORDS(x, y) FixCoords(x,y)
		#else
			#define EASYFIXCOORDS(x,y)
		#endif
	#endif

	// Renderer stuff
	#if RENDERER == REND_SDL
		#define CrossTexture SDL_Texture
		#include <SDL2/SDL.h>
		#include <SDL2/SDL_image.h>
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

	// _windowWidth and _windowHeight are recommendations for the Window size. Will be ignored on Android, Vita, etc.
	void initGraphics(int _windowWidth, int _windowHeight, int* _storeWindowWidth, int* _storeWindowHeight){
		#if RENDERER == REND_SDL
			SDL_Init(SDL_INIT_VIDEO);
			// If platform is Android, make the window fullscreen and store the screen size in the arguments.
			#if SUBPLATFORM == SUB_ANDROID
				SDL_DisplayMode displayMode;
				if( SDL_GetCurrentDisplayMode( 0, &displayMode ) == 0 ){
					mainWindow = SDL_CreateWindow( "HappyWindo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, displayMode.w, displayMode.h, SDL_WINDOW_SHOWN );
				}else{
					printf("Failed to get display mode....\n");
				}
				*_storeWindowWidth=displayMode.w;
				*_storeWindowHeight=displayMode.h;
			#else
				mainWindow = SDL_CreateWindow( "HappyWindo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, _windowWidth, _windowHeight, SDL_WINDOW_SHOWN );
				*_storeWindowWidth=_windowWidth;
				*_storeWindowHeight=_windowHeight;
			#endif
			if (USEVSYNC){
				mainWindowRenderer = SDL_CreateRenderer( mainWindow, -1, SDL_RENDERER_PRESENTVSYNC);
			}else{
				mainWindowRenderer = SDL_CreateRenderer( mainWindow, -1, SDL_RENDERER_ACCELERATED);
			}
			showErrorIfNull(mainWindowRenderer);
			IMG_Init( IMG_INIT_PNG );
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
		#else
			#if RENDERER == REND_VITA2D
				vita2d_init();
			#else
				#error Hi, Nathan here. I have to make graphics init function for this renderer. RENDERER
			#endif
		#endif
	}

	void StartDrawing(){
		#if RENDERER == REND_VITA2D
			vita2d_start_drawing();
			vita2d_clear_screen();
		#elif RENDERER == REND_SDL
			SDL_RenderClear(mainWindowRenderer);
		#elif RENDERER == REND_SF2D
			sf2d_start_frame(GFX_TOP, GFX_LEFT);
		#endif
	}
	
	void EndDrawing(){
		#if PLATFORM == PLAT_WINDOWS
			DrawTouchControlsHelp();
		#endif
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
	/*
	=================================================
	== IMAGES
	=================================================
	*/
	
	/*
	/////////////////////////////////////////////////////
	////// CROSS PLATFORM DRAW FUNCTIONS
	////////////////////////////////////////////////////
	*/
	void SetClearColor(int r, int g, int b, int a){
		if (a!=255){
			printf("You're a moron\n");
		}
		#if RENDERER == REND_SDL
			SDL_SetRenderDrawColor( mainWindowRenderer, r, g, b, a );
		#elif RENDERER == REND_VITA2D
			vita2d_set_clear_color(RGBA8(r, g, b, a));
		#elif RENDERER == REND_SF2D
			sf2d_set_clear_color(RGBA8(r, g, b, a));
		#endif
	}

	void DrawRectangle(int x, int y, int w, int h, int r, int g, int b, int a){
		EASYFIXCOORDS(&x,&y);
		#if RENDERER == REND_VITA2D
			vita2d_draw_rectangle(x,y,w,h,RGBA8(r,g,b,a));
		#elif RENDERER == REND_SDL
			unsigned char oldr;
			unsigned char oldg;
			unsigned char oldb;
			unsigned char olda;
			SDL_GetRenderDrawColor(mainWindowRenderer,&oldr,&oldg,&oldb,&olda);
			SDL_SetRenderDrawColor(mainWindowRenderer,r,g,b,a);
			SDL_Rect tempRect;
			tempRect.x=x;
			tempRect.y=y;
			tempRect.w=w;
			tempRect.h=h;
			SDL_RenderFillRect(mainWindowRenderer,&tempRect);
			SDL_SetRenderDrawColor(mainWindowRenderer,oldr,oldg,oldb,olda);
		#elif RENDERER == REND_SF2D
			sf2d_draw_rectangle(x,y,w,h,RGBA8(r,g,b,a));
		#endif
	}

#endif