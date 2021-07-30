/*
  OpenGL Connector Module for Zen Multimedia Desktop System
  Renders textured triangles to framebuffers and combines framebuffers with different shaders
  Textures can be internal or external
 */

#ifndef gl_connector_h
#define gl_connector_h

#include "gl_floatbuffer.c"
#include "gl_shader.c"
#include "zc_bitmap.c"
#include "zm_math4.c"
#include <GL/glew.h>
#include <stdio.h>
#include <stdlib.h>

#define TEX_CTX 9

typedef enum _gl_sha_typ_t
{
  SH_TEXTURE,
  SH_COLOR,
  SH_BLUR,
  SH_DRAW,
} gl_sha_typ_t;

typedef struct _glrect_t
{
  int x;
  int y;
  int w;
  int h;
} glrect_t;

void gl_init();
void gl_destroy();
void gl_new_texture(uint32_t i, int width, int height);
void gl_rel_texture(uint32_t i);
void gl_upload_vertexes(fb_t* fb);
void gl_upload_to_texture(int page, int x, int y, int w, int h, void* data);
void gl_save_framebuffer(bm_t* bm);
void gl_clear_framebuffer(int page, float r, float g, float b, float a);
void gl_draw_vertexes_in_framebuffer(int page, int start, int end, glrect_t source_region, glrect_t target_region, gl_sha_typ_t shader, int domask, int tex_w, int tex_h);
void gl_draw_framebuffer_in_framebuffer(int src_ind, int tgt_ind, glrect_t source_region, glrect_t target_region, glrect_t window, gl_sha_typ_t shader);

#endif

#if __INCLUDE_LEVEL__ == 0

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct _glbuf_t
{
  GLuint vbo;
  GLuint vao;
  fb_t*  flo_buf;
} glbuf_t;

typedef struct _gltex_t
{
  GLuint index;
  GLuint tx;
  GLuint fb;
  GLuint w;
  GLuint h;
} gltex_t;

struct gl_connector_t
{
  glsha_t shaders[4];
  gltex_t textures[10];
  glbuf_t vertexes[10];
} gl = {0};

void gl_errors(const char* place)
{
  GLenum error = 0;
  do
  {
    GLenum error = glGetError();
    if (error > GL_NO_ERROR)
      printf("GL Error at %s : %i\n", place, error);
  } while (error > GL_NO_ERROR);
}

