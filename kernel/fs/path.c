#include <string.h>

/**
 *  lookup(path, start)
 *
 * This function returns the inode for a path.
 * It handles mountpoints and (soon) symlinks.
 */
inode_t *lookup(char *path, inode_t *start)
{
	if (path[0] == FS_ROOT_DIR) {
		start = fs_root;
		path++;
	}


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