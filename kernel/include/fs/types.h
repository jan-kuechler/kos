#ifndef FS_TYPES_H
#define FS_TYPES_H

#include <stdint.h>
#include <types.h>

/*
 * maximal length for a file name
 */
#define FS_MAX_NAME 255

struct file;
struct inode;
struct request;

typedef void (*fscallback_t)(struct file *, int, void *, dword);

/*
 * file operations
 */
struct file_ops
{
	int (*close)(struct file *file);

	int (*read)(struct file *file, void *buffer, dword size, dword offset);
	int (*write)(struct file *file, void *buffer, dword size, dword offset);
	int (*read_async)(struct request *rq);
	int (*write_async)(struct request *rq);

	int (*seek)(struct file *file, dword offset, dword index);
};

/*
 * Info for file type inodes
 */
struct file
{
	struct inode *inode;

	dword pos;

	struct file_ops *fops;
};

/*
 * Operations that can be done on every inode
 */
struct inode_ops
{
	struct inode* (*create)(struct inode *dir, char *name, dword flags);
	int (*open)(struct inode *ino, struct file *file, dword flags);
	int (*unlink)(struct inode *ino);

	struct dirent *(*readdir)(struct inode *ino, dword index);
	struct inode *(*finddir)(struct inode *ino, char *name);
};

/*
 * Any inode data comes here
 */
struct inode
{
	char *name;
	dword flags;

	dword perm;
	dword length;

	uint32_t uid;
	uint32_t gid;

	uint32_t opencount;

	dword impl;

	struct inode *link;

	struct inode_ops *ops;
	struct superblock *sb;
};

struct sb_ops
{
	int  (*remount)(struct superblock *sb, dword flags);
};

struct superblock
{
	struct inode *root;
	dword flags;
	struct sb_ops *ops;
};

struct fstype
{
	char  *name;
	dword flags;
	int (*mount)(struct superblock *sb, char *device, int flags);
};

struct dirent
{
	char name[FS_MAX_NAME];
	struct inode *inode;
};

#endif /*FS_TYPES_H*/
