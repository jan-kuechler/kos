#ifndef KEYMAP_H
#define KEYMAP_H

#define NUM_KEYS 128

typedef struct keymap_entry
{
	char normal;
	char shift;
	char altgr;
} keymap_entry_t;

typedef keymap_entry_t *keymap_t;

#endif /*KEYMAP_H*/
