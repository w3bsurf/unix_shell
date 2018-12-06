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

// Declare functions
int parse_line(char *line, char **args, char **args2, int *num_of_args, int *saved_stdout);

int built_in_exit(int num_of_args, char **line, char **path, char **path_args);

void built_in_cd(int num_of_args, char *args[]);

void built_in_path(char **path, char **args);

void run_command(char *path, char *paths[], char path_args[], char **args);

void executor(char **arg, int *saved_stdout);

int main(int argc, char **argv) {
	char *line;
	size_t buffer_size = MAXLEN;
	int saved_stdout = dup(1);
	FILE* file;

	line = (char *)malloc(buffer_size * sizeof(char));
    if( line == NULL) {
        perror("Unable to allocate buffer");
        exit(1);
    }

	if (argc == 1 || argc == 2) {
		while (1) {
			/* Restore stdout */
			// Source: https://stackoverflow.com/questions/11042218/c-restore-stdout-to-terminal
			dup2(saved_stdout, 1);
			close(saved_stdout);
			
			if (argc == 1) { /* Run in interactive mode if no argument given */
				/* Print the prompt */
				printf("wish> ");
			
				/* Read the users command */
				getline(&line, &buffer_size, stdin);
				/* Execute command */
				executor(&line, &saved_stdout);
				continue;

			} else {  /* Run in batch mode if argument given */
				if ((file = fopen(argv[1], "r")) == NULL) {
					write(STDERR_FILENO, error_message, strlen(error_message));
					exit(1);
				}
				/* Read batch file for commands */
				while ((getline(&line, &buffer_size, file)!=-1))  {
					/* Execute command */
					executor(&line, &saved_stdout);
				}
				free(line);
				fclose(file);
				exit(0);
			}							
		}
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

/* Parses given line and redirects ouput to file if needed */
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
			while ((output[i] = strtok(args2[1], " \t")) != NULL) {
				i++;
				args2[1] = NULL;
			}
			if (i!=1) { /* If more or less than 1 output file --> error */
				write(STDERR_FILENO, error_message, strlen(error_message));
				return 1;
			} else {
				i = 0;
				/* Split the command line */
				while ((args[i] = strtok(args2[0], " \t")) != NULL) {
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
		while ((args[i] = strtok(cmd, " \t")) != NULL) {
			i++;
			cmd = NULL;
			*num_of_args = i;
		}
	}
	return 0;
}

/* Forks and runs command in given search path with execv */
void run_command(char *path, char *paths[], char path_args[], char **args) {
	char *temp_path;
	int pid, i, status;
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
			if (wait(&status) == -1) {
	            perror("wait");
	            exit(1);
	        }
			break;
	}
	return;
}

/* Checks conditions and executes code and calls functions approriately */
void executor(char **arg, int *saved_stdout) {
	char *paths[MAXNUM], *args[MAXNUM], *args2[MAXNUM], *lines[MAXNUM];
	char *cmd, *path = "/bin";
	char *path_args = NULL;
	int i, num_of_args;
	char *line = *arg;
	
	if (line == NULL) {
		printf("\nlogout\n");
		exit(0);
	}

	if (strlen(line) == 0) {
		return;
	}

	line[strlen(line) - 1] = '\0';

	if (strpbrk(line, "&")!=0) { /* Run multiple commands in parallel if "&" in input*/
		i = 0;
		cmd = line;
		/* Split input into separate commands */
		while ((lines[i] = strsep(&cmd, "&")) != NULL) { 
			i++;
		}
		/* Parse and run each separate command */
		for (int x = 0; x<i; x++) {
			if (parse_line(lines[x], args, args2, &num_of_args, saved_stdout)==1) {
				return;
			}
			/* Skip command if NULL */
			if (args[0]==NULL) {
				continue;
			}
			/* Check commands for built-ins */
			if (strcmp(args[0], "exit")==0) { 
				if (built_in_exit(num_of_args, &line, &path, &path_args)==1) {
					return;
				}
			} else if (strcmp(args[0], "cd")==0) {
				built_in_cd(num_of_args, args);
				return;
			} else if (strcmp(args[0], "path")==0) {
				built_in_path(&path, args);
				return;
			} else { 	/* If no built-in command given, run with execv */
				/* Run command with given arguments */
				run_command(path, paths, path_args, args);

				/* Restore stdout */
				dup2(*saved_stdout, 1);
				close(*saved_stdout);
			}	
		}
		return;
	} else { /* Run single command if no "&" in input */
		if (parse_line(line, args, args2, &num_of_args, saved_stdout)==1) {
			return;
		}
		/* Skip command if NULL */
		if (args[0]==NULL) {
			return;
		}
		/* Check commands for built-ins */
		if (strcmp(args[0], "exit")==0) { 
			if (built_in_exit(num_of_args, &line, &path, &path_args)==1) {
				return;
			}
		} else if (strcmp(args[0], "cd")==0) {
			built_in_cd(num_of_args, args);
			return;
		} else if (strcmp(args[0], "path")==0) {
			built_in_path(&path, args);
			return;
		} else { 	/* If no built-in command given, run with execv */
			/* Run command with given arguments */
			run_command(path, paths, path_args, args);
			return;
		}				
	}
}