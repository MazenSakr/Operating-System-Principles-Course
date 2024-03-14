#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

// Function prototypes
void setup_environment();
void shell_loop();
void execute_command(char **args, char bg);
void handle_sigchld(int sig);
void register_signal_handler();

int main(int argc, char **argv) {
    setup_environment();
    register_signal_handler();
    shell_loop();
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char **parse_input(char *line) {
    int bufsize = 64, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

    if (!tokens) {
        fprintf(stderr, "mysh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, " \t\r\n\a");
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += 64;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "mysh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, " \t\r\n\a");
    }
    tokens[position] = NULL;
    return tokens;
}



void shell_loop() {
    char *line;
    size_t len = 0;
    ssize_t read;
    char **args;
    int status;
    char background = 0;

do {
    printf("> ");
    read = getline(&line, &len, stdin);
    if (read == -1) {
        break;
    }

    // Check for '&' at the end of the command
    if (line[strlen(line) - 2] == '&') {
        background = 1;
        line[strlen(line) - 2] = '\0'; // Remove '&' from the command
    }

    args = parse_input(line);
    if (args == NULL) {
        free(line);
        continue;
    }

    execute_command(args, background);
    
} while (strcmp(args[0], "exit") != 0);
    free(args);
    free(line);
}

void execute_command(char **args, char bg) {
    if (strcmp(args[0], "cd") == 0) {
            if ((args[1] == NULL) || ((strcmp(args[1], "~") == 0))) {
        chdir(getenv("HOME"));

    }
    else {
        int flag = 0;
        flag = chdir(args[1]);
        if (flag != 0) {
            printf("Error, the directory is not found\n");
        }
    }
    } else if (strcmp(args[0], "echo") == 0) {
        // Implement echo command
                for (int i = 1; args[i] != NULL; i++) {
            printf("%s ", args[i]);
        }
        printf("\n"); // Print a newline character at the end        
    } else if (strcmp(args[0], "exit") == 0) {
        exit(0);
    } else {
        // Assume it's an external command
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            if (execvp(args[0], args) == -1) {
                perror("mysh");
            }
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            // Parent process
            if (!bg) {
                int status;
                do {
                    waitpid(pid, &status, WUNTRACED);
                } while (!WIFEXITED(status) && !WIFSIGNALED(status));
            }
        } else {
            perror("mysh");
        }
    }
}


void handle_sigchld(int sig) {
    // Reap the child process
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;

    // Append to log file
    FILE *logfile = fopen("shell_log.txt", "a");
    if (logfile == NULL) {
        perror("mysh");
    } else {
        fprintf(logfile, "Child process was terminated\n");
        fclose(logfile);
    }
}

void register_signal_handler() {
    struct sigaction sa;
    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("mysh");
        exit(1);
    }
}


void setup_environment() {
    // Set the current working directory
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working directory: %s\n", cwd);
    } else {
        perror("mysh");
    }
}

