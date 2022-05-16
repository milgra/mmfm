#ifndef lib_h
#define lib_h

#include "zc_channel.c"
#include "zc_map.c"

void lib_read_files(char* libpath, map_t* db);
void lib_delete_file(char* libpath, map_t* entry);
int  lib_rename_file(char* old, char* new, char* new_dirs);
void lib_analyze_files(ch_t* channel, map_t* files);

#endif

#if __INCLUDE_LEVEL__ == 0

#define __USE_XOPEN_EXTENDED 1 // needed for linux
#include <ftw.h>

#include "coder.c"
#include "files.c"
#include "zc_cstring.c"
#include "zc_cstrpath.c"
#include "zc_log.c"
#include <errno.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

static int lib_file_data_step(const char* fpath, const struct stat* sb, int tflag, struct FTW* ftwbuf);
int        analyzer_thread(void* chptr);

struct lib_t
{
  map_t* files;
  vec_t* paths;
  char   lock;
  char*  path;
} lib = {0};

void lib_read_files(char* lib_path, map_t* files)
{
  assert(lib_path != NULL);

  lib.files = files;
  lib.path  = lib_path;

  map_t* parent = MNEW();

  MPUTR(parent, "type", cstr_new_format(5, "%s", "d"));
  MPUTR(parent, "path", cstr_new_format(PATH_MAX + NAME_MAX, "%s", ".."));

  nftw(lib_path, lib_file_data_step, 20, FTW_PHYS);

  MPUT(files, "..", parent); // use relative path as path

  LOG("lib : scanned, files : %i", files->count);
}

static int lib_file_data_step(const char* fpath, const struct stat* sb, int tflag, struct FTW* ftwinfo)
{
  /* printf("%-3s %2d %7jd   %-40s %d %s\n", */
  /*        (tflag == FTW_D) ? "d" : (tflag == FTW_DNR) ? "dnr" : (tflag == FTW_DP) ? "dp" : (tflag == FTW_F) ? "f" : (tflag == FTW_NS) ? "ns" : (tflag == FTW_SL) ? "sl" : (tflag == FTW_SLN) ? "sln" : "???", */
  /*        ftwinfo->level, */
  /*        (intmax_t)sb->st_size, */
  /*        fpath, */
  /*        ftwinfo->base, */
  /*        fpath + ftwinfo->base); */

  if (ftwinfo->level > 1) return 0;

  map_t* file = MNEW();

  MPUTR(file, "type", cstr_new_format(5, "%s", (tflag == FTW_D) ? "d" : (tflag == FTW_DNR) ? "dnr"
                                                                    : (tflag == FTW_DP)    ? "dp"
                                                                    : (tflag == FTW_F)     ? "f"
                                                                    : (tflag == FTW_NS)    ? "ns"
                                                                    : (tflag == FTW_SL)    ? "sl"
                                                                    : (tflag == FTW_SLN)   ? "sln"
                                                                                           : "???"));
  MPUTR(file, "path", cstr_new_format(PATH_MAX + NAME_MAX, "%s", fpath));
  MPUTR(file, "basename", cstr_new_format(NAME_MAX, "%s", fpath + ftwinfo->base));
  MPUTR(file, "level", cstr_new_format(20, "%li", ftwinfo->level));
  MPUTR(file, "device", cstr_new_format(20, "%li", sb->st_dev));
  MPUTR(file, "size", cstr_new_format(20, "%li", sb->st_size));
  MPUTR(file, "inode", cstr_new_format(20, "%li", sb->st_ino));
  MPUTR(file, "links", cstr_new_format(20, "%li", sb->st_nlink));
  MPUTR(file, "userid", cstr_new_format(20, "%li", sb->st_uid));
  MPUTR(file, "groupid", cstr_new_format(20, "%li", sb->st_gid));
  MPUTR(file, "deviceid", cstr_new_format(20, "%li", sb->st_rdev));
  MPUTR(file, "blocksize", cstr_new_format(20, "%li", sb->st_blksize));
  MPUTR(file, "blocks", cstr_new_format(20, "%li", sb->st_blocks));
  MPUTR(file, "last_access", cstr_new_format(20, "%li", sb->st_atime));
  MPUTR(file, "last_modification", cstr_new_format(20, "%li", sb->st_mtime));
  MPUTR(file, "last_status", cstr_new_format(20, "%li", sb->st_ctime));

  struct passwd* pws;
  pws = getpwuid(sb->st_uid);

  MPUTR(file, "username", cstr_new_format(100, "%s", pws->pw_name));

  struct group* grp;
  grp = getgrgid(sb->st_gid);

  MPUTR(file, "groupname", cstr_new_format(100, "%s", grp->gr_name));

  MPUT(lib.files, fpath + strlen(lib.path) + 1, file); // use relative path as path

  return 0; /* To tell nftw() to continue */
}

