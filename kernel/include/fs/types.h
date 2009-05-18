#ifndef FS_TYPES_H
#define FS_TYPES_H

#include <types.h>

/*
 * maximal length for a file name
 */
#define FS_MAX_NAME 255

struct inode;
struct superblock;
struct dirent;

typedef void (*fscallback_t)(struct inode *, dword, void *, dword);

/*
 * file operations
 */
struct file_ops
{
	int (*open)(struct inode *ino, struct file *file, dword flags);
	int (*close)(struct file *file);

	int (*read)(struct file *file, void *buffer, dword size);
	int (*write)(struct file *file, void *buffer, dword size);

	int (*mknod)(struct file *file, char *name, dword flags);
	struct dirent *(*readdir)(struct file *file, dword index);
	struct file *(*finddir)(struct file *file, char *name);
};

/*
 * Info for file type inodes
 */
struct file
{
	struct inode *inode;

	dword pos;

	struct file_ops fops;
};

/*
 * Block device operations
 */
struct blockdev_ops
{
	int (*open)(struct inode *ino, struct blockdev *dev, dword flags);
	int (*close)(struct blockdev *dev);

	int (*read)(struct blockdev *dev, void *buffer, dword size, dword offset);
	int (*write)(struct blockdev *dev, void *buffer, dword size, dword offset);
};

/*
 * Info for block device type inodes
 */
struct blockdev
{
	struct inode *inode;

	dword bsize;

	struct blockdev_ops bops;
};

/*
 * Operations that can be done on every inode
 */
struct inode_ops
{
	int (*create)(struct inode *ino, dword flags);

	int (*seek)(struct inode *ino, dword index, dword offset);
};

/*
 * Any inode data comes here
 */
struct inode
{
	char *name;
	dword flags;

	dword mask;
	dword length;

	uid_t uid;
	gid_t gid;

	dword impl;

	union {
		struct file *file;
		struct blockdev *bdev;
	} type;

	struct inode *link;

	struct inode_ops ops;
	struct superblock *sb;
};

struct sb_ops
{
	void (*read_inode)(struct inode *inode);
	void (*write_inode)(struct inode *inode);
	void (*release_inode)(struct inode *inode);

	void (*write_super)(struct superblock *sb);
	void (*release_super)(struct superblock *sb);

	int  (*remount)(struct superblock *sb, dword flags);
};

struct superblock
{
	struct inode *root;

	struct sb_ops *ops;
};

struct fstype
{
	char  *name;
	dword flags;
	int (*get_sb)(struct superblock *sb, char *device, int flags);
};

struct dirent
{
	char name[FS_MAX_NAME];
	struct inode *inode;
};

#endif /*FS_TYPES_H*/
