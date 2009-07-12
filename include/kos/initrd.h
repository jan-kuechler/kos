#ifndef KOS_INITRD_H
#define KOS_INITRD_H

#include <stdint.h>

#define ID_VERSION 0

#define ID_MAGIC0 'k'
#define ID_MAGIC1 'I'
#define ID_MAGIC2 'D'

#define ID_TYPE_FILE 0
#define ID_TYPE_DIR  1

struct id_header
{
	uint8_t magic[3];  /* The characters 'kID                 */
	uint8_t version;   /* The version of the initrd           */
};

/* Note: All offsets count from the beginning of the     *
 *       initrd file, not from the entry's beginning .   */

struct id_entry
{
	uint32_t type;     /* The type of this entry              */

	uint32_t name;     /* Offset to a zero terminated string  *
	                    * containing the name of this entry   */

	uint32_t count;    /* For files this is the file size,    *
	                    * and for directories the number of   *
	                    * files in it                         */

	uint32_t content;  /* Offset to the content of this entry */

	uint32_t next;     /* Offset to the next entry in this    *
	                    * directory, or 0 if this entry is    *
	                    * the last one.                       */
};

#define id_get(e,w,d) (((e)->w) ? ((d) + ((e)->w)) : 0)

#endif /*KOS_INITRD_H*/
