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

	char path[MAXLEN] = "/bin/";

	line = (char *)malloc(buffer_size * sizeof(char));
    if( line == NULL) {
        perror("Unable to allocate buffer");
        exit(1);
    }

	if (argc == 1) { /* Run in interactive mode if invoked with no arguments */
		while (1) {
			background = 0;
			char * new_str;

						
			/* Print the prompt */
			printf("wish> ");
			/* Set the timeout for alarm signal (autologout) */
			alarm(LOGOUT);
			
			/* Read the users command */
			getline(&line, &buffer_size, stdin); 
			if (line == NULL) {
				printf("\nlogout\n");
				exit(0);
			}
			line[strlen(line) - 1] = '\0';
			
			if (strlen(line) == 0) {
				continue;
			}
			
			/* Start to background? */
			if (line[strlen(line)-1] == '&') {
				line[strlen(line)-1]=0;
				background = 1;
			}
			
			/* Split the command line */
			i = 0;
			cmd = line;
			while ((args[i] = strtok(cmd, " ")) != NULL) {
				printf("arg %d: %s\n", i, args[i]);
				i++;
				cmd = NULL;
			}
			
			if (strcmp(args[0],"exit")==0) {
				free(line);
				free(new_str);
				exit(0);
			}

			
			if((new_str = malloc(strlen(path)+strlen(args[0])+1)) != NULL){
			    new_str[0] = '\0';
			    strcat(new_str,path);
			    strcat(new_str,args[0]);
			} else {
			   	perror("Malloc");
			    exit(1);
			}
			
			/* Fork to run the command */
			switch (pid = fork()) {
				case -1:
					/* error */
					perror("fork");
					continue;
				case 0:
					/* child process */
					execv(new_str, args);
					perror("execv");
					free(line);
					free(new_str);
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
		
	} else if (argc == 2) { /* Run in batch mode if invoked with one argument */
		printf("Batch mode\n");
	} else {  /* Terminate program if shell invoked with more than one argument */
		printf("Shell can only be invoked with no arguments or a single argument\n");
		exit(1);
	}
	
	return 0;
}
