#ifndef FS_INTERN_H
#define FS_INTERN_H

#include <fs/fs.h>

/*
 * The following functions are for
 * internal use of the virtual filesystem
 * and its syscalls.
 */

/**
 *  fs_mount(ino, type, device, flags)
 *
 * Mounts a filesystem specified by type to a device.
 */
int fs_mount(inode_t *ino, fstype_t *type, char *device, int flags);

int fs_umount(superblock_t *sb);


/**
 *  lookup_dir(path, start)
 *
 * Returns the inode for the last directory
 * in the given path.
 */
int fs_lookup_dir(char *path, inode_t *start, inode_t **result);

/**
 *  lookup(path, start)
 *
 * This function returns the inode for a path.
 * It handles mountpoints and (soon) symlinks.
 */
inode_t *lookup(char *path, inode_t *start);

#endif /*FS_INTERN_H*/
