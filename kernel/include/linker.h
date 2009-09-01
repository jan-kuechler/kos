#ifndef SECTIONS_H
#define SECTIONS_H

#define linker_symbol(name) extern void name(void)

#define __section(s) __attribute__((section(#s)))

#define __init      __section(.init)
#define __initdata  __section(.init.data)
#define __initparam __section(.init.param)

#endif /*SECTIONS_H*/
