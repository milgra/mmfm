#ifndef filemanager_h
#define filemanager_h

#include "zc_map.c"
#define __USE_XOPEN_EXTENDED 1 // needed for linux
#include <ftw.h>

int  fm_create(char* file_path, mode_t mode);
void fm_delete(char* fmpath, map_t* en);
int  fm_rename(char* old, char* new, char* new_dirs);
int  fm_exists(char* path);
void fm_list(char* fmpath, map_t* db);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "zc_cstring.c"
#include "zc_log.c"
#include "zc_path.c"
#include <assert.h>
#include <errno.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

struct fm_t
{
    map_t* files;
    vec_t* paths;
    char   lock;
    char*  path;
} fm = {0};

int fm_create(char* file_path, mode_t mode)
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

void fm_delete(char* fm_path, map_t* entry)
{
    assert(fm_path != NULL);

    char* rel_path  = MGET(entry, "file/path");
    char* file_path = cstr_new_format(PATH_MAX + NAME_MAX, "%s/%s", fm_path, rel_path); // REL 0

    int error = remove(file_path);
    if (error)
	zc_log_error("fm : cannot remove file %s : %s", file_path, strerror(errno));
    else
	zc_log_error("fm : file %s removed.", file_path);

    REL(file_path); // REL 0
}

int fm_rename(char* old_path, char* new_path, char* new_dirs)
{
    zc_log_info("fm : renaming %s to %s", old_path, new_path);

    int error = fm_create(new_dirs, 0777);

    if (error == 0)
    {
	error = rename(old_path, new_path);
	return error;
    }
    return error;
}

int fm_exists(char* path)
{
    struct stat sb;

    if (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode))
	return 1;
    else
	return 0;
}

static int fm_file_data_step(const char* fpath, const struct stat* sb, int tflag, struct FTW* ftwinfo)
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

    MPUT(fm.files, fpath + strlen(fm.path) + 1, file); // use relative path as path

    return 0; /* To tell nftw() to continue */
}

void fm_list(char* fm_path, map_t* files)
{
    assert(fm_path != NULL);

    fm.files = files;
    fm.path  = fm_path;

    map_t* parent = MNEW();

    MPUTR(parent, "type", cstr_new_format(5, "%s", "d"));
    MPUTR(parent, "path", cstr_new_format(PATH_MAX + NAME_MAX, "%s", ".."));

    nftw(fm_path, fm_file_data_step, 20, FTW_PHYS);

    //  MPUTR(files, "..", parent); // use relative path as path

    zc_log_info("%s scanned, files : %i", fm_path, files->count);
}

#endif
