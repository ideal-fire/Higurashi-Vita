#ifndef GENERALGOODTEXTHEADER
#define GENERALGOODTEXTHEADER
	// Text Stuff
	#if TEXTRENDERER == TEXT_FONTCACHE
		#include <SDL_FontCache/SDL_FontCache.h>
		#define CrossFont FC_Font
	#elif TEXTRENDERER == TEXT_DEBUG
		#define CrossFont CrossTexture
	#elif TEXTRENDERER == TEXT_VITA2D
		#define CrossFont vita2d_font
	#endif

	CrossFont* fontImage=NULL;
	#if TEXTRENDERER == TEXT_DEBUG
		float fontSize = 1.7;
	#endif
	#if TEXTRENDERER == TEXT_FONTCACHE
		//int fontSize = 20;
		int fontSize=50;
	#endif
	#if TEXTRENDERER == TEXT_VITA2D
		int fontSize=32;
	#endif

	void LoadFont(char* filename){
		#if TEXTRENDERER == TEXT_DEBUG
			#warning MAKE SURE THE FONT FILE EXISTS
			fontImage=LoadEmbeddedPNG("Stuff/Font.png");
		#elif TEXTRENDERER == TEXT_FONTCACHE
			//fontSize = (SCREENHEIGHT-TEXTBOXY)/3.5;
			FC_FreeFont(fontImage);
			fontImage = NULL;
			fontImage = FC_CreateFont();
			FC_LoadFont(fontImage, mainWindowRenderer, filename, fontSize, FC_MakeColor(0,0,0,255), TTF_STYLE_NORMAL);
		#elif TEXTRENDERER == TEXT_VITA2D
			fontImage = vita2d_load_font_file(filename);
		#endif
	}

	int TextHeight(float scale){
		#if TEXTRENDERER == TEXT_DEBUG
			return (8*scale);
		#elif TEXTRENDERER == TEXT_VITA2D
			return vita2d_font_text_height(fontImage,scale,"a");
		#elif TEXTRENDERER == TEXT_FONTCACHE
			return floor(FC_GetRealHeight(fontImage));
		#endif
	}

	// Please always use the same font size
	int TextWidth(float scale, const char* message){
		#if TEXTRENDERER == TEXT_DEBUG
			return floor((8*scale)*strlen(message)+strlen(message));
		#elif TEXTRENDERER == TEXT_VITA2D
			return vita2d_font_text_width(fontImage,scale,message);
		#elif TEXTRENDERER == TEXT_FONTCACHE
			return FC_GetWidth(fontImage,"%s",message);
		#endif
	}
	
	#if TEXTRENDERER == TEXT_DEBUG
		void DrawLetter(int letterId, int _x, int _y, float size){
			DrawTexturePartScale(fontImage,_x,_y,(letterId-32)*(8),0,8,8,size,size);
		}
		void DrawLetterColor(int letterId, int _x, int _y, float size, unsigned char r, unsigned char g, unsigned char b){
			DrawTexturePartScaleTint(fontImage,_x,_y,(letterId-32)*(8),0,8,8,size,size,r,g,b);
		}
	#endif
	void GoodDrawText(int x, int y, const char* text, float size){
		#if TEXTRENDERER == TEXT_VITA2D
			EASYFIXCOORDS(&x,&y);
			vita2d_font_draw_text(fontImage,x,y+TextHeight(size), RGBA8(255,255,255,255),floor(size),text);
		#elif TEXTRENDERER == TEXT_DEBUG
			int i=0;
			for (i = 0; i < strlen(text); i++){
				DrawLetter(text[i],(x+(i*(8*size))+i),(y),size);
			}
		#elif TEXTRENDERER == TEXT_FONTCACHE
			EASYFIXCOORDS(&x,&y);
			FC_Draw(fontImage, mainWindowRenderer, x, y, "%s", text);
		#endif
	}
	
	void GoodDrawTextColored(int x, int y, const char* text, float size, unsigned char r, unsigned char g, unsigned char b){
		#if TEXTRENDERER == TEXT_VITA2D
			EASYFIXCOORDS(&x,&y);
			vita2d_font_draw_text(fontImage,x,y+TextHeight(size), RGBA8(r,g,b,255),floor(size),text);
		#elif TEXTRENDERER == TEXT_DEBUG
			int i=0;
			int notICounter=0;
			for (i = 0; i < strlen(text); i++){
				DrawLetterColor(text[i],(x+(notICounter*(8*size))+notICounter),(y),size,r,g,b);
				notICounter++;
			}
		#elif TEXTRENDERER == TEXT_FONTCACHE
			EASYFIXCOORDS(&x,&y);
			SDL_Color _tempcolor;
			_tempcolor.r = r;
			_tempcolor.g = g;
			_tempcolor.b = b;
			_tempcolor.a = 255;
			FC_DrawColor(fontImage, mainWindowRenderer, x, y, _tempcolor ,"%s", text);
		#endif
	}

#endif