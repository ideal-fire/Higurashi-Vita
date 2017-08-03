#ifndef GENERALGOODCONFIGURATION
#define GENERALGOODCONFIGURATION

	#define PRE_UNDEFINED 0
	#define PRE_WINDOWS 1
	#define PRE_VITA 2
	#define PRE_ANDROID 3


	#define PLAT_UNDEFINED 0
	#define PLAT_WINDOWS 1
	#define PLAT_VITA 2
	#define PLAT_3DS 3
	
	#define REND_UNDEFINED
	#define REND_SDL 1
	#define REND_VITA2D 2
	#define REND_SF2D 3

	#define SND_NONE 0
	#define SND_SDL 1

	#define SUB_NONE 0
	#define SUB_ANDROID 1
	
	#define TEXT_NONE 0
	#define TEXT_VITA2D 1
	#define TEXT_FONTCACHE 2
	#define TEXT_DEBUG 3

	// CHANGE THIS IF YOU'RE COMPILING FOR A DIFFERENT PLATFORM
	#define PRESET PRE_VITA

	// Constants that change the code so it runs on a specific platform
	#if PRESET == PRE_VITA
		#define RENDERER REND_VITA2D
		#define PLATFORM PLAT_VITA
		#define SUBPLATFORM SUB_NONE
		#define SOUNDPLAYER SND_SDL
		#define TEXTRENDERER TEXT_VITA2D
		#define SYSTEMSTRING "Vita"
		#define SELECTBUTTONNAME "X"
		#define BACKBUTTONNAME "O"
	#elif PRESET == PRE_WINDOWS
		#define RENDERER REND_SDL
		#define PLATFORM PLAT_WINDOWS
		#define SUBPLATFORM SUB_NONE
		#define SOUNDPLAYER SND_SDL
		#define TEXTRENDERER TEXT_FONTCACHE
		#define SYSTEMSTRING "Windows"
		#define SELECTBUTTONNAME "Z"
		#define BACKBUTTONNAME "X"
	#elif PRESET == PRE_ANDROID
		#define RENDERER REND_SDL
		#define PLATFORM PLAT_WINDOWS
		#define SUBPLATFORM SUB_ANDROID
		#define SOUNDPLAYER SND_SDL
		#define TEXTRENDERER TEXT_FONTCACHE
		#define SYSTEMSTRING "Android"
		#define SELECTBUTTONNAME "GREEN" // I have no idea what to call these.
		#define BACKBUTTONNAME "RED" // I have no idea what to call these.
	#else // These are the defaults for no preset. Change these if you're not choosing a preset.
		#define RENDERER REND_UNDEFINED // 0
		#define PLATFORM PLAT_UNDEFINED // 0
		#define SUBPLATFORM SUB_NONE // 0
		#define SOUNDPLAYER SND_NONE // 0
		#define TEXTRENDERER TEXT_NONE // 0
		#define SYSTEMSTRING "UNDEFINED PLATFORM"
		#define SELECTBUTTONNAME "SELECT" // Generic
		#define BACKBUTTONNAME "BACK" // Generic
	#endif

#endif