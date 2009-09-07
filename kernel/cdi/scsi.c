#include <bitop.h>
#include <compiler.h>
#include <string.h>
#include <cdi/scsi.h>
#include <cdi/storage.h>
#include "cdi_impl.h"
#include "scsi.h"
#include "mm/kmalloc.h"

static int scsi_read(struct cdi_storage_device *dev, uint64_t start,
                     uint64_t count, void *buffer);
static int scsi_write(struct cdi_storage_device *dev, uint64_t start,
                      uint64_t count, void *buffer);

static int scsi_open(struct cdi_storage_device *dev);
static int scsi_close(struct cdi_storage_device *dev);

static struct cdi_storage_driver scsi_storage_driver = {
	.read_blocks = scsi_read,
	.write_blocks = scsi_write,

	.open = scsi_open,
	.close = scsi_close,
};

struct scsi_impl
{
	struct cdi_storage_device *front;
	struct scsi_inquiry_data   inquiry;
};

static void init_scsi_storage(struct cdi_scsi_device* device);

struct cdi_scsi_packet* cdi_scsi_packet_alloc(size_t size)
{ LOG
	cdi_check_init(NULL);
	cdi_check_arg(size, > 0, NULL);

	struct cdi_scsi_packet *packet = kcalloc(1, sizeof(*packet));

	packet->buffer = kcalloc(1, size);
	packet->bufsize = size;

	return packet;
}

void cdi_scsi_packet_free(struct cdi_scsi_packet* packet)
{ LOG
	cdi_check_init();
	cdi_check_arg(packet, != NULL);

	kfree(packet->buffer);
	kfree(packet);
}

void cdi_scsi_driver_init(struct cdi_scsi_driver* driver)
{ LOG
	cdi_check_init();
	cdi_check_arg(driver, != NULL);

	driver->drv.type = CDI_SCSI;
	cdi_driver_init(&driver->drv);
}

void cdi_scsi_driver_destroy(struct cdi_scsi_driver* driver)
{ LOG
	cdi_check_init();
	cdi_check_arg(driver, != NULL);

	cdi_driver_destroy(&driver->drv);
}

void cdi_scsi_driver_register(struct cdi_scsi_driver* driver)
{ LOG
	cdi_check_init();
	cdi_check_arg(driver, != NULL);

	cdi_driver_register(&driver->drv);
}

void cdi_scsi_device_init(struct cdi_scsi_device* device)
{ LOG
	cdi_check_init();
	cdi_check_arg(device, != NULL);

	switch (device->type) {
	case CDI_STORAGE:
		init_scsi_storage(device);
		break;

	default:
		cdi_error("SCSI Devices of this type are not supported.");
	}
}

static int send_inquiry(struct cdi_scsi_device *dev)
{
	struct cdi_scsi_driver *drv = (struct cdi_scsi_driver*)dev->dev.driver;
	struct cdi_scsi_packet packet;
	struct scsi_impl *impl = dev->impl;

	memset(&packet, 0, sizeof(packet));
	memset(&impl->inquiry, 0, sizeof(impl->inquiry));

	packet.direction = CDI_SCSI_READ;
	packet.buffer    = &impl->inquiry;
	packet.bufsize   = sizeof(impl->inquiry);
	packet.cmdsize   = 6;

	packet.command[0] = SCSI_OP_INQUIRY;
	packet.command[4] = sizeof(impl->inquiry);

	int err = drv->request(dev, &packet);
	if (!err) {
		impl->inquiry.vendor_id[7] = '\0';
		impl->inquiry.product_id[7] = '\0';
	}
	return err;
}

static int ctrl_motor(struct cdi_scsi_device *dev, enum scsi_motor_media_cmd ctrl)
{
	struct cdi_scsi_driver *drv = (struct cdi_scsi_driver*)dev->dev.driver;
	struct cdi_scsi_packet packet;
	union scsi_cmd *cmd = (union scsi_cmd*)&packet.command;

	memset(&packet, 0, sizeof(packet));

	packet.direction = CDI_SCSI_NODATA;
	packet.cmdsize   = 6;

	packet.command[0] = SCSI_OP_STARTSTOP;
	packet.command[4] = (uint8_t)ctrl;

	return drv->request(dev, &packet);
}

static int get_capacity(struct cdi_scsi_device *dev,
                        uint32_t *scount, uint32_t *ssize)
{
	struct cdi_scsi_driver *drv = (struct cdi_scsi_driver*)dev->dev.driver;
	struct cdi_scsi_packet packet;
	union scsi_cmd *cmd = (union scsi_cmd*)&packet.command;
	uint32_t data[2];

	memset(&packet, 0, sizeof(packet));
	memset(data, 0, sizeof(data));

	packet.direction = CDI_SCSI_READ;
	packet.buffer    = data;
	packet.bufsize   = sizeof(data);
	packet.cmdsize   = 10;

	cmd->ext.opcode = SCSI_OP_CAPACITY;

	int err = drv->request(dev, &packet);

	if (!err) {
		*scount = big_endian_dword(data[0]);
		*ssize  = big_endian_dword(data[1]);
	}
	return err;
}

