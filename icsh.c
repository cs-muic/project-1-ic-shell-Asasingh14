/* ICCS227: Project 1: icsh
 * Name: Abhipob Sethi
 * StudentID: 6480192
 */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define MAX_CMD_BUFFER 255

char prev_command[MAX_CMD_BUFFER] = ""; // For the previous command !!

void echo(char *input)
{ // Echo Command
    printf("%s\n", input);
}

void save_command(char* command) { // Save Command
    strncpy(prev_command, command, MAX_CMD_BUFFER);
}

void repeat_command(){ //Handle !! Command
    if (strlen(prev_command) > 0) {
        printf("%s\n", prev_command);
        echo(prev_command + 5); // we skip "echo " part here
    }
}

void exit_shell(int exit_code)
{ // Handle exit command
    printf("Bye\n");
    exit(exit_code);
}

int main()
{
    char buffer[MAX_CMD_BUFFER];
    while (1)
    {
        printf("icsh $ ");
        fgets(buffer, 255, stdin);

        buffer[strcspn(buffer, "\n")] = 0; // Removes \n

        // printf("you said: '%s'\n", buffer);

        if (strlen(buffer) == 0) // Ignore empty command
            continue; 

        else if (strncmp(buffer, "echo ", 5) == 0) { // Check if command is 'echo'                                
            char *echo_text = buffer + 5; // pointer arithmetic to skip 'echo '
            echo(echo_text);
            save_command(buffer);
        }
        else if (strcmp(buffer, "!!") == 0) { // Check if command is '!!'
            repeat_command();
        }
        else if (strncmp(buffer, "exit ", 5) == 0){ // Check if command is 'exit'
            int exit_code = atoi(buffer + 5) & 0xFF; // Convert to integer and make it 8 bits
            exit_shell(exit_code);
        }
        else
        {
            printf("bad command\n");
        }
    }
}
