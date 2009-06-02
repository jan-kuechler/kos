#ifndef MODULE_H
#define MODULE_H

void init_mod();
int mod_load(int n, void **module, char **params);

void mod_exec(int n);
void mod_load_all();

#endif /*MODULE_H*/
