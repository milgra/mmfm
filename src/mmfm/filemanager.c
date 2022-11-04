#ifndef filemanager_h
#define filemanager_h

#include "zc_map.c"
#define __USE_XOPEN 1
#define __USE_XOPEN_EXTENDED 1 // needed for linux
#include <ftw.h>

int  fm_create(char* file_path, mode_t mode);
void fm_delete(char* path);
int  fm_rename(char* old, char* new, char* new_dirs);
int  fm_rename1(char* old, char* new);
int  fm_copy(char* old, char* new);
int  fm_exists(char* path);
void fm_list(char* fmpath, map_t* db);
void fm_listdir(char* fm_path, map_t* files);
void fm_detail(map_t* file);

#endif

#if __INCLUDE_LEVEL__ == 0

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <limits.h>
#include <linux/stat.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "coder.c"
#include "zc_cstr_ext.c"
#include "zc_cstring.c"
#include "zc_log.c"
#include "zc_path.c"

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

int fm_remove_cb(const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf)
{
    int rv = remove(fpath);

    zc_log_debug("removing %s\n", fpath);
    if (rv)
	perror(fpath);

    return rv;
}

void fm_delete(char* path)
{
    int error = nftw(path, fm_remove_cb, 64, FTW_DEPTH | FTW_PHYS);
    if (error)
	zc_log_error("fm : cannot remove path %s : %s", path, strerror(errno));
    else
	zc_log_debug("fm : path %s removed.", path);
}

int fm_rename(char* old_path, char* new_path, char* new_dirs)
{
    zc_log_info("fm : renaming %s to %s", old_path, new_path);

    int error = 0;
    if (new_dirs) error = fm_create(new_dirs, 0777);

    if (error == 0)
    {
	error = rename(old_path, new_path);
	return error;
    }
    return error;
}

int fm_rename1(char* old_path, char* new_path)
{
    zc_log_info("fm : renaming %s to %s", old_path, new_path);

    int error = 0;

    error = rename(old_path, new_path);
    return error;
}

int fm_copy(char* old_path, char* new_path)
{
    char* command = cstr_new_format(7 + PATH_MAX * 2, "cp -r %s %s", old_path, new_path);

    int res = system(command);

    zc_log_debug("%s result : %i", command, res);

    return res;
}

int fm_exists(char* path)
{
    struct stat sb;

    if (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode))
	return 1;
    else
	return 0;
}

