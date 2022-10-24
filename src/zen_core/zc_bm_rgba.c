#ifndef zc_bm_rgba_h
#define zc_bm_rgba_h

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct _bm_rgba_t bm_rgba_t;
struct _bm_rgba_t
{
    int w;
    int h;

    uint8_t* data;
    uint32_t size;
};

bm_rgba_t* bm_rgba_new(int the_w, int the_h);
bm_rgba_t* bm_rgba_new_clone(bm_rgba_t* bm);
void       bm_rgba_reset(bm_rgba_t* bm);
void       bm_rgba_describe(void* p, int level);
void       bm_rgba_insert(bm_rgba_t* dst, bm_rgba_t* src, int sx, int sy);
void       bm_rgba_blend(bm_rgba_t* dst, bm_rgba_t* src, int sx, int sy);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "zc_memory.c"
#include <assert.h>
#include <string.h>

void bm_rgba_describe_data(void* p, int level);

void bm_rgba_del(void* pointer)
{
    bm_rgba_t* bm = pointer;

    if (bm->data != NULL) REL(bm->data); // REL 1
}

bm_rgba_t* bm_rgba_new(int the_w, int the_h)
{
    assert(the_w > 0 && the_h > 0);

    bm_rgba_t* bm = CAL(sizeof(bm_rgba_t), bm_rgba_del, bm_rgba_describe); // REL 0

    bm->w = the_w;
    bm->h = the_h;

    bm->size = 4 * the_w * the_h;
    bm->data = CAL(bm->size * sizeof(unsigned char), NULL, bm_rgba_describe_data); // REL 1

    return bm;
}

bm_rgba_t* bm_rgba_new_clone(bm_rgba_t* the_bm)
{
    bm_rgba_t* bm = bm_rgba_new(the_bm->w, the_bm->h);
    memcpy(bm->data, the_bm->data, the_bm->size);
    return bm;
}

void bm_rgba_reset(bm_rgba_t* bm)
{
    memset(bm->data, 0, bm->size);
}

void bm_rgba_insert(bm_rgba_t* dst, bm_rgba_t* src, int sx, int sy)
{
    if (sx < 0) sx = 0;
    if (sy < 0) sy = 0;
    if (sx >= dst->w) return;
    if (sy >= dst->h) return;

    int ex = sx + src->w;
    int ey = sy + src->h;

    if (ex >= dst->w) ex = dst->w;
    if (ey >= dst->h) ey = dst->h;

    uint32_t* d = (uint32_t*) dst->data;
    uint32_t* s = (uint32_t*) src->data;
    uint32_t  l = (ex - sx) * 4; // length

    d += (sy * dst->w) + sx;

    for (int y = sy; y < ey; y++)
    {
	memcpy(d, s, l);

	d += dst->w;
	s += src->w;
    }
}

void bm_rgba_blend(bm_rgba_t* dst, bm_rgba_t* src, int sx, int sy)
{
    if (sx < 0) sx = 0;
    if (sy < 0) sy = 0;
    if (sx >= dst->w) return;
    if (sy >= dst->h) return;

    int ex = sx + src->w;
    int ey = sy + src->h;

    if (ex >= dst->w) ex = dst->w;
    if (ey >= dst->h) ey = dst->h;

    uint32_t* d  = (uint32_t*) dst->data; // dest bytes
    uint32_t* s  = (uint32_t*) src->data; // source byes
    uint32_t* td = (uint32_t*) dst->data; // temporary bytes
    uint32_t* ts = (uint32_t*) src->data; // temporary bytes

    d += (sy * dst->w) + sx;

    for (int y = sy; y < ey; y++)
    {
	td = d;
	ts = s;

	for (int x = sx; x < ex; x++)
	{
	    uint32_t dc = *td;
	    uint32_t sc = *ts;
	    int      sa = sc & 0xFF;

	    if (sa == 255)
	    {
		*td = sc; // dest color is source color
	    }
	    else if (sa == 0)
	    {
		// dest remains the same
	    }
	    else
	    {
		static const int AMASK    = 0xFF000000;
		static const int RBMASK   = 0x00FF00FF;
		static const int GMASK    = 0x0000FF00;
		static const int AGMASK   = AMASK | GMASK;
		static const int ONEALPHA = 0x01000000;

		unsigned int a  = (sc & AMASK) >> 24;
		unsigned int na = 255 - a;
		unsigned int rb = ((na * (dc & RBMASK)) + (a * (sc & RBMASK))) >> 8;
		unsigned int ag = (na * ((dc & AGMASK) >> 8)) + (a * (ONEALPHA | ((sc & GMASK) >> 8)));
		*td             = ((rb & RBMASK) | (ag & AGMASK));
	    }

	    td++;
	    ts++;
	}
	d += dst->w;
	s += src->w;
    }
}

void bm_rgba_describe(void* p, int level)
{
    bm_rgba_t* bm = p;
    printf("width %i height %i size %u", bm->w, bm->h, bm->size);
}

void bm_rgba_describe_data(void* p, int level)
{
    printf("bm data\n");
}

#endif
