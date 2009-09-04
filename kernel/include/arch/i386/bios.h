#ifndef I386_BIOS_H
#define I386_BIOS_H

#include <types.h>

struct bios_data_area
{
	/* COM port I/O ports, or 0 if none */
	word com_io[4];

	/* LP I/O ports, or 0 if none */
	word lp_io[4];

	word equipment;

	byte post_status;;

	word base_mem; /* kB */

	byte reserved1;
	byte reserved2;

	word keyb_flags;
	byte numpad_data;
	word keyb_buf_next;
	word keyb_buf_last;
	byte keyb_buffer[32];

	struct floppy_info
	{
		byte recalibrate_status;
		byte motor_status;
		byte timeout_count;
		byte status;
		byte cmd[7];
	} floppy_info;

	struct video_info
	{
		byte cur_mode;
		word cols;
		word page_size;
		word cur_page_start;
		byte csr_pos[16];
		word csr_type;
		byte cur_page_num;
		word crt_base_addr;
		byte mode_select;
		byte cga_palette;
	} video_info;

	dword rm_reentry;
	byte  last_unexp_intr;

	dword ticks_since_midnight;
	byte  timer_overflow;

	byte keyb_ctrl_brk;

	word reset_flag;

	byte disk_op_status;
	byte disk_num_drives;
	byte disk_ctrl;
	byte disk_io_port;

	byte lp_timeout[4];
	byte com_timeout[4];

	word keyb_buf_start;
	word keyb_buf_end;

	struct video_info2
	{
		byte rows;
		word char_height;
		byte ctrl;
		byte switches;
		byte mode_set_ctrl;
		byte dcc_index;
	} video_info2;

	struct floppy_info2
	{
		byte media_ctrl;
		byte status;
		byte error;
		byte intr;
		byte info;
		byte media_state[2];
		byte media_prev_state[2];
		byte cur_track[2];
	} floppy_info2;

	word keyb_status;

	dword user_flag_ptr;
	dword user_wait_cnt;
	byte  wait_flag;

	byte  network_reserved[7];

	dword video_mode_table;

	byte  user_reserved[68];

	byte  app_com_area[16];

} __attribute__((packed));

#define BIOS_DATA_ADDR 0x0400


#endif /* I386_BIOS_H */
