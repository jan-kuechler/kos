#include <cdi.h>
#include "cdi_impl.h"
#include "util/list.h"

static list_t *cdi_drivers = 0;

int cdi_initialized = 0;

void cdi_init()
{
	if (!cdi_initialized) {
		cdi_drivers = list_create();

		cdi_initialized = 1;
	}
}

void cdi_run_drivers(void)
{
	UNIMPLEMENTED
}

void cdi_driver_init(struct cdi_driver *driver)
{
	cdi_check_init();

	if (!driver) {
		cdi_error("driver is NULL");
		return;
	}

	driver->devices = cdi_list_create();
}

void cdi_driver_destroy(struct cdi_driver *driver)
{
	cdi_check_init();

	if (!driver) {
		cdi_error("driver is NULL");
		return;
	}

	cdi_list_destroy(driver->devices);
}

void cdi_driver_register(struct cdi_driver *driver)
{
	cdi_check_init();

	if (!driver) {
		cdi_error("driver is NULL");
		return;
	}

	list_add_back(cdi_drivers, driver);

	/* TODO: Create device file for driver. */
}
