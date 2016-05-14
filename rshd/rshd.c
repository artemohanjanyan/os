#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <iostream>

int const BUF_SIZE = 1024;

int make_non_blocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char *argv[])
{
	/* ----------
		Run server
	   ---------- */

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);

	{
		sockaddr_in address;
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		if (argc > 1)
			address.sin_port = htons(atoi(argv[1]));
		else
		{
			//printf("Wrong argument format.\n");
			//printf("usage: ./rshd <port>");
			//goto server_fd_destruct;
			address.sin_port = htons(2539);
		}

		int enable = 1;
		setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));
		//make_non_blocking(server_fd);

		if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof address) == -1)
		{
			printf("Error during bind: %s\n", strerror(errno));
			goto server_fd_destruct;
		}
	}

	if (listen(server_fd, SOMAXCONN) == -1)
	{
		printf("Error during listen: %s\n", strerror(errno));
		goto server_fd_destruct;
	}

	int client_fd;
	if ((client_fd = accept(server_fd, NULL, NULL)) == -1)
	{
		printf("Error during accept: %s\n", strerror(errno));
		goto server_fd_destruct;
	}

	make_non_blocking(client_fd);

	printf("Connected\n");


	/* -----------------------
		Communicate with client
	   ----------------------- */

	int read_pipe[2];
	int write_pipe[2];
	if (pipe(read_pipe) == -1)
	{
		printf("Error during pipe: %s\n", strerror(errno));
		goto read_pipe_destruct;
	}
	if (pipe(write_pipe) == -1)
	{
		printf("Error during pipe: %s\n", strerror(errno));
		goto write_pipe_destruct;
	}

	int child_pid;
	child_pid = fork();
	if (child_pid == -1)
	{
		printf("Error during fork: %s\n", strerror(errno));
		goto client_fd_destruct;
	}

	if (child_pid == 0)
	{
		dup2(read_pipe[0], STDIN_FILENO);
		dup2(write_pipe[1], STDOUT_FILENO);
		close(read_pipe[0]);
		close(read_pipe[1]);
		close(write_pipe[0]);
		close(write_pipe[1]);
		execlp("/bin/sh", "/bin/sh", NULL);
		return 0;
	}

	make_non_blocking(read_pipe[1]);
	make_non_blocking(write_pipe[0]);

	char buf[BUF_SIZE];
	ssize_t length;

	while ((length = read(client_fd, buf, BUF_SIZE)) != -1 || errno == EAGAIN)
	{
		ssize_t written = 0;
		while (written < length)
		{
			ssize_t it_written = write(read_pipe[1], buf + written, length - written);
			if (it_written == -1 && errno != EAGAIN)
			{
				length = -1;
				goto destruct;
			}
			written += it_written;
		}

		length = read(write_pipe[0], buf, BUF_SIZE);
		if (length == -1 && errno != EAGAIN)
			goto destruct;

		written = 0;
		while (written < length)
		{
			ssize_t it_written = write(client_fd, buf + written, length - written);
			if (it_written == -1 && errno != EAGAIN)
			{
				length = -1;
				goto destruct;
			}
			written += it_written;
		}
	}

destruct:
	if (length == -1 && errno != EAGAIN)
		printf("Error during read or write: %s", strerror(errno));
write_pipe_destruct:
	close(write_pipe[0]);
	close(write_pipe[1]);
read_pipe_destruct:
	close(read_pipe[0]);
	close(read_pipe[1]);
client_fd_destruct:
	close(client_fd);
server_fd_destruct:
	close(server_fd);

	return 0;
}
