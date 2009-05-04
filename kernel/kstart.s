extern kmain
global kstart

section .text
kstart:
	mov esp, kernelstack
	
	push 0 ; Stack backtrace ends here
	push 0
	mov ebp, esp
	
	push ebx
	push eax
	
	mov  eax, kmain
	call eax
	
	cli
	hlt
	
multi_boot_header:
align 4
	MB_MAGIC	equ	0x1BADB002
	MB_FLAGS	equ	0x03             
	MB_CHECK	equ	-(MB_MAGIC + MB_FLAGS)
	
	dd	 MB_MAGIC
	dd	 MB_FLAGS
	dd	 MB_CHECK
	
section .bss
	resb 16384
kernelstack:
