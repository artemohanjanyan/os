#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

int const BUF_SIZE = 1024;

void cat(int infd, int outfd)
{
	char buf[BUF_SIZE];
	int length;
	while (1)
	{
		length = read(infd, buf, BUF_SIZE);
		if (length == -1)
		{
			if (errno == EINTR)
				continue;
			else
				break;
		}
		if (length == 0)
			break;

		int i = 0;
		while (i < length)
		{
			int ret = write(outfd, buf + i, length - i);
			if (ret == -1)
			{
				if (errno != EINTR)
					fprintf(stderr, "Error during write(): %s\n", strerror(errno));
				else
					continue;
				return;
			}

			i += ret;
		}
	}

	if (length == -1)
	{
		fprintf(stderr, "Error during read(): %s\n", strerror(errno));
		return;
	}
}

int main(int argc, char** argv)
{
	int infd = STDIN_FILENO;
	if (argc > 1)
	{
		infd = open(argv[1], O_RDONLY);
		if (infd == -1)
		{
			fprintf(stderr, "Error during open(): %s\n", strerror(errno));
			return;
		}
	}

	cat(infd, STDOUT_FILENO);

	if (infd != 0)
		close(infd);

	return 0;
}
