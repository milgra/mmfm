#ifndef pdf_h
#define pdf_h

#include "ku_bitmap.c"

int          pdf_count(char* filename);
ku_bitmap_t* pdf_render(char* filename, int page);

#endif

#if __INCLUDE_LEVEL__ == 0

#include <mupdf/fitz.h>

int pdf_count(char* filename)
{
    char*        input;
    float        zoom, rotate;
    int          page_number, page_count;
    fz_context*  ctx;
    fz_document* doc;

    /* if (argc < 3) */
    /* { */
    /*   fprintf(stderr, "usage: example input-file page-number [ zoom [ rotate ] ]\n"); */
    /*   fprintf(stderr, "\tinput-file: path of PDF, XPS, CBZ or EPUB document to open\n"); */
    /*   fprintf(stderr, "\tPage numbering starts from one.\n"); */
    /*   fprintf(stderr, "\tZoom level is in percent (100 percent is 72 dpi).\n"); */
    /*   fprintf(stderr, "\tRotation is in degrees clockwise.\n"); */
    /*   return EXIT_FAILURE; */
    /* } */

    input       = filename;
    page_number = 0;
    zoom        = 200.0;
    rotate      = 0.0;

    /* Create a context to hold the exception stack and various caches. */
    ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
    if (!ctx)
    {
	fprintf(stderr, "cannot create mupdf context\n");
    }

    /* Register the default file types to handle. */
    fz_try(ctx)
	fz_register_document_handlers(ctx);
    fz_catch(ctx)
    {
	fprintf(stderr, "cannot register document handlers: %s\n", fz_caught_message(ctx));
	fz_drop_context(ctx);
    }

    /* Open the document. */
    fz_try(ctx)
	doc = fz_open_document(ctx, input);
    fz_catch(ctx)
    {
	fprintf(stderr, "cannot open document: %s\n", fz_caught_message(ctx));
	fz_drop_context(ctx);
    }

    /* Count the number of pages. */
    fz_try(ctx)
	page_count = fz_count_pages(ctx, doc);
    fz_catch(ctx)
    {
	fprintf(stderr, "cannot count number of pages: %s\n", fz_caught_message(ctx));
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

    /* if (argc < 3) */
    /* { */
    /*   fprintf(stderr, "usage: example input-file page-number [ zoom [ rotate ] ]\n"); */
    /*   fprintf(stderr, "\tinput-file: path of PDF, XPS, CBZ or EPUB document to open\n"); */
    /*   fprintf(stderr, "\tPage numbering starts from one.\n"); */
    /*   fprintf(stderr, "\tZoom level is in percent (100 percent is 72 dpi).\n"); */
    /*   fprintf(stderr, "\tRotation is in degrees clockwise.\n"); */
    /*   return EXIT_FAILURE; */
    /* } */

    input  = filename;
    zoom   = 200.0;
    rotate = 0.0;

    /* Create a context to hold the exception stack and various caches. */
    ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
    if (!ctx)
    {
	fprintf(stderr, "cannot create mupdf context\n");
    }

    /* Register the default file types to handle. */
    fz_try(ctx)
	fz_register_document_handlers(ctx);
    fz_catch(ctx)
    {
	fprintf(stderr, "cannot register document handlers: %s\n", fz_caught_message(ctx));
	fz_drop_context(ctx);
    }

    /* Open the document. */
    fz_try(ctx)
	doc = fz_open_document(ctx, input);
    fz_catch(ctx)
    {
	fprintf(stderr, "cannot open document: %s\n", fz_caught_message(ctx));
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
	fprintf(stderr, "cannot render page: %s\n", fz_caught_message(ctx));
	fz_drop_document(ctx, doc);
	fz_drop_context(ctx);
    }

    /* Print image data in ascii PPM format. */
    printf("P3\n");
    printf("w %d h %d s %td\n", pix->w, pix->h, pix->stride);
    printf("255\n");

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
	    /* if (x > 0) */
	    /*   printf("  "); */
	    /* if (p[0] < 255) */
	    /*   printf("%3d %3d %3d", p[0], p[1], p[2]); */
	    p += pix->n;
	}
	// printf("\n");
    }

    /* Clean up. */
    fz_drop_pixmap(ctx, pix);
    fz_drop_document(ctx, doc);
    fz_drop_context(ctx);

    return res;
}

#endif
