#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/file.h>

#define LOGOUT 40
#define MAXNUM 40
#define MAXLEN 160

char error_message[30] = "An error has occurred\n";

int parse_line(char *line, char **args, char **args2, int *num_of_args, int *saved_stdout);

int built_in_exit(int num_of_args, char **line, char **path, char **path_args);

void built_in_cd(int num_of_args, char *args[]);

void built_in_path(char **path, char **args);

void run_command(char *path, char *paths[], char path_args[], char **args, int background);

int main(int argc, char **argv) {
	char *cmd, *line;
	char *paths[MAXNUM], *args[MAXNUM], *args2[MAXNUM], *lines[MAXNUM];
	char *path = "/bin";
	size_t buffer_size = MAXLEN;
	int background, i, num_of_args;
	int saved_stdout = dup(1);;

	line = (char *)malloc(buffer_size * sizeof(char));
    if( line == NULL) {
        perror("Unable to allocate buffer");
        exit(1);
    }

	if (argc == 1) { /* Run in interactive mode if invoked with no arguments */
		while (1) {
			background = 0;
			char * path_args = NULL;

			/* Restore stdout */
			// Source: https://stackoverflow.com/questions/11042218/c-restore-stdout-to-terminal
			dup2(saved_stdout, 1);
			close(saved_stdout);
						
			/* Print the prompt */
			printf("wish> ");
			
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
			/*if (line[strlen(line)-1] == '&') {
				line[strlen(line)-1]=0;
				background = 1;
			}*/
			
			if (strpbrk(line, "&")!=0) {
				i = 0;
				cmd = line;
				while ((lines[i] = strsep(&cmd, "&")) != NULL) {
					printf("Line %d: %s\n", i, lines[i]);
					i++;
				}
				for (int x = 0; x<i; x++) {
					if (parse_line(lines[x], args, args2, &num_of_args, &saved_stdout)==1) {
						continue;
					}
					/* Run command with given arguments */
					run_command(path, paths, path_args, args, background);
					
					/* Restore stdout */
					dup2(saved_stdout, 1);
					close(saved_stdout);
					continue;
				}
			} else {
				if (parse_line(line, args, args2, &num_of_args, &saved_stdout)==1) {
					continue;
				}
				if (strcmp(args[0], "exit")==0) { 
					if (built_in_exit(num_of_args, &line, &path, &path_args)==1) {
						continue;
					}
				} else if (strcmp(args[0], "cd")==0) {
					built_in_cd(num_of_args, args);
					continue;
				} else if (strcmp(args[0], "path")==0) {
					built_in_path(&path, args);
					printf("%s\n", path);
					continue;
				} else {
					/* Run command with given arguments */
					run_command(path, paths, path_args, args, background);
					continue;
				}				
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

/* Built-in 'exit' command */
int built_in_exit(int num_of_args, char **line, char **path, char **path_args) {
	if (num_of_args > 1) {
		write(STDERR_FILENO, error_message, strlen(error_message));
		return 1;
	} else {
		free(*line);
		free(*path_args);
		free(*path);
		exit(0);
	}
	return 0;
}

/* Built-in 'cd' command */
void built_in_cd(int num_of_args, char *args[]) {
	if (num_of_args == 2) { 	/* Check input for correct num of args */
		if (chdir(args[1])==-1) {
			write(STDERR_FILENO, error_message, strlen(error_message));
		}
	} else {
		write(STDERR_FILENO, error_message, strlen(error_message));
	}
	return;
}

/* Built-in 'path' command: Overwrites shell search path */
void built_in_path(char **path, char **args) {
	int i = 1;
	*path = NULL;

	while (args[i] != NULL) {
		if (*path == NULL) {
			*path = (char *)malloc((strlen(args[i])+2));
			strcpy(*path, args[i]);
			strcat(*path, " ");
			i++;
		} else {
			*path = (char *)realloc(*path, (strlen(*path)+strlen(args[i])+2));
			strcat(*path, args[i]);
			strcat(*path, " ");
			i++;
		}
	}
	strcat(*path, "\0");
	return;
}

int parse_line(char *line, char **args, char **args2, int *num_of_args, int *saved_stdout) {
	int i, fd;
	char *cmd, *output[MAXNUM];

	i = 0;
	cmd = line;

	if (strpbrk(cmd, ">")!=0) { 	/*If ">" in input, redirect output to file */ 
		/* Split input in two: cmd and output file */
		while ((args2[i] = strsep(&cmd, ">")) != NULL) {
			i++;
		}
		if (i>2) { 	/* If more than 1 ">" --> error */
			write(STDERR_FILENO, error_message, strlen(error_message));
			return 1;
		} else {
			i = 0;
			/* Check output side of ">" for num of files */
			while ((output[i] = strtok(args2[1], " ")) != NULL) {
				printf("O %d: %s\n", i, output[i]);
				i++;
				args2[1] = NULL;
			}
			if (i!=1) { /* If more or less than 1 output file --> error */
				write(STDERR_FILENO, error_message, strlen(error_message));
				return 1;
			} else {
				i = 0;
				/* Split the command line */
				while ((args[i] = strtok(args2[0], " ")) != NULL) {
					printf("arg %d: %s\n", i, args[i]);
					i++;
					args2[0] = NULL;
					*num_of_args = i;
				}
				/* Open output file */
				fd = open(output[0], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	            if (fd == -1) {
	                perror("Unable to open file");
	                return 1;
	            }
	            /* Save current stdout for use later */
	            *saved_stdout = dup(1);
	            /* Direct stdout to file */
	            dup2(fd, 1);
	            close(fd);
			}
		}
	} else {
		/* Split the command line */
		while ((args[i] = strtok(cmd, " ")) != NULL) {
			printf("arg %d: %s\n", i, args[i]);
			i++;
			cmd = NULL;
			*num_of_args = i;
		}
	}
	return 0;
}

void run_command(char *path, char *paths[], char path_args[], char **args, int background) {
	char *temp_path;
	int pid, i;
	switch (pid = fork()) {
		case -1:
			/* error */
			perror("fork");
			exit(1);
		case 0:
			i = 0;
			temp_path = path;
			if (strcmp(path, "/bin")!=0){ /* Check if path has been changed */
				while ((paths[i] = strtok(temp_path, " ")) != NULL) {
					i++;
					temp_path = NULL;
				}
				i = 0;
				/* Combine path and args[0] for execv */
				while (paths[i] != NULL) {
					if((path_args = malloc(strlen(paths[i])+strlen(args[0])+2)) != NULL){
					    path_args[0] = '\0';
					    
					    strcat(path_args, paths[i]);
					    strcat(path_args, "/");
					    strcat(path_args, args[0]);

					    if (access(path_args, X_OK)==0) { /* Check if path can access file */
							if (execv(path_args, args)==-1) {
								perror("execv");
								exit(1);
							}
						}
					    i++;
					} else {
					   	perror("Malloc");
					    exit(1);
					}
				}
				write(STDERR_FILENO, error_message, strlen(error_message));
				return;

			} else {  /* Default path --> */
				/* Combine path and args[0] for execv */
				if((path_args = malloc(strlen(path)+strlen(args[0])+2)) != NULL){
				    path_args[0] = '\0';
				    strcat(path_args, path);
				    strcat(path_args, "/");
				    strcat(path_args, args[0]);
				} else {
				   	perror("Malloc");
				    exit(1);
				}
				if (execv(path_args, args)==-1) {
					perror("execv");
					exit(1);
				}
			}
			break;
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
	return;
}