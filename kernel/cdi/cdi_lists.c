#include <cdi/lists.h>
#include "cdi_impl.h"

struct cdi_list_implementation
{
};

cdi_list_t cdi_list_create(void)
{
	UNIMPLEMENTED

	return (void*)0;
}

void cdi_list_destroy(cdi_list_t list)
{
	UNIMPLEMENTED
}

cdi_list_t cdi_list_push(cdi_list_t list, void* value)
{
	UNIMPLEMENTED

	return (void*)0;
}

void* cdi_list_pop(cdi_list_t list)
{
	UNIMPLEMENTED

	return (void*)0;
}

size_t cdi_list_empty(cdi_list_t list)
{
	UNIMPLEMENTED

	return 0;
}

void* cdi_list_get(cdi_list_t list, size_t index)
{
	UNIMPLEMENTED

	return (void*)0;
}

cdi_list_t cdi_list_insert(cdi_list_t list, size_t index, void* value)
{
	UNIMPLEMENTED

	return (void*)0;
}

void* cdi_list_remove(cdi_list_t list, size_t index)
{
	UNIMPLEMENTED

	return (void*)0;
}

size_t cdi_list_size(cdi_list_t list)
{
	UNIMPLEMENTED

	return 0;
}
