#ifndef MODULE_H
#define MODULE_H

#include <types.h>

/**
 *  mod_load(n, module, params)
 *
 * Loads the nth module and returns it size.
 */
size_t mod_load(int n, void **module, char **params);

/**
 *  mod_size(n)
 *
 * Returns the size of the nth module.
 * If the module was not loaded it returns 0.
 */
size_t mod_size(int n);

void init_mod(void);

#endif /*MODULE_H*/
