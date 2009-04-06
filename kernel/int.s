section .text

%define IRQ_BASE 0x20     ; as in idt.h
%define GDT_SEL_DATA 0x10 ; as in gdt.h

extern idt_handle_int

%macro isr 1
global isr_stub_%1
isr_stub_%1:
	push dword 0
	push dword %1
	jmp  isr_common
%endmacro

%macro isr_ec 1
global isr_stub_%1
isr_stub_%1:
	nop
	nop
	push dword %1
	jmp  isr_common
%endmacro

%macro irq 1
global irq_stub_%1
irq_stub_%1:
	push dword 0
	push dword %1 + IRQ_BASE
	jmp  isr_common
%endmacro

global isr_null_handler
isr_null_handler:
	push dword 0
	push dword 0xD00F
	jmp  isr_common
	
global syscall_stub
syscall_stub:
	push dword 0
	push dword 0x30
	jmp  isr_common
	
isr     0
isr     1
isr     2
isr     3
isr     4
isr     5
isr     6
isr     7
isr_ec  8
isr     9
isr_ec 10
isr_ec 11
isr_ec 12
isr_ec 13
isr_ec 14
isr    15
isr    16
isr    17
isr    18
isr    19

irq     0
irq     1
irq     2
irq     3
irq     4
irq     5
irq     6
irq     7
irq     8
irq     9
irq    10
irq    11
irq    12
irq    13
irq    14
irq    15

isr_common:
	; save all registers
	; see regs.h
	push    gs
	push    fs
	push    es
	push    ds
	pusha
	
	cld
	
	; set the segment registers to the kernel data segment
	mov ax, GDT_SEL_DATA
	mov ds, eax
	mov es, eax
	mov fs, eax
	mov gs, eax
	
	; arg for idt_handle_int
	mov eax, esp
	push eax
	call idt_handle_int
	
	mov esp, eax
	
	popa
	pop ds
	pop es
	pop fs
	pop gs
	add esp, 8 ; ignore intr and errc
	iret
	