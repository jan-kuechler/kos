#include <page.h>
#include <cdi/misc.h>
#include "cdi_impl.h"
#include "idt.h"
#include "timer.h"
#include "mm/mm.h"
#include "mm/virt.h"

void cdi_register_irq(uint8_t irq, void (*handler)(struct cdi_device*),
    struct cdi_device* device)
{
	UNIMPLEMENTED
}

int cdi_reset_wait_irq(uint8_t irq)
{
	cdi_check_init(-1);
	cdi_check_arg(irq, < NUM_IRQ, -1);

	idt_reset_irq_counter(irq);

	return 0;
}

int cdi_wait_irq(uint8_t irq, uint32_t timeout)
{
	cdi_check_init(-1);
	cdi_check_arg(irq, < NUM_IRQ, -1);

	if (idt_wait_irq(irq, true, timeout))
		return 0;
	return -1;
}

int cdi_alloc_phys_mem(size_t size, void** vaddr, void** paddr)
{
	cdi_check_init(-1);
	cdi_check_arg(vaddr, != NULL, -1);
	cdi_check_arg(paddr, != NULL, -1);

	size_t pages = NUM_PAGES(size);
	if (!pages)
		return -1;

	paddr_t phys = mm_alloc_range(pages);
	if (paddr == NO_PAGE)
		return -1;

	vaddr_t virt = km_alloc_addr(phys, PE_PRESENT | PE_READWRITE, size);
	if (!virt)
		return -1;

	*vaddr = virt;
	*paddr = phys;

	return 0;
}

void* cdi_alloc_phys_addr(size_t size, uintptr_t paddr)
{
	UNIMPLEMENTED

	return (void*)0;
}

int cdi_ioports_alloc(uint16_t start, uint16_t count)
{
	cdi_check_init(-1);
	return 0;
}

int cdi_ioports_free(uint16_t start, uint16_t count)
{
	cdi_check_init(-1);
	return 0;
}

void cdi_sleep_ms(uint32_t ms)
{
	cdi_check_init();

	ksleep(ms);
}
