#ifndef ku_gl_h
#define ku_gl_h

#include "ku_bitmap.c"
#include "zc_vector.c"
/* #include <GL/glew.h> */
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <stdio.h>

void ku_gl_init(int max_dev_width, int max_dev_height);
void ku_gl_render(ku_bitmap_t* bitmap);
void ku_gl_render_quad(ku_bitmap_t* bitmap, uint32_t index, bmr_t mask);
void ku_gl_add_textures(vec_t* views, int force);
void ku_gl_add_vertexes(vec_t* views);
void ku_gl_clear_rect(ku_bitmap_t* bitmap, int x, int y, int w, int h);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "ku_gl_atlas.c"
#include "ku_gl_floatbuffer.c"
#include "ku_gl_shader.c"
#include "ku_view.c"
#include "zc_log.c"
#include "zc_util3.c"

EGLDisplay glDisplay;
EGLConfig  glConfig;
EGLContext glContext;
EGLSurface glSurface;

typedef struct _glbuf_t
{
    GLuint vbo;
    GLuint vao;
} glbuf_t;

typedef struct _gltex_t
{
    GLuint index;
    GLuint tx;
    GLuint fb;
    GLuint w;
    GLuint h;
} gltex_t;

glsha_t           shader;
glbuf_t           buffer;
gltex_t           tex_atlas;
gltex_t           tex_frame;
ku_gl_atlas_t*    atlas       = NULL;
ku_floatbuffer_t* floatbuffer = NULL;
ku_bitmap_t*      pixelmap;

int texture_size = 0;

glsha_t ku_gl_create_texture_shader()
{
    char* vsh =
	"#version 100\n"
	"attribute vec3 position;"
	"attribute vec4 texcoord;"
	"uniform mat4 projection;"
	"varying highp vec4 vUv;"
	"void main ( )"
	"{"
	"    gl_Position = projection * vec4(position,1.0);"
	"    vUv = texcoord;"
	"}";

    char* fsh =
	"#version 100\n"
	"uniform sampler2D sampler[10];"
	"uniform int domask;"
	"uniform highp vec2 texdim;"
	"varying highp vec4 vUv;"
	"void main( )"
	"{"
	" highp float alpha = vUv.w;"
	" highp vec4 col = texture2D(sampler[0], vUv.xy);"
	" if (alpha < 1.0) col.w *= alpha;"
	" gl_FragColor = col;"
	"}";

    glsha_t sha = ku_gl_shader_create(vsh, fsh, 2, ((const char*[]){"position", "texcoord"}), 13, ((const char*[]){"projection", "sampler[0]", "sampler[1]", "sampler[2]", "sampler[3]", "sampler[4]", "sampler[5]", "sampler[6]", "sampler[7]", "sampler[8]", "sampler[9]", "domask", "texdim"}));

    glUseProgram(sha.name);

    glUniform1i(sha.uni_loc[1], 0);
    glUniform1i(sha.uni_loc[2], 1);
    glUniform1i(sha.uni_loc[3], 2);
    glUniform1i(sha.uni_loc[4], 3);
    glUniform1i(sha.uni_loc[5], 4);
    glUniform1i(sha.uni_loc[6], 5);
    glUniform1i(sha.uni_loc[7], 6);
    glUniform1i(sha.uni_loc[8], 7);
    glUniform1i(sha.uni_loc[9], 8);

    return sha;
}

glbuf_t ku_gl_create_vertex_buffer()
{
    glbuf_t vb = {0};

    glGenBuffers(1, &vb.vbo); // DEL 0
    glBindBuffer(GL_ARRAY_BUFFER, vb.vbo);
    glGenVertexArrays(1, &vb.vao); // DEL 1
    glBindVertexArray(vb.vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 24, 0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 24, (const GLvoid*) 8);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return vb;
}

void ku_gl_delete_vertex_buffer(glbuf_t buf)
{
    glDeleteBuffers(1, &buf.vbo);
    glDeleteVertexArrays(1, &buf.vao);
}

gltex_t ku_gl_create_texture(int index, uint32_t w, uint32_t h)
{
    gltex_t tex = {0};

    tex.index = index;
    tex.w     = w;
    tex.h     = h;

    glGenTextures(1, &tex.tx); // DEL 0

    glActiveTexture(GL_TEXTURE0 + tex.index);
    glBindTexture(GL_TEXTURE_2D, tex.tx);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    /* glGenFramebuffers(1, &tex.fb); // DEL 1 */

    /* glBindFramebuffer(GL_FRAMEBUFFER, tex.fb); */
    /* glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex.tx, 0); */
    /* glClearColor(1.0, 0.0, 0.0, 1.0); */
    /* glClear(GL_COLOR_BUFFER_BIT); */
    /* glBindFramebuffer(GL_FRAMEBUFFER, 0); */

    return tex;
}

