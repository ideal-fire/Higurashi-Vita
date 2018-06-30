#ifndef GENERALGOOD_H
#define GENERALGOOD_H

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
	#include <3ds/svc.h>
	#include <3ds/types.h>
	#include <3ds/services/fs.h>
#endif

#if PLATFORM == PLAT_COMPUTER
	#define CROSSDIR DIR*
	#define CROSSDIRSTORAGE struct dirent*
#elif PLATFORM == PLAT_VITA
	#define CROSSDIR SceUID
	#define CROSSDIRSTORAGE SceIoDirent
#elif PLATFORM == PLAT_3DS
	#define CROSSDIR Handle
	#define CROSSDIRSTORAGE FS_DirectoryEntry
#else
	#warning NO DIRECTORY LISTING YET
	#define CROSSDIR int
	#define CROSSDIRSTORAGE int
#endif

#if PLATFORM == PLAT_VITA
	typedef struct{
		char* filename; // Malloc
		int internalPosition;
		FILE* fp;
	}vitaFile;
	#define CROSSFILE vitaFile
#elif RENDERER == REND_SDL
	#define CROSSFILE SDL_RWops
	#define CROSSFILE_START RW_SEEK_SET
	#define CROSSFILE_CUR RW_SEEK_CUR
	#define CROSSFILE_END RW_SEEK_END
#else
	#define CROSSFILE FILE
#endif
// Defaults
#ifndef CROSSFILE_START
	#define CROSSFILE_START SEEK_SET
	#define CROSSFILE_CUR SEEK_CUR
	#define CROSSFILE_END SEEK_END
#endif


#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#if SUBPLATFORM == SUB_UNIX
	#include <sys/types.h>
	#include <sys/stat.h>
#endif

#if PLATFORM == PLAT_COMPUTER
	// Header for directory functions
	#include <dirent.h>
#endif
// For stuff like uint8_t
#include <stdint.h>

// Headers for wait function
#if RENDERER == REND_SDL
	#include <SDL2/SDL.h>
#elif PLATFORM == PLAT_COMPUTER
	#include <windows.h>
#endif

// Subplatform Stuff
#if SUBPLATFORM == SUB_ANDROID
	// For mkdir
	#include <sys/stat.h>
	// So we can see console output with adb logcat
	#define printf SDL_Log
#endif

typedef uint8_t 	u8;
typedef uint16_t 	u16;
typedef uint32_t	u32;
typedef uint64_t	u64;
typedef int8_t		s8;
typedef int16_t		s16;
typedef int32_t		s32;
typedef int64_t		s64;

void generalGoodQuit();
void generalGoodInit();
signed char checkFileExist(const char* location);
void createDirectory(const char* path);
int crossfclose(CROSSFILE* stream);
char crossfeof(CROSSFILE* fp);
CROSSFILE* crossfopen(const char* filename, const char* mode);
size_t crossfread(void* buffer, size_t size, size_t count, CROSSFILE* stream);
int crossfseek(CROSSFILE* stream, long int offset, int origin);
long int crossftell(CROSSFILE* fp);
int crossgetc(CROSSFILE* fp);
void directoryClose(CROSSDIR passedir);
char directoryExists(const char* filepath);
int directoryRead(CROSSDIR* passedir, CROSSDIRSTORAGE* passedStorage);
char dirOpenWorked(CROSSDIR passedir);
char* getDirectoryResultName(CROSSDIRSTORAGE* passedStorage);
u64 getTicks();
signed char isNewLine(CROSSFILE* fp, unsigned char _temp);
CROSSDIR openDirectory(const char* filepath);
void quitApplication();
void removeNewline(char* _toRemove);
char showErrorIfNull(void* _thingie);
void wait(int miliseconds);
int crossungetc(int c, CROSSFILE* stream);
 
#endif /* GENERALGOOD_H */