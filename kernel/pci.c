#include <bitop.h>
#include <stdbool.h>
#include <ports.h>
#include "pci.h"
#include "mm/kmalloc.h"

enum pci_config
{
	PCI_CONFIG_ADDR = 0xCF8,
	PCI_CONFIG_DATA = 0xCFC,
};

enum pci_offset
{
	PCI_VENDOR_ID  = 0x00,
	PCI_DEVICE_ID  = 0x02,
	PCI_COMMAND    = 0x04,
	PCI_STATUS     = 0x06,
	PCI_REVISION   = 0x08,
	PCI_PROG_IF    = 0x09,
	PCI_SUBCLASS   = 0x0A,
	PCI_CLASS      = 0x0B,
	PCI_HEADERTYPE = 0x0E,
	PCI_IRQ        = 0x3C,
};

#define PCI_HTYPE_BIT 7

list_t *pci_devices;

static uint32_t cfg_read(uint32_t bus, uint32_t dev, uint32_t func,
                         enum pci_offset reg)
{
	uint32_t offs = reg % 4;
	uint32_t addr = 0x80000000 | (bus << 16) | ((dev & 0x1F) << 11) |
	                ((func & 0x7) << 8) | ((uint32_t)reg & 0xFC);

	outl(PCI_CONFIG_ADDR, addr);
	return inl(PCI_CONFIG_DATA) >> (offs * 8);
}

static void cfg_write(uint32_t bus, uint32_t dev, uint32_t func,
                      enum pci_offset reg, uint32_t value)
{
}

static bool has_func(uint32_t bus, uint32_t dev)
{
	return bissetn(cfg_read(bus, dev, 0, PCI_HEADERTYPE), PCI_HTYPE_BIT);
}

static struct pci_dev *load(uint32_t bus, uint32_t dev, uint32_t func)
{
	/* only load present devices */
	uint16_t vendor_id = bmask(cfg_read(bus, dev, func, PCI_VENDOR_ID), BMASK_WORD);
	if (!vendor_id || vendor_id == 0xFFFF)
		return NULL;

	struct pci_dev *device = kmalloc(sizeof(*device));

	device->bus = bus;
	device->dev = dev;
	device->func = func;

	device->vendor_id = vendor_id;
	device->device_id = bmask(cfg_read(bus, dev, func, PCI_DEVICE_ID), BMASK_WORD);

	device->revision  = bmask(cfg_read(bus, dev, func, PCI_REVISION), BMASK_BYTE);
	device->prog_if   = bmask(cfg_read(bus, dev, func, PCI_PROG_IF),  BMASK_BYTE);
	device->subclass  = bmask(cfg_read(bus, dev, func, PCI_SUBCLASS), BMASK_BYTE);
	device->classcode = bmask(cfg_read(bus, dev, func, PCI_CLASS),    BMASK_BYTE);

	device->irq       = bmask(cfg_read(bus, dev, func, PCI_IRQ), BMASK_BYTE);

	return device;
}

static void add_dev(uint32_t bus, uint32_t dev, uint32_t func)
{
	if (func && !has_func(bus, dev))
		return;

	struct pci_dev *device = load(bus, dev, func);
	if (!device)
		return;

	list_add_back(pci_devices, device);
}

void pci_print_table(void)
{
	kout_printf("PCI Devices: \n");
	kout_printf("+-----+-----+------+-----------+-----------+-------+----------+-----------+\n");
	kout_printf("| Bus | Dev | Func | Vendor ID | Device ID | Class | Subclass | Interface |\n");
	kout_printf("+-----+-----+------+-----------+-----------+-------+----------+-----------+\n");

	list_entry_t *e;
	list_iterate(e, pci_devices) {
		struct pci_dev *dev = e->data;
		kout_printf("|   %d |  %02d |    %d |    0x%04x |    0x%04x |  0x%02x |     0x%02x |      0x%02x |\n",
		            dev->bus, dev->dev, dev->func, dev->vendor_id, dev->device_id,
		            dev->classcode, dev->subclass, dev->prog_if);
	}

	kout_printf("+-----+-----+------+-----------+-----------+-------+----------+-----------+\n");
}

void init_pci(void)
{
	pci_devices = list_create();

	int bus = 0;
	int dev = 0;
	int func = 0;

	for (; bus < 8; bus++) {
		for (dev = 0; dev < 32; ++dev) {
			for (func = 0; func < 8; ++func) {
				add_dev(bus, dev, func);
			}
		}
	}
}
