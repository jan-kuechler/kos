#ifndef PORTS_H
#define PORTS_H

#include <types.h>

static inline word inw(word _port)
{
	word result;
	asm volatile ("inw %1, %0" : "=a" (result) : "Nd" (_port));
	return result;
}

static inline byte inb(word _port)
{
	byte result;
	asm volatile ("inb %1, %0" : "=a" (result) : "Nd" (_port));
	return result;
}

static inline dword inl(word _port)
{
	dword result;
	asm volatile ("inl %1, %0" : "=a" (result) : "Nd" (_port));
	return result;
}

static inline void outw(word _port, word _data)
{
	asm volatile ("outw %0, %1" : : "a" (_data), "Nd" (_port));
}

static inline void outb(word _port, byte _data)
{
	asm volatile ("outb %0, %1" : : "a" (_data), "Nd" (_port));
}

static inline void outl(word _port, dword _data)
{
	asm volatile ("outl %0, %1" : : "a"(_data), "Nd" (_port));
}

static inline void outb_wait(word _port, byte _data)
{
	asm volatile ("outb %0, %1\njmp 1f\n1: jmp 1f\n1:" : : "a" (_data), "Nd" (_port));
}

#endif /*PORTS_H*/
