#include <stdlib.h>
#include <kos/error.h>
#include "fs/devfs.h"
#include "fs/fs.h"
#include "fs/request.h"
#include "mm/kmalloc.h"

typedef struct
{
	fs_file_t file;
	dword     index;
} devfs_file_holder_t;

#define FINDEX(f) ((devfs_file_holder_t*)f)->index

static fs_driver_t devfs;
static fs_filesystem_t fs;

static fs_devfile_t **devfiles;
static fs_file_t    **files;
static dword          numfiles;

static fs_filesystem_t *mount(fs_driver_t *driver, const char *path, const char *dev, dword flags)
{
	if (fs.path)
		return NULL;

	char *copy = kmalloc(strlen(path) + 1);
	strcpy(copy, path);
	fs.path = copy;
	return &fs;
}

static int umount(fs_filesystem_t *fs)
{
	kfree(fs->path);
	fs->path = 0;
	return OK;
}

static fs_file_t *open(fs_filesystem_t *fs, const char *path)
{
	dword index = -1;
	int i=0;
	for (; i < numfiles; ++i) {
		if (strcmp(devfiles[i]->path, path) == 0) {
			index = i;
			break;
		}
	}
	if (index == -1) {
		return NULL;
	}

	if (files[index])
		return files[index];

	devfs_file_holder_t *file = kmalloc(sizeof(devfs_file_holder_t));
	files[index] = &file->file;

	memset(&file->file, 0, sizeof(fs_file_t));
	file->file.fs = fs;
	file->index   = index;

	return files[index];
}

static int query(fs_filesystem_t *fs, fs_request_t *rq)
{
	switch (rq->type) {
	case RQ_OPEN:
		{
			rq->file = open(fs, rq->buf);
			if (!rq->file) {
				rq->status = E_NOT_FOUND;
				rq->result = -1;
			}
			else {
				dword findex = FINDEX(rq->file);
				devfiles[findex]->query(devfiles[findex], rq);
			}
			fs_finish_rq(rq);

			return OK;
		}
	case RQ_CLOSE:
		{
			dword findex = FINDEX(rq->file);
			devfiles[findex]->query(devfiles[findex], rq);
			fs_finish_rq(rq);

			return OK;
		}
	default:
		{
			dword findex = FINDEX(rq->file);
			if (!devfiles[findex]->query) {
				return E_NOT_SUPPORTED;
			}
			return devfiles[findex]->query(devfiles[findex], rq);
		}
	}

	return E_UNKNOWN;
}

/**
 *  fs_create_dev(file)
 *
 * Creates a device file
 */
int fs_create_dev(fs_devfile_t *file)
{
	devfiles = krealloc(devfiles, ++numfiles * sizeof(fs_devfile_t*));
	files    = krealloc(files, numfiles * sizeof(fs_file_t*));
	devfiles[numfiles-1] = file;
	files[numfiles-1]    = 0;

	return OK;
}

/**
 *  fs_destroy_dev(file)
 *
 * Destroys a device file
 */
int fs_destroy_dev(fs_devfile_t *file)
{
	return E_NOT_IMPLEMENTED;
}

/**
 *  init_devfs()
 */
void init_devfs(void)
{
	memset(&fs, 0, sizeof(fs));
	fs.driver = &devfs;
	fs.umount = umount;
	fs.query  = query;

	devfs.flags = FS_DRV_SINGLE | FS_DRV_NOMOUNT | FS_DRV_NODATA;
	devfs.mount = mount;

	fs_register_driver(&devfs, "devfs");
}
