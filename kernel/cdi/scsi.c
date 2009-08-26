#include <string.h>
#include <cdi/scsi.h>
#include "cdi_impl.h"
#include "mm/kmalloc.h"

struct cdi_scsi_packet* cdi_scsi_packet_alloc(size_t size)
{ LOG
	cdi_check_init(NULL);
	cdi_check_arg(size, > 0, NULL);

	struct cdi_scsi_packet *packet = kmalloc(sizeof(*packet));
	memset(packet, 0, sizeof(*packet));

	packet->buffer = kmalloc(size);
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
{
	UNIMPLEMENTED
}

void cdi_scsi_driver_register(struct cdi_scsi_driver* driver)
{
	UNIMPLEMENTED
}

void cdi_scsi_device_init(struct cdi_scsi_device* device)
{
	UNIMPLEMENTED
}
