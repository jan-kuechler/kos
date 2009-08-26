#ifndef PCI_H
#define PCI_H

#include <stdint.h>
#include "util/list.h"

struct pci_dev
{
	uint16_t bus;
	uint16_t dev;
	uint16_t func;

	uint16_t vendor_id;
	uint16_t device_id;

	uint8_t revision;
	uint8_t prog_if;
	uint8_t subclass;
	uint8_t classcode;

	uint8_t irq;
};

struct pci_res
{
};

extern list_t *pci_devices;

void init_pci(void);

void pci_print_table(void);


#endif /*PCI_H*/
