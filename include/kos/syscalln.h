#ifndef SYSCALLN_H
#define SYSCALLN_H

/*   IMPORTANT!
 * Keep this the same as newlib/libc/sys/kos/syscalln.h !!
 */

#define SC_TEST          0

/* Newlib interface */
#define SC_EXIT          1
#define SC_WAIT          2

#define SC_KILL          3

#define SC_FORK          4
#define SC_EXECVE        5
#define SC_GETPID        6

#define SC_OPEN          7
#define SC_CLOSE         8
#define SC_READ          9
#define SC_WRITE        10

#define SC_LINK         11
#define SC_UNLINK       12
#define SC_READLINK     13
#define SC_SYMLINK      14

#define SC_CHOWN        15
#define SC_FSTAT        16
#define SC_STAT         17
#define SC_ISATTY       18
#define SC_LSEEK        19

#define SC_GETTIMEOFDAY 20
#define SC_TIMES        21

#define SC_SBRK         22

/* kOS interface */
#define SC_ANSWER       23

#define SC_OPEN_STD     24

#define SC_YIELD        25
#define SC_SLEEP        26

#define SC_SEND         27
#define SC_RECV         28

#define SC_WAITPID      29

#define NUM_SYSCALLS    30

#endif /*SYSCALLN_H*/