glsha_t create_texture_shader()
{
  char* vsh =
      "#version 120\n"
      "attribute vec3 position;"
      "attribute vec4 texcoord;"
      "uniform mat4 projection;"
      "varying vec4 vUv;"
      "void main ( )"
      "{"
      "    gl_Position = projection * vec4(position,1.0);"
      "    vUv = texcoord;"
      "}";

  char* fsh =
      "#version 120\n"
      "uniform sampler2D sampler[10];"
      "uniform int domask;"
      "uniform ivec2 texdim;"
      "varying vec4 vUv;"
      "void main( )"
      "{"
      " vec4 col = vec4(1.0);"
      " int unit = int(vUv.z);"
      " float alpha = vUv.w;"
      "	if (unit == 0) col = texture2D(sampler[0], vUv.xy);"
      "	else if (unit == 1) col = texture2D(sampler[1], vUv.xy);"
      "	else if (unit == 2) col = texture2D(sampler[2], vUv.xy);"
      "	else if (unit == 3) col = texture2D(sampler[3], vUv.xy);"
      "	else if (unit == 4) col = texture2D(sampler[4], vUv.xy);"
      "	else if (unit == 5) col = texture2D(sampler[5], vUv.xy);"
      "	else if (unit == 6) col = texture2D(sampler[6], vUv.xy);"
      "	else if (unit == 7) col = texture2D(sampler[7], vUv.xy);"
      "	else if (unit == 8) col = texture2D(sampler[8], vUv.xy);"
      " if (domask == 1)"
      " {"
      "  vec4 msk = texture2D(sampler[1], vec2(gl_FragCoord.x / texdim.x, gl_FragCoord.y / texdim.y));"
      "  if (msk.a < col.a) col.a = msk.a;"
      " }"
      " if (alpha < 1.0) col.w *= alpha;"
      " gl_FragColor = col;"
      "}";

  glsha_t sha = gl_shader_create(vsh,
                                 fsh,
                                 2,
                                 ((const char*[]){"position", "texcoord"}),
                                 13,
                                 ((const char*[]){"projection", "sampler[0]", "sampler[1]", "sampler[2]",
                                                  "sampler[3]", "sampler[4]", "sampler[5]", "sampler[6]",
                                                  "sampler[7]", "sampler[8]", "sampler[9]", "domask", "texdim"}));

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

glsha_t create_color_shader()
{
  char* vsh =
      "#version 120\n"
      "attribute vec3 position;"
      "attribute vec4 texcoord;"
      "uniform mat4 projection;"
      "void main ( )"
      "{"
      "  gl_Position = projection * vec4(position,1.0);"
      "}";

  char* fsh =
      "#version 120\n"
      "void main( )"
      "{"
      "  gl_FragColor = vec4(0.0,0.0,0.0,1.0);"
      "}";

  return gl_shader_create(vsh,
                          fsh,
                          2,
                          ((const char*[]){"position", "texcoord"}),
                          1,
                          ((const char*[]){"projection"}));
}

glsha_t create_blur_shader()
{

  char* vsh =
      "#version 120\n"
      "attribute vec3 position;"
      "attribute vec4 texcoord;"
      "uniform mat4 projection;"
      "uniform sampler2D samplera;"
      "varying vec4 vUv;"
      "void main ( )"
      "{"
      "  gl_Position = projection * vec4(position,1.0);"
      "  vUv = texcoord;"
      "}";

  char* fsh =
      "#version 120\n"
      "uniform sampler2D samplera;\n"
      "varying vec4 vUv;"

      "void main()"
      "{"
      " float Pi = 6.28318530718;" // pi * 2

      " float Directions = 16.0;" // BLUR DIRECTIONS (Default 16.0 - More is better but slower)
      " float Quality    = 4.0;"  // BLUR QUALITY (Default 4.0 - More is better but slower)
      " float Size       = 10.0;" // BLUR SIZE (Radius)
      " vec2 Radius = Size / vec2(4096,4096);"

      // Pixel colour
      " vec4 Color = texture2D(samplera, vUv.xy);"

      // Blur calculations
      " for (float d = 0.0; d < Pi; d += Pi / Directions)"
      " {"
      "  for (float i = 1.0 / Quality; i <= 1.0; i += 1.0 / Quality)"
      "  {"
      "   Color += texture2D(samplera, vUv.xy + vec2(cos(d), sin(d)) * Radius * i);"
      "  }"
      " }"
      // Output to screen
      " Color /= Quality * Directions;"
      " gl_FragColor = Color;"
      "}";

  return gl_shader_create(vsh,
                          fsh,
                          2,
                          ((const char*[]){"position", "texcoord"}),
                          2,
                          ((const char*[]){"projection", "samplera"}));
}

glsha_t create_draw_shader()
{

  char* vsh =
      "#version 120\n"
      "attribute vec3 position;"
      "attribute vec4 texcoord;"
      "uniform mat4 projection;"
      "uniform sampler2D samplera;"
      "varying vec4 vUv;"
      "void main ( )"
      "{"
      "  gl_Position = projection * vec4(position,1.0);"
      "  vUv = texcoord;"
      "}";

  char* fsh =
      "#version 120\n"
      "uniform sampler2D samplera;\n"
      "varying vec4 vUv;"
      "void main()"
      "{"
      " gl_FragColor = texture2D(samplera, vUv.xy);"
      "}";

  return gl_shader_create(vsh,
                          fsh,
                          2,
                          ((const char*[]){"position", "texcoord"}),
                          2,
                          ((const char*[]){"projection", "samplera"}));
}

gltex_t gl_create_texture(int page, uint32_t w, uint32_t h)
{
  gltex_t tex = {0};

  tex.index = page;
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

  glGenFramebuffers(1, &tex.fb); // DEL 1

  glBindFramebuffer(GL_FRAMEBUFFER, tex.fb);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex.tx, 0);
  glClearColor(1.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  return tex;
}

void gl_delete_texture(gltex_t tex)
{
  glDeleteTextures(1, &tex.tx);     // DEL 0
  glDeleteFramebuffers(1, &tex.fb); // DEL 1
}

glbuf_t create_buffer()
{
  glbuf_t vb = {0};

  glGenBuffers(1, &vb.vbo); // DEL 0
  glBindBuffer(GL_ARRAY_BUFFER, vb.vbo);
  glGenVertexArrays(1, &vb.vao); // DEL 1
  glBindVertexArray(vb.vao);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 24, 0);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 24, (const GLvoid*)8);
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  return vb;
}

void gl_delete_vertex_buffer(glbuf_t buf)
{
  glDeleteBuffers(1, &buf.vbo);
  glDeleteVertexArrays(1, &buf.vao);
}

void gl_init(width, height)
{
  glewInit();

  gl.shaders[SH_TEXTURE] = create_texture_shader();
  gl.shaders[SH_COLOR]   = create_color_shader();
  gl.shaders[SH_BLUR]    = create_blur_shader();
  gl.shaders[SH_DRAW]    = create_draw_shader();

  /* last is preserved for context's default buffer */
  GLint context_fb;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &context_fb);
  gl.textures[TEX_CTX].fb = context_fb;

  /* buffer 0 is preserved for framebuffer drawing */
  gl.vertexes[0] = create_buffer(); // DEL 0
  gl.vertexes[1] = create_buffer(); // DEL 1
}

void gl_destroy()
{
  gl_delete_vertex_buffer(gl.vertexes[0]);
  gl_delete_vertex_buffer(gl.vertexes[1]);
}

void gl_new_texture(uint32_t page, int width, int height)
{
  assert(page < TEX_CTX); /* 0 is reserved for context's default framebuffer */

  gl.textures[page] = gl_create_texture(page, width, height);
}

