#include <page.h>
#include <string.h>
#include <types.h>

#include "mm/mm.h"

struct mem_node
{
	size_t	size;
	void    *ptr;

	struct mem_node *next;
};

struct mem_node *used_nodes; /* a list of all nodes that point to used memory */
struct mem_node *free_nodes; /* a list of all nodes that point to free memory */
struct mem_node *unused_nodes; /* a list of nodes, that do not yet point to any memory */

void append_node(struct mem_node **list, struct mem_node *node)
{
	node->next = *list;
	*list = node;
}

struct mem_node* create_unused_nodes(void)
{
	int i = 0;
	struct mem_node *nodes = mm_alloc_page();
	memset(nodes, 0, PAGE_SIZE);

	for (; i < (PAGE_SIZE / sizeof(struct mem_node)); ++i) {
		append_node(&unused_nodes, nodes + i);
	}

	return nodes;
}

struct mem_node *create_node(size_t size, void *ptr, byte is_free)
{
	struct mem_node *node = free_nodes;

	if (!node)
		node = create_unused_nodes();
	else
		free_nodes = free_nodes->next;

	node->size = size;
	node->ptr  = ptr;

	append_node(is_free ? &free_nodes : &used_nodes, node);

	return node;
}

void *malloc(size_t size)
{
	if (size < 1) return NULL;

	struct mem_node *prev = 0;
	struct mem_node *cur = free_nodes;

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

		void *ptr = mm_alloc_range(num_pages);

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

void free(void *ptr)
{
	struct mem_node *prev = 0;
	struct mem_node *cur = used_nodes;

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

void *calloc(size_t num, size_t size)
{
	size_t block_size = num * size;
	void *ptr = malloc(block_size);

	if (ptr)
		memset(ptr, 0, block_size);

	return ptr;
}

void *realloc(void *ptr, size_t size)
{
	if (!ptr && !size) {
		return NULL;
	}
	else if (!ptr) {
		return malloc(size);
	}
	else if (!size) {
		free(ptr);
		return NULL;
	}
	else {
		size_t origs = 0;
		void *new_ptr = 0;
		struct mem_node *cur = used_nodes;
		while (cur) {
			if (cur->ptr == ptr) {
				origs = cur->size;
				break;
			}
			cur = cur->next;
		}

		if (!origs)
			return NULL;

		new_ptr = malloc(size);

		if (!new_ptr)
			return NULL;

		memcpy(new_ptr, ptr, size > origs ? origs : size);

		free(ptr);

		return new_ptr;
	}
}
