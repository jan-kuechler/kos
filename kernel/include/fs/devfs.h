#ifndef DEVFS_H
#define DEVFS_H

#include "fs/fs.h"

void init_devfs(void);

/**
 *  devfs_register(file)
 *
 * Registers a device file with the devfs.
 * The owner may not invalidate the file until
 * it calls devfs_unregister.
 */
int devfs_register(inode_t *file);

/**
 *  devfs_unregister(file)
 *
 * Unregisters a previously registered file from devfs.
 */
int devfs_unregister(inode_t *file);

#endif /*DEVFS_H*/
