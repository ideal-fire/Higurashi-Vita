#ifndef GENERALGOODSTUFF
#define GENERALGOODSTUFF

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
			SceRtcTick temp;
			sceRtcGetCurrentTick(&temp);
			return temp.tick;
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

	/*
	================================================
	== SOUND
	=================================================
	*/
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
		Mix_PlayChannel( -1, toPlay, timesToPlay-1 );
	}
	void PlayMusic(CROSSMUSIC* toPlay){
		Mix_PlayMusic(toPlay,-1);
	}
	
	void FreeSound(CROSSSFX* toFree){
		Mix_FreeChunk(toFree);
	}
	void FreeMusic(CROSSMUSIC* toFree){
		Mix_FreeMusic(toFree);
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

	void DrawRectangle(int x, int y, int w, int h, int r, int g, int b, int a){
		#if RENDERER == REND_VITA2D
			vita2d_draw_rectangle(x,y,w,h,RGBA8(r,g,b,a));
		#elif RENDERER == REND_SDL
			SDL_SetRenderDrawColor(mainWindowRenderer,r,g,b,a);
			SDL_Rect tempRect;
			tempRect.x=x;
			tempRect.y=y;
			tempRect.w=w;
			tempRect.h=h;
			SDL_RenderFillRect(mainWindowRenderer,&tempRect);
		#elif RENDERER == REND_SF2D
			sf2d_draw_rectangle(x,y,w,h,RGBA8(r,g,b,a));
		#endif
	}
	
	void DrawTexture(CrossTexture* passedTexture, int _destX, int _destY){
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
	
	void DrawTexturePartScale(CrossTexture* passedTexture, int destX, int destY, int texX, int texY, int texW, int texH, float texXScale, float texYScale){
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
	
			_destRect.x=destX;
			_destRect.y=destY;
	
			SDL_RenderCopy(mainWindowRenderer, passedTexture, &_srcRect, &_destRect );
		#elif RENDERER==REND_SF2D
			sf2d_draw_texture_part_scale(passedTexture,destX,destY,texX,texY,texW, texH, texXScale, texYScale);
		#endif
	}
	
	void DrawTextureScale(CrossTexture* passedTexture, int destX, int destY, float texXScale, float texYScale){
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
	
	// TODO MAKE ROTATE ON WINDOWS
	void DrawTexturePartScaleRotate(CrossTexture* texture, float x, float y, float tex_x, float tex_y, float tex_w, float tex_h, float x_scale, float y_scale, float rad){
		#if RENDERER == REND_VITA2D
			vita2d_draw_texture_part_scale_rotate(texture,x,y,tex_x,tex_y,tex_w,tex_h,x_scale,y_scale,rad);
		#elif RENDERER == REND_SDL
			DrawTexturePartScale(texture,x,y,tex_x,tex_y,tex_w,tex_h,x_scale,y_scale);
		#elif RENDERER == REND_SF2D
			sf2d_draw_texture_part_rotate_scale(texture,x,y,rad,tex_x,tex_y,tex_w,tex_h,x_scale,y_scale);
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

#endif