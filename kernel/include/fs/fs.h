#ifndef FS_H
#define FS_H

#include <types.h>
#include "pm.h"
#include "fs/types.h"

#define FS_MAX_SYMLOOP 20

/*
 * Inode type flags.
 * Note: FS_MOUNTP and FS_DIR can be or'd together
 */
#define FS_FILE     0x01
#define FS_DIR      0x02
#define FS_CHARDEV  0x03
#define FS_BLOCKDEV 0x04
#define FS_PIPE     0x05
#define FS_SYMLINK  0x06
#define FS_MOUNTP   0x08

/*
 * Fstype flags
 */
#define FST_NEED_DEV 0x01

/*
 * Mount flags
 */
#define FSM_READ   0x01
#define FSM_WRITE  0x02
#define FSM_EXEC   0x03
#define FSM_UMOUNT 0x08

/*
 * Open flags
 */
#define FSO_READ   0x01
#define FSO_WRITE  0x02


/* The root of the file system */
extern inode_t *fs_root;

/**
 *  fs_register(type)
 *
 * Registers a new filesystem type.
 */
int fs_register(fstype_t *type);

/**
 *  fs_unregister(type)
 *
 * Unregisters a previously registered filesystem type.
 */
int fs_unregister(fstype_t *type);

/**
 *  fs_find_type(name)
 *
 * Returns the filesystem type with the given name or 0.
 */
fstype_t *fs_find_type(char *name);

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

int fs_open(inode_t *inode, dword flags);
int fs_close(inode_t *inode);
int fs_read(inode_t *inode, dword offset, void *buffer, dword size);
int fs_write(inode_t *inode, dword offset, void *buffer, dword size);
int fs_mknod(inode_t *inode, char *name, dword flags);
struct dirent *fs_readdir(inode_t *inode, dword index);
inode_t *fs_finddir(inode_t *inode, char *name);

void init_fs(void);

#endif /*FS_H*/
