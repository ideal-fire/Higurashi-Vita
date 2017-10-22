#ifndef GENERALGOODIMAGESHEADER
#define GENERALGOODIMAGESHEADER
	// Renderer stuff
	#if RENDERER == REND_SDL
		#define CrossTexture SDL_Texture
		#include <SDL2/SDL.h>
		#include <SDL2/SDL_image.h>	
	#endif
	#if RENDERER == REND_VITA2D
		#include <vita2d.h>
	#endif
	
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
			showErrorIfNull(_tempSurface);
			// Make good
			_returnTexture = SDL_CreateTextureFromSurface( mainWindowRenderer, _tempSurface );
			showErrorIfNull(_returnTexture);
			// Free memori
			SDL_FreeSurface(_tempSurface);
			return _returnTexture;
		#elif RENDERER==REND_SF2D
			return sfil_load_PNG_file(path,SF2D_PLACE_RAM);
		#endif
	}
	CrossTexture* LoadJPG(char* path){
		#if RENDERER==REND_VITA2D
			return vita2d_load_JPEG_file(path);
		#elif RENDERER==REND_SDL
			return LoadPNG(path);
		#elif RENDERER==REND_SF2D
			return sfil_load_JPEG_file(path,SF2D_PLACE_RAM);
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

#endif