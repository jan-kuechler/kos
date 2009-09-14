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
#define FS_CHARDEV  0x04
#define FS_BLOCKDEV 0x08
#define FS_PIPE     0x10
#define FS_SYMLINK  0x20
#define FS_MOUNTP   0x40

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
extern int fs_error;

inline int vfs_geterror()
{
	return fs_error;
}

inline void vfs_seterror(int err)
{
	fs_error = err;
}

int vfs_register(struct fstype *type);
int vfs_unregister(struct fstype *type);
struct fstype *vfs_gettype(char *name);

int vfs_mount(struct fstype *type, struct inode *point, char *device, dword flags);
int vfs_umount(struct inode *point);

struct inode *vfs_lookup(const char *path, struct inode *start);
struct dirent *vfs_lookup_dir(const char *path, struct inode *start);

struct inode *vfs_create(struct inode *dir, char *name, dword flags);
int vfs_unlink(struct inode *ino);
struct file *vfs_open(struct inode *ino, dword flags);
int vfs_close(struct file *file);

int vfs_read(struct file *file, void *buffer, dword count, dword offset);
int vfs_write(struct file *file, void *buffer, dword count, dword offset);
int vfs_read_async(struct request *rq);
int vfs_write_async(struct request *rq);

struct dirent *vfs_readdir(struct inode *ino, dword index);
struct inode *vfs_finddir(struct inode *ino, char *name);

void init_fs(void);

#endif /*FS_H*/
