#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

#define MAX_COMMAND_LENGTH 1024

int main() {
    int status;
    char command[MAX_COMMAND_LENGTH];
    char welcome_message[] = "Bienvenue dans le Shell ENSEA.\n";
    char exit_message[] = "Pour quitter, tapez 'exit'.\n";
    char exit_bye[] = "Bye bye...'.\n";
    char prompt[] = "enseash %% ";
    char error_message[] = "error_message \n";
    char *fortune_args[] = {"fortune", NULL};
    pid_t pid = fork();
    write(STDOUT_FILENO, welcome_message, strlen(welcome_message));
    write(STDOUT_FILENO, exit_message, strlen(exit_message));
    
    while (1) {

        waitpid(-1, &status, 0);
        WIFEXITED(status);
        printf("enseash [exit:%d] %% ", WEXITSTATUS(status));
		printf("enseash [sign:%d] %% ", WTERMSIG(status));
		
		printf("%d", pid);


        write(STDOUT_FILENO, prompt, strlen(prompt));
        fgets(command, MAX_COMMAND_LENGTH, stdin);
        WIFSIGNALED(status);

        command[strcspn(command, "\n")] = 0;


        if ((strcmp(command, "exit") == 0)||feof(stdin)) {
            write(STDOUT_FILENO, exit_bye, strlen(exit_bye));
            break;
        } else if (strcmp(command, "fortune") == 0) {
            if (pid == 0) {

            execvp(fortune_args[0], fortune_args);
            WIFEXITED(status);

              
            }
        } else {
            write(STDOUT_FILENO, error_message, strlen(error_message));
            WIFEXITED(status);
        }

    }

    return 0;
}
