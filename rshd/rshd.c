#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int const BUF_SIZE = 1024;
int const MAX_EVENTS = 1024;

char buf[BUF_SIZE];

int make_non_blocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

struct event_data;
struct client_data;

struct pipes_data
{
	int read_pipe[2], write_pipe[2];
	client_data *client;
};

struct client_data
{
	int client_fd;
	pipes_data *pipes;
};

enum data_type
{
	SERVER, CLIENT, CONNECTION
};

struct event_data
{
	enum data_type type;

	union
	{
		int             *server;
		client_data     *client;
		pipes_data *pipes;
	};
};

int fork_term(pipes_data *pipes)
{
	int child_pid;
	child_pid = fork();
	if (child_pid == -1)
		return -1;

	if (child_pid == 0)
	{
		bool dup_fail = dup2(pipes->read_pipe[0],  STDIN_FILENO) == -1 ||
		                dup2(pipes->write_pipe[1], STDOUT_FILENO) == -1;
		close(pipes->read_pipe[0]);
		close(pipes->read_pipe[1]);
		close(pipes->write_pipe[0]);
		close(pipes->write_pipe[1]);
		if (!dup_fail)
			execlp("/bin/sh", "/bin/sh", NULL);
		return 0;
	}
	else
		return child_pid;
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
			printf("Wrong argument format.\n");
			printf("Usage: ./rshd <port>\n");
			goto server_destruct;
			//address.sin_port = htons(2539);
		}

		int enable = 1;
		setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));
		//make_non_blocking(server_fd);

		if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof address) == -1)
		{
			printf("Error during bind: %s\n", strerror(errno));
			goto server_destruct;
		}
	}

	if (listen(server_fd, SOMAXCONN) == -1)
	{
		printf("Error during listen: %s\n", strerror(errno));
		goto server_destruct;
	}

	/* -----------
		Setup epoll
		----------- */

	int epoll_fd;
	if ((epoll_fd = epoll_create1(0)) == -1)
	{
		printf("Error during epoll_create1: %s\n", strerror(errno));
		goto server_destruct;
	}

	event_data server_data;
	server_data.type = SERVER;
	server_data.server = &server_fd;

	epoll_event event;
	event.events = EPOLLIN | EPOLLRDHUP;
	event.data.ptr = &server_data;

	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);

	/* -----------------------
		Communicate with client
	   ----------------------- */

	//int client_fd;
	//if ((client_fd = accept(server_fd, NULL, NULL)) == -1)
	//{
	//	printf("Error during accept: %s\n", strerror(errno));
	//	goto server_destruct;
	//}

	//make_non_blocking(client_fd);

	//printf("Connected\n");

	epoll_event events[MAX_EVENTS];
	while (int event_n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1))
		for (int event_i = 0; event_i < event_n; ++event_i)
		{
			event_data *data = (event_data*) events[event_i].data.ptr;
			switch (data->type)
			{
				case SERVER:
				{
					client_data *client = (client_data*) malloc(sizeof(client_data));
					pipes_data  *pipes  = (pipes_data*)  malloc(sizeof(pipes_data));

					client->pipes = pipes;
					pipes->client = client;

					client->client_fd = accept(server_fd, NULL, NULL);

					if (pipe(pipes->read_pipe) == -1)
						break;
					if (pipe(pipes->write_pipe) == -1)
					{
						close(pipes->read_pipe[0]);
						close(pipes->read_pipe[1]);
					}

					fork_term(pipes);

					data = (event_data*) malloc(sizeof(event_data));
					data->type = CLIENT;
					data->client = client;
					event.data.ptr = data;
					epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client->client_fd, &event);

					data = (event_data*) malloc(sizeof(event_data));
					data->type = CONNECTION;
					data->pipes = pipes;
					event.data.ptr = data;
					epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pipes->write_pipe[0], &event);

					break;
				}

				case CLIENT:
				{
					if (events[event_i].events & EPOLLIN)
					{
						ssize_t length = read(data->client->client_fd, buf, BUF_SIZE);
						ssize_t written = 0;
						while (written < length)
						{
							ssize_t n = write(data->client->pipes->read_pipe[1], buf + written, length - written);
							if (n != -1)
								written += n;
							else
								break;
						}
					}
					else
					{
						epoll_ctl(epoll_fd, EPOLL_CTL_DEL, data->client->client_fd, NULL);
						close(data->client->client_fd);
						close(data->client->pipes->read_pipe[0]);
						close(data->client->pipes->read_pipe[1]);
						free(data->client);
						free(data);

						fprintf(stderr, "client rdhup");
					}
					break;
				}

				case CONNECTION:
				{
					if (events[event_i].events & EPOLLIN)
					{
						ssize_t length = read(data->pipes->write_pipe[0], buf, BUF_SIZE);
						ssize_t written = 0;
						while (written < length)
						{
							ssize_t n = write(data->pipes->client->client_fd, buf + written, length - written);
							if (n != -1)
								written += n;
							else
								break;
						}
					}
					else
					{
						epoll_ctl(epoll_fd, EPOLL_CTL_DEL, data->pipes->write_pipe[0], NULL);
						close(data->pipes->write_pipe[0]);
						close(data->pipes->write_pipe[1]);
						free(data->pipes);
						free(data);

						fprintf(stderr, "connection rdhup");
					}
					break;
				}
			}
		}

	close(epoll_fd);
server_destruct:
	close(server_fd);

	return 0;
}
