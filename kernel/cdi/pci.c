#include <cdi/pci.h>
#include "cdi_impl.h"
#include "pci.h"
#include "mm/kmalloc.h"

static struct cdi_pci_device *make_cdi_dev(struct pci_dev *dev)
{
	struct cdi_pci_device *cdidev = kmalloc(sizeof(*cdidev));

	cdidev->bus = dev->bus;
	cdidev->dev = dev->dev;
	cdidev->function = dev->func;

	cdidev->vendor_id = dev->vendor_id;
	cdidev->device_id = dev->device_id;

	cdidev->class_id = dev->classcode;
	cdidev->subclass_id = dev->subclass;
	cdidev->interface_id = dev->prog_if;
	cdidev->rev_id   = dev->revision;

	cdidev->irq = dev->irq;

	cdidev->resources = cdi_list_create();

	return cdidev;
}

void cdi_pci_get_all_devices(cdi_list_t list)
{ LOG
	cdi_check_init();
	cdi_check_arg(list, != NULL);

	list_entry_t *e;
	list_iterate(e, pci_devices) {
		struct cdi_pci_device *dev = make_cdi_dev(e->data);
		cdi_list_push(list, dev);
	}
}

void cdi_pci_device_destroy(struct cdi_pci_device* device)
{ LOG
	cdi_check_init();
	cdi_check_arg(device, != NULL);

	cdi_list_destroy(device->resources);
	kfree(device);
}

void cdi_pci_alloc_ioports(struct cdi_pci_device* device)
{ LOG
	cdi_check_init();
	cdi_check_arg(device, != NULL);

	/* DUMMY - Nothing to do here... */
}

void cdi_pci_free_ioports(struct cdi_pci_device* device)
{ LOG
	cdi_check_init();
	cdi_check_arg(device, != NULL);

	/* DUMMY - Nothing to do here... */
}

void cdi_pci_alloc_memory(struct cdi_pci_device* device)
{ //LOG
	UNIMPLEMENTED
}

void cdi_pci_free_memory(struct cdi_pci_device* device)
{ //LOG
	UNIMPLEMENTED
}
