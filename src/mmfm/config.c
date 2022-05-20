#ifndef config_h
#define config_h

#include "zc_map.c"

void  config_init();
void  config_destroy();
void  config_read(char* path);
void  config_write(char* path);
void  config_set(char* key, char* value);
char* config_get(char* key);
int   config_get_int(char* key);
int   config_get_bool(char* key);
void  config_set_bool(char* key, int val);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "filemanager.c"
#include "kvlist.c"
#include "zc_cstring.c"
#include "zc_log.c"
#include "zc_path.c"
#include <limits.h>

map_t* confmap;

void config_init()
{
  confmap = MNEW(); // REL 0
}

void config_destroy()
{
  REL(confmap); // REL 0
}

void config_read(char* path)
{
  map_t* data = MNEW(); // REL 0

  kvlist_read(path, data, "id");

  map_t* cfdb = MGET(data, "config");

  if (cfdb)
  {
    vec_t* keys = VNEW(); // REL 1
    map_keys(cfdb, keys);

    for (int index = 0; index < keys->length; index++)
    {
      char* key = keys->data[index];
      MPUT(confmap, key, MGET(cfdb, key));
    }

    REL(keys);

    printf("config loaded from %s, entries : %i\n", path, confmap->count);
  }

  REL(data); // REL 0
}

void config_write(char* path)
{
  map_t* data    = MNEW();                               // REL 0
  char*  dirpath = path_new_remove_last_component(path); // REL 1

  printf("CONFIG DIR %s FILE %s\n", dirpath, path);

  MPUTR(confmap, "id", cstr_new_cstring("config")); // put id in config db
  MPUT(data, "id", confmap);                        // put config db in final data with same id

  int error = fm_create(dirpath, 0777);

  if (error == 0)
  {
    int res = kvlist_write(path, data);
    if (res < 0)
      zc_log_error("ERROR config_write cannot write config\n");
    else
      zc_log_info("config written");
  }
  else
    zc_log_error("ERROR config_write cannot create config path\n");

  REL(dirpath); // REL 1
  REL(data);    // REL 0
}

void config_set(char* key, char* value)
{
  char* str = cstr_new_cstring(value); // REL 0
  MPUT(confmap, key, str);
  REL(str); // REL 0
}

char* config_get(char* key)
{
  return MGET(confmap, key);
}

int config_get_bool(char* key)
{
  char* val = MGET(confmap, key);
  if (val && strcmp(val, "true") == 0)
    return 1;
  else
    return 0;
}

int config_get_int(char* key)
{
  char* val = MGET(confmap, key);
  if (val)
    return atoi(val);
  else
    return 0;
}

void config_set_bool(char* key, int val)
{
  if (val)
  {
    MPUTR(confmap, key, cstr_new_cstring("true"));
  }
  else
  {
    MPUTR(confmap, key, cstr_new_cstring("false"));
  }
}

#endif
