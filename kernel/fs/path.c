#include <bitop.h>
#include <string.h>
#include "debug.h"
#include "mm/kmalloc.h"
#include "fs/fs.h"

// TODO: permissions

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


int fs_lookup_dir(char *path, inode_t *start, inode_t **result)
{
	return -1;
}

inode_t *lookup(char *path, inode_t *start)
{
	/* adjust absolut paths */
	if (*path == '/') {
		start = fs_root;
		while (*(++path) == '/')
			;
	}

	char part[FS_MAX_NAME];
	inode_t *ino = start;
	int  pidx  = 0;
	int symc = 0;

	while (next_part(path, &pidx, part)) {
		if (!ino) {
			return NULL;
		}

		if (bisset(ino->flags, FS_SYMLINK)) {
			ino = ino->link;

			if (++symc >= FS_MAX_SYMLOOP) {
				return NULL;
			}
		}
		else if (bisset(ino->flags, FS_MOUNTP)) {
			ino = ino->link;
		}
		else if (bnotset(ino->flags, FS_DIR)) {
			return NULL;
		}

		ino = fs_finddir(ino, part);
	}

	return ino;
}
