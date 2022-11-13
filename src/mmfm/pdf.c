#ifndef pdf_h
#define pdf_h

#include "ku_bitmap.c"

int          pdf_count(char* filename);
ku_bitmap_t* pdf_render(char* filename, int page);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "mt_log.c"
#include <mupdf/fitz.h>

int pdf_count(char* filename)
{
    char*        input;
    float        zoom, rotate;
    int          page_number, page_count;
    fz_context*  ctx;
    fz_document* doc;

    input       = filename;
    page_number = 0;
    zoom        = 200.0;
    rotate      = 0.0;

    /* Create a context to hold the exception stack and various caches. */
    ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
    if (!ctx) mt_log_error("cannot create mupdf context\n");

    /* Register the default file types to handle. */
    fz_try(ctx)
	fz_register_document_handlers(ctx);
    fz_catch(ctx)
    {
	mt_log_error("cannot register document handlers: %s", fz_caught_message(ctx));
	fz_drop_context(ctx);
    }

    /* Open the document. */
    fz_try(ctx)
	doc = fz_open_document(ctx, input);
    fz_catch(ctx)
    {
	mt_log_error("cannot open document: %s", fz_caught_message(ctx));
	fz_drop_context(ctx);
    }

    /* Count the number of pages. */
    fz_try(ctx)
	page_count = fz_count_pages(ctx, doc);
    fz_catch(ctx)
    {
	mt_log_error("cannot count number of pages: %s", fz_caught_message(ctx));
	fz_drop_document(ctx, doc);
	fz_drop_context(ctx);
    }

    return page_count;
}

ku_bitmap_t* pdf_render(char* filename, int page_number)
{
    char*        input;
    float        zoom, rotate;
    fz_context*  ctx;
    fz_document* doc;
    fz_pixmap*   pix;
    fz_matrix    ctm;
    int          x, y;

    input  = filename;
    zoom   = 200.0;
    rotate = 0.0;

    /* Create a context to hold the exception stack and various caches. */
    ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
    if (!ctx) mt_log_error("cannot create mupdf context");

    /* Register the default file types to handle. */
    fz_try(ctx)
	fz_register_document_handlers(ctx);
    fz_catch(ctx)
    {
	mt_log_error("cannot register document handlers: %s", fz_caught_message(ctx));
	fz_drop_context(ctx);
    }

    /* Open the document. */
    fz_try(ctx)
	doc = fz_open_document(ctx, input);
    fz_catch(ctx)
    {
	mt_log_error("cannot open document: %s", fz_caught_message(ctx));
	fz_drop_context(ctx);
    }

    /* Compute a transformation matrix for the zoom and rotation desired. */
    /* The default resolution without scaling is 72 dpi. */
    ctm = fz_scale(zoom / 100, zoom / 100);
    ctm = fz_pre_rotate(ctm, rotate);

    /* Render page to an RGB pixmap. */
    fz_try(ctx)
	pix = fz_new_pixmap_from_page_number(ctx, doc, page_number, ctm, fz_device_rgb(ctx), 0);
    fz_catch(ctx)
    {
	mt_log_error("cannot render page: %s", fz_caught_message(ctx));
	fz_drop_document(ctx, doc);
	fz_drop_context(ctx);
    }

    ku_bitmap_t* res  = ku_bitmap_new(pix->w, pix->h);
    uint8_t*     data = res->data;

    for (y = 0; y < pix->h; ++y)
    {
	unsigned char* p = &pix->samples[y * pix->stride];
	for (x = 0; x < pix->w; ++x)
	{
	    data[0] = p[0];
	    data[1] = p[1];
	    data[2] = p[2];
	    data[3] = 255;
	    data += 4;
	    p += pix->n;
	}
    }

    /* Clean up. */
    fz_drop_pixmap(ctx, pix);
    fz_drop_document(ctx, doc);
    fz_drop_context(ctx);

    return res;
}

#endif
