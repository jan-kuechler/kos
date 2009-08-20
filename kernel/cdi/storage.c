#include <cdi/storage.h>
#include "cdi_impl.h"

void cdi_storage_driver_init(struct cdi_storage_driver* driver)
{
	LOG

	cdi_check_init();
	cdi_check_arg(driver, != NULL);

	driver->drv.type = CDI_STORAGE;
	cdi_driver_init(&driver->drv);
}

void cdi_storage_driver_destroy(struct cdi_storage_driver* driver)
{
	UNIMPLEMENTED
}

void cdi_storage_driver_register(struct cdi_storage_driver* driver)
{
	UNIMPLEMENTED
}

void cdi_storage_device_init(struct cdi_storage_device* device)
{
	UNIMPLEMENTED
}
