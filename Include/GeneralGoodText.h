#ifndef GENERALGOODTEXT_H
#define GENERALGOODTEXT_H

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

// These two functions are unused mostly.
void drawLetterColor(int letterId, int _x, int _y, float size, unsigned char r, unsigned char g, unsigned char b);
void drawLetter(int letterId, int _x, int _y, float size);

void goodDrawTextColored(int x, int y, const char* text, float size, unsigned char r, unsigned char g, unsigned char b);
void goodDrawText(int x, int y, const char* text, float size);
void loadFont(char* filename);
int textHeight(float scale);
int textWidth(float scale, const char* message);

#endif /* GENERALGOODIMAGES_H */