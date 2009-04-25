#include <string.h>
#include "mm/kmalloc.h"

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

	/*char *part = kmalloc(end - *start + 1);
	strncpy(part, path + *start, end - *start);
	part[end - *start] = '\0';*/

	strncpy(buffer, path + *start, end - *start);
	buffer[end - *start] = '\0';

	if (path[end] == '/')
		end++;

	*start = end;

	return 1;
}


int fs_lookup_dir(char *path, inode_t *start, inode_t **result)
{
	/* adjust absolut paths */
	if (*path == '/') {
		start = fs_root;
		while (*(++path) == '/')
			;
	}


	inode_t *cur = start;
	inode_t *prev = 0;
	char *part = NULL;
	int pidx = 0;
	int symc = 0;

	while (part = next_part(path, &pidx)) {
		prev = cur;
		cur  = fs_finddir(cur, part);
		kfree(part);

		if (bisset(cur->flags, FS_SYMLINK)) {
			cur = cur->link;

			if (++symc >= FS_MAX_SYMLOOP) {
				return -ELOOP;
			}
		}
		else if (bisset(cur->flags, FS_MOUNTP)) {
			cur = cur->link;
		}
		else if (bnotset(cur->flags, FS_DIR)) {
			return -ENOENT;
		}
	}


}

/**
 *  lookup(path, start)
 *
 * This function returns the inode for a path.
 * It handles mountpoints and (soon) symlinks.
 */
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

/** PSEUDO CODE **

inode = start

while (part = get_next_part(path)) {
	if (!inode)
		error

	if (inode != DIRECTORY || SYMLINK || MOUNTPOINT)
		error

	if (inode == MOUNTPOINT)
		inode = inode->mount_inode;
	if (inode == SYMLINK)
		inode = inode->symlink_inode;

	inode = fs_finddir(inode, part)

}

return inode;

******************/