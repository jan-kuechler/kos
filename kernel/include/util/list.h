#ifndef UTIL_LIST_H
#define UTIL_LIST_H

typedef struct list_entry
{
	void *data;
	struct list_entry *next, *prev;
} list_entry_t;

typedef struct list
{
	list_entry_t *front, *back;
	int size;
} list_t;

list_t *list_create();
void list_destroy(list_t *list);

int list_empty(list_t *list);
int list_size(list_t *list);

list_t *list_add_front(list_t *list, void *data);
list_t *list_add_back(list_t *list, void *data);

void *list_del_front(list_t *list);
void *list_del_back(list_t *list);
void *list_del_entry(list_t *list, list_entry_t *entry);

void *list_front(list_t *list);
void *list_back(list_t *list);

#define list_iterate(pos, list) for (pos = list->front; pos != 0; pos = pos->next)

#define list_riterate(pos, list) for (pos = list->back; pos != 0; pos = pos->prev)

#endif /*UTIL_LIST_H*/
