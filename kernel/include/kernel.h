#ifndef KERNEL_H
#define KERNEL_H

#include <multiboot.h>
#include "linker.h"

#define DEFAULT_LOGLEVEL 6

#define KERN_EMERG      "<0>"   /* system is unusable                   */
#define KERN_ALERT      "<1>"   /* action must be taken immediately     */
#define KERN_CRIT       "<2>"   /* critical conditions                  */
#define KERN_ERR        "<3>"   /* error conditions                     */
#define KERN_WARNING    "<4>"   /* warning conditions                   */
#define KERN_NOTICE     "<5>"   /* normal but significant condition     */
#define KERN_INFO       "<6>"   /* informational                        */
#define KERN_DEBUG      "<7>"   /* debug-level messages                 */

#define KERN_DEFAULT    "<d>"

extern int printk(const char *fmt, ...);

extern multiboot_info_t multiboot_info;

linker_symbol(kernel_phys_start);
linker_symbol(kernel_phys_end);
linker_symbol(kernel_start);
linker_symbol(kernel_end);
linker_symbol(kernel_size);

struct regs;
void print_state(struct regs *regs);

void shutdown();
void panic(const char *fmt, ...);

#endif /*KERNEL_H*/
