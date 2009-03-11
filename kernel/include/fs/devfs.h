#ifndef DEVFS_H
#define DEVFS_H

#include "fs/fs.h"

typedef struct fs_devfile
{
	const char *path;

	dword id;

	int (*query)(struct fs_devfile *file, struct fs_request *request);
} fs_devfile_t;

void init_devfs(void);

int fs_create_dev(fs_devfile_t *file);
int fs_destroy_dev(fs_devfile_t *file);

#endif /*DEVFS_H*/
