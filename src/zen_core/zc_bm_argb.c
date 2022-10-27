#ifndef zc_bm_argb_h
#define zc_bm_argb_h

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct _bm_argb_t bm_argb_t;
struct _bm_argb_t
{
    int w;
    int h;

    uint8_t* data;
    uint32_t size;
};

bm_argb_t* bm_argb_new(int the_w, int the_h);
bm_argb_t* bm_argb_new_clone(bm_argb_t* bm);
void       bm_argb_reset(bm_argb_t* bm);
void       bm_argb_describe(void* p, int level);
void       bm_argb_insert(bm_argb_t* dst, bm_argb_t* src, int sx, int sy);
void       bm_argb_blend(bm_argb_t* dst, bm_argb_t* src, int sx, int sy);
void       bm_argb_blend_with_alpha_mask(bm_argb_t* dst, bm_argb_t* src, int sx, int sy, int mask);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "zc_memory.c"
#include <assert.h>
#include <string.h>

void bm_argb_describe_data(void* p, int level);

void bm_argb_del(void* pointer)
{
    bm_argb_t* bm = pointer;

    if (bm->data != NULL) REL(bm->data); // REL 1
}

bm_argb_t* bm_argb_new(int the_w, int the_h)
{
    assert(the_w > 0 && the_h > 0);

    bm_argb_t* bm = CAL(sizeof(bm_argb_t), bm_argb_del, bm_argb_describe); // REL 0

    bm->w = the_w;
    bm->h = the_h;

    bm->size = 4 * the_w * the_h;
    bm->data = CAL(bm->size * sizeof(unsigned char), NULL, bm_argb_describe_data); // REL 1

    return bm;
}

bm_argb_t* bm_argb_new_clone(bm_argb_t* the_bm)
{
    bm_argb_t* bm = bm_argb_new(the_bm->w, the_bm->h);
    memcpy(bm->data, the_bm->data, the_bm->size);
    return bm;
}

void bm_argb_reset(bm_argb_t* bm)
{
    memset(bm->data, 0, bm->size);
}

void bm_argb_insert(bm_argb_t* dst, bm_argb_t* src, int sx, int sy)
{
    if (sx > dst->w) return;
    if (sy > dst->h) return;
    if (sx + src->w < 0) return;
    if (sy + src->h < 0) return;

    int sox = 0; // src offset
    int soy = 0;
    int dox = sx; // dst offset
    int doy = sy;

    if (sx < 0)
    {
	dox = 0;
	sox = -sx;
    }
    if (sy < 0)
    {
	doy = 0;
	soy = -sy;
    }

    int dex = sx + src->w; // dst endpoints
    int dey = sy + src->h;

    if (dex >= dst->w) dex = dst->w;
    if (dey >= dst->h) dey = dst->h;

    uint32_t* d = (uint32_t*) dst->data;
    uint32_t* s = (uint32_t*) src->data;
    uint32_t  l = (dex - dox) * 4; // length

    d += (doy * dst->w) + dox;
    s += (soy * src->w) + sox;

    for (int y = doy; y < dey; y++)
    {
	memcpy(d, s, l);

	d += dst->w;
	s += src->w;
    }
}