void gl_rel_texture(uint32_t page)
{
  gl_delete_texture(gl.textures[page]);
}

void gl_upload_vertexes(fb_t* fb)
{
  glBindBuffer(GL_ARRAY_BUFFER, gl.vertexes[1].vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * fb->pos, fb->data, GL_DYNAMIC_DRAW);
  gl.vertexes[1].flo_buf = fb;
}

void gl_upload_to_texture(int page, int x, int y, int w, int h, void* data)
{
  gltex_t texture = gl.textures[page];
  glActiveTexture(GL_TEXTURE0 + texture.index);
  glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

void gl_save_framebuffer(bm_t* bm)
{
  glReadPixels(0, 0, bm->w, bm->h, GL_RGBA, GL_UNSIGNED_BYTE, bm->data);
}

void gl_clear_framebuffer(int page, float r, float g, float b, float a)
{
  glBindFramebuffer(GL_FRAMEBUFFER, gl.textures[page].fb);
  glClearColor(r, g, b, a);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void gl_draw_rectangle(glrect_t src_reg, glrect_t tgt_reg)
{
  GLfloat data[] =
      {
          0.0,
          0.0,
          0.0,
          (float)src_reg.h / 4096.0,
          0.0,
          1.0,

          tgt_reg.w,
          tgt_reg.h,
          (float)src_reg.w / 4096.0,
          0.0,
          0.0,
          1.0,

          0.0,
          tgt_reg.h,
          0.0,
          0.0,
          0.0,
          1.0,

          0.0,
          0.0,
          0.0,
          (float)src_reg.h / 4096.0,
          0.0,
          1.0,

          tgt_reg.w,
          0.0,
          (float)src_reg.w / 4096.0,
          (float)src_reg.h / 4096.0,
          0.0,
          1.0,

          tgt_reg.w,
          tgt_reg.h,
          (float)src_reg.w / 4096.0,
          0.0,
          0.0,
          1.0,
      };

  glBindBuffer(GL_ARRAY_BUFFER, gl.vertexes[0].vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 6, data, GL_DYNAMIC_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, 6);
}

void gl_draw_vertexes_in_framebuffer(int          page,
                                     int          start,
                                     int          end,
                                     glrect_t     reg_src,
                                     glrect_t     reg_tgt,
                                     gl_sha_typ_t shader,
                                     int          domask,
                                     int          tex_w,
                                     int          tex_h)
{
  matrix4array_t projection;
  projection.matrix = m4_defaultortho(0.0, reg_src.w, reg_src.h, 0, 0.0, 1.0);

  glUseProgram(gl.shaders[shader].name);

  if (shader == SH_TEXTURE)
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUniform1i(gl.shaders[shader].uni_loc[11], domask);
    glUniform2i(gl.shaders[shader].uni_loc[12], tex_w, tex_h);

    glUniformMatrix4fv(gl.shaders[shader].uni_loc[0], 1, 0, projection.array);
    glViewport(0, 0, reg_tgt.w, reg_tgt.h);
  }
  else if (shader == SH_COLOR)
  {
    glUniformMatrix4fv(gl.shaders[shader].uni_loc[0], 1, 0, projection.array);
    glViewport(0, 0, reg_tgt.w, reg_tgt.h);
  }

  glBindVertexArray(gl.vertexes[1].vao);
  glBindFramebuffer(GL_FRAMEBUFFER, gl.textures[page].fb);

  glDrawArrays(GL_TRIANGLES, start, end - start);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindVertexArray(0);
}

void gl_draw_framebuffer_in_framebuffer(int          src_page,
                                        int          tgt_page,
                                        glrect_t     src_reg,
                                        glrect_t     tgt_reg,
                                        glrect_t     window,
                                        gl_sha_typ_t shader)
{
  glUseProgram(gl.shaders[shader].name);

  if (shader == SH_DRAW)
  {
    glUniform1i(gl.shaders[shader].uni_loc[1], gl.textures[src_page].index);
  }
  else if (shader == SH_BLUR)
  {
    glUniform1i(gl.shaders[shader].uni_loc[1], gl.textures[src_page].index);
  }

  matrix4array_t projection;
  projection.matrix = m4_defaultortho(0.0, tgt_reg.w, tgt_reg.h, 0.0, 0.0, 1.0);
  glUniformMatrix4fv(gl.shaders[shader].uni_loc[0], 1, 0, projection.array);

  glViewport(0, 0, tgt_reg.w, tgt_reg.h);

  glBindFramebuffer(GL_FRAMEBUFFER, gl.textures[tgt_page].fb);
  glBindVertexArray(gl.vertexes[0].vao);

  if (window.w > 0)
  {
    glEnable(GL_SCISSOR_TEST);
    glScissor(window.x, tgt_reg.h - window.y - window.h, window.w, window.h); // force upside down
  }

  gl_draw_rectangle(src_reg, tgt_reg);

  glDisable(GL_SCISSOR_TEST);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

#endif
