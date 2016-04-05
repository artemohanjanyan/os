#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

#include <string>
#include <vector>
#include <iostream>
#include <functional>

int const BUF_SIZE = 1024;

void error_check(int res, std::string command = "some_func")
{
	if (res == -1 && errno != EINTR)
	{
		fprintf(stderr, "Error during %s(): %s\n", command.c_str(), strerror(errno));
		exit(0);
	}
}

void write_all(int fd, std::string &str, std::function<bool()> pred = [](){return true;})
{
	while (str.size() > 0 && pred())
	{
		int written = write(fd, str.c_str(), str.size());
		if (written != -1)
			str.erase(0, written);
		if (!pred())
			break;
		error_check(written);
	}
}

void write_all(int fd, std::string &&str, std::function<bool()> pred = [](){return true;})
{
	std::string str1{str};
	write_all(fd, str1, pred);
}

int handled_signal;
// TODO handle consecutive signals
void handler(int signum)
{
	handled_signal = signum;
}

int main()
{
	struct sigaction act;
	act.sa_handler = &handler;
	error_check(sigemptyset(&act.sa_mask));
	error_check(sigaddset(&act.sa_mask, SIGINT));
	error_check(sigaddset(&act.sa_mask, SIGCHLD));
	error_check(sigaction(SIGINT, &act, NULL));
	error_check(sigaction(SIGCHLD, &act, NULL));

	std::string input;

	char buf[BUF_SIZE];
	int length;

	write_all(STDOUT_FILENO, "$ ");

	while (true)
	{
		length = read(STDIN_FILENO, buf, BUF_SIZE);
		if (length == 0)
			exit(0);
		error_check(length, "read");
		if (length != -1)
			input += {buf, static_cast<size_t>(length)};

		size_t pos = input.find('\n');

		if (pos != std::string::npos)
		{
			std::string command = input.substr(0, pos) + '|';
			input.erase(0, pos + 1);

			int *pipefd = new int[2];
			pipe(pipefd);

			int *cur_pipefd = pipefd;
			bool is_first = true;
			bool is_closed = false;
			
			int command_n = 0;

			std::vector<int> pids;

			while (command.length() > 0)
			{
				pos = command.find('|');
				std::string program = command.substr(0, pos) + " ";
				command.erase(0, pos + 1); // pos + '|'

				std::vector<std::string> words;
				while (program.length() > 0)
				{
					pos = program.find(' ');
					if (pos != 0)
						words.push_back(program.substr(0, pos));
					program.erase(0, pos + 1);
				}

				char *args[words.size() + 1];
				for (size_t i = 0; i < words.size(); ++i)
					args[i] = const_cast<char*>(words[i].c_str()); // я не смог в сишные массивы
				args[words.size()] = nullptr;

				int *out_pipefd = new int[2];
				pipe(out_pipefd);

				int *in_pipefd = cur_pipefd;
				cur_pipefd = out_pipefd;

				int pid = fork();
				if (pid == 0)
				{
					dup2(in_pipefd[0], STDIN_FILENO);
					if (command.length() > 0) // if command is last then print to STDOUT
						dup2(out_pipefd[1], STDOUT_FILENO);
					close(in_pipefd[0]);
					close(in_pipefd[1]);
					close(out_pipefd[0]);
					close(out_pipefd[1]);
					delete[] in_pipefd;
					delete[] out_pipefd;
					error_check(execvp(args[0], args), "execvp");
					exit(0);
				}
				else
				{
					pids.push_back(pid);
					if (is_first)
						is_first = false;
					else
					{
						close(in_pipefd[0]);
						close(in_pipefd[1]);
					}
					handled_signal = 0;
				}
			}

			int finished = 0;
			while (handled_signal != SIGINT && finished < pids.size())
			{
				// write input
				write_all(pipefd[1], input, [](){return handled_signal == 0;});
				length = read(STDIN_FILENO, buf, BUF_SIZE);

				error_check(length, "read");
				if (length != -1)
					input += {buf, static_cast<size_t>(length)};

				if (length == 0)
				{
					close(pipefd[1]);
					is_closed = true;
				}

				if (handled_signal == SIGCHLD)
					++finished;
			}
			if (handled_signal == SIGINT)
				for (int pid : pids)
					kill(pid, SIGINT);

			if (!is_closed)
				close(pipefd[1]);
			close(pipefd[0]);

			write_all(STDOUT_FILENO, "$ ");

			error_check(sigaction(SIGINT, &act, NULL));
			error_check(sigaction(SIGCHLD, &act, NULL));
		}
	}

	return 0;
}
