#ifndef FS_H
#define FS_H

#include <types.h>
#include "pm.h"

struct fs_request;

struct proc_t;

typedef struct fs_filesystem
{
	char *path; // path at which this filesystem is mounted

	struct fs_handle *device; // device for this filesystem
	struct fs_driver *driver; // driver for this filesystem

	int (*umount)(struct fs_filesystem *fs); // callback for unmounting

	int (*query)(struct fs_filesystem *fs, struct fs_request *request); // callback for requests
} fs_filesystem_t;

typedef struct fs_driver
{
	byte flags;

	fs_filesystem_t* (*mount)(struct fs_driver *driver, const char *path, const char *dev, dword flags);

} fs_driver_t;

typedef struct fs_file
{
	dword refs;
	fs_filesystem_t *fs;
} fs_file_t;

typedef struct fs_handle
{
	fs_file_t *file;
	dword      pos;
	dword      flags;
} fs_handle_t;

#define FS_DRV_SINGLE  0x1 /* Driver can only create 1 filesystem */
#define FS_DRV_NOMOUNT 0x2 /* Cannot be mounted manually */
#define FS_DRV_NODATA  0x4 /* Does not need any data source */

#define FS_READ 0
#define FS_WRITE 1

void init_fs(void);

int fs_register_driver(fs_driver_t *driver, const char *name);
int fs_unregister_driver(fs_driver_t *driver);

fs_driver_t *fs_get_driver(const char *name);

int fs_mount(fs_driver_t *driver, const char *path, const char *device, dword flags);
int fs_umount(const char *path);

fs_handle_t *fs_open(const char *name, dword mode);
fs_handle_t *fs_open_as_proc(const char *name, dword mode, proc_t *proc);

int fs_close(fs_handle_t *file);
int fs_mknod(const char *name, dword mode);

int fs_readwrite(fs_handle_t *file, char *buf, int size, int mode);

int fs_seek(fs_handle_t *file, dword offs, int orig);



#endif /*FS_H*/
