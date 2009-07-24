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
 *  CONF_KOOP_MULTITASKING
 *
 * If this is defined the kOS scheduler uses kooperative
 * multitasking.
 */
//#define CONF_KOOP_MULTITASKING

/**
 *  CONF_PROG_BASE_MIN
 *
 * This is the minimal linking base for programs
 * to run under kOS.
 */
#define CONF_PROG_BASE_MIN 0x40000000

/**
 *  CONF_CDI_SECURE
 *
 * Define this to enable extra checks for CDI functions.
 */
#define CONF_CDI_SECURE 1

/**
 *  CONF_CDI_ERR_FATAL
 *
 * Define this to make cdi_error emit a kernel panic.
 * Otherwise it's just a dbg_error output.
 */
#define CONF_CDI_ERR_FATAL 1

#endif /*KOS_CONFIG_H*/
