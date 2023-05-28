/* ICCS227: Project 1: icsh
 * Name: Abhipob Sethi
 * StudentID: 6480192
 */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_CMD_BUFFER 255

char prev_command[MAX_CMD_BUFFER] = ""; // For the previous command !!

void echo(char *input)
{ // Echo Command
    printf("%s\n", input);
}

void save_command(char *command)
{ // Save Command
    strncpy(prev_command, command, MAX_CMD_BUFFER);
}

void repeat_command()
{ // Handle !! Command
    if (strlen(prev_command) > 0)
    {
        printf("%s\n", prev_command);
        echo(prev_command + 5); // we skip "echo " part here
    }
}

void exit_shell(int exit_code)
{ // Handle exit command
    printf("Bye\n");
    exit(exit_code);
}

void process_command(char *buffer)
{
    if (strlen(buffer) == 0) // Ignore empty command
        return;

    else if (strncmp(buffer, "echo ", 5) == 0)
    {                                 // Check if command is 'echo'
        char *echo_text = buffer + 5; // pointer arithmetic to skip 'echo '
        echo(echo_text);
        save_command(buffer);
    }
    else if (strcmp(buffer, "!!") == 0)
    { // Check if command is '!!'
        repeat_command();
    }
    else if (strncmp(buffer, "exit ", 5) == 0)
    {                                            // Check if command is 'exit'
        int exit_code = atoi(buffer + 5) & 0xFF; // Convert to integer and make it 8 bits
        exit_shell(exit_code);
    }
    else
    {
        // printf("bad command\n");
        pid_t pid = fork();
        if (pid == 0)
        {
            // Child process
            char *command = strtok(buffer, " ");
            char *args[MAX_CMD_BUFFER];
            args[0] = command;

            int i = 1;
            while ((args[i] = strtok(NULL, " ")))
                i++;

            execvp(command, args);
            printf("Failed to execute command: %s\n", command);
            exit(1);
        }
        else if (pid < 0)
        {
            // Error forking
            printf("Failed to fork a child process\n");
        }
        else
        {
            // Parent process
            int status;
            waitpid(pid, &status, 0);
        }
    }
}

int main(int argc, char *argv[])
{
    char buffer[MAX_CMD_BUFFER];

    if (argc > 1)
    { // Script Handle
        FILE *script = fopen(argv[1], "r");
        if (!script)
        {
            printf("Failed to open script: %s\n", argv[1]);
            return 1;
        }

        while (fgets(buffer, sizeof(buffer), script) != NULL)
        {
            buffer[strcspn(buffer, "\n")] = 0; // Remove newline character at the end
            process_command(buffer);
        }

        fclose(script);
    }
    else
    {
        while (1)
        {
            printf("icsh $ ");
            fgets(buffer, 255, stdin);

            buffer[strcspn(buffer, "\n")] = 0; // Removes \n

            // printf("you said: '%s'\n", buffer);

            process_command(buffer);
        }
    }
}
