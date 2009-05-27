#include <errno.h>
#include <string.h>

#include "fs/fs.h"
#include "mm/kmalloc.h"
#include "util/list.h"

static struct fstype ramfs;

static struct inode_ops iops  = {};
static struct sb_ops    sbops = {};

static struct inode ramfs_root = {
	.name  = "ram",
	.flags = FS_DIR,
	.ops   = &iops,
};