void fm_list(char* fm_path, map_t* files)
{
    DIR* dirp = opendir(fm_path);
    chdir(fm_path);

    if (dirp != NULL)
    {
	struct dirent* dp = readdir(dirp);

	while (dp != NULL)
	{
	    char path[PATH_MAX + 1];
	    snprintf(path, PATH_MAX, "%s/%s", fm_path, dp->d_name);

	    struct stat sb;

	    int status = stat(path, &sb);

	    if (status == 0)
	    {
		/* printf("ID of containing device:  [%jx,%jx]\n", (uintmax_t) major(sb.st_dev), (uintmax_t) minor(sb.st_dev)); */
		/* printf("File type:                "); */

		/* switch (sb.st_mode & S_IFMT) */
		/* { */
		/*     case S_IFBLK: printf("block device\n"); break; */
		/*     case S_IFCHR: printf("character device\n"); break; */
		/*     case S_IFDIR: printf("directory\n"); break; */
		/*     case S_IFIFO: printf("FIFO/pipe\n"); break; */
		/*     case S_IFLNK: printf("symlink\n"); break; */
		/*     case S_IFREG: printf("regular file\n"); break; */
		/*     case S_IFSOCK: printf("socket\n"); break; */
		/*     default: printf("unknown?\n"); break; */
		/* } */

		/* printf("I-node number:            %ju\n", (uintmax_t) sb.st_ino); */
		/* printf("Mode:                     %jo (octal)\n", (uintmax_t) sb.st_mode); */
		/* printf("Link count:               %ju\n", (uintmax_t) sb.st_nlink); */
		/* printf("Ownership:                UID=%ju   GID=%ju\n", (uintmax_t) sb.st_uid, (uintmax_t) sb.st_gid); */
		/* printf("Preferred I/O block size: %jd bytes\n", (intmax_t) sb.st_blksize); */
		/* printf("File size:                %jd bytes\n", (intmax_t) sb.st_size); */
		/* printf("Blocks allocated:         %jd\n", (intmax_t) sb.st_blocks); */
		/* printf("Last status change:       %s", ctime(&sb.st_ctime)); */
		/* printf("Last file access:         %s", ctime(&sb.st_atime)); */
		/* printf("Last file modification:   %s", ctime(&sb.st_mtime)); */

		char* type = "";
		switch (sb.st_mode & S_IFMT)
		{
		    case S_IFBLK: type = "block device"; break;
		    case S_IFCHR: type = "character device"; break;
		    case S_IFDIR: type = "directory"; break;
		    case S_IFIFO: type = "FIFO/pipe"; break;
		    case S_IFLNK: type = "symlink"; break;
		    case S_IFREG: type = "regular file"; break;
		    case S_IFSOCK: type = "socket"; break;
		    default: type = "unknown"; break;
		}

		map_t* file = MNEW();

		MPUTR(file, "file/type", cstr_new_cstring(type));
		MPUTR(file, "file/parent", cstr_new_cstring(fm_path));
		MPUTR(file, "file/path", cstr_new_format(PATH_MAX + NAME_MAX, "%s", path));
		MPUTR(file, "file/basename", cstr_new_cstring(dp->d_name));
		MPUTR(file, "file/device", cstr_new_format(20, "%li", sb.st_dev));
		MPUTR(file, "file/size", cstr_new_format(20, "%li", sb.st_size));
		MPUTR(file, "file/inode", cstr_new_format(20, "%li", sb.st_ino));
		MPUTR(file, "file/links", cstr_new_format(20, "%li", sb.st_nlink));
		MPUTR(file, "file/userid", cstr_new_format(20, "%li", sb.st_uid));
		MPUTR(file, "file/groupid", cstr_new_format(20, "%li", sb.st_gid));
		MPUTR(file, "file/deviceid", cstr_new_format(20, "%li", sb.st_rdev));
		MPUTR(file, "file/blocksize", cstr_new_format(20, "%li", sb.st_blksize));
		MPUTR(file, "file/blocks", cstr_new_format(20, "%li", sb.st_blocks));
		struct tm* at = localtime(&sb.st_atime);
		MPUTR(file, "file/last_access", cstr_new_format(100, "%i/%.2i/%.2i %.2i:%.2i:%.2i", 1900 + at->tm_year, at->tm_mon, at->tm_mday, at->tm_hour, at->tm_min, at->tm_sec));
		struct tm* mt = localtime(&sb.st_mtime);
		MPUTR(file, "file/last_modification", cstr_new_format(100, "%i/%.2i/%.2i %.2i:%.2i:%.2i", 1900 + mt->tm_year, mt->tm_mon, mt->tm_mday, mt->tm_hour, mt->tm_min, mt->tm_sec));
		struct tm* ct = localtime(&sb.st_ctime);
		MPUTR(file, "file/last_status", cstr_new_format(100, "%i/%.2i/%.2i %.2i:%.2i:%.2i", 1900 + ct->tm_year, ct->tm_mon, ct->tm_mday, ct->tm_hour, ct->tm_min, ct->tm_sec));

		if (strcmp(dp->d_name, ".") != 0) MPUT(files, path, file); // use relative path as path

		REL(file);
	    }
	    else
		zc_log_error("CANNOT STAT %s, status %i errno %i", path, status, errno);

	    dp = readdir(dirp);
	}

	closedir(dirp);
    }
}

void fm_detail(map_t* file)
{
    char* parent = MGET(file, "file/parent");
    char* path   = MGET(file, "file/path");
    char* name   = MGET(file, "file/basename");

    chdir(parent);

    /* get user and group */

    char* uid = MGET(file, "file/userid");
    char* gid = MGET(file, "file/groupid");

    struct passwd* pws;
    pws = getpwuid(atoi(uid));

    MPUTR(file, "file/username", cstr_new_format(100, "%s", pws->pw_name));

    /* printf("FILE\n"); */
    /* mem_describe(file, 0); */

    /* struct group* grp; */
    /* grp = getgrgid(atoi(gid)); */

    /* MPUTR(file, "file/groupname", cstr_new_format(100, "%s", grp->gr_name)); */

    // get mime type with file command

    char  buff[500];
    char* mime    = cstr_new_cstring("");                        // REL L0
    char* command = cstr_new_format(80, "file -b \"%s\"", name); // REL L1
    FILE* pipe    = popen(command, "r");                         // CLOSE 0
    while (fgets(buff, sizeof(buff), pipe) != NULL) mime = cstr_append(mime, buff);
    pclose(pipe); // CLOSE 0
    REL(command); // REL L1

    MPUTR(file, "file/mime", mime);

    // get media metadata

    coder_load_metadata_into(path, file);
}

#endif
