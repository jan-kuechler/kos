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
	list_entry_t *e;
	list_iterate(e, cdi_drivers) {
		struct cdi_driver *driver = e->data;

		list_entry_t *de;
		list_iterate(de, driver->devices->list) {
			struct cdi_device *dev = de->data;
			dev->driver = driver;
			if (driver->init_device) {
				driver->init_device(dev);
			}
		}

	}
}

void cdi_driver_init(struct cdi_driver *driver)
{
	cdi_check_init();
	cdi_check_arg(driver, != NULL);

	driver->devices = cdi_list_create();
}

void cdi_driver_destroy(struct cdi_driver *driver)
{
	cdi_check_init();
	cdi_check_arg(driver, != NULL);

	cdi_list_destroy(driver->devices);
}

void cdi_driver_register(struct cdi_driver *driver)
{
	cdi_check_init();
	cdi_check_arg(driver, != NULL);

	list_add_back(cdi_drivers, driver);
}
