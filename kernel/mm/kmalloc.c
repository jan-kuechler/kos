#include <page.h>
#include <string.h>

#include <kos/config.h>

#include "kernel.h"
#include "mm/kmalloc.h"
#include "mm/mm.h"
#include "mm/virt.h"

typedef struct mem_node
{
	size_t size;
	void  *ptr;

	struct mem_node *next;
} mem_node_t;

static mem_node_t *used_nodes;
static mem_node_t *free_nodes;
static mem_node_t *unused_nodes;

static inline void append_node(mem_node_t **list, mem_node_t *node)
{
	node->next = *list;
	*list = node;
}

static mem_node_t* create_unused_nodes(void)
{
	int i = 0;
	mem_node_t *nodes = km_alloc_page();
	memset(nodes, 0, PAGE_SIZE);

	for (; i < (PAGE_SIZE / sizeof(mem_node_t)); ++i) {
		append_node(&unused_nodes, nodes + i);
	}

	return nodes;
}

static mem_node_t *create_node(size_t size, void *ptr, byte is_free)
{
	mem_node_t *node = free_nodes;

	if (!node)
		node = create_unused_nodes();
	else
		free_nodes = free_nodes->next;

	node->size = size;
	node->ptr  = ptr;

	append_node(is_free ? &free_nodes : &used_nodes, node);

	return node;
}

void *kmallocu(size_t size)
{
	if (size < 1) return NULL;

	mem_node_t *prev = 0;
	mem_node_t *cur = free_nodes;

	while (cur) {
		if (cur->size >= size)
			break;

		prev = cur;
		cur  = cur->next;
	}

	if (!cur) { /* get some more memory */
		size_t num_pages = size / PAGE_SIZE;

		if ((size % PAGE_SIZE) != 0)
			num_pages++;

		void *ptr = km_alloc_range(num_pages);

		cur = create_node(size, ptr, 0);

		/* there's enough free size to get another node in there */
		if ((num_pages * PAGE_SIZE) > size) {
			create_node((num_pages * PAGE_SIZE) - size, ptr + size, 1);
		}
	}
	else { /* got a free node with the right size, remove it from the list and return the pointer */
		if (!prev)
			free_nodes = cur->next;
		else
			prev->next = cur->next;

		append_node(&used_nodes, cur);

		if  (cur->size > size) { /* create a block for the rest of the memory */
			create_node(cur->size - size, cur->ptr + size, 1);
			cur->size = size;
		}
	}
	return cur->ptr;
}

void *kmalloc(size_t size)
{
	if (size < 1) return NULL;

	void *ptr = kmallocu(size);

#ifdef CONF_SAFE_KMALLOC
	if (!ptr) {
		panic("kmalloc: failed to alloc %d bytes.", size);
	}
#endif

	return ptr;
}

void kfree(void *ptr)
{
	mem_node_t *prev = 0;
	mem_node_t *cur = used_nodes;

	while (cur) {
		if (cur->ptr == ptr) {
			if (!prev)
				used_nodes = cur->next;
			else
				prev->next = cur->next;

			append_node(&free_nodes, cur);

			return;
		}
		prev = cur;
		cur  = cur->next;
	}
}

void *kcalloc(size_t num, size_t size)
{
	size_t block_size = num * size;
	void *ptr = kmalloc(block_size);

	if (ptr)
		memset(ptr, 0, block_size);

	return ptr;
}

void *krealloc(void *ptr, size_t size)
{
	if (!ptr && !size) {
		return NULL;
	}
	else if (!ptr) {
		return kmalloc(size);
	}
	else if (!size) {
		kfree(ptr);
		return NULL;
	}
	else {
		size_t origs = 0;
		void *new_ptr = 0;
		mem_node_t *cur = used_nodes;
		while (cur) {
			if (cur->ptr == ptr) {
				origs = cur->size;
				break;
			}
			cur = cur->next;
		}

		if (!origs)
			return NULL;

		new_ptr = kmalloc(size);

		if (!new_ptr)
			return NULL;

		memcpy(new_ptr, ptr, size > origs ? origs : size);

		kfree(ptr);

		return new_ptr;
	}
}

