#ifndef GENERALGOODTEXT_H
#define GENERALGOODTEXT_H
// Text Stuff
	#if TEXTRENDERER == TEXT_FONTCACHE
		//#include <SDL_FontCache.h>
		#define CrossFont FC_Font
	#elif TEXTRENDERER == TEXT_DEBUG
		#define CrossFont CrossTexture
	#elif TEXTRENDERER == TEXT_VITA2D
		#define CrossFont vita2d_font
	#endif
#if TEXTRENDERER == TEXT_DEBUG
	extern float fontSize;
	extern char bitmapFontWidth;
	extern char bitmapFontHeight;
	extern short bitmapFontLettersPerImage;
#endif
#if TEXTRENDERER == TEXT_FONTCACHE
	//int fontSize = 20;
	extern int fontSize;
#endif
#if TEXTRENDERER == TEXT_VITA2D
	extern int fontSize;
#endif

// These two functions are unused mostly.
void drawLetterColor(int letterId, int _x, int _y, float size, unsigned char r, unsigned char g, unsigned char b);
void drawLetter(int letterId, int _x, int _y, float size);

void goodDrawTextColored(int x, int y, const char* text, float size, unsigned char r, unsigned char g, unsigned char b);
void goodDrawText(int x, int y, const char* text, float size);
void loadFont(char* filename);
int textHeight(float scale);
int textWidth(float scale, const char* message);

#endif /* GENERALGOODIMAGES_H */