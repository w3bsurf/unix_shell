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
	char *cmd, *line = NULL, *args[MAXNUM], *paths[MAXNUM], *temp_path, *path = "/bin";
	size_t buffer_size = MAXLEN;
	int background, i;
	int pid;

	signal(SIGALRM, signal_handler);
	signal(SIGINT, signal_handler);

	line = (char *)malloc(buffer_size * sizeof(char));
    if( line == NULL) {
        perror("Unable to allocate buffer");
        exit(1);
    }

	if (argc == 1) { /* Run in interactive mode if invoked with no arguments */
		while (1) {
			background = 0;
			char * new_str = NULL;

						
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
			
			if (strcmp(args[0],"exit")==0) { /* Built-in 'exit' command */
				free(line);
				free(new_str);
				free(path);
				exit(0);
			}

			if (strcmp(args[0], "path")==0) { /* Built-in 'path' command: Overwrites shell search path */
				i = 1;
				path = NULL;

				while (args[i] != NULL) {
					if (path == NULL) {
						path = (char *)malloc((strlen(args[i])+2));
						strcpy(path, args[i]);
						strcat(path, " ");
						i++;
					} else {
						path = (char *)realloc(path, (strlen(path)+strlen(args[i])+2));
						strcat(path, args[i]);
						strcat(path, " ");
						i++;
					}
				}
				strcat(path, "\0");
				continue;
			}
			
			/* Fork to run the command */
			switch (pid = fork()) {
				case -1:
					/* error */
					perror("fork");
					continue;
				case 0:
					i = 0;
					temp_path = path;
					if (strcmp(path, "/bin")!=0){
						while ((paths[i] = strtok(temp_path, " ")) != NULL) {
							i++;
							temp_path = NULL;
						}
						i = 0;
						while (paths[i] != NULL) {
							if((new_str = malloc(strlen(paths[i])+strlen(args[0])+2)) != NULL){
							    new_str[0] = '\0';
							    
							    strcat(new_str, paths[i]);
							    strcat(new_str, "/");
							    strcat(new_str, args[0]);

							    if (access(new_str, X_OK)==0) { /* Check if path can access file */
									execv(new_str, args);
									perror("execv");
									free(line);
									free(new_str);
									exit(1);
								}
							    i++;
							} else {
							   	perror("Malloc");
							    exit(1);
							}
						}
						printf("ERROR!\n");
						exit(1);

					} else {
						if((new_str = malloc(strlen(path)+strlen(args[0])+2)) != NULL){
						    new_str[0] = '\0';
						    strcat(new_str,path);
						    strcat(new_str,"/");
						    strcat(new_str,args[0]);
						} else {
						   	perror("Malloc");
						    exit(1);
						}
							execv(new_str, args);
							perror("execv");
							free(line);
							free(new_str);
							exit(1);
					}
					
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
