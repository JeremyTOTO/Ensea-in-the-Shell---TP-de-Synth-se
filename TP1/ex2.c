#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#define MAX_COMMAND_LENGTH 1024

int main() {
    char command[MAX_COMMAND_LENGTH];
    char welcome_message[] = "Bienvenue dans le Shell ENSEA.\n";
    char exit_message[] = "Pour quitter, tapez 'exit'.\n";
    char prompt[] = "enseash %% ";
    char error_message[] = "error_message \n";
    char *fortune_args[] = {"fortune", NULL};
    pid_t pid = fork();
    write(STDOUT_FILENO, welcome_message, strlen(welcome_message));
    write(STDOUT_FILENO, exit_message, strlen(exit_message));

    while (1) {
        write(STDOUT_FILENO, prompt, strlen(prompt));
        fgets(command, MAX_COMMAND_LENGTH, stdin);

        command[strcspn(command, "\n")] = 0;

        if (strcmp(command, "exit") == 0) {
            break;
        } else if (strcmp(command, "fortune") == 0 ) {
            if (pid == 0) {
				
            execvp(fortune_args[0], fortune_args);
              
            }
        } else {
            write(STDOUT_FILENO, error_message, strlen(error_message));
        }
    }

    return 0;
}
