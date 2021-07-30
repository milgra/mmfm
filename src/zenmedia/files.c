#ifndef files_h
#define files_h

#include <sys/stat.h>
#include <sys/types.h>

int files_path_exists(char* path);
int files_mkpath(char* file_path, mode_t mode);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "zc_cstring.c"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

int files_path_exists(char* path)
{
  struct stat sb;

  if (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode))
    return 1;
  else
    return 0;
}

int files_mkpath(char* file_path, mode_t mode)
{
  assert(file_path && *file_path);
  for (char* p = strchr(file_path + 1, '/');
       p;
       p = strchr(p + 1, '/'))
  {
    *p = '\0';
    if (mkdir(file_path, mode) == -1)
    {
      if (errno != EEXIST)
      {
        *p = '/';
        return -1;
      }
    }
    *p = '/';
  }
  return 0;
}

#endif
