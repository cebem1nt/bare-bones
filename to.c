/*
 * Shell ">" command 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define BUF_SIZE 256

void
msg_exit()
{
	perror("to");
	exit(EXIT_FAILURE);
}

int
main(int argc, char** argv) 
{
	// Hello from termux

	if (argc != 2) {
		printf("Usage to < <input> <output>\n");
		return 1;
	}

	int out_fd;
	int flags = O_RDWR | O_CREAT | O_TRUNC;
	int mode = 0644;

	char buf[BUF_SIZE];
	int n;

	if ((out_fd = open(argv[1], flags, mode)) == -1)
		msg_exit();

	while ((n = read(STDIN_FILENO, buf, BUF_SIZE))) {
		if (n == -1)
			msg_exit();

		if (write(out_fd, buf, n) == -1)
			msg_exit();
	} 

	return 0;
}
