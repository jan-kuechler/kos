#include <cdi/lists.h>
#include "cdi_impl.h"
#include "mm/kmalloc.h"
#include "util/list.h"

struct cdi_list_implementation
{
	list_t *list;
};

cdi_list_t cdi_list_create(void)
{
	cdi_check_init(return NULL);

	cdi_list_t list = kmalloc(sizeof(struct cdi_list_implementation));

	list->list = list_create();

	return list;
}

void cdi_list_destroy(cdi_list_t list)
{
	cdi_check_init();
	cdi_check_arg(list, != NULL);

	list_destroy(list->list);
	kfree(list);
}

cdi_list_t cdi_list_push(cdi_list_t list, void* value)
{
	cdi_check_init(return NULL);
	cdi_check_arg(list, != NULL);

	list_add_front(list->list, value);

	return list;
}

void* cdi_list_pop(cdi_list_t list)
{
	cdi_check_init(return NULL);
	cdi_check_arg(list, != NULL);

	return list_del_front(list->list);
}

size_t cdi_list_empty(cdi_list_t list)
{
	cdi_check_init(return 0);
	cdi_check_arg(list, != NULL);

	return list_empty(list->list);
}

void* cdi_list_get(cdi_list_t list, size_t index)
{
	cdi_check_init(return NULL);
	cdi_check_arg(list, != NULL);

	list_entry_t *e;
	int i = 0;
	list_iterate(e, list->list) {
		if (i == index)
			return e->data;
		++i;
	}

	return NULL;
}

cdi_list_t cdi_list_insert(cdi_list_t list, size_t index, void* value)
{
	cdi_check_init(return NULL);
	cdi_check_arg(list, != NULL);

	list_entry_t *e;
	int i = 0;
	list_iterate(e, list->list) {
		if (i == index) {
			list_add_after(list->list, e, value);
			return list;
		}
		++i;
	}

	return (void*)0;
}

void* cdi_list_remove(cdi_list_t list, size_t index)
{
	cdi_check_init(return NULL);
	cdi_check_arg(list, != NULL);

	list_entry_t *e;
	int i = 0;
	list_iterate(e, list->list) {
		if (i == index) {
			return list_del_entry(list->list, e);
		}
		++i;
	}

	return (void*)0;
}

size_t cdi_list_size(cdi_list_t list)
{
	cdi_check_init(return 0);
	cdi_check_arg(list, != NULL);

	return list_size(list->list);
}
