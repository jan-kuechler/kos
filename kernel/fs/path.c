#include <bitop.h>
#include <errno.h>
#include <string.h>
#include "debug.h"
#include "mm/kmalloc.h"
#include "fs/fs.h"

#define ret_null_and_err(err) do { fs_error = err; return NULL; } while (0);

/**
 *  next_part(path, start)
 *
 * Returns the next part of the path starting from a
 * given index.
 */
static inline int next_part(char *path, int *start, char *buffer)
{
	int end = *start;
	while (path[end] && path[end] != '/') {
		end++;
	}

	if (end <= *start)
		return 0;

	kassert((end - *start) < FS_MAX_NAME);

	strncpy(buffer, path + *start, end - *start);
	buffer[end - *start] = '\0';

	if (path[end] == '/')
		end++;

	*start = end;

	return 1;
}

static int advance_root(char **path)
{
	char *p = *path;
	if (*p == '/') {
		while (*(++p) == '/')
			;
		*path = p;
		return 1;
	}
	return 0;
}


struct dirent *vfs_lookup_dir(char *path, struct inode *start)
{
	if (advance_root(&path))
		start = fs_root;

	// TODO

	return NULL;
}

struct inode *vfs_lookup(char *path, struct inode *start)
{
	/* adjust absolut paths */
	//if (*path == '/') {
	//	start = fs_root;
	//	while (*(++path) == '/')
	//		;
	//}

	dbg_vprintf(DBG_FS, "vfs_lookup('%s', inode: '%s')\n", path, start->name);
	if (advance_root(&path)) {
		dbg_vprintf(DBG_FS, " was root, path is now '%s'\n", path);
		start = fs_root;
	}

	char part[FS_MAX_NAME];
	struct inode *ino = start;
	int  pidx  = 0;
	int symc = 0;

	while (next_part(path, &pidx, part)) {
		dbg_vprintf(DBG_FS, " part: '%s'\n", part);

		if (!ino) {
			dbg_vprintf(DBG_FS, " prev parts inode was not found, returning...\n");
			return NULL;
		}

		if (bisset(ino->flags, FS_SYMLINK)) {
			dbg_vprintf(DBG_FS, " symlink\n");
			ino = ino->link;

			if (++symc >= FS_MAX_SYMLOOP) {
				dbg_vprintf(DBG_FS, " symloop!\n");
				ret_null_and_err(-ELOOP);
				return NULL;
			}
		}
		else if (bisset(ino->flags, FS_MOUNTP)) {
			dbg_vprintf(DBG_FS, " mountpoint\n");
			ino = ino->link;
		}
		else if (bnotset(ino->flags, FS_DIR)) {
			dbg_vprintf(DBG_FS, " not a directory\n");
			ret_null_and_err(-ENOENT);
		}

		ino = vfs_finddir(ino, part);

	}

	return ino;
}
