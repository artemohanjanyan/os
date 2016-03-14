#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
	int pipefd[2];
	if (pipe(pipefd) == -1)
		fprintf(stderr, "Error during pipe(): %s\n", strerror(errno));

	int pid1 = fork();
	if (pid1 == -1)
		fprintf(stderr, "Error during fork(): %s\n", strerror(errno));

	if (pid1 == 0)
	{
		// cat
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[0]);
		close(pipefd[1]);
		if (execlp("cat", "cat", argv[1], NULL) == -1)
			fprintf(stderr, "Error during fork(): %s\n", strerror(errno));
	}
	else
	{
		int pid2 = fork();
		if (pid2 == -1)
			fprintf(stderr, "Error during fork(): %s\n", strerror(errno));

		if (pid2 == 0)
		{
			// grep
			dup2(pipefd[0], STDIN_FILENO);
			close(pipefd[0]);
			close(pipefd[1]);
			if (execlp("grep", "grep", "int", NULL) == -1)
				fprintf(stderr, "Error during fork(): %s\n", strerror(errno));
		}
		else
		{
			close(pipefd[0]);
			close(pipefd[1]);

			wait(NULL);
			wait(NULL);
		}
	}

	return 0;
}
