#ifndef db_h
#define db_h

#include "zc_map.c"

void db_init();
void db_destroy();
void db_read(char* libpath);
void db_write(char* libpath);
void db_add_entry(char* path, map_t* entry);
void db_remove_entry(map_t* entry);
void db_update(map_t* map);

int db_organize_entry(char* libpath, map_t* db, map_t* entry);
int db_organize(char* libpath, map_t* db);

void db_get_genres(vec_t* vec);
void db_get_artists(vec_t* vec);

map_t*   db_get_db();
uint32_t db_count();
void     db_reset();

void db_update_metadata(char* path, map_t* changed, vec_t* removed);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "kvlist.c"
#include "library.c"
#include "zc_cstring.c"
#include "zc_cstrpath.c"
#include "zc_log.c"
#include "zc_vector.c"
#include <ctype.h>
#include <limits.h>

map_t* db;

void db_init()
{
  db = MNEW(); // REL 0
}

void db_destroy()
{
  // mem_describe(db, 0);

  REL(db); // REL 1
}

void db_read(char* libpath)
{
  assert(libpath != NULL);

  char* dbpath = cstr_new_path_append(libpath, "zenmusic.kvl"); // REL 0

  LOG("db : reading db %s", dbpath);

  kvlist_read(dbpath, db, "file/path");

  LOG("db : loaded, entries : %i", db->count);

  REL(dbpath); // REL 0
}

void db_write(char* libpath)
{
  assert(libpath != NULL);

  char* dbpath = cstr_new_format(PATH_MAX + NAME_MAX, "/%s/zenmusic.kvl", libpath); // REL 0

  int res = kvlist_write(dbpath, db);

  if (res < 0) LOG("ERROR db_write cannot write database %s\n", dbpath);

  LOG("db : written");

  REL(dbpath); // REL 0
}

void db_add_entry(char* path, map_t* entry)
{
  MPUT(db, path, entry);
}

void db_remove_entry(map_t* entry)
{
  char* path = MGET(entry, "file/path");
  MDEL(db, path);
}

void db_reset()
{
  map_reset(db);
}

map_t* db_get_db()
{
  return db;
}

uint32_t db_count()
{
  return db->count;
}

void db_update(map_t* files)
{
  // first remove deleted files from db
  vec_t* paths = VNEW(); // REL 0
  map_keys(db, paths);

  for (int index = 0; index < paths->length; index++)
  {
    char*  path = paths->data[index];
    map_t* map  = MGET(files, path);
    if (!map)
    {
      // db path is missing from file path, file was removed
      MDEL(db, path);
      printf("LOG file is missing for path %s, song entry was removed from db\n", path);
    }
  }

  // go through lib paths
  vec_reset(paths);
  map_keys(files, paths);

  for (int index = 0; index < paths->length; index++)
  {
    char*  path = paths->data[index];
    map_t* map  = MGET(db, path);
    if (map)
    {
      // path exist in db, removing entry from files
      MDEL(files, path);
    }
  }

  REL(paths); // REL 0
}

char* db_replace_char(char* str, char find, char replace)
{
  char* current_pos = strchr(str, find);
  while (current_pos)
  {
    *current_pos = replace;
    current_pos  = strchr(current_pos + 1, find);
  }
  return str;
}

