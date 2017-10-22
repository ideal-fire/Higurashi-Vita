#ifndef GENERALGOODSTUFF
#define GENERALGOODSTUFF

	#ifndef ISUSINGEXTENDED
		#define ISUSINGEXTENDED 0
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
	
	#if PLATFORM == PLAT_WINDOWS
		// Header for directory functions
		#include <dirent.h>
	#endif
	// For stuff like uint8_t
	#include <stdint.h>
	
	// Headers for wait function
	#if RENDERER == REND_SDL
		#include <SDL2/SDL.h>
	#elif PLATFORM == PLAT_WINDOWS
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

	// Waits for a number of miliseconds.
	void wait(int miliseconds){
		#if PLATFORM == PLAT_VITA
			sceKernelDelayThread(miliseconds*1000);
		#elif RENDERER == REND_SDL
			SDL_Delay(miliseconds);
		#elif PLATFORM == PLAT_3DS
			svcSleepThread(miliseconds*1000000);
		#elif PLATFORM == PLAT_WINDOWS
			Sleep(miliseconds);
		#endif
	}
	
	u64 getTicks(){
		#if PLATFORM == PLAT_VITA
			// Convert to milliseconds?
			return  (sceKernelGetProcessTimeWide() / 1000);
		#elif RENDERER == REND_SDL
			return SDL_GetTicks();
		#elif PLATFORM == PLAT_3DS
			return osGetTime();
		#elif PLATFORM == PLAT_WINDOWS
			LARGE_INTEGER s_frequency;
			char s_use_qpc = QueryPerformanceFrequency(&s_frequency);
			if (s_use_qpc) {
			    LARGE_INTEGER now;
			    QueryPerformanceCounter(&now);
			    return (1000LL * now.QuadPart) / s_frequency.QuadPart;
			} else {
			    return GetTickCount();
			}
		#endif
	}

	char showErrorIfNull(void* _thingie){
		#if RENDERER == REND_SDL
			if (_thingie==NULL){
				printf("Error: %s\n",SDL_GetError());
				return 1;
			}
			return 0;
		#elif PLATFORM == PLAT_VITA || PLATFORM == PLAT_3DS
			if (_thingie==NULL){
				printf("Some wacky thingie is null");
				return 1;
			}
			return 0;
		#endif
	}

	signed char checkFileExist(const char* location){
		#if PLATFORM == PLAT_VITA
			SceUID fileHandle = sceIoOpen(location, SCE_O_RDONLY, 0777);
			if (fileHandle < 0){
				return 0;
			}else{
				sceIoClose(fileHandle);
				return 1;
			}
		#elif PLATFORM == PLAT_WINDOWS
			if( access( location, F_OK ) != -1 ) {
				return 1;
			} else {
				return 0;
			}
		#endif
	}

	void createDirectory(const char* path){
		#if PLATFORM == PLAT_VITA
			sceIoMkdir(path,0777);
		#elif PLATFORM == PLAT_WINDOWS
			#if SUBPLATFORM == (SUB_ANDROID || SUBPLATFORM == SUB_UNIX)
				mkdir(path,0777);
			#else
				mkdir(path);
			#endif
		#endif
	}
	#ifndef ISUSINGEXTENDED
		void quitApplication(){
			#if RENDERER == REND_SDL
				SDL_Quit();
			#elif PLATFORM == PLAT_VITA
				sceKernelExitProcess(0);
			#else
				printf("No quit function avalible.");
			#endif
		}
	#endif

	/*
	================================================
	== ETC
	=================================================
	*/
	// PLEASE MAKE DIR PATHS END IN A SLASH
	#if PLATFORM == PLAT_WINDOWS
		#define CROSSDIR DIR*
		#define CROSSDIRSTORAGE struct dirent*
	#elif PLATFORM == PLAT_VITA
		#define CROSSDIR SceUID
		#define CROSSDIRSTORAGE SceIoDirent
	#endif

	char dirOpenWorked(CROSSDIR passedir){
		#if PLATFORM == PLAT_WINDOWS
			if (passedir==NULL){
				return 0;
			}
		#elif PLATFORM == PLAT_VITA
			if (passedir<0){
				return 0;
			}
		#endif
		return 1;
	}

	CROSSDIR openDirectory(const char* filepath){
		#if PLATFORM == PLAT_WINDOWS
			return opendir(filepath);
		#elif PLATFORM == PLAT_VITA
			return (sceIoDopen(filepath));
		#endif
	}

	char* getDirectoryResultName(CROSSDIRSTORAGE* passedStorage){
		#if PLATFORM == PLAT_WINDOWS
			return ((*passedStorage)->d_name);
		#elif PLATFORM == PLAT_VITA
			//WriteToDebugFile
			return ((passedStorage)->d_name);
		#endif
	}

	int directoryRead(CROSSDIR* passedir, CROSSDIRSTORAGE* passedStorage){
		#if PLATFORM == PLAT_WINDOWS
			*passedStorage = readdir (*passedir);
			if (*passedStorage != NULL){
				if (strcmp((*passedStorage)->d_name,".")==0 || strcmp((*passedStorage)->d_name,"..")==0){
					return directoryRead(passedir,passedStorage);
				}
			}
			if (*passedStorage == NULL){
				return 0;
			}else{
				return 1;
			}
		#elif PLATFORM == PLAT_VITA
			int _a = sceIoDread(*passedir,passedStorage);
			return _a;
			
		#endif
	}

	void directoryClose(CROSSDIR passedir){
		#if PLATFORM == PLAT_WINDOWS
			closedir(passedir);
		#elif PLATFORM == PLAT_VITA
			sceIoDclose(passedir);
		#endif
	}

	char directoryExists(const char* filepath){
		CROSSDIR _tempdir = openDirectory(filepath);
		if (dirOpenWorked(_tempdir)==1){
			directoryClose(_tempdir);
			return 1;
		}else{
			return 0;
		}
	}
	/*
	==========================================================
	== CROSS PLATFORM FILE WRITING AND READING
	==========================================================
	*/

	#if RENDERER == REND_SDL
		#define CROSSFILE SDL_RWops
		#define CROSSFILE_START RW_SEEK_SET
		#define CROSSFILE_CUR RW_SEEK_CUR
		#define CROSSFILE_END RW_SEEK_END
	#else
		#define CROSSFILE FILE
		#define CROSSFILE_START SEEK_SET
		#define CROSSFILE_CUR SEEK_CUR
		#define CROSSFILE_END SEEK_END
	#endif

	// Removes all 0x0D and 0x0A from last two characters of string by moving null character.
	void removeNewline(char* _toRemove){
		int _cachedStrlen = strlen(_toRemove);
		int i;
		for (i=0;i!=2;i++){
			if (!(((_toRemove)[_cachedStrlen-(i+1)]==0x0A) || ((_toRemove)[_cachedStrlen-(i+1)]==0x0D))){
				break;
			}
		}
		(_toRemove)[_cachedStrlen-i] = '\0';
	}

	// Returns number of elements read
	size_t crossfread(void* buffer, size_t size, size_t count, CROSSFILE* stream){
		#if RENDERER == REND_SDL
			return SDL_RWread(stream,buffer,size,count);
		#else
			return fread(buffer,size,count,stream);
		#endif
	}

	CROSSFILE* crossfopen(const char* filename, const char* mode){
		#if RENDERER == REND_SDL
			return SDL_RWFromFile(filename,mode);
		#else
			return fopen(filename,mode);
		#endif
	}

	// Returns 0 on success.
	// Returns negative number of failure
	int crossfclose(CROSSFILE* stream){
		#if RENDERER == REND_SDL
			return SDL_RWclose(stream);
		#else
			return fclose(stream);
		#endif
	}

	// stream, offset, CROSSFILE_START, CROSSFILE_END, CROSSFILE_CUR
	// For SDL, returns new position
	// Otherwise, returns 0 when it works
	int crossfseek(CROSSFILE* stream, long int offset, int origin){
		#if RENDERER == REND_SDL
			return SDL_RWseek(stream,offset,origin);
		#else
			return fseek(stream,offset,origin);
		#endif
	}

	long int crossftell(CROSSFILE* fp){
		#if RENDERER == REND_SDL
			return crossfseek(fp,0,CROSSFILE_CUR);
		#else
			return ftell(fp);
		#endif
	}

	// No platform specific code here
	int crossgetc(CROSSFILE* fp){
		char _readChar;
		if (crossfread(&_readChar,1,1,fp)==0){
			return EOF;
		}
		return _readChar;
	}

	// No platform specific code here
	char crossfeof(CROSSFILE* fp){
		if (crossgetc(fp)==EOF){
			return 1;
		}
		crossfseek(fp,-1,CROSSFILE_CUR);
		return 0;
	}

	// Checks if the byte is the one for a newline
	// If it's 0D, it seeks past the 0A that it assumes is next and returns 1
	// TODO - Add support for \r only new line. It's pre OSX new line
	signed char isNewLine(CROSSFILE* fp, unsigned char _temp){
		if (_temp==0x0D){
			//fseek(fp,1,SEEK_CUR);
			crossfseek(fp,1,CROSSFILE_CUR);
			return 1;
		}
		if (_temp=='\n' || _temp==0x0A){
			// It seems like the other newline char is skipped for me?
			return 1;
		}
		return 0;
	}

#endif