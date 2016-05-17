#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BUF_SIZE 1024
#define MAX_EVENTS 10

char buf[BUF_SIZE];

struct event_data;
struct client_data;

struct pty_data
{
	int master_fd;
	client_data *client;
	event_data *data;
};

struct client_data
{
	int client_fd;
	pty_data *pty;
	event_data *data;
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
		int         *server;
		client_data *client;
		pty_data    *pty;
	};
};

int fork_term(int master_fd, int slave_fd)
{
	int child_pid;
	child_pid = fork();
	if (child_pid == -1)
		return -1;

	if (child_pid == 0)
	{
		bool dup_fail = dup2(slave_fd, STDIN_FILENO) == -1 ||
		                dup2(slave_fd, STDOUT_FILENO) == -1 ||
		                dup2(slave_fd, STDERR_FILENO) == -1;
		close(master_fd);
		close(slave_fd);
		if (!dup_fail && setsid() != -1 && ioctl(0, TIOCSCTTY, 1) != -1)
			execlp("/bin/sh", "/bin/sh", NULL);
		exit(0);
	}
	else
	{
		close(slave_fd);
		return child_pid;
	}
}

bool is_deleted(void *ptr, void *deleted[], int deleted_cnt)
{
	int i = 0;
	for (; i < deleted_cnt; ++i)
		if (ptr == deleted[i])
			break;
	return i < deleted_cnt && ptr == deleted[i];
}

int demonize()
{
	int pid = fork();
	if (pid == -1)
		return -1;

	if (pid != 0)
		exit(0);

	int sid = setsid();
	if (sid == -1)
		return -1;

	pid = fork();
	if (pid == -1)
		return -1;

	if (pid != 0)
	{
		FILE *file = fopen("/tmp/rshd.pid", "w");
		if (file == NULL)
			exit(0);
		fprintf(file, "%d\n", pid);
		fclose(file);
		exit(0);
	}

	return 0;
}

ssize_t write_str(int fd, char *str, ssize_t str_size)
{
	ssize_t written = 0;
	while (written < str_size)
	{
		ssize_t n = write(fd, str + written, str_size - written);
		if (n != -1)
			written += n;
		else
			break;
	}
	return written;
}

int main(int argc, char *argv[])
{
	if (argc == 1)
	{
		printf("Wrong argument format.\n");
		printf("Usage: ./rshd <port>\n");
		return EXIT_FAILURE;
	}

	if (demonize() == -1)
	{
		printf("Error during demonization: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	/* ----------
	   Run server
	   ---------- */

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);

	{
		sockaddr_in address;
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons(atoi(argv[1]));

		int enable = 1;
		setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));

		if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof address) == -1)
		{
			printf("Error during bind: %s\n", strerror(errno));
			goto close_server;
		}
	}

	if (listen(server_fd, SOMAXCONN) == -1)
	{
		printf("Error during listen: %s\n", strerror(errno));
		goto close_server;
	}

	/* -----------
	   Setup epoll
	   ----------- */

	int epoll_fd;
	if ((epoll_fd = epoll_create1(0)) == -1)
	{
		printf("Error during epoll_create1: %s\n", strerror(errno));
		goto close_server;
	}

	event_data server_data;
	server_data.type = SERVER;
	server_data.server = &server_fd;

	epoll_event event;
	event.events = EPOLLIN | EPOLLRDHUP;
	event.data.ptr = &server_data;

	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1)
		goto close_epoll;

	/* ------------------------
	   Communicate with clients
	   ------------------------ */

	epoll_event events[MAX_EVENTS];
	while (int event_n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1))
	{
		void *deleted_ptrs[MAX_EVENTS * 2];
		int deleted_n = 0;

		for (int event_i = 0; event_i < event_n; ++event_i)
		{
			event_data *data = (event_data*) events[event_i].data.ptr;
			switch (data->type)
			{
				case SERVER:
				{
					int master_fd, slave_fd;
					if ((master_fd = posix_openpt(O_RDWR)) == -1 ||
							grantpt(master_fd) == -1 ||
							unlockpt(master_fd) == -1 ||
							(slave_fd = open(ptsname(master_fd), O_RDWR)) == -1)
						goto close_master;

					if (fork_term(master_fd, slave_fd) == -1)
					{
						close(slave_fd);
						goto close_master;
					}

					client_data *client;
					client = (client_data*) malloc(sizeof(client_data));
					pty_data *pty;
					pty = (pty_data*) malloc(sizeof(pty_data));

					client->pty = pty;
					pty->client = client;

					client->client_fd = accept(server_fd, NULL, NULL);
					pty->master_fd = master_fd;

					if (client->client_fd == -1)
						goto free_data;

					data = (event_data*) malloc(sizeof(event_data));
					if (data == NULL)
						goto close_client;
					data->type = CLIENT;
					data->client = client;
					client->data = data;
					event.data.ptr = data;
					if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client->client_fd, &event) == -1)
						goto free_client_data;

					data = (event_data*) malloc(sizeof(event_data));
					if (data == NULL)
						goto unregister_client;
					data->type = CONNECTION;
					data->pty = pty;
					pty->data = data;
					event.data.ptr = data;
					if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pty->master_fd, &event) == -1)
						goto free_pty_data;

					goto server_finally;

				free_pty_data:
					free(data);
				unregister_client:
					epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client->client_fd, NULL);
				free_client_data:
					free(data);
				close_client:
					close(client->client_fd);
				free_data:
					free(client);
					free(pty);
				close_master:
					if (master_fd != -1)
						close(master_fd);
					printf("Cant open pty: %s\n", strerror(errno));

				server_finally:
					break;
				}

				case CLIENT:
				{
					if (is_deleted(events[event_i].data.ptr, deleted_ptrs, deleted_n))
						break;

					if (events[event_i].events & EPOLLIN)
					{
						ssize_t length = read(data->client->client_fd, buf, BUF_SIZE);
						ssize_t written = write_str(data->client->pty->master_fd, buf, length);
						++written;
						while (written > 0)
						{
							ssize_t read_n = read(data->client->pty->master_fd, buf, length);
							if (read_n <= 0)
								break;
							written -= read_n;
						}
					}

					if (events[event_i].events & ~EPOLLIN)
					{
						epoll_ctl(epoll_fd, EPOLL_CTL_DEL, data->client->client_fd, NULL);
						epoll_ctl(epoll_fd, EPOLL_CTL_DEL, data->client->pty->master_fd, NULL);
						close(data->client->client_fd);
						close(data->client->pty->master_fd);

						deleted_ptrs[deleted_n++] = data->client->pty;

						free(data->client->pty->data);
						free(data->client->pty);
						free(data->client);
						free(data);
					}

					break;
				}

				case CONNECTION:
				{
					if (is_deleted(events[event_i].data.ptr, deleted_ptrs, deleted_n))
						break;

					if (events[event_i].events & EPOLLIN)
					{
						ssize_t length = read(data->pty->master_fd, buf, BUF_SIZE);
						write_str(data->pty->client->client_fd, buf, length);
					}

					if (events[event_i].events & ~EPOLLIN)
					{
						epoll_ctl(epoll_fd, EPOLL_CTL_DEL, data->pty->master_fd, NULL);
						epoll_ctl(epoll_fd, EPOLL_CTL_DEL, data->pty->client->client_fd, NULL);
						close(data->pty->master_fd);
						close(data->pty->client->client_fd);

						deleted_ptrs[deleted_n++] = data->pty->client;

						free(data->client->pty->data);
						free(data->client->pty);
						free(data->client);
						free(data);
					}

					break;
				}
			}
		}
	}

close_epoll:
	close(epoll_fd);
close_server:
	close(server_fd);

	return 0;
}