int db_organize_entry(char* libpath, map_t* db, map_t* entry)
{
  assert(libpath != NULL);

  int changed = 0;

  char* path   = MGET(entry, "file/path");
  char* artist = MGET(entry, "meta/artist");
  char* album  = MGET(entry, "meta/album");
  char* title  = MGET(entry, "meta/title");
  char* track  = MGET(entry, "meta/track");

  // remove slashes before directory creation

  db_replace_char(artist, '/', ' ');
  db_replace_char(title, '/', ' ');

  // get extension

  char* ext = cstr_new_path_extension(path); // REL -1

  char* old_path     = cstr_new_format(PATH_MAX + NAME_MAX, "%s/%s", libpath, path);              // REL 0
  char* new_dirs     = cstr_new_format(PATH_MAX + NAME_MAX, "%s/%s/%s/", libpath, artist, album); // REL 1
  char* new_path     = NULL;
  char* new_path_rel = NULL;

  if (track)
  {
    int trackno  = atoi(track);
    new_path     = cstr_new_format(PATH_MAX + NAME_MAX, "%s/%s/%s/%.3i %s.%s", libpath, artist, album, trackno, title, ext); // REL 2
    new_path_rel = cstr_new_format(PATH_MAX + NAME_MAX, "%s/%s/%.3i %s.%s", artist, album, trackno, title, ext);             // REL 3
  }
  else
  {
    new_path     = cstr_new_format(PATH_MAX + NAME_MAX, "%s/%s/%s/%s.%s", libpath, artist, album, title, ext); // REL 2
    new_path_rel = cstr_new_format(PATH_MAX + NAME_MAX, "%s/%s/%s.%s", artist, album, title, ext);             // REL 3
  }

  if (strcmp(old_path, new_path) != 0)
  {
    int error = lib_rename_file(old_path, new_path, new_dirs);
    if (error == 0)
    {
      LOG("db : updating path");
      MPUT(entry, "file/path", new_path_rel);
      MPUT(db, new_path_rel, entry);
      MDEL(db, path);
      changed = 1;
    }
  }

  REL(ext);          // REL -1
  REL(old_path);     // REL 0
  REL(new_dirs);     // REL 1
  REL(new_path);     // REL 2
  REL(new_path_rel); // REL 3

  return 0;
}

int db_organize(char* libpath, map_t* db)
{
  LOG("db : organizing database");

  // go through all db entries, check path, move if needed

  int    changed = 0;
  vec_t* paths   = VNEW(); // REL 0

  map_keys(db, paths);

  for (int index = 0; index < paths->length; index++)
  {
    char*  path  = paths->data[index];
    map_t* entry = MGET(db, path);

    if (entry)
    {
      changed |= db_organize_entry(libpath, db, entry);
    }
  }

  REL(paths); // REL 0

  return changed;
}

int db_comp_text(void* left, void* right)
{
  char* la = left;
  char* ra = right;

  return strcmp(la, ra);
}

void db_get_genres(vec_t* vec)
{
  int ei, gi; // entry, genre index

  vec_t* songs  = VNEW(); // REL 0
  map_t* genres = MNEW(); // REL 1

  map_values(db, songs);

  for (ei = 0;
       ei < songs->length;
       ei++)
  {
    map_t* entry = songs->data[ei];
    char*  genre = MGET(entry, "meta/genre");

    if (genre)
    {
      MPUT(genres, genre, genre);
    }
  }

  map_values(genres, vec);
  vec_sort(vec, VSD_ASC, db_comp_text);

  REL(genres); // REL 1
  REL(songs);  // REL 0
}

void db_get_artists(vec_t* vec)
{
  int ei;

  vec_t* songs   = VNEW(); // REL 0
  map_t* artists = MNEW(); // REL 1

  map_values(db, songs);

  for (ei = 0;
       ei < songs->length;
       ei++)
  {
    map_t* entry  = songs->data[ei];
    char*  artist = MGET(entry, "meta/artist");

    if (artist) MPUT(artists, artist, artist);
  }

  map_values(artists, vec);
  vec_sort(vec, VSD_ASC, db_comp_text);

  REL(artists); // REL 1
  REL(songs);   // REL 0
}

void db_update_metadata(char* path, map_t* changed, vec_t* removed)
{
  map_t* song = MGET(db, path);
  vec_t* keys = VNEW(); // REL 0
  map_keys(changed, keys);

  // update changed

  for (int index = 0; index < keys->length; index++)
  {
    char* key = keys->data[index];
    MPUT(song, key, MGET(changed, key));
  }

  // remove removed

  for (int index = 0; index < removed->length; index++)
  {
    char* field = keys->data[index];
    MDEL(song, field);
  }

  REL(keys); // REL 0
}

#endif
