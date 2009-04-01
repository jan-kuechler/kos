#ifndef KOS_CONFIG_H
#define KOS_CONFIG_H

/**
 *  CONF_DEBUG
 *
 * This controls debug builds
 */
#define CONF_DEBUG 1

/**
 *  CONF_SAFE_KMALLOC
 *
 * If this is defined a kmalloc return of NULL results in
 * a panic.
 */
#define CONF_SAFE_KMALLOC 1

/**
 *  CONF_PROG_ENTRY_MIN
 *
 * This is the minimal linking base
 * to run under kOS.
 */
#define CONF_PROG_BASE_MIN 0x40000000

#endif /*KOS_CONFIG_H*/
