#include <string.h>
#include <cdi/storage.h>
#include "cdi_impl.h"
#include "fs/fs.h"
#include "fs/devfs.h"
#include "mm/kmalloc.h"

static int dev_open(struct inode *ino, struct file *file, dword flags);
static int dev_read(struct file *file, void *buffer, dword size, dword offset);
static int dev_write(struct file *file, void *buffer, dword size, dword offset);
static int dev_close(struct file *file);

static struct inode_ops iops = {
	.open = dev_open,
};

static struct file_ops fops = {
	.read  = dev_read,
	.write = dev_write,
	.close = dev_close,
};

void cdi_storage_driver_init(struct cdi_storage_driver* driver)
{	LOG
	cdi_check_init();
	cdi_check_arg(driver, != NULL);

	driver->drv.type = CDI_STORAGE;
	cdi_driver_init(&driver->drv);
}

void cdi_storage_driver_destroy(struct cdi_storage_driver* driver)
{ LOG
	cdi_check_init();
	cdi_check_arg(driver, != NULL);

	cdi_driver_destroy(&driver->drv);
}

void cdi_storage_driver_register(struct cdi_storage_driver* driver)
{ LOG
	cdi_check_init();
	cdi_check_arg(driver, != NULL);

	cdi_driver_register(&driver->drv);
}

void cdi_storage_device_init(struct cdi_storage_device* device)
{ LOG
	cdi_check_init();
	cdi_check_arg(device, != NULL);

	device->dev.type = CDI_STORAGE;

	// FIXME: Memory leak! The inode is not tracked anywhere...
	struct inode *ino = kmalloc(sizeof(*ino));
	memset(ino, 0, sizeof(*ino));

	ino->flags = FS_BLOCKDEV;
	ino->name = (char *)device->dev.name;
	ino->ops = &iops;

	ino->impl = (dword)device;

	devfs_register(ino);
}

static int dev_open(struct inode *ino, struct file *file, dword flags)
{
	file->pos = 0;
	file->fops = &fops;

	struct cdi_storage_device *dev = (struct cdi_storage_device*)file->inode->impl;
	struct cdi_storage_driver *drv = (struct cdi_storage_driver*)dev->dev.driver;

	if (drv->open) {
		return drv->open(dev);
	}
	return 0;
}

static int dev_read(struct file *file, void *buffer, dword size, dword offset)
{
	/* a buffer for unaligned requests.                 */
	/* it may live through many reads,                 */
	/* so theres no nead to alloc a new one every time */
	static uint8_t *tmpbuffer = 0;
	static size_t bufsize = 0;

	enum { MAX_ALIVE = 0x4000 }; /* max buffer size, that keeps allocated */

	struct cdi_storage_device *dev = (struct cdi_storage_device*)file->inode->impl;
	struct cdi_storage_driver *drv = (struct cdi_storage_driver*)dev->dev.driver;
	kassert(dev->dev.type == CDI_STORAGE);

	size_t bsize = dev->block_size;
	uint64_t startblock = offset / bsize;
	uint64_t endblock = (offset + size) / bsize;
	uint64_t numblocks = endblock - startblock;

	if ((offset % bsize) == 0 && ((offset + size) % bsize) == 0) {
		/* request fits directly to blocks, that's easy! */
		int err = drv->read_blocks(dev, startblock, numblocks, buffer);
		if (err != 0)
			return err;
	}
	else {
		/* slightly harder, the request is not aligned to block boundary. */
		size_t needed = (numblocks + 1) * bsize;
		if (needed > bufsize) {
			/* get a new buffer, the old one is not large enough */
			kfree(tmpbuffer);
			tmpbuffer = kmalloc(needed);
			bufsize = needed;
		}

		/* read enough blocks in a temporary buffer */
		int err = drv->read_blocks(dev, startblock, numblocks + 1, tmpbuffer);
		if (err != 0) return err;

		/* and copy the date from there to the destination */
		size_t offs = offset % bsize;
		memcpy(buffer, tmpbuffer + offs, size);

		/* don't hold too large buffers, that would waste memory */
		if (bufsize > MAX_ALIVE) {
			bufsize = 0;
			kfree(tmpbuffer);
		}
	}
	return size;
}

static int dev_write(struct file *file, void *buffer, dword size, dword offset)
{
	/* a buffer for single blocks                          */
	/* the block size is likely the same for two requests, */
	/* so this is only freed and allocated when it varies  */
	static uint8_t *blockbuffer = 0;
	static uint32_t bbufsize = 0;

	struct cdi_storage_device *dev = (struct cdi_storage_device*)file->inode->impl;
	struct cdi_storage_driver *drv = (struct cdi_storage_driver*)dev->dev.driver;
	kassert(dev->dev.type == CDI_STORAGE);

	size_t orig_size = size;
	uint8_t *src = buffer;
	size_t bsize = dev->block_size;
	uint64_t startblock = offset / bsize;

	if (bsize > bbufsize) {
		kfree(blockbuffer);
		blockbuffer = kmalloc(bsize);
		bbufsize = bsize;
	}

	size_t unaligned = offset % bsize;


	if (unaligned) {
		size_t tmpsize = bsize - unaligned;
		tmpsize = tmpsize > size ? size : tmpsize;

		/* the start position is not block aligned, so read the original block, */
		/* modify the changed parts and write it back.                          */
		int err = drv->read_blocks(dev, startblock, 1, blockbuffer);
		if (err != 0)
			return err;

		memcpy(blockbuffer + unaligned, src, tmpsize);

		err = drv->write_blocks(dev, startblock, 1, blockbuffer);
		if (err != 0)
			return err;

		size -= tmpsize;
		src += tmpsize;
		startblock++;
	}

	size_t numblocks = size / bsize;
	if (numblocks) {
		/* write numblocks contigious blocks to the device */
		int err = drv->write_blocks(dev, startblock, numblocks, src);
		if (err != 0)
			return err;

		size -= numblocks * bsize;
		src  += numblocks * bsize;
		startblock += numblocks;
	}

	if (size) {
		kassert(size < bsize);
		/* there's still something to write... */
		/* same procedure as for the first unaligned block */

		int err = drv->read_blocks(dev, startblock, 1, blockbuffer);
		if (err != 0)
			return err;

		memcpy(blockbuffer, src, size);

		err = drv->write_blocks(dev, startblock, 1, blockbuffer);
		if (err != 0)
			return err;
	}

	return orig_size;
}

static int dev_close(struct file *file)
{
	struct cdi_storage_device *dev = (struct cdi_storage_device*)file->inode->impl;
	struct cdi_storage_driver *drv = (struct cdi_storage_driver*)dev->dev.driver;

	if (drv->close) {
		return drv->close(dev);
	}
	return 0;
	return 0;
}