void bm_argb_blend(bm_argb_t* dst, bm_argb_t* src, int sx, int sy)
{
    if (sx > dst->w) return;
    if (sy > dst->h) return;
    if (sx + src->w < 0) return;
    if (sy + src->h < 0) return;

    int sox = 0; // src offset
    int soy = 0;
    int dox = sx; // dst offset
    int doy = sy;

    if (sx < 0)
    {
	dox = 0;
	sox = -sx;
    }
    if (sy < 0)
    {
	doy = 0;
	soy = -sy;
    }

    int dex = sx + src->w; // dst endpoints
    int dey = sy + src->h;

    if (dex >= dst->w) dex = dst->w;
    if (dey >= dst->h) dey = dst->h;

    uint32_t* d  = (uint32_t*) dst->data; // dest bytes
    uint32_t* s  = (uint32_t*) src->data; // source byes
    uint32_t* td = (uint32_t*) dst->data; // temporary dst bytes
    uint32_t* ts = (uint32_t*) src->data; // temporary src bytes

    d += (doy * dst->w) + dox;
    s += (soy * src->w) + sox;

    for (int y = doy; y < dey; y++)
    {
	td = d;
	ts = s;

	for (int x = dox; x < dex; x++)
	{
	    static const int AMASK    = 0xFF000000;
	    static const int RBMASK   = 0x00FF00FF;
	    static const int GMASK    = 0x0000FF00;
	    static const int AGMASK   = AMASK | GMASK;
	    static const int ONEALPHA = 0x01000000;

	    uint32_t dc = *td;
	    uint32_t sc = *ts;
	    int      sa = (sc & AMASK) >> 24;

	    if (sa == 256)
	    {
		*td = sc; // dest color is source color
	    }
	    else if (sa > 0)
	    {

		unsigned int na = 255 - sa;
		unsigned int rb = ((na * (dc & RBMASK)) + (sa * (sc & RBMASK))) >> 8;
		unsigned int ag = (na * ((dc & AGMASK) >> 8)) + (sa * (ONEALPHA | ((sc & GMASK) >> 8)));
		*td             = ((rb & RBMASK) | (ag & AGMASK));
	    }
	    // else dst remains the same

	    td++;
	    ts++;
	}
	d += dst->w;
	s += src->w;
    }

    /* // do compositing */
    /* pixman_image_composite( */
    /* 	PIXMAN_OP_OVER, */
    /* 	src->image, */
    /* 	NULL, */
    /* 	dst->image, */
    /* 	// src_x, src_y */
    /* 	sox, */
    /* 	soy, */
    /* 	// mask_x, mask_y */
    /* 	0, */
    /* 	0, */
    /* 	// dest_x, dest_y, width, height */
    /* 	dox, */
    /* 	doy, */
    /* 	dst->w, */
    /* 	dst->h); */
}

void bm_argb_blend_with_alpha_mask(bm_argb_t* dst, bm_argb_t* src, int sx, int sy, int mask)
{
    if (mask == 255) return;

    printf("mask %i\n", mask);

    if (sx > dst->w) return;
    if (sy > dst->h) return;
    if (sx + src->w < 0) return;
    if (sy + src->h < 0) return;

    int sox = 0; // src offset
    int soy = 0;
    int dox = sx; // dst offset
    int doy = sy;

    if (sx < 0)
    {
	dox = 0;
	sox = -sx;
    }
    if (sy < 0)
    {
	doy = 0;
	soy = -sy;
    }

    int dex = sx + src->w; // dst endpoints
    int dey = sy + src->h;

    if (dex >= dst->w) dex = dst->w;
    if (dey >= dst->h) dey = dst->h;

    uint32_t* d  = (uint32_t*) dst->data; // dest bytes
    uint32_t* s  = (uint32_t*) src->data; // source byes
    uint32_t* td = (uint32_t*) dst->data; // temporary dst bytes
    uint32_t* ts = (uint32_t*) src->data; // temporary src bytes

    d += (doy * dst->w) + dox;
    s += (soy * src->w) + sox;

    for (int y = doy; y < dey; y++)
    {
	td = d;
	ts = s;

	for (int x = dox; x < dex; x++)
	{
	    static const int AMASK    = 0xFF000000;
	    static const int RBMASK   = 0x00FF00FF;
	    static const int GMASK    = 0x0000FF00;
	    static const int AGMASK   = AMASK | GMASK;
	    static const int ONEALPHA = 0x01000000;

	    uint32_t dc = *td;
	    uint32_t sc = *ts;
	    int      sa = ((sc & AMASK) >> 24) - mask;

	    if (sa == 256)
	    {
		*td = sc; // dest color is source color
	    }
	    else if (sa > 0)
	    {
		unsigned int na = 255 - sa;
		unsigned int rb = ((na * (dc & RBMASK)) + (sa * (sc & RBMASK))) >> 8;
		unsigned int ag = (na * ((dc & AGMASK) >> 8)) + (sa * (ONEALPHA | ((sc & GMASK) >> 8)));
		*td             = ((rb & RBMASK) | (ag & AGMASK));
	    }
	    // else dst remains the same

	    td++;
	    ts++;
	}
	d += dst->w;
	s += src->w;
    }
}

void bm_argb_describe(void* p, int level)
{
    bm_argb_t* bm = p;
    printf("width %i height %i size %u", bm->w, bm->h, bm->size);
}

void bm_argb_describe_data(void* p, int level)
{
    printf("bm data\n");
}

#endif
