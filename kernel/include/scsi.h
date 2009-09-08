#ifndef SCSI_H
#define SCSI_H

#include <compiler.h>
#include <stdint.h>

enum scsi_opcodes
{
	SCSI_OP_REQSENSE  = 0x03,
	SCSI_OP_INQUIRY   = 0x12,
	SCSI_OP_STARTSTOP = 0x1B,
	SCSI_OP_CAPACITY  = 0x25,
	SCSI_OP_READ_12   = 0xA8,
};

#define SCSI_CMDSIZE 12
#define SCSI_OPCODE 0

enum scsi_motor_media_cmd
{
	SCSI_STOP_MOTOR  = 0x00, // 0b00
	SCSI_START_MOTOR = 0x01, // 0b01
	SCSI_EJECT_MEDIA = 0x02, // 0b10
	SCSI_LOAD_MEDIA  = 0x03, // 0b11
};

struct scsi_inquiry_data
{
	int dev_type          : 5;
	int qualifier         : 3;

	int dev_type_modifier : 7;
	int removable         : 1;

	int ansi_version      : 3;
	int ecma_version      : 3;
	int iso_version       : 2;

	int response_format   : 4;
	int rsvd0             : 2;
	int terminate_ioproc  : 1;
	int async_notify      : 1;

	int add_length        : 8;

	int rsvd1             : 8;

	int rsvd2             : 8;

	int soft_reset        : 1;
	int cmd_queuing       : 1;
	int rsvd3             : 1;
	int linked            : 1;
	int sync_transfers    : 1;
	int wbus_16           : 1;
	int wbus_32           : 1;
	int real_addr_mode    : 1;

	char vendor_id[8];
	char product_id[8];
} __packed;

struct scsi_capacity_data
{
	uint32_t lba;
	uint32_t block_size;
} __packed;

#endif /*SCSI_H*/
