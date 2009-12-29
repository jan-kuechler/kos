#ifndef CONSOLE_H
#define CONSOLE_H

/**
 * A console driver
 */
struct console
{
	int (*initialize)(const char *cmdline); /**< Initializes the console */
	int (*free)(void);                      /**< Frees the console */

	int (*puts)(const char *str);           /**< Prints a string */
	int (*putc)(char c);                    /**< Prints a character */

	int (*set_cursor)(unsigned int x, unsigned int y);
	int (*get_cursor)(int *x, int *y);
	int (*get_dimension)(int *width, int *height);
};

extern struct console *cur_console;

#endif /*CONSOLE_H*/
