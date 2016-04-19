#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

int handled_signal = 0; // 0 is not a signal number
pid_t origin_pid;

int alarm_triggered = 0;

void handler(int signum, siginfo_t *siginfo, void *_)
{
	if (signum != SIGALRM)
	{
		if (handled_signal == 0)
		{
			handled_signal = signum;
			origin_pid = siginfo->si_pid;
		}
	}
	else
		alarm_triggered = 1;
}

int main()
{
	struct sigaction act;
	act.sa_flags = SA_SIGINFO;
	act.sa_sigaction = &handler;

	if (sigfillset(&act.sa_mask))
		fprintf(stderr, "Error during sig...set(): %s\n", strerror(errno));
	
	int i;
	for (i = 1; i < 32; ++i)
	{
		if (i != SIGKILL && i != SIGSTOP)
			sigaction(i, &act, NULL);
		//if (sigaction(i, &act, NULL))
		//	fprintf(stderr, "Error during sigaction(): %s\n", strerror(errno));
	}
			
	//if (sigaction(SIGUSR1, &act, NULL) || sigaction(SIGUSR2, &act, NULL) || sigaction(SIGALRM, &act, NULL))

	alarm(10);
	while (alarm_triggered == 0)
		pause();
	//sleep(10);

	if (handled_signal == 0)
		printf("No signals were caught\n");
	else
		printf("%d from %d\n", handled_signal, origin_pid);

	return 0;
}
