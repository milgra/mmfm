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

#include "zc_vector.c"
#include <stdio.h>

struct evrec_t
{
    FILE*    file;
    vec_t*   events;
    int      index;
    uint32_t delay;
    uint32_t lasttime; // last event's timestamp
    uint32_t normtime; // normalized time
} rec = {0};

void evrec_init_recorder(char* path)
{
    FILE* file = fopen(path, "w"); // CLOSE 0
    if (!file) printf("evrec recorder : cannot open file %s\n", path);
    rec.file = file;
}

void evrec_init_player(char* path)
{
    FILE* file = fopen(path, "r");
    if (!file) printf("evrec player : cannot open file %s\n", path);

    rec.file   = file;
    rec.events = VNEW(); // REL 0

    char       line[1000] = {0};
    char       type[100]  = {0};
    char       done       = 1;
    ku_event_t ev         = {0};

    while (1)
    {

	if (fgets(line, 1000, file) != NULL)
	{
	    if (done)
	    {
		sscanf(line, "%u %s", &ev.time, type);

		done = 0;
	    }
	    else
	    {
		if (strcmp(type, "mmove") == 0) sscanf(line, "%i %i %f %f %i\n", &ev.x, &ev.y, &ev.dx, &ev.dy, &ev.drag);
		if (strcmp(type, "mdown") == 0) sscanf(line, "%i %i %i %i %i %i\n", &ev.x, &ev.y, &ev.button, &ev.dclick, &ev.ctrl_down, &ev.shift_down);
		if (strcmp(type, "mup") == 0) sscanf(line, "%i %i %i %i %i %i\n", &ev.x, &ev.y, &ev.button, &ev.dclick, &ev.ctrl_down, &ev.shift_down);
		if (strcmp(type, "scroll") == 0) sscanf(line, "%f %f\n", &ev.dx, &ev.dy);
		if (strcmp(type, "kdown") == 0) sscanf(line, "%i\n", &ev.keycode);
		if (strcmp(type, "kup") == 0) sscanf(line, "%i\n", &ev.keycode);
		if (strcmp(type, "text") == 0) memcpy(ev.text, line, strlen(line) - 1);
		if (strcmp(type, "resize") == 0) sscanf(line, "%i %i\n", &ev.w, &ev.h);

		if (strcmp(type, "mmove") == 0) ev.type = KU_EVENT_MMOVE;
		if (strcmp(type, "mdown") == 0) ev.type = KU_EVENT_MDOWN;
		if (strcmp(type, "mup") == 0) ev.type = KU_EVENT_MUP;
		if (strcmp(type, "scroll") == 0) ev.type = KU_EVENT_SCROLL;
		if (strcmp(type, "kdown") == 0) ev.type = KU_EVENT_KDOWN;
		if (strcmp(type, "kup") == 0) ev.type = KU_EVENT_KUP;
		if (strcmp(type, "text") == 0) ev.type = KU_EVENT_TEXT;
		if (strcmp(type, "resize") == 0) ev.type = KU_EVENT_RESIZE;

		VADDR(rec.events, HEAP(ev));

		done = 1;
	    }
	}
	else
	    break;
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
    // normalize time to skip inactive parts of the test
    if (rec.lasttime > 0 && ev.time > rec.lasttime + 1000) rec.delay += ev.time - rec.lasttime;
    rec.lasttime = ev.time;
    rec.normtime = ev.time - rec.delay;

    if (ev.type == KU_EVENT_MMOVE) fprintf(rec.file, "%u mmove\n%i %i %f %f %i\n", rec.normtime, ev.x, ev.y, ev.dx, ev.dy, ev.drag);
    if (ev.type == KU_EVENT_MDOWN) fprintf(rec.file, "%u mdown\n%i %i %i %i %i %i\n", rec.normtime, ev.x, ev.y, ev.button, ev.dclick, ev.ctrl_down, ev.shift_down);
    if (ev.type == KU_EVENT_MUP) fprintf(rec.file, "%u mup\n%i %i %i %i %i %i\n", rec.normtime, ev.x, ev.y, ev.button, ev.dclick, ev.ctrl_down, ev.shift_down);
    if (ev.type == KU_EVENT_SCROLL) fprintf(rec.file, "%u scroll\n%f %f\n", rec.normtime, ev.dx, ev.dy);
    if (ev.type == KU_EVENT_KDOWN) fprintf(rec.file, "%u kdown\n%i\n", rec.normtime, ev.keycode);
    if (ev.type == KU_EVENT_KUP) fprintf(rec.file, "%u kup\n%i\n", rec.normtime, ev.keycode);
    if (ev.type == KU_EVENT_TEXT) fprintf(rec.file, "%u text\n%s\n", rec.normtime, ev.text);
    if (ev.type == KU_EVENT_RESIZE) fprintf(rec.file, "%u resize\n%i %i\n", rec.normtime, ev.w, ev.h);
}

ku_event_t* evrec_replay(uint32_t time)
{
    if (rec.index < rec.events->length)
    {
	ku_event_t* event = rec.events->data[rec.index];

	// printf("time %u event time %u event type %i\n", time, event->time, event->type);

	if (event->time < time)
	{
	    rec.index++;
	    return event;
	}
    }

    return NULL;
}

#endif
