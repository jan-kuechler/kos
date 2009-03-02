#ifndef PORTS_H
#define PORTS_H

#include <types.h>

static inline word inw(word _port)
{
	word result;
	__asm__ volatile ("inw %1, %0" : "=a" (result) : "Nd" (_port));
	return result;
}

static inline byte inb(word _port)
{
	byte result;
	__asm__ volatile ("inb %1, %0" : "=a" (result) : "Nd" (_port));
	return result;
}

static inline dword inl(word _port)
{
	dword result;
	__asm__ volatile ("inl %1, %0" : "=a" (result) : "Nd" (_port));
	return result;
}

static inline void outw(word _port, word _data)
{
	__asm__ volatile ("outw %0, %1" : : "a" (_data), "Nd" (_port));
}

static inline void outb(word _port, byte _data)
{
	__asm__ volatile ("outb %0, %1" : : "a" (_data), "Nd" (_port));
}

static inline void outl(word _port, dword _data)
{
	__asm__ volatile ("outl %0, %1" : : "a"(_data), "Nd" (_port));
}

static inline void outb_wait(word _port, byte _data)
{
	__asm__ volatile ("outb %0, %1\njmp 1f\n1: jmp 1f\n1:" : : "a" (_data), "Nd" (_port));
}

#endif /*PORTS_H*/
