// See libvita2d-License.txt for the license of just this file

// The header file has functions. Same as the ones from libvita2d, but uses doubles so it can be better
#ifndef VITA2DDRAWGOODHEADER
#define VITA2DDRAWGOODHEADER

#if PLATFORM == PLAT_VITA
	#include <psp2/display.h>
	#include <psp2/gxm.h>
	#include <psp2/types.h>
	#include <psp2/kernel/sysmem.h>
	#include <psp2/message_dialog.h>
	#include <psp2/sysmodule.h>
	extern SceGxmContext* _vita2d_context;
	extern void *indices_buf_addr;

	extern SceGxmVertexProgram *_vita2d_textureVertexProgram;
	extern SceGxmFragmentProgram *_vita2d_textureFragmentProgram;
	extern const SceGxmProgramParameter *_vita2d_colorWvpParam;
	extern float _vita2d_ortho_matrix[4*4];
	extern SceGxmFragmentProgram *_vita2d_textureTintFragmentProgram;
	extern const SceGxmProgramParameter *_vita2d_textureWvpParam;
	extern SceGxmProgramParameter *_vita2d_textureTintColorParam;
#endif

void drawTextureScaleGood(const CrossTexture* texture, float x, float y, double x_scale, double y_scale){
	#if PLATFORM == PLAT_VITA
		vita2d_texture_vertex *vertices = (vita2d_texture_vertex *)vita2d_pool_memalign(4 * sizeof(vita2d_texture_vertex), sizeof(vita2d_texture_vertex));
	
		const float w = x_scale * vita2d_texture_get_width(texture);
		const float h = y_scale * vita2d_texture_get_height(texture);
	
		vertices[0].x = x;
		vertices[0].y = y;
		vertices[0].z = +0.5f;
		vertices[0].u = 0.0f;
		vertices[0].v = 0.0f;
	
		vertices[1].x = x + w;
		vertices[1].y = y;
		vertices[1].z = +0.5f;
		vertices[1].u = 1.0f;
		vertices[1].v = 0.0f;
	
		vertices[2].x = x;
		vertices[2].y = y + h;
		vertices[2].z = +0.5f;
		vertices[2].u = 0.0f;
		vertices[2].v = 1.0f;
	
		vertices[3].x = x + w;
		vertices[3].y = y + h;
		vertices[3].z = +0.5f;
		vertices[3].u = 1.0f;
		vertices[3].v = 1.0f;
	
		sceGxmSetFragmentTexture(_vita2d_context, 0, &texture->gxm_tex);
	
		sceGxmSetVertexStream(_vita2d_context, 0, vertices);
		sceGxmDraw(_vita2d_context, SCE_GXM_PRIMITIVE_TRIANGLE_STRIP, SCE_GXM_INDEX_FORMAT_U16, indices_buf_addr, 4);
	#else
		drawTextureScale((CrossTexture*)texture,x,y,x_scale,y_scale);
	#endif
}
void drawTextureScaleAlphaGood(const CrossTexture* texture, float x, float y, double x_scale, double y_scale, unsigned char alpha){
	#if PLATFORM == PLAT_VITA	
		// first function
		sceGxmSetVertexProgram(_vita2d_context, _vita2d_textureVertexProgram);
		sceGxmSetFragmentProgram(_vita2d_context, _vita2d_textureTintFragmentProgram);
		// second function	
		void *vertex_wvp_buffer;
		sceGxmReserveVertexDefaultUniformBuffer(_vita2d_context, &vertex_wvp_buffer);
		sceGxmSetUniformDataF(vertex_wvp_buffer, _vita2d_textureWvpParam, 0, 16, _vita2d_ortho_matrix);
		// third function
		unsigned int color = RGBA8(255,255,255,alpha);
		void *texture_tint_color_buffer;
		sceGxmReserveFragmentDefaultUniformBuffer(_vita2d_context, &texture_tint_color_buffer);
		float *tint_color = vita2d_pool_memalign(4 * sizeof(float),sizeof(float));
		tint_color[0] = ((color >> 8*0) & 0xFF)/255.0f;
		tint_color[1] = ((color >> 8*1) & 0xFF)/255.0f;
		tint_color[2] = ((color >> 8*2) & 0xFF)/255.0f;
		tint_color[3] = ((color >> 8*3) & 0xFF)/255.0f;
		sceGxmSetUniformDataF(texture_tint_color_buffer, _vita2d_textureTintColorParam, 0, 4, tint_color);
		// final
		drawTextureScaleGood(texture, x, y, x_scale, y_scale);
	#else
		drawTextureScaleAlpha((CrossTexture*)texture,x,y,x_scale,y_scale,alpha);
	#endif
}



#endif