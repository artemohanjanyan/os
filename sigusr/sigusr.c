#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

int handled_signal = 0; // 0 is not a signal number
pid_t origin_pid;

void handler(int signum, siginfo_t *siginfo, void *_)
{
	handled_signal = signum;
	origin_pid = siginfo->si_pid;
}

int main()
{
	struct sigaction act;
	act.sa_flags = SA_SIGINFO;
	act.sa_sigaction = &handler;

	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGUSR1);
	sigaddset(&act.sa_mask, SIGUSR2);

	if (sigaction(SIGUSR1, &act, NULL) || sigaction(SIGUSR2, &act, NULL))
		fprintf(stderr, "Error during write(): %s\n", strerror(errno));

	sleep(10);

	if (handled_signal == 0)
		printf("No signals were caught\n");
	else
		printf("%s from %d\n", handled_signal == SIGUSR1 ? "SIGUSR1" : "SIGUSR2", origin_pid);

	return 0;
}