void ku_gl_delete_texture(gltex_t tex)
{
    glDeleteTextures(1, &tex.tx);     // DEL 0
    glDeleteFramebuffers(1, &tex.fb); // DEL 1
}

void ku_gl_init(int max_dev_width, int max_dev_height)
{
    /* EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY); */
    /* if (display == EGL_NO_DISPLAY) */
    /* { */
    /* 	printf("ERROR 0\n"); */
    /* } */

    /* if (eglInitialize(display, NULL, NULL) != EGL_TRUE) */
    /* { */
    /* 	printf("ERROR 1\n"); */
    /* } */

    /* EGLConfig config; */
    /* EGLint    num_config = 0; */
    /* if (eglChooseConfig(display, NULL, &config, 1, &num_config) != EGL_TRUE) */
    /* { */
    /* 	printf("ERROR 2\n"); */
    /* } */
    /* if (num_config == 0) */
    /* { */
    /* 	printf("ERROR 3\n"); */
    /* } */

    /* eglBindAPI(EGL_OPENGL_API); */
    /* EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, NULL); */
    /* if (eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context) != EGL_TRUE) */
    /* { */
    /* 	printf("ERROR 4\n"); */
    /* } */

    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
	printf("ERROR 5 %s\n", glewGetErrorString(err));
    }

    shader = ku_gl_create_texture_shader();
    buffer = ku_gl_create_vertex_buffer();

    floatbuffer = ku_floatbuffer_new();

    /* glbuf_t vb = {0}; */

    /* glGenBuffers(1, &pbo1); // DEL 0 */
    /* glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo1); */
    /* glBufferData(GL_PIXEL_PACK_BUFFER, sizeof(GLuint) * 4096 * 4096, 0, GL_STREAM_READ); */

    /* glGenBuffers(1, &pbo2); // DEL 0 */
    /* glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo2); */
    /* glBufferData(GL_PIXEL_PACK_BUFFER, sizeof(GLuint) * 4096 * 4096, 0, GL_STREAM_READ); */

    /* glBindBuffer(GL_PIXEL_PACK_BUFFER, 0); */

    /* pbos[0] = pbo1; */
    /* pbos[1] = pbo2; */

    /* pbindex  = 0; */
    /* pbnindex = 1; */

    /* pixelmap = ku_bitmap_new(4096, 4096); */
    /* REL(pixelmap->data); */

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* calculate needed texture size, should be four times bigger than the highest resolution display */

    int size = max_dev_width > max_dev_height ? max_dev_width : max_dev_height;
    size *= 2;
    int binsize = 2;
    while (binsize < size) binsize *= 2;

    int value;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &value); // Returns 1 value

    if (binsize > value) binsize = value;

    zc_log_info("Texture size will be %i", binsize);

    texture_size = binsize;

    atlas = ku_gl_atlas_new(texture_size, texture_size);

    tex_atlas = ku_gl_create_texture(0, texture_size, texture_size);
}

void ku_gl_add_textures(vec_t* views, int force)
{
    /* reset atlas */
    if (force)
    {
	if (texture_size != atlas->width)
	{
	    ku_gl_delete_texture(tex_atlas);
	    tex_atlas = ku_gl_create_texture(0, texture_size, texture_size);
	}
	if (atlas) REL(atlas);
	atlas = ku_gl_atlas_new(texture_size, texture_size);
    }

    int reset = 0;

    /* add texture to atlas */
    for (int index = 0; index < views->length; index++)
    {
	ku_view_t* view = views->data[index];

	/* does it fit into the texture atlas? */
	if (view->texture.bitmap &&
	    view->texture.bitmap->w < texture_size &&
	    view->texture.bitmap->h < texture_size)
	{
	    /* do we have to upload it? */
	    if (view->texture.uploaded == 0 || force)
	    {
		ku_gl_atlas_coords_t coords = ku_gl_atlas_get(atlas, view->id);

		/* did it's size change or is it zero yet? */
		if (view->texture.bitmap->w != coords.w || view->texture.bitmap->h != coords.h || force)
		{
		    int success = ku_gl_atlas_put(atlas, view->id, view->texture.bitmap->w, view->texture.bitmap->h);

		    if (success < 0)
		    {
			zc_log_debug("Texture Atlas Reset\n");
			if (force == 0) reset = 1;
			break;
		    }

		    coords = ku_gl_atlas_get(atlas, view->id);
		}

		glActiveTexture(GL_TEXTURE0 + tex_atlas.index);
		glTexSubImage2D(GL_TEXTURE_2D, 0, coords.x, coords.y, coords.w, coords.h, GL_RGBA, GL_UNSIGNED_BYTE, view->texture.bitmap->data);

		view->texture.uploaded = 1;
	    }
	}
	else
	{
	    if (force == 0)
	    {
		/* increase texture and atlas size to fit texture */

		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texture_size); // Returns 1 value

		reset = 1;
	    }

	    /* TODO : auto test texture resize */
	}
    }

    if (reset == 1) ku_gl_add_textures(views, 1);
}

