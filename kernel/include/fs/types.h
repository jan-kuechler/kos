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

/*
 * Operations that can be done on an inode
 */
typedef struct inode_ops
{
	int (*open)(struct inode* inode, dword flags);
	int (*close)(struct inode* inode);

	int (*read)(struct inode* inode, dword offset, void *buffer, dword size);
	int (*write)(struct inode* inode, dword offset, void *buffer, dword size);

	int (*mknod)(struct inode* inode, char *name, dword flags);
	struct dirent* (*readdir)(struct inode *inode, dword index);
	struct inode*  (*finddir)(struct inode *inode, char *name);

} inode_ops_t;

/*
 * Any inode data comes here
 */
typedef struct inode
{
	char *name;
	dword flags;

	dword mask;
	dword length;

	uid_t uid;
	gid_t gid;

	dword impl;

	struct inode *link;

	inode_ops_t *ops;
	struct superblock *sb;
} inode_t;

typedef struct sb_ops
{
	void (*read_inode)(inode_t *inode);
	void (*write_inode)(inode_t *inode);
	void (*release_inode)(inode_t *inode);

	void (*write_super)(struct superblock *sb);
	void (*release_super)(struct superblock *sb);

	int  (*remount)(struct superblock *sb, dword flags);
} sb_ops_t;

typedef struct superblock
{
	inode_t *root;

	sb_ops_t *ops;
} superblock_t;

typedef struct fstype
{
	char  *name;
	dword flags;
	int (*get_sb)(superblock_t *sb, char *device, int flags);
} fstype_t;

typedef struct dirent
{
	char name[FS_MAX_NAME];
	dword inode;
} dirent_t;

#endif /*FS_TYPES_H*/
