#ifndef INITRD_H
#define INITRD_H

void id_init(void);

void id_start_dir(const char *name);
void id_end_dir(void);
void id_add_file(const char *name, const char *path);

void id_write(const char *file);

#endif /*INITRD_H*/