void ku_gl_add_vertexes(vec_t* views)
{
    ku_floatbuffer_reset(floatbuffer);

    /* add vertexes to buffer */
    for (int index = 0; index < views->length; index++)
    {
	ku_view_t* view = views->data[index];

	ku_rect_t            rect = view->frame.global;
	ku_gl_atlas_coords_t text = ku_gl_atlas_get(atlas, view->id);

	/* expand with blur thickness */

	if (view->style.shadow_blur > 0)
	{
	    rect.x -= view->style.shadow_blur;
	    rect.y -= view->style.shadow_blur;
	    rect.w += 2 * view->style.shadow_blur;
	    rect.h += 2 * view->style.shadow_blur;
	}

	float data[36];

	data[0] = rect.x;
	data[1] = rect.y;

	data[6] = rect.x + rect.w;
	data[7] = rect.y + rect.h;

	data[12] = rect.x;
	data[13] = rect.y + rect.h;

	data[18] = rect.x + rect.w;
	data[19] = rect.y;

	data[24] = rect.x;
	data[25] = rect.y;

	data[30] = rect.x + rect.w;
	data[31] = rect.y + rect.h;

	data[2] = text.ltx;
	data[3] = text.lty;

	data[8] = text.rbx;
	data[9] = text.rby;

	data[14] = text.ltx;
	data[15] = text.rby;

	data[20] = text.rbx;
	data[21] = text.lty;

	data[26] = text.ltx;
	data[27] = text.lty;

	data[32] = text.rbx;
	data[33] = text.rby;

	data[4]  = (float) 0;
	data[10] = (float) 0;
	data[16] = (float) 0;
	data[22] = (float) 0;
	data[28] = (float) 0;
	data[34] = (float) 0;

	data[5]  = view->texture.alpha;
	data[11] = view->texture.alpha;
	data[17] = view->texture.alpha;
	data[23] = view->texture.alpha;
	data[29] = view->texture.alpha;
	data[35] = view->texture.alpha;

	ku_floatbuffer_add(floatbuffer, data, 36);
    }

    glBindBuffer(GL_ARRAY_BUFFER, buffer.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * floatbuffer->pos, floatbuffer->data, GL_DYNAMIC_DRAW);
}

void ku_gl_render(ku_bitmap_t* bitmap)
{
    /* glBindFramebuffer(GL_FRAMEBUFFER, tex_frame.fb); */
    glBindVertexArray(buffer.vao);

    glActiveTexture(GL_TEXTURE0 + tex_atlas.index);
    glBindTexture(GL_TEXTURE_2D, tex_atlas.tx);

    glClearColor(1.0, 1.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shader.name);

    matrix4array_t projection;
    projection.matrix = m4_defaultortho(0.0, bitmap->w, bitmap->h, 0, 0.0, 1.0);

    glUniformMatrix4fv(shader.uni_loc[0], 1, 0, projection.array);
    glViewport(0, 0, bitmap->w, bitmap->h);

    glDrawArrays(GL_TRIANGLES, 0, floatbuffer->pos);
    glBindVertexArray(0);
}

void ku_gl_render_quad(ku_bitmap_t* bitmap, uint32_t index, bmr_t mask)
{
    /* glBindFramebuffer(GL_FRAMEBUFFER, tex_frame.fb); */
    glBindVertexArray(buffer.vao);

    glActiveTexture(GL_TEXTURE0 + tex_atlas.index);
    glBindTexture(GL_TEXTURE_2D, tex_atlas.tx);

    glUseProgram(shader.name);

    /* draw masked region only */

    matrix4array_t projection;
    projection.matrix = m4_defaultortho(mask.x, mask.z, mask.w, mask.y, 0.0, 1.0);

    glUniformMatrix4fv(shader.uni_loc[0], 1, 0, projection.array);

    /* viewport origo is in the left bottom corner */

    glViewport(mask.x, bitmap->h - mask.w, mask.z - mask.x, mask.w - mask.y);

    glDrawArrays(GL_TRIANGLES, index * 6, 6);

    glBindVertexArray(0);
}

void ku_gl_clear_rect(ku_bitmap_t* bitmap, int x, int y, int w, int h)
{
    glEnable(GL_SCISSOR_TEST);
    glScissor(x, bitmap->h - (x + w), w, h);
    /* glScissor(100, 100, 500, 500); */

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
}

#endif