void lib_delete_file(char* lib_path, map_t* entry)
{
  assert(lib_path != NULL);

  char* rel_path  = MGET(entry, "file/path");
  char* file_path = cstr_new_format(PATH_MAX + NAME_MAX, "%s/%s", lib_path, rel_path); // REL 0

  int error = remove(file_path);
  if (error)
    LOG("lib : cannot remove file %s : %s", file_path, strerror(errno));
  else
    LOG("lib : file %s removed.", file_path);

  REL(file_path); // REL 0
}

int lib_rename_file(char* old_path, char* new_path, char* new_dirs)
{
  LOG("lib : renaming %s to %s", old_path, new_path);

  int error = files_mkpath(new_dirs, 0777);

  if (error == 0)
  {
    error = rename(old_path, new_path);
    return error;
  }
  return error;
}

void lib_analyze_files(ch_t* channel, map_t* files)
{
  if (lib.lock) return;

  lib.lock  = 1;
  lib.files = RET(files); // REL 0
  lib.paths = VNEW();     // REL 1

  map_keys(files, lib.paths);

  SDL_CreateThread(analyzer_thread, "analyzer", channel);
}

int analyzer_thread(void* chptr)
{
  ch_t*    channel = chptr;
  map_t*   song    = NULL;
  uint32_t total   = lib.paths->length;
  int      ratio   = -1;

  while (lib.paths->length > 0)
  {
    if (!song)
    {
      song = MNEW();

      char* path = vec_tail(lib.paths);
      char* size = MGET(lib.files, path);

      char* time_str = CAL(80, NULL, cstr_describe); // REL 0
      // snprintf(time_str, 20, "%lu", time(NULL));

      time_t now;
      time(&now);
      struct tm ts = *localtime(&now);
      strftime(time_str, 80, "%Y-%m-%d %H:%M", &ts);

      // add file data

      MPUT(song, "file/path", path);
      MPUT(song, "file/size", size);
      MPUT(song, "file/added", time_str);
      MPUTR(song, "file/last_played", cstr_new_cstring("0"));
      MPUTR(song, "file/last_skipped", cstr_new_cstring("0"));
      MPUTR(song, "file/play_count", cstr_new_cstring("0"));
      MPUTR(song, "file/skip_count", cstr_new_cstring("0"));

      char* real = cstr_new_format(PATH_MAX + NAME_MAX, "%s/%s", lib.path, path); // REL 1

      LOG("lib : analyzing %s", real);

      // read and add file and meta data

      int res = coder_load_metadata_into(real, song);

      if (res == 0)
      {
        if (MGET(song, "meta/artist") == NULL) MPUTR(song, "meta/artist", cstr_new_cstring("Unknown"));
        if (MGET(song, "meta/album") == NULL) MPUTR(song, "meta/album", cstr_new_cstring("Unknown"));
        if (MGET(song, "meta/title") == NULL) MPUTR(song, "meta/title", cstr_new_path_filename(path));

        // try to send it to main thread
        if (ch_send(channel, song)) song = NULL;
      }
      else
      {
        // file is not a media file readable by ffmpeg, we skip it
        REL(song);
        song = NULL;
      }

      // cleanup
      REL(real);     // REL 1
      REL(time_str); // REL 0

      // remove entry from remaining
      vec_rem_at_index(lib.paths, lib.paths->length - 1);
    }
    else
    {
      // wait for main thread to process new entries
      SDL_Delay(5);
      if (ch_send(channel, song)) song = NULL;
    }

    // show progress
    int ratio_new = (int)((float)(total - lib.paths->length) / (float)total * 100.0);
    if (ratio != ratio_new)
    {
      ratio = ratio_new;
      LOG("lib : analyzer progress : %i%%", ratio);
    }
  }

  // send empty song to initiaite finish
  song = MNEW();                                        // REL 2
  MPUTR(song, "file/path", cstr_new_cstring("//////")); // impossible path
  ch_send(channel, song);                               // send finishing entry

  REL(lib.files); // REL 0
  REL(lib.paths); // REL 1

  lib.files = NULL;
  lib.paths = NULL;

  lib.lock = 0;
  return 0;
}

#endif
