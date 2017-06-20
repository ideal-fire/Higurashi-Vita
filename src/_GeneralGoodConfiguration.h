#ifndef GENERALGOODCONFIGURATION
#define GENERALGOODCONFIGURATION

	#define PLAT_WINDOWS 1
	#define PLAT_VITA 2
	#define PLAT_3DS 3
	
	#define REND_SDL 0
	#define REND_VITA2D 1
	#define REND_SF2D 2

	#define SND_NONE 0
	#define SND_SDL 1

	// Change these depending on the target platform
	#define RENDERER REND_VITA2D
	#define PLATFORM PLAT_VITA
	#define SOUNDPLAYER SND_SDL

#endif