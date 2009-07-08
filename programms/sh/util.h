#ifndef UTIL_H
#define UTIL_H

struct cmd
{
	unsigned int argc;
	char **argv;
};

char *prepare(char *input);
int split_cmd(char *input, struct cmd *cmd);
void free_argv(struct cmd *cmd);

#endif /*UTIL_H*/
