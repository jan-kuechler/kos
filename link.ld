LOAD_ADDR = 0x100000;
LINK_ADDR = 0x100000;
VIRT_TO_PHYS = LINK_ADDR - LOAD_ADDR;

ENTRY(kstart)
SECTIONS
{
	. = LINK_ADDR;

	kernel_start = .;
	kernel_phys_start = . - VIRT_TO_PHYS;

	.header : AT(ADDR(.header) - VIRT_TO_PHYS)
	{
		*(.header)
	}
	.init : AT(ADDR(.init) - VIRT_TO_PHYS)
	{
		*(.init)
	}
	.text : AT(ADDR(.text) - VIRT_TO_PHYS)
	{
		*(.text)
	}
	.rodata ALIGN(4096) : AT(ADDR(.rodata) - VIRT_TO_PHYS)
	{
		*(.data)
	}
	.data ALIGN(4096) : AT(ADDR(.data) - VIRT_TO_PHYS)
	{
		*(.data)
	}
	.bss ALIGN(4096) : AT(ADDR(.bss) - VIRT_TO_PHYS)
	{
		*(.bss)
	}

	. = ALIGN(4096);

	kernel_end = .;
	kernel_phys_end = . - VIRT_TO_PHYS;

	kernel_size = kernel_end - kernel_start;
}
