#ifndef KEYMAP_H
#define KEYMAP_H

#define NUM_KEYS 128

#define EOT 0xFF

typedef struct keymap_entry
{
	char normal;
	char shift;
	char altgr;
	char ctrl;
} keymap_entry_t;

typedef keymap_entry_t *keymap_t;

extern keymap_entry_t keymap_de[];

#endif /*KEYMAP_H*/
