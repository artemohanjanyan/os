#define _XOPEN_SOURCE 600

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
#define MAX_EVENTS 50

char glob_buf[BUF_SIZE];

struct event_data;
struct client_data;

struct buffer
{
	char *buf;
	size_t length;
};

void buffer_init(struct buffer *buf)
{
	buf->buf = NULL;
	buf->length = 0;
}

void buffer_append(struct buffer *buf, char str[], size_t length)
{
	// TODO check for realloc error
	buf->buf = (char*) realloc(buf->buf, buf->length + length);
	for (size_t i = 0; i < length; ++i)
		buf->buf[i + buf->length] = str[i];
	buf->length += length;
}

void buffer_drop(struct buffer *buf, size_t length)
{
	if (length > buf->length)
		length = buf->length;
	char *new_buf = (char*) malloc(buf->length - length);
	for (size_t i = 0; i < buf->length - length; ++i)
		new_buf[i] = buf->buf[length + i];
	free(buf->buf);
	buf->buf = new_buf;
	buf->length -= length;
}

void buffer_free(struct buffer *buf)
{
	free(buf->buf);
	free(buf);
}

struct pty_data
{
	int master_fd;
	struct client_data *client;
	struct buffer *buf;
	ssize_t skip;
	struct event_data *data;
};

struct client_data
{
	int client_fd;
	struct pty_data *pty;
	struct buffer *buf;
	struct event_data *data;
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
		int                *server;
		struct client_data *client;
		struct pty_data    *pty;
	};
};

