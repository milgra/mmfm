#ifndef zc_callbacks_h
#define zc_callbacks_h

#include "zc_callback.c"

void   callbacks_init();
void   callbacks_destroy();
void   callbacks_set(char* id, cb_t* cb);
cb_t*  callbacks_get(char* id);
void   callbacks_call(char* id, void* data);
map_t* callbacks_get_data();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "zc_log.c"
#include "zc_map.c"

map_t* callbacks;

void callbacks_init()
{
  callbacks = MNEW(); // REL 0
}

void callbacks_destroy()
{
  printf("callbacks destroy\n");
  REL(callbacks); // REL 0
}

void callbacks_set(char* id, cb_t* cb)
{
  MPUT(callbacks, id, cb);
}

cb_t* callbacks_get(char* id)
{
  return MGET(callbacks, id);
}

void callbacks_call(char* id, void* data)
{
  cb_t* cb = MGET(callbacks, id);
  if (cb)
    (*cb->fp)(cb->userdata, data);
  else
    LOG("ERROR callback %s doesn't exist", id);
}

map_t* callbacks_get_data()
{
  return callbacks;
}

#endif
