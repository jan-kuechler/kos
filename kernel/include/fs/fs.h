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
extern struct inode *fs_root;

/**
 *  fs_register(type)
 *
 * Registers a new filesystem type.
 */
int fs_register(struct fstype *type);

/**
 *  fs_unregister(type)
 *
 * Unregisters a previously registered filesystem type.
 */
int fs_unregister(struct fstype *type);

/**
 *  fs_find_type(name)
 *
 * Returns the filesystem type with the given name or 0.
 */
struct fstype *fs_find_type(char *name);

/**
 *  fs_mount(ino, type, device, flags)
 *
 * Mounts a filesystem specified by type to a device.
 */
int fs_mount(struct inode *ino, struct fstype *type, char *device, int flags);
int fs_umount(struct superblock *sb);

/**
 *  lookup_dir(path, start)
 *
 * Returns the inode for the last directory
 * in the given path.
 */
int fs_lookup_dir(char *path, struct inode *start, struct inode **result);

/**
 *  lookup(path, start)
 *
 * This function returns the inode for a path.
 * It handles mountpoints and (soon) symlinks.
 */
struct inode *lookup(char *path, struct inode *start);

int fs_open(struct inode *inode, dword flags);
int fs_close(struct inode *inode);
int fs_read(struct inode *inode, void *buffer, dword size, dword offset);
int fs_write(struct inode *inode, void *buffer, dword size, dword offset);
int fs_read_async(struct inode *inode, void *buffer, dword size, dword offset, fscallback_t func);
int fs_write_async(struct inode *inode, void *buffer, dword size, dword offset, fscallback_t func);
int fs_mknod(struct inode *inode, char *name, dword flags);
struct dirent *fs_readdir(struct inode *inode, dword index);
struct inode *fs_finddir(struct inode *inode, char *name);

void init_fs(void);

#endif /*FS_H*/
