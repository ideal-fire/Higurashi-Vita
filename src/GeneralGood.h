#ifndef GENERALGOODSTUFF
#define GENERALGOODSTUFF

	#ifndef ISUSINGEXTENDED
		#define ISUSINGEXTENDED 0
	#endif

	#if ISUSINGEXTENDED == 0
		int FixX(int x);
		int FixY(int y);
	#endif

	#include <stdio.h>
	#include <math.h>
	#include <string.h>
	#include <stdlib.h>
	#include <unistd.h>
	
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

	typedef uint8_t 	u8;
	typedef uint16_t 	u16;
	typedef uint32_t	u32;
	typedef uint64_t	u64;
	typedef int8_t		s8;
	typedef int16_t		s16;
	typedef int32_t		s32;
	typedef int64_t		s64;

	// Waits for a number of miliseconds.
	void Wait(int miliseconds){
		#if PLATFORM == PLAT_VITA
			sceKernelDelayThread(miliseconds*1000);
		#elif PLATFORM == PLAT_WINDOWS
			SDL_Delay(miliseconds);
		#elif PLATFORM == PLAT_3DS
			svcSleepThread(miliseconds*1000000);
		#endif
	}
	
	u64 GetTicks(){
		#if PLATFORM == PLAT_VITA
			//SceRtcTick temp;
			//sceRtcGetCurrentTick(&temp);
			//return temp.tick;
			// Convert to milliseconds?
			return  (sceKernelGetProcessTimeWide() / 1000);
		#elif PLATFORM == PLAT_WINDOWS
			return SDL_GetTicks();
		#elif PLATFORM == PLAT_3DS
			return osGetTime();
		#endif
	}

	char ShowErrorIfNull(void* _thingie){
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

	signed char CheckFileExist(const char* location){
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

	void MakeDirectory(const char* path){
		#if PLATFORM == PLAT_VITA
			sceIoMkdir(path,0777);
		#elif PLATFORM == PLAT_WINDOWS
			#if SUBPLATFORM == SUB_ANDROID
				mkdir(path,0777);
			#else
				mkdir(path);
			#endif
		#endif
	}
	#ifndef ISUSINGEXTENDED
		void QuitApplication(){
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
	== SOUND
	=================================================
	*/

	void InitAudio(){
		#if SOUNDPLAYER == SND_SDL
			SDL_Init( SDL_INIT_AUDIO );
			Mix_Init(MIX_INIT_OGG);
			Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 );
		#endif
	}

	int GetMusicVolume(){
		#if SOUNDPLAYER == SND_SDL
			return Mix_VolumeMusic(-1);
		#endif
	}

	void SetSFXVolume(CROSSSFX* tochange, int toval){
		#if SOUNDPLAYER == SND_SDL
			Mix_VolumeChunk(tochange,toval);
		#endif
	}

	void FadeoutMusic(int time){
		#if SOUNDPLAYER == SND_SDL
			Mix_FadeOutMusic(time);
		#endif
	}

	void SetMusicVolume(int vol){
		#if SOUNDPLAYER == SND_SDL
			Mix_VolumeMusic(vol);
		#endif
	}

	CROSSSFX* LoadSound(char* filepath){
		#if SOUNDPLAYER == SND_SDL
			return Mix_LoadWAV(filepath);
			//FILE *fp;
			//fp = fopen("ux0:data/HIGURASHI/a.txt", "w+");
			//fprintf(fp,"%s\n",SDL_GetError());
			//fclose(fp);
		#endif
	}
	
	CROSSMUSIC* LoadMusic(char* filepath){
		#if SOUNDPLAYER == SND_SDL
			return Mix_LoadMUS(filepath);
		#endif
	}
	
	void PauseMusic(){
		#if SOUNDPLAYER == SND_SDL
			Mix_PauseMusic();
		#endif
	}
	void ResumeMusic(){
		#if SOUNDPLAYER == SND_SDL
			Mix_ResumeMusic();
		#endif
	}
	void StopMusic(){
		#if SOUNDPLAYER == SND_SDL
			Mix_HaltMusic();
		#endif
	}
	
	void PlaySound(CROSSSFX* toPlay, int timesToPlay){
		#if SOUNDPLAYER == SND_SDL
			Mix_PlayChannel( -1, toPlay, timesToPlay-1 );
		#endif
	}
	void PlayMusic(CROSSMUSIC* toPlay){
		#if SOUNDPLAYER == SND_SDL
			Mix_PlayMusic(toPlay,-1);
		#endif
	}
	
	void FreeSound(CROSSSFX* toFree){
		#if SOUNDPLAYER == SND_SDL
			Mix_FreeChunk(toFree);
		#endif
	}
	void FreeMusic(CROSSMUSIC* toFree){
		#if SOUNDPLAYER == SND_SDL
			Mix_FreeMusic(toFree);
		#endif
	}

	/*
	=================================================
	== IMAGES
	=================================================
	*/

	CrossTexture* LoadPNG(char* path){
		#if RENDERER==REND_VITA2D
			return vita2d_load_PNG_file(path);
		#elif RENDERER==REND_SDL
			// Real one we'll return
			SDL_Texture* _returnTexture;
			// Load temp and sho error
			SDL_Surface* _tempSurface = IMG_Load(path);
			ShowErrorIfNull(_tempSurface);
			// Make good
			_returnTexture = SDL_CreateTextureFromSurface( mainWindowRenderer, _tempSurface );
			ShowErrorIfNull(_returnTexture);
			// Free memori
			SDL_FreeSurface(_tempSurface);
			return _returnTexture;
		#elif RENDERER==REND_SF2D
			return sfil_load_PNG_file(path,SF2D_PLACE_RAM);
		#endif
	}

	void FreeTexture(CrossTexture* passedTexture){
		#if RENDERER == REND_VITA2D
			vita2d_wait_rendering_done();
			sceDisplayWaitVblankStart();
			vita2d_free_texture(passedTexture);
			passedTexture=NULL;
		#elif RENDERER == REND_SDL
			SDL_DestroyTexture(passedTexture);
			passedTexture=NULL;
		#elif RENDERER == REND_SF2D
			sf2d_free_texture(passedTexture);
			passedTexture=NULL;
		#endif
	}
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

	int GetTextureWidth(CrossTexture* passedTexture){
		#if RENDERER == REND_VITA2D
			return vita2d_texture_get_width(passedTexture);
		#elif RENDERER == REND_SDL
			int w, h;
			SDL_QueryTexture(passedTexture, NULL, NULL, &w, &h);
			return w;
		#elif RENDERER == REND_SF2D
			return passedTexture->width;
		#endif
	}
	
	int GetTextureHeight(CrossTexture* passedTexture){
		#if RENDERER == REND_VITA2D
			return vita2d_texture_get_height(passedTexture);
		#elif RENDERER == REND_SDL
			int w, h;
			SDL_QueryTexture(passedTexture, NULL, NULL, &w, &h);
			return h;
		#elif RENDERER == REND_SF2D
			return passedTexture->height;
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
	
	void DrawTexture(CrossTexture* passedTexture, int _destX, int _destY){
		EASYFIXCOORDS(&_destX,&_destY);
		#if RENDERER == REND_VITA2D
			vita2d_draw_texture(passedTexture,_destX,_destY);
		#elif RENDERER == REND_SDL
			SDL_Rect _srcRect;
			SDL_Rect _destRect;
	
			SDL_QueryTexture(passedTexture, NULL, NULL, &(_srcRect.w), &(_srcRect.h));
	
			_destRect.w=_srcRect.w;
			_destRect.h=_srcRect.h;
	
			_destRect.x=_destX;
			_destRect.y=_destY;
			
			_srcRect.x=0;
			_srcRect.y=0;
		
			SDL_RenderCopy(mainWindowRenderer, passedTexture, &_srcRect, &_destRect );
		#elif RENDERER == REND_SF2D
			sf2d_draw_texture(passedTexture,_destX,_destY);
		#endif
	}
	
	////////////////// ALPHA
		void DrawTextureAlpha(CrossTexture* passedTexture, int _destX, int _destY, unsigned char alpha){
			EASYFIXCOORDS(&_destX,&_destY);
			#if RENDERER == REND_VITA2D
				vita2d_draw_texture_tint(passedTexture,_destX,_destY,RGBA8(255,255,255,alpha));
			#elif RENDERER == REND_SDL
				unsigned char oldAlpha;
				SDL_GetTextureAlphaMod(passedTexture, &oldAlpha);
				SDL_SetTextureAlphaMod(passedTexture, alpha);
				SDL_Rect _srcRect;
				SDL_Rect _destRect;
		
				SDL_QueryTexture(passedTexture, NULL, NULL, &(_srcRect.w), &(_srcRect.h));
		
				_destRect.w=_srcRect.w;
				_destRect.h=_srcRect.h;
		
				_destRect.x=_destX;
				_destRect.y=_destY;
				
				_srcRect.x=0;
				_srcRect.y=0;
			
				SDL_RenderCopy(mainWindowRenderer, passedTexture, &_srcRect, &_destRect );
				SDL_SetTextureAlphaMod(passedTexture, oldAlpha);
			#elif RENDERER == REND_SF2D
				sf2d_draw_texture(passedTexture,_destX,_destY);
			#endif
		}
		void DrawTextureScaleAlpha(CrossTexture* passedTexture, int destX, int destY, float texXScale, float texYScale, unsigned char alpha){
			EASYFIXCOORDS(&destX,&destY);
			#if RENDERER == REND_VITA2D
				vita2d_draw_texture_tint_scale(passedTexture,destX,destY,texXScale,texYScale,RGBA8(255,255,255,alpha));
			#elif RENDERER == REND_SDL
				unsigned char oldAlpha;
				SDL_GetTextureAlphaMod(passedTexture, &oldAlpha);
				SDL_SetTextureAlphaMod(passedTexture, alpha);
				SDL_Rect _srcRect;
				SDL_Rect _destRect;
				SDL_QueryTexture(passedTexture, NULL, NULL, &(_srcRect.w), &(_srcRect.h));
				
				_srcRect.x=0;
				_srcRect.y=0;
			
				_destRect.w=(_srcRect.w*texXScale);
				_destRect.h=(_srcRect.h*texYScale);
		
				_destRect.x=destX;
				_destRect.y=destY;
		
				SDL_RenderCopy(mainWindowRenderer, passedTexture, &_srcRect, &_destRect );
				SDL_SetTextureAlphaMod(passedTexture, oldAlpha);
			#elif RENDERER == REND_SF2D
				sf2d_draw_texture_scale(passedTexture,destX,destY,texXScale,texYScale);
			#endif
		}

	void DrawTexturePartScale(CrossTexture* passedTexture, int destX, int destY, int texX, int texY, int texW, int texH, float texXScale, float texYScale){
		EASYFIXCOORDS(&destX,&destY);
		#if RENDERER == REND_VITA2D
			vita2d_draw_texture_part_scale(passedTexture,destX,destY,texX,texY,texW, texH, texXScale, texYScale);
		#elif RENDERER == REND_SDL
			SDL_Rect _srcRect;
			SDL_Rect _destRect;
			_srcRect.w=texW;
			_srcRect.h=texH;
			
			_srcRect.x=texX;
			_srcRect.y=texY;
		
			_destRect.w=_srcRect.w*texXScale;
			_destRect.h=_srcRect.h*texYScale;
			//printf("Dest dimensionds is %dx%d;%.6f;%.6f\n",_destRect.w,_destRect.h,texXScale,texYScale);
			_destRect.x=destX;
			_destRect.y=destY;
	
			SDL_RenderCopy(mainWindowRenderer, passedTexture, &_srcRect, &_destRect );
		#elif RENDERER==REND_SF2D
			sf2d_draw_texture_part_scale(passedTexture,destX,destY,texX,texY,texW, texH, texXScale, texYScale);
		#endif
	}

	void DrawTextureScaleTint(CrossTexture* passedTexture, int destX, int destY, float texXScale, float texYScale, unsigned char r, unsigned char g, unsigned char b){
		EASYFIXCOORDS(&destX,&destY);
		#if RENDERER == REND_VITA2D
			vita2d_draw_texture_tint_scale(passedTexture,destX,destY,texXScale,texYScale,RGBA8(r,g,b,255));
		#elif RENDERER == REND_SDL
			unsigned char oldr;
			unsigned char oldg;
			unsigned char oldb;
			SDL_GetTextureColorMod(passedTexture,&oldr,&oldg,&oldb);
			SDL_SetTextureColorMod(passedTexture, r,g,b);
			SDL_Rect _srcRect;
			SDL_Rect _destRect;
			SDL_QueryTexture(passedTexture, NULL, NULL, &(_srcRect.w), &(_srcRect.h));
			
			_srcRect.x=0;
			_srcRect.y=0;
		
			_destRect.w=(_srcRect.w*texXScale);
			_destRect.h=(_srcRect.h*texYScale);
	
			_destRect.x=destX;
			_destRect.y=destY;
	
			SDL_RenderCopy(mainWindowRenderer, passedTexture, &_srcRect, &_destRect );
			SDL_SetTextureColorMod(passedTexture, oldr, oldg, oldb);
		#elif RENDERER == REND_SF2D
			sf2d_draw_texture_tint_scale(passedTexture,destX,destY,texXScale,texYScale);
		#endif
	}

	void DrawTexturePartScaleTint(CrossTexture* passedTexture, int destX, int destY, int texX, int texY, int texW, int texH, float texXScale, float texYScale, unsigned char r, unsigned char g, unsigned b){
		EASYFIXCOORDS(&destX,&destY);
		#if RENDERER == REND_VITA2D
			vita2d_draw_texture_tint_part_scale(passedTexture,destX,destY,texX,texY,texW, texH, texXScale, texYScale,RGBA8(r,g,b,255));
		#elif RENDERER == REND_SDL
			unsigned char oldr;
			unsigned char oldg;
			unsigned char oldb;
			SDL_GetTextureColorMod(passedTexture,&oldr,&oldg,&oldb);
			SDL_SetTextureColorMod(passedTexture, r,g,b);
			SDL_Rect _srcRect;
			SDL_Rect _destRect;
			_srcRect.w=texW;
			_srcRect.h=texH;
			
			_srcRect.x=texX;
			_srcRect.y=texY;
		
			_destRect.w=_srcRect.w*texXScale;
			_destRect.h=_srcRect.h*texYScale;
	
			_destRect.x=destX;
			_destRect.y=destY;
	
			SDL_RenderCopy(mainWindowRenderer, passedTexture, &_srcRect, &_destRect );
			SDL_SetTextureColorMod(passedTexture, oldr, oldg, oldb);
		#elif RENDERER==REND_SF2D
			sf2d_draw_texture_part_scale(passedTexture,destX,destY,texX,texY,texW, texH, texXScale, texYScale);
		#endif
	}
	
	void DrawTextureScale(CrossTexture* passedTexture, int destX, int destY, float texXScale, float texYScale){
		EASYFIXCOORDS(&destX,&destY);
		#if RENDERER == REND_VITA2D
			vita2d_draw_texture_scale(passedTexture,destX,destY,texXScale,texYScale);
		#elif RENDERER == REND_SDL
			SDL_Rect _srcRect;
			SDL_Rect _destRect;
			SDL_QueryTexture(passedTexture, NULL, NULL, &(_srcRect.w), &(_srcRect.h));
			
			_srcRect.x=0;
			_srcRect.y=0;
		
			_destRect.w=(_srcRect.w*texXScale);
			_destRect.h=(_srcRect.h*texYScale);
	
			_destRect.x=destX;
			_destRect.y=destY;
	
			SDL_RenderCopy(mainWindowRenderer, passedTexture, &_srcRect, &_destRect );
		#elif RENDERER == REND_SF2D
			sf2d_draw_texture_scale(passedTexture,destX,destY,texXScale,texYScale);
		#endif
	}

	void DrawTextureScaleSize(CrossTexture* passedTexture, int destX, int destY, float texXScale, float texYScale){
		EASYFIXCOORDS(&destX,&destY);
		#if RENDERER == REND_VITA2D
			vita2d_draw_texture_scale(passedTexture,destX,destY,texXScale/(double)GetTextureWidth(passedTexture),texYScale/(double)GetTextureHeight(passedTexture));
		#elif RENDERER == REND_SDL
			SDL_Rect _srcRect;
			SDL_Rect _destRect;
			SDL_QueryTexture(passedTexture, NULL, NULL, &(_srcRect.w), &(_srcRect.h));
			
			_srcRect.x=0;
			_srcRect.y=0;
		
			_destRect.w=(texXScale);
			_destRect.h=(texYScale);
	
			_destRect.x=destX;
			_destRect.y=destY;
	
			SDL_RenderCopy(mainWindowRenderer, passedTexture, &_srcRect, &_destRect );
		#elif RENDERER == REND_SF2D
			sf2d_draw_texture_scale(passedTexture,destX,destY,texXScale,texYScale);
		#endif
	}
	
	// TODO MAKE ROTATE ON WINDOWS
	void DrawTexturePartScaleRotate(CrossTexture* texture, int x, int y, float tex_x, float tex_y, float tex_w, float tex_h, float x_scale, float y_scale, float rad){
		EASYFIXCOORDS(&x,&y);
		#if RENDERER == REND_VITA2D
			vita2d_draw_texture_part_scale_rotate(texture,x,y,tex_x,tex_y,tex_w,tex_h,x_scale,y_scale,rad);
		#elif RENDERER == REND_SDL
			DrawTexturePartScale(texture,x,y,tex_x,tex_y,tex_w,tex_h,x_scale,y_scale);
		#elif RENDERER == REND_SF2D
			sf2d_draw_texture_part_rotate_scale(texture,x,y,rad,tex_x,tex_y,tex_w,tex_h,x_scale,y_scale);
		#endif
	}

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

	char DirOpenWorked(CROSSDIR passedir){
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

	CROSSDIR OpenDirectory(const char* filepath){
		#if PLATFORM == PLAT_WINDOWS
			return opendir(filepath);
		#elif PLATFORM == PLAT_VITA
			return (sceIoDopen(filepath));
		#endif
	}

	char* GetDirectoryResultName(CROSSDIRSTORAGE* passedStorage){
		#if PLATFORM == PLAT_WINDOWS
			return ((*passedStorage)->d_name);
		#elif PLATFORM == PLAT_VITA
			//WriteToDebugFile
			return ((passedStorage)->d_name);
		#endif
	}

	int DirectoryRead(CROSSDIR* passedir, CROSSDIRSTORAGE* passedStorage){
		#if PLATFORM == PLAT_WINDOWS
			*passedStorage = readdir (*passedir);
			if (*passedStorage != NULL){
				if (strcmp((*passedStorage)->d_name,".")==0 || strcmp((*passedStorage)->d_name,"..")==0){
					return DirectoryRead(passedir,passedStorage);
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

	void DirectoryClose(CROSSDIR passedir){
		#if PLATFORM == PLAT_WINDOWS
			closedir(passedir);
		#elif PLATFORM == PLAT_VITA
			sceIoDclose(passedir);
		#endif
	}

	char DirectoryExists(const char* filepath){
		CROSSDIR _tempdir = OpenDirectory(filepath);
		if (DirOpenWorked(_tempdir)==1){
			DirectoryClose(_tempdir);
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

	// Returns number of elements read
	size_t Crossfread(void* buffer, size_t size, size_t count, CROSSFILE* stream){
		#if RENDERER == REND_SDL
			return SDL_RWread(stream,buffer,size,count);
		#else
			return fread(buffer,size,count,stream);
		#endif
	}

	CROSSFILE* Crossfopen(const char* filename, const char* mode){
		#if RENDERER == REND_SDL
			return SDL_RWFromFile(filename,mode);
		#else
			return fopen(filename,mode);
		#endif
	}

	// Returns 0 on success.
	// Returns negative number of failure
	int Crossfclose(CROSSFILE* stream){
		#if RENDERER == REND_SDL
			return SDL_RWclose(stream);
		#else
			return fclose(stream);
		#endif
	}

	// stream, offset, CROSSFILE_START, CROSSFILE_END, CROSSFILE_CUR
	// For SDL, returns new position
	// Otherwise, returns 0 when it works
	int Crossfseek(CROSSFILE* stream, long int offset, int origin){
		#if RENDERER == REND_SDL
			return SDL_RWseek(stream,offset,origin);
		#else
			return fseek(stream,offset,origin);
		#endif
	}

	long int Crossftell(CROSSFILE* fp){
		#if RENDERER == REND_SDL
			return Crossfseek(fp,0,CROSSFILE_CUR);
		#else
			return ftell(fp);
		#endif
	}

	// No platform specific code here
	int Crossgetc(CROSSFILE* fp){
		char _readChar;
		if (Crossfread(&_readChar,1,1,fp)==0){
			return EOF;
		}
		return _readChar;
	}

	// No platform specific code here
	char Crossfeof(CROSSFILE* fp){
		if (Crossgetc(fp)==EOF){
			return 1;
		}
		Crossfseek(fp,-1,CROSSFILE_CUR);
		return 0;
	}

	// Checks if the byte is the one for a newline
	// If it's 0D, it seeks past the 0A that it assumes is next and returns 1
	// TODO - Add support for \r only new line. It's pre OSX new line
	signed char IsNewLine(CROSSFILE* fp, unsigned char _temp){
		if (_temp==0x0D){
			//fseek(fp,1,SEEK_CUR);
			Crossfseek(fp,1,CROSSFILE_CUR);
			return 1;
		}
		if (_temp=='\n' || _temp==0x0A){
			// It seems like the other newline char is skipped for me?
			return 1;
		}
		return 0;
	}

#endif