/* Kinetic UI bitmap */
#ifndef ku_bitmap_h
#define ku_bitmap_h

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// TODO use this

enum bm_format
{
    KU_BITMAP_ARGB,
    KU_BITMAP_RGBA
};

typedef struct _bmr_t bmr_t;
struct _bmr_t
{
    int x;
    int y;
    int z;
    int w;
};

typedef struct _bm_t bm_t;
struct _bm_t
{
    int w;
    int h;

    uint8_t*       data;
    uint32_t       size;
    enum bm_format type;
};

bm_t* bm_new(int the_w, int the_h);
bm_t* bm_new_clone(bm_t* bm);
void  bm_reset(bm_t* bm);
void  bm_describe(void* p, int level);
void  bm_insert(bm_t* dst, bm_t* src, int sx, int sy, int mx, int my, int mw, int mh);
void  bm_blend(bm_t* dst, bm_t* src, int sx, int sy, int mx, int my, int mw, int mh);
void  bm_blend_with_alpha_mask(bm_t* dst, bm_t* src, int sx, int sy, int mx, int my, int mw, int mh, int mask);
void  bm_blend_rect(bm_t* dst, int x, int y, int w, int h, uint32_t c);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "zc_memory.c"
#include <assert.h>
#include <string.h>

#define BMMIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define BMMAX(X, Y) (((X) > (Y)) ? (X) : (Y))

void bm_describe_data(void* p, int level);

void bm_del(void* pointer)
{
    bm_t* bm = pointer;

    if (bm->data != NULL) REL(bm->data); // REL 1
}

bm_t* bm_new(int the_w, int the_h)
{
    assert(the_w > 0 && the_h > 0);

    bm_t* bm = CAL(sizeof(bm_t), bm_del, bm_describe); // REL 0

    bm->w = the_w;
    bm->h = the_h;

    bm->size = 4 * the_w * the_h;
    bm->data = CAL(bm->size * sizeof(unsigned char), NULL, bm_describe_data); // REL 1
    bm->type = KU_BITMAP_ARGB;

    return bm;
}

bm_t* bm_new_clone(bm_t* the_bm)
{
    bm_t* bm = bm_new(the_bm->w, the_bm->h);
    memcpy(bm->data, the_bm->data, the_bm->size);
    return bm;
}

void bm_reset(bm_t* bm)
{
    memset(bm->data, 0, bm->size);
}

bmr_t bm_is(bmr_t l, bmr_t r)
{
    bmr_t f = {0};
    if (!(l.z < r.x || r.z < l.x || l.w < r.y || r.w < l.y))
    {
	f.x = BMMAX(l.x, r.x);
	f.y = BMMAX(l.y, r.y);
	f.z = BMMIN(r.z, l.z);
	f.w = BMMIN(r.w, l.w);
    }

    return f;
}

void bm_insert(bm_t* dst, bm_t* src, int sx, int sy, int mx, int my, int mw, int mh)
{
    bmr_t dr = {0, 0, dst->w, dst->h};
    bmr_t sr = {sx, sy, sx + src->w, sy + src->h};
    bmr_t mr = {mx, my, mx + mw, my + mh};
    bmr_t f  = bm_is(sr, mr);

    if (f.x == 0 && f.w == 0) return;
    if (f.y == 0 && f.z == 0) return;

    f = bm_is(f, dr);

    int sox = f.x - sx; // src offset
    int soy = f.y - sy;

    int dox = f.x; // dst offset
    int doy = f.y;

    int dex = f.z; // dst endpoints
    int dey = f.w;

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

void bm_blend(bm_t* dst, bm_t* src, int sx, int sy, int mx, int my, int mw, int mh)
{
    bmr_t dr = {0, 0, dst->w, dst->h};
    bmr_t sr = {sx, sy, sx + src->w, sy + src->h};
    bmr_t mr = {mx, my, mx + mw, my + mh};
    bmr_t f  = bm_is(sr, mr);

    if (f.x == 0 && f.w == 0) return;
    if (f.y == 0 && f.z == 0) return;

    f = bm_is(f, dr);

    int sox = f.x - sx; // src offset
    int soy = f.y - sy;

    int dox = f.x; // dst offset
    int doy = f.y;

    int dex = f.z; // dst endpoints
    int dey = f.w;

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
}

void bm_blend_with_alpha_mask(bm_t* dst, bm_t* src, int sx, int sy, int mx, int my, int mw, int mh, int mask)
{
    if (mask == 255) return;

    bmr_t dr = {0, 0, dst->w, dst->h};
    bmr_t sr = {sx, sy, sx + src->w, sy + src->h};
    bmr_t mr = {mx, my, mx + mw, my + mh};
    bmr_t f  = bm_is(sr, mr);

    if (f.x == 0 && f.w == 0) return;
    if (f.y == 0 && f.z == 0) return;

    f = bm_is(f, dr);

    int sox = f.x - sx; // src offset
    int soy = f.y - sy;

    int dox = f.x; // dst offset
    int doy = f.y;

    int dex = f.z; // dst endpoints
    int dey = f.w;

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

void bm_blend_rect(bm_t* dst, int x, int y, int w, int h, uint32_t c)
{
    if (x > dst->w) return;
    if (y > dst->h) return;
    if (x + w < 0) return;
    if (y + h < 0) return;

    int dox = x; // dst offset
    int doy = y;

    if (x < 0)
    {
	dox = 0;
    }
    if (y < 0)
    {
	doy = 0;
    }

    int dex = x + w; // dst endpoints
    int dey = y + h;

    if (dex >= dst->w) dex = dst->w;
    if (dey >= dst->h) dey = dst->h;

    uint32_t* d  = (uint32_t*) dst->data; // dest bytes
    uint32_t* td = (uint32_t*) dst->data; // temporary dst bytes

    d += (doy * dst->w) + dox;

    for (int y = doy; y < dey; y++)
    {
	td = d;

	for (int x = dox; x < dex; x++)
	{
	    static const int AMASK    = 0xFF000000;
	    static const int RBMASK   = 0x00FF00FF;
	    static const int GMASK    = 0x0000FF00;
	    static const int AGMASK   = AMASK | GMASK;
	    static const int ONEALPHA = 0x01000000;

	    uint32_t dc = *td;
	    uint32_t sc = c;
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
	}
	d += dst->w;
    }
}

void bm_describe(void* p, int level)
{
    bm_t* bm = p;
    printf("width %i height %i size %u", bm->w, bm->h, bm->size);
}

void bm_describe_data(void* p, int level)
{
    printf("bm data\n");
}

#endif
