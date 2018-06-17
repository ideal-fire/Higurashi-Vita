#ifndef GENERALGOODTEXT_H
#define GENERALGOODTEXT_H
// Text Stuff
	#if TEXTRENDERER == TEXT_FONTCACHE
		#define CrossFont FC_Font
	#elif TEXTRENDERER == TEXT_DEBUG
		#define CrossFont CrossTexture
	#elif TEXTRENDERER == TEXT_VITA2D
		#define CrossFont vita2d_font
	#endif
#if TEXTRENDERER == TEXT_DEBUG
	extern float fontSize;
#endif
#if TEXTRENDERER == TEXT_FONTCACHE
	//int fontSize = 20;
	extern int fontSize;
#endif
#if TEXTRENDERER == TEXT_VITA2D
	extern int fontSize;
#endif
#if TEXTRENDERER == TEXT_UNDEFINED
	extern int fontSize;
#endif

void goodDrawTextColored(int x, int y, const char* text, float size, unsigned char r, unsigned char g, unsigned char b);
void goodDrawText(int x, int y, const char* text, float size);
void goodDrawTextColoredAlpha(int x, int y, const char* text, float size, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void loadFont(char* filename);
int textHeight(float scale);
int textWidth(float scale, const char* message);

#endif /* GENERALGOODIMAGES_H */