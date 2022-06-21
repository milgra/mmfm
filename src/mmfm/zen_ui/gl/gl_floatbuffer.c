
#ifndef fb_h
#define fb_h

#include "zc_memory.c"
#include <GL/glew.h>
#include <string.h>

typedef struct fb_t fb_t;
struct fb_t
{
  GLfloat* data;
  uint32_t pos;
  uint32_t cap;
  char     changed;
};

fb_t* fb_new(void);
void  fb_del(void* fb);
void  fb_reset(fb_t* fb);
void  fb_add(fb_t* fb, GLfloat* data, size_t count);

#endif

#if __INCLUDE_LEVEL__ == 0

void fb_desc(void* p, int level)
{
  printf("fb");
}

void fb_desc_data(void* p, int level)
{
  printf("fb data");
}

fb_t* fb_new()
{
  fb_t* fb = CAL(sizeof(fb_t), fb_del, fb_desc);
  fb->data = CAL(sizeof(GLfloat) * 10, NULL, fb_desc_data);
  fb->pos  = 0;
  fb->cap  = 10;

  return fb;
}

void fb_del(void* pointer)
{
  fb_t* fb = pointer;
  REL(fb->data);
}

void fb_reset(fb_t* fb)
{
  fb->pos = 0;
}

void fb_expand(fb_t* fb)
{
  assert(fb->cap < UINT32_MAX / 2);
  fb->cap *= 2;
  fb->data = mem_realloc(fb->data, sizeof(void*) * fb->cap);
}

void fb_add(fb_t* fb, GLfloat* data, size_t count)
{
  while (fb->pos + count >= fb->cap) fb_expand(fb);
  memcpy(fb->data + fb->pos, data, sizeof(GLfloat) * count);
  fb->pos += count;
  fb->changed = 1;
}

#endif
