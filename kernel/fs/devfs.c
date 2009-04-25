#include "fs/fs.h"
#include "intern.h"

static fstype_t devfs;
static inode_t  devfs_root;
static sb_ops_t sb_ops;

static int get_sb(superblock_t *sb, char *dev, int flags)
{
	// ignore any device or flags
	sb->root = &devfs_root;
	sb->ops  = &sb_ops;
	return 0;
}

void init_devfs()
{
	devfs.name   = "devfs";
	devfs.flags  = 0;
	devfs.get_sb = get_sb;

	sb_ops.read_inode    = 0;
	sb_ops.write_inode   = 0;
	sb_ops.release_inode = 0;
	sb_ops.write_super   = 0;
	sb_ops.release_super = 0;
	sb_ops.remount       = 0;

	fs_register(&devfs);
}
