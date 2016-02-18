#include <stdio.h>
#include <string.h>
#include <errno.h>
//#include <sys/types.h>
//#include <sys/stat.h>
#include <fcntl.h>

int const BUF_SIZE = 1024;

void cat(int infd, int outfd, int errfd)
{
	char buf[BUF_SIZE];
	int length;
	while ((length = read(infd, buf, BUF_SIZE)) > 0)
	{
		int i = 0;
		while (i < length)
		{
			int ret = write(outfd, buf + i, length - i);
			if (ret == -1)
			{
				printf("Error during write(): %s\n", strerror(errno));
				return;
			}

			i += ret;
		}
	}

	if (length == -1)
	{
		printf("Error during read(): %s\n", strerror(errno));
		return;
	}
}

int main(int argc, char** argv)
{
	int infd = 0;
	if (argc > 1)
		infd = open(argv[1], O_RDONLY);

	cat(infd, 1, 2);

	if (infd != 0)
		close(infd);

	return 0;
}
