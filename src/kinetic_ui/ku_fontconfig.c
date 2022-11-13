#ifndef ku_fontconfig_h
#define ku_fontconfig_h

char* ku_fontconfig_path(char* face_desc);
void  ku_fontconfig_delete();

#endif

#if __INCLUDE_LEVEL__ == 0

#define _POSIX_C_SOURCE 200112L
#include "zc_cstring.c"
#include "zc_map.c"
#include "zc_memory.c"
#include <limits.h>
#include <linux/limits.h>
#include <stdio.h>
#include <string.h>

map_t* ku_fontconfig_cache = NULL;

char* ku_fontconfig_path(char* face_desc)
{
    char* filename = NULL;
    if (face_desc)
    {
	if (ku_fontconfig_cache == NULL) ku_fontconfig_cache = MNEW();
	filename = MGET(ku_fontconfig_cache, face_desc);

	if (filename == NULL)
	{
	    char buff[PATH_MAX];
	    filename      = cstr_new_cstring("");                                                // REL 0
	    char* command = cstr_new_format(80, "fc-match \"%s\" --format=%%{file}", face_desc); // REL 1
	    FILE* pipe    = popen(command, "r");                                                 // CLOSE 0
	    while (fgets(buff, sizeof(buff), pipe) != NULL) filename = cstr_append(filename, buff);
	    pclose(pipe); // CLOSE 0
	    REL(command); // REL 1

	    MPUTR(ku_fontconfig_cache, face_desc, filename);
	}
    }
    return filename;
}

void ku_fontconfig_delete()
{
    if (ku_fontconfig_cache) REL(ku_fontconfig_cache);
}

#endif
