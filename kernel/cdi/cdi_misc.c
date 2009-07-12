#include <cdi/misc.h>
#include "cdi_impl.h"

void cdi_register_irq(uint8_t irq, void (*handler)(struct cdi_device*),
    struct cdi_device* device)
{
	UNIMPLEMENTED
}

int cdi_reset_wait_irq(uint8_t irq)
{
	UNIMPLEMENTED

	return 0;
}

int cdi_wait_irq(uint8_t irq, uint32_t timeout)
{
	UNIMPLEMENTED

	return 0;
}

int cdi_alloc_phys_mem(size_t size, void** vaddr, void** paddr)
{
	UNIMPLEMENTED

	return 0;
}

void* cdi_alloc_phys_addr(size_t size, uintptr_t paddr)
{
	UNIMPLEMENTED

	return (void*)0;
}

int cdi_ioports_alloc(uint16_t start, uint16_t count)
{
	UNIMPLEMENTED

	return 0;
}

int cdi_ioports_free(uint16_t start, uint16_t count)
{
	UNIMPLEMENTED

	return 0;
}

void cdi_sleep_ms(uint32_t ms)
{
	UNIMPLEMENTED
}