int fork_term(int master_fd, int slave_fd, int server_fd, int epoll_fd)
{
	int child_pid;
	child_pid = fork();
	if (child_pid == -1)
		return -1;

	if (child_pid == 0)
	{
		int dup_fail = dup2(slave_fd, STDIN_FILENO) == -1 ||
		               dup2(slave_fd, STDOUT_FILENO) == -1 ||
		               dup2(slave_fd, STDERR_FILENO) == -1;
		close(master_fd);
		close(slave_fd);
		close(server_fd);
		close(epoll_fd);
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

int is_deleted(void *ptr, void *deleted[], size_t deleted_cnt)
{
	size_t i = 0;
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

int main(int argc, char *argv[])
{
	if (argc == 1)
	{
		printf("Wrong argument format.\n");
		printf("Usage: ./rshd <port>\n");
		return EXIT_FAILURE;
	}

	//if (demonize() == -1)
	//{
	//	printf("Error during demonization: %s\n", strerror(errno));
	//	return EXIT_FAILURE;
	//}

	/* ----------
	   Run server
	   ---------- */

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);

	{
		struct sockaddr_in address;
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons(atoi(argv[1]));

		int enable = 1;
		setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));

		if (bind(server_fd, (struct sockaddr*) &address, sizeof address) == -1)
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

	struct event_data server_data;
	server_data.type = SERVER;
	server_data.server = &server_fd;

	struct epoll_event event;
	event.events = EPOLLIN | EPOLLRDHUP;
	event.data.ptr = &server_data;

	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1)
		goto close_epoll;

	/* ------------------------
	   Communicate with clients
	   ------------------------ */

	struct epoll_event events[MAX_EVENTS];
	int event_n;
	while ((event_n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1)) > 0)
	{
		void *deleted_ptrs[MAX_EVENTS * 2];
		size_t deleted_n = 0;

		for (int event_i = 0; event_i < event_n; ++event_i)
		{
			struct event_data *data = (struct event_data*) events[event_i].data.ptr;
			switch (data->type)
			{
				case SERVER:
				{
					fprintf(stderr, "Server\n");

					int master_fd, slave_fd;
					if ((master_fd = posix_openpt(O_RDWR)) == -1 ||
							grantpt(master_fd) == -1 ||
							unlockpt(master_fd) == -1 ||
							(slave_fd = open(ptsname(master_fd), O_RDWR)) == -1)
						goto close_master;

					if (fork_term(master_fd, slave_fd, server_fd, epoll_fd) == -1)
					{
						close(slave_fd);
						goto close_master;
					}

					struct client_data *client;
					client = (struct client_data*) malloc(sizeof(struct client_data));
					struct pty_data *pty;
					pty = (struct pty_data*) malloc(sizeof(struct pty_data));
					pty->skip = 0;

					client->pty = pty;
					pty->client = client;

					client->client_fd = accept(server_fd, NULL, NULL);
					pty->master_fd = master_fd;

					if (client->client_fd == -1)
						goto free_data;

					data = (struct event_data*) malloc(sizeof(struct event_data));
					if (data == NULL)
						goto close_client;
					data->type = CLIENT;
					data->client = client;
					client->data = data;
					event.data.ptr = data;
					if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client->client_fd, &event) == -1)
						goto free_client_data;

					data = (struct event_data*) malloc(sizeof(struct event_data));
					if (data == NULL)
						goto unregister_client;
					data->type = CONNECTION;
					data->pty = pty;
					pty->data = data;
					event.data.ptr = data;
					if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pty->master_fd, &event) == -1)
						goto free_pty_data;

					client->buf = (struct buffer*) malloc(sizeof(struct buffer));
					if (client->buf == NULL)
						goto free_pty_data;
					buffer_init(client->buf);

					pty->buf = (struct buffer*) malloc(sizeof(struct buffer));
					if (pty->buf == NULL)
						goto free_client_buffer;
					buffer_init(pty->buf);

					goto server_finally;

				free_client_buffer:
					free(client->buf);
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

					if (events[event_i].events & ~((uint32_t) EPOLLIN | EPOLLOUT))
					{
						fprintf(stderr, "Client close\n");

						epoll_ctl(epoll_fd, EPOLL_CTL_DEL, data->client->client_fd, NULL);
						epoll_ctl(epoll_fd, EPOLL_CTL_DEL, data->client->pty->master_fd, NULL);
						close(data->client->client_fd);
						close(data->client->pty->master_fd);

						deleted_ptrs[deleted_n++] = data->client->pty->data;

						buffer_free(data->client->pty->buf);
						free(data->client->pty->data);
						free(data->client->pty);
						free(data->client->buf);
						free(data->client);
						free(data);

						break;
					}


					if (events[event_i].events & EPOLLIN)
					{
						ssize_t length = read(data->client->client_fd, glob_buf, BUF_SIZE);
						if (length > 0)
						{
							buffer_append(data->client->pty->buf, glob_buf, (size_t) length);

							event.data.ptr = data->client->pty->data;
							event.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
							epoll_ctl(epoll_fd, EPOLL_CTL_MOD, data->client->pty->master_fd, &event);

							data->client->pty->skip += length + 1;
						}
					}

					if (events[event_i].events & EPOLLOUT)
					{
						ssize_t written = write(data->client->client_fd, data->client->buf->buf, data->client->buf->length);
						if (written > 0)
							buffer_drop(data->client->buf, (size_t) written);
						if (data->client->buf->length == 0)
						{
							event.data.ptr = data;
							event.events = EPOLLIN | EPOLLRDHUP;
							epoll_ctl(epoll_fd, EPOLL_CTL_MOD, data->client->client_fd, &event);
						}
					}

					break;
				}

				case CONNECTION:
				{
					if (is_deleted(events[event_i].data.ptr, deleted_ptrs, deleted_n))
						break;

					if (events[event_i].events & ~((uint32_t) EPOLLIN | EPOLLOUT))
					{
						fprintf(stderr, "Connection close\n");

						epoll_ctl(epoll_fd, EPOLL_CTL_DEL, data->pty->master_fd, NULL);
						epoll_ctl(epoll_fd, EPOLL_CTL_DEL, data->pty->client->client_fd, NULL);
						close(data->pty->master_fd);
						close(data->pty->client->client_fd);

						deleted_ptrs[deleted_n++] = data->pty->client->data;

						buffer_free(data->pty->client->buf);
						free(data->pty->client->data);
						free(data->pty->client);
						free(data->pty->buf);
						free(data->pty);
						free(data);

						break;
					}

					if (events[event_i].events & EPOLLIN)
					{
						ssize_t length = read(data->pty->master_fd, glob_buf, BUF_SIZE);
						if (length > 0)
						{
							if (data->pty->skip <= length)
							{
								char *skipped_buf = glob_buf;
								skipped_buf += data->pty->skip;
								length -= data->pty->skip;
								data->pty->skip = 0;

								buffer_append(data->pty->client->buf, skipped_buf, (size_t) length);
								event.data.ptr = data->pty->client->data;
								event.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
								epoll_ctl(epoll_fd, EPOLL_CTL_MOD, data->pty->client->client_fd, &event);
							}
							else
								data->pty->skip -= length;
						}
					}

					if (events[event_i].events & EPOLLOUT)
					{
						ssize_t written = write(data->pty->master_fd, data->pty->buf->buf, data->pty->buf->length);
						if (written > 0)
							buffer_drop(data->pty->buf, (size_t) written);
						if (data->pty->buf->length == 0)
						{
							event.data.ptr = data;
							event.events = EPOLLIN | EPOLLRDHUP;
							epoll_ctl(epoll_fd, EPOLL_CTL_MOD, data->pty->master_fd, &event);
						}
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
