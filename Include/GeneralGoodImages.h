#ifndef GENERALGOODIMAGES_H
#define GENERALGOODIMAGES_H

#if RENDERER == REND_SDL
	#define CrossTexture SDL_Texture
	#include <SDL2/SDL.h>
	#include <SDL2/SDL_image.h>	
#endif
#if RENDERER == REND_VITA2D
	#define CrossTexture vita2d_texture
	#include <vita2d.h>
#endif
#if RENDERER == REND_SF2D
	#define CrossTexture sf2d_texture
	#include <3ds.h>
	#include <stdio.h>
	#include <sf2d.h>
	#include <sfil.h>
	#include <3ds/svc.h>
#endif

void drawTexturePartScaleRotate(CrossTexture* texture, int x, int y, float tex_x, float tex_y, float tex_w, float tex_h, float x_scale, float y_scale, float rad);
void drawTexturePartScaleTint(CrossTexture* passedTexture, int destX, int destY, int texX, int texY, int texW, int texH, float texXScale, float texYScale, unsigned char r, unsigned char g, unsigned b);
void drawTexturePartScale(CrossTexture* passedTexture, int destX, int destY, int texX, int texY, int texW, int texH, float texXScale, float texYScale);
void drawTextureScaleSize(CrossTexture* passedTexture, int destX, int destY, float texXScale, float texYScale);
void drawTextureScaleTint(CrossTexture* passedTexture, int destX, int destY, float texXScale, float texYScale, unsigned char r, unsigned char g, unsigned char b);
void drawTextureScale(CrossTexture* passedTexture, int destX, int destY, float texXScale, float texYScale);
void drawTexture(CrossTexture* passedTexture, int _destX, int _destY);
void freeTexture(CrossTexture* passedTexture);
int getTextureHeight(CrossTexture* passedTexture);
int getTextureWidth(CrossTexture* passedTexture);
CrossTexture* loadJPG(char* path);
CrossTexture* loadPNG(char* path);
CrossTexture* loadPNGBuffer(void* _passedBuffer, int _passedBufferSize);
CrossTexture* loadJPGBuffer(void* _passedBuffer, int _passedBufferSize);
void drawTextureAlpha(CrossTexture* passedTexture, int _destX, int _destY, unsigned char alpha);
void drawTextureScaleAlpha(CrossTexture* passedTexture, int destX, int destY, float texXScale, float texYScale, unsigned char alpha);

#endif /* GENERALGOODGRAPHICS_H */