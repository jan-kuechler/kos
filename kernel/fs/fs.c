#include <stdlib.h>
#include <types.h>
#include <kos/error.h>
#include "pm.h"
#include "fs/fs.h"
#include "fs/request.h"

typedef struct fs_driver_entry
{
	const char  *name;
	fs_driver_t *driver;
	byte         active;
} fs_drent_t;

static fs_drent_t *drivers;
static dword       num_drivers;

static fs_filesystem_t **filesystems;
static dword       num_filesystems;

static fs_filesystem_t *filesys_from_path(const char *path, dword *match)
{
	int i=0;
	int matched = 0;
	fs_filesystem_t *fs = NULL;
	fs_filesystem_t *tmp;

	for (; i < num_filesystems; ++i) {
		tmp = filesystems[i];

		dword len = strlen(tmp->path);
		if (len  <= matched)
			continue;

		if (strncmp(tmp->path, path, len) == 0) {
			matched = len;
			fs = tmp;
		}
	}

	*match = matched;

	return fs;
}

/**
 *  fs_register_driver(driver, name)
 *
 * Registers a driver with fs.
 */
int fs_register_driver(fs_driver_t *driver,  const char *name)
{
	drivers = realloc(drivers, ++num_drivers * sizeof(fs_driver_t));
	drivers[num_drivers-1].driver = driver;
	drivers[num_drivers-1].name   = name;
	drivers[num_drivers-1].active = 1;
	return OK;
}

/**
 *  fs_unregister_driver(driver)
 *
 * Unregisters a driver from fs.
 */
int fs_unregister_driver(fs_driver_t *driver)
{
	/* TODO: free the memory */
	int i=0;
	for (; i < num_drivers; ++i) {
		if (drivers[i].driver == driver) {
			drivers[i].active = 0;
			return OK;
		}
	}
	return E_NOT_FOUND;
}

/**
 *  fs_get_driver(name)
 *
 * Returns the driver with the given name.
 */
fs_driver_t *fs_get_driver(const char *name)
{
	int i=0;
	for (; i < num_drivers; ++i) {
		if (strcmp(drivers[i].name, name) == 0) {
			return drivers[i].driver;
		}
	}

	return NULL;
}

/**
 *  fs_mount(driver, path, dev, flags)
 *
 * Mounts a device to the given path using the given driver.
 */
int fs_mount(fs_driver_t *driver, const char *path, const char *dev, dword flags)
{
	if (!driver) return E_INVALID_ARG;

	fs_filesystem_t *fs = driver->mount(driver, path, dev, flags);
	if (!fs) return E_UNKNOWN;

	filesystems = realloc(filesystems, ++num_filesystems * sizeof(fs_filesystem_t));
	filesystems[num_filesystems-1] = fs;

	return OK;
}

/**
 *  fs_umount(path)
 *
 * Unmounts a path
 */
int fs_umount(const char *path)
{
	return E_NOT_IMPLEMENTED;
}

/**
 *  fs_open(name, mode)
 */
fs_handle_t *fs_open(const char *name, dword mode)
{
	return fs_open_as_proc(name, mode, cur_proc);
}

/**
 *  fs_open_as_proc(name, mode, proc)
 */
fs_handle_t *fs_open_as_proc(const char *name, dword mode, proc_t *proc)
{
	dword matched = 0;
	fs_filesystem_t *fs = filesys_from_path(name, &matched);
	name += matched;

	fs_request_t rq;
	memset(&rq, 0, sizeof(rq));

	rq.type   = RQ_OPEN;
	rq.fs     = fs;
	rq.buflen = strlen(name) + 1;
	rq.buf    = (void*)name;
	rq.flags  = mode;

	int status = fs_query_rq(&rq, 1, proc);
	if (status != OK) return NULL;

	if (rq.status != OK || rq.result != OK) return NULL;

	fs_handle_t *fh = malloc(sizeof(fs_handle_t));
	fh->file  = rq.file;
	fh->pos   = 0;
	fh->flags = mode;

	return fh;
}

/**
 *  fs_close(handle)
 */
int fs_close(fs_handle_t *handle)
{
	return E_NOT_IMPLEMENTED;
}

/**
 *
 */
int fs_readwrite(fs_handle_t *handle, char *buf, int size, int mode)
{
	if (!handle)
		return E_INVALID_ARG;

	fs_request_t rq;
	memset(&rq, 0, sizeof(fs_request_t));
	rq.type = mode == FS_READ ? RQ_READ : RQ_WRITE;
	rq.fs   = handle->file->fs;
	rq.file = handle->file;
	rq.buflen = size;
	rq.buf  = buf;
	rq.offs = handle->pos;

	int status = fs_query_rq(&rq, 1, cur_proc);

	if (status != OK) return -1;
	if (rq.status != OK) return -1;

	handle->pos += rq.result;
	return rq.result;
}


/**
 *  init_fs()
 *
 * Initializes internal fs structures.
 */
void init_fs(void)
{
	drivers = NULL;
	num_drivers = 0;

	filesystems = NULL;
	num_filesystems = 0;
}
