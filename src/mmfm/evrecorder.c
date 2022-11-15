#ifndef evrec_h
#define evrec_h

#include "ku_event.c"

void        evrec_init_recorder(char* path);
void        evrec_init_player(char* path);
void        evrec_destroy();
void        evrec_record(ku_event_t event);
ku_event_t* evrec_replay(uint32_t time);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "mt_path.c"
#include "mt_vector.c"
#include <stdio.h>

struct evrec_t
{
    FILE*        file;
    int          index;
    mt_vector_t* events;
} rec = {0};

void evrec_init_recorder(char* path)
{
    char* newpath = mt_path_new_append(path, "session.rec");
    FILE* file    = fopen(newpath, "w"); // CLOSE 0
    if (!file) printf("evrec recorder : cannot open file %s\n", newpath);
    rec.file = file;
    REL(newpath);
}

void evrec_init_player(char* path)
{
    char* newpath = mt_path_new_append(path, "session.rec");

    FILE* file = fopen(newpath, "r");
    if (!file) printf("evrec player : cannot open file %s\n", path);

    rec.file   = file;
    rec.events = VNEW(); // REL 0

    while (1)
    {
	ku_event_t ev = ku_event_read(file);
	VADDR(rec.events, HEAP(ev));
	if (feof(file)) break;
    }

    printf("%i events read\n", rec.events->length);
}

void evrec_destroy()
{
    fclose(rec.file);                // CLOSE 0
    if (rec.events) REL(rec.events); // REL 0
}

void evrec_record(ku_event_t ev)
{
    ku_event_write(rec.file, ev);
}

ku_event_t* evrec_replay(uint32_t frame)
{
    if (rec.index < rec.events->length)
    {
	ku_event_t* event = rec.events->data[rec.index];

	if (event->frame <= frame)
	{
	    rec.index++;
	    return event;
	}
    }

    return NULL;
}

#endif
