#include <errno.h>
#include <string.h>

#include "fs/fs.h"
#include "mm/kmalloc.h"
#include "util/list.h"

static struct fstype devfs;

static struct inode_ops iops;
static struct inode devfs_root = {
	.name = "dev",
	.flags = FS_DIR,
	.ops = &iops,
};
static sb_ops_t sbops;

static list_t *devices;

static int get_sb(struct superblock *sb, char *dev, int flags)
{
	// ignore any device or flags
	sb->root = &devfs_root;
	sb->ops  = &sbops;

	devfs_root.sb = sb;
	return 0;
}

static struct dirent *readdir(struct inode *ino, dword index)
{
	if (ino != &devfs_root)
		return NULL;

	if (index >= list_size(devices))
		return NULL;

	int i=0;
	list_entry_t *pos;
	struct dirent *dirent = kmalloc(sizeof(struct dirent));

	list_iterate(pos, devices) {
		if (index == i) {
			struct inode *dev = pos->data;
			strcpy(dirent->name, dev->name);
			dirent->inode = (dword)dev;
			return dirent;
		}
		i++;
	}

	return NULL;
}

static struct inode *finddir(struct inode *ino, char *name)
{
	if (ino != &devfs_root)
		return NULL;

	list_entry_t *pos;

	list_iterate(pos, devices) {
		struct inode *dev = pos->data;
		if (strcmp(dev->name, name) == 0)
			return dev;
	}

	return NULL;
}

int devfs_register(struct inode *ino)
{
	file->sb = devfs_root.sb;
	list_add_back(devices, ino);

	return 0;
}

int devfs_unregister(struct inode *ino)
{
	list_entry_t *pos;

	list_iterate(pos, devices) {
		if (ino == pos->data) {
			list_del_entry(devices, pos);
			return 0;
		}
	}
	return -ENOENT;
}

void init_devfs()
{
	devfs.name   = "devfs";
	devfs.flags  = 0;
	devfs.get_sb = get_sb;

	memset(&sbops, 0, sizeof(sbops));

	memset(&iops, 0, sizeof(iops));
	iops.readdir = readdir;
	iops.finddir = finddir;

	devices = list_create();

	fs_register(&devfs);
}
