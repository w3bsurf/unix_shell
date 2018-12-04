#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#define LOGOUT 40
#define MAXNUM 40
#define MAXLEN 160

void signal_handler(int sig) {
	switch (sig) {
		case SIGALRM:
			printf("\nautologout\n");
			exit(0);
		default:
			break;
	}
	return;
}

int main(int argc, char **argv) {
	char *cmd, *line, * args[MAXNUM];
	size_t buffer_size = MAXLEN;
	int background, i;
	int pid;
	
	signal(SIGALRM, signal_handler);
	signal(SIGINT, signal_handler);
	
	if (argc == 1) {
		while (1) {
		background = 0;
		
		/* print the prompt */
		printf("wish> ");
		/* set the timeout for alarm signal (autologout) */
		alarm(LOGOUT);
		
		/* read the users command */
		getline(&line, &buffer_size, stdin); 
		if (line == NULL) {
			printf("\nlogout\n");
			exit(0);
		}
		line[strlen(line) - 1] = '\0';
		
		if (strlen(line) == 0) {
			continue;
		}
		
		/* start to background? */
		if (line[strlen(line)-1] == '&') {
			line[strlen(line)-1]=0;
			background = 1;
		}
		
		/* split the command line */
		i = 0;
		cmd = line;
		while ((args[i] = strtok(cmd, " ")) != NULL) {
			printf("arg %d: %s\n", i, args[i]);
			i++;
			cmd = NULL;
		}
		
		if (strcmp(args[0],"exit")==0) {
			exit(0);
		}
		
		/* fork to run the command */
		switch (pid = fork()) {
			case -1:
				/* error */
				perror("fork");
				continue;
			case 0:
				/* child process */
				execvp(args[0], args);
				perror("execvp");
				exit(1);
			default:
				/* parent (shell) */
				if (!background) {
					alarm(0);
					//waitpid(pid, NULL, 0);
					while (wait(NULL)!=pid)
						printf("Some other child process exited\n");
				}
				break;
		}
	}
		
	} else if (argc == 2) {
		printf("Batch mode\n");
	} else {  /* Terminate program if shell invoked with more than 1 argument */
		perror("Shell can only be invoked with no arguments or a single argument\n");
		exit(1);
	}
	
	return 0;
}