static int read_sector(struct cdi_scsi_device *dev, uint32_t sector,
                       uint32_t bsize, void *buffer)
{
	struct cdi_scsi_driver *drv = (struct cdi_scsi_driver*)dev->dev.driver;
	struct cdi_scsi_packet packet;
	union scsi_cmd *cmd = (union scsi_cmd*)&packet.command;

	memset(&packet, 0, sizeof(packet));
	packet.direction = CDI_SCSI_READ;
	packet.buffer    = buffer;
	packet.bufsize   = bsize;
	packet.cmdsize   = 12;

	cmd->ext.opcode = SCSI_OP_READ_12;
	cmd->ext.addr   = big_endian_dword(sector);
	cmd->ext.len    = 0x01000000; /* FIXME: Why 0x01000000 ??*/

	return drv->request(dev, &packet);
}

static int request_sense(struct cdi_scsi_device *dev, uint8_t *key,
                         uint8_t *code, uint8_t *qualifier)
{
	struct cdi_scsi_driver *drv = (struct cdi_scsi_driver*)dev->dev.driver;
	struct cdi_scsi_packet packet;
	union scsi_cmd *cmd = (union scsi_cmd*)&packet.command;

	uint8_t data[18];

	memset(&packet, 0, sizeof(packet));
	packet.direction = CDI_SCSI_READ;
	packet.buffer    = data;
	packet.bufsize   = sizeof(data);
	packet.cmdsize   = 12;

	cmd->dfl.opcode = SCSI_OP_REQSENSE;
	packet.command[4] = sizeof(data);

	int err = drv->request(dev, &packet);

	if (!err) {
		*key       = data[2];
		*code      = data[12];
		*qualifier = data[13];
	}
	return err;
}

static void print_sense(struct cdi_scsi_device *dev)
{
	uint8_t key = 0;
	uint8_t code = 0;
	uint8_t qualifier = 0;

	int err = request_sense(dev, &key, &code, &qualifier);

	if (!err) {
		dbg_printf(DBG_CDI, "SCSI Sense for %s\n"
	                    "Key:       %x\n"
	                    "Code:      %x\n"
	                    "Qualifier: %x\n",
		           dev->dev.name, bmask(key, BMASK_4BIT), code, qualifier);
	}
	else {
		dbg_printf(DBG_CDI, "Error while reading SCSI sense for %s: %d\n",
		           dev->dev.name, err);
	}
}

static void print_device(struct cdi_scsi_device *dev)
{
	struct scsi_impl *impl = dev->impl;

	dbg_printf(DBG_CDI, "SCSI device %s:\n", dev->dev.name);
	dbg_printf(DBG_CDI, "  Type:    0x%02x\n", impl->inquiry.dev_type);
	dbg_printf(DBG_CDI, "  Vendor:  '%s'\n", impl->inquiry.vendor_id);
	dbg_printf(DBG_CDI, "  Product: '%s'\n", impl->inquiry.product_id);
}

static void init_scsi_storage(struct cdi_scsi_device* device)
{
	struct scsi_impl *impl = kcalloc(1, sizeof(*impl));
	device->impl = impl;

	int err;
	if ((err = send_inquiry(device)) != 0) {
		cdi_error("Inquiry command failed with code %d.", err);
		goto error;
	}
	print_device(device);

	if ((err = ctrl_motor(device, SCSI_START_MOTOR)) != 0) {
		cdi_error("Motor start failed with code %d.", err);
		goto error;
	}

	uint32_t sector_count = 0;
	uint32_t sector_size = 0;
	get_capacity(device, &sector_count, &sector_size);
	dbg_printf(DBG_CDI, "Capacity for %s: %d x %d b\n", device->dev.name, sector_count, sector_size);

	struct cdi_storage_device *front = kcalloc(1, sizeof(*front));

	front->dev.driver = (struct cdi_driver*)&scsi_storage_driver;
	front->dev.type   = CDI_STORAGE;
	front->dev.name   = device->dev.name;

	front->block_size  = sector_size;
	front->block_count = sector_count;

	front->dev.impl = device;
	impl->front = front;

	cdi_storage_device_init(front);

error:
	kfree(impl);
	return;
}

static int scsi_read(struct cdi_storage_device *dev, uint64_t start,
                     uint64_t count, void *buffer)
{
	uint8_t *dst = buffer;
	struct cdi_scsi_device *scsi_dev = dev->dev.impl;

	while (count--) {
		if (read_sector(scsi_dev, start, dev->block_size, dst) != 0) {
			return -1;
		}

		start++;
		dst += dev->block_size;
	}

	return 0;
}

static int scsi_write(struct cdi_storage_device *dev, uint64_t start,
                      uint64_t count, void *buffer)
{
	return -1;
}

static int scsi_open(struct cdi_storage_device *dev)
{
	struct cdi_scsi_device *scsi_dev = dev->dev.impl;

	int err = ctrl_motor(scsi_dev, SCSI_START_MOTOR);
	if (err)
		return err;

	if (!dev->block_count) {
		uint32_t sector_count = 0;
		uint32_t sector_size = 0;
		err = get_capacity(scsi_dev, &sector_count, &sector_size);

		if (err)
			return err;

		dev->block_size  = sector_size;
		dev->block_count = sector_count;
	}
}

static int scsi_close(struct cdi_storage_device *dev)
{
	struct cdi_scsi_device *scsi_dev = dev->dev.impl;
	return ctrl_motor(scsi_dev, SCSI_STOP_MOTOR);
}
