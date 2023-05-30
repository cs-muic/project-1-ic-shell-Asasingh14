
/* ICCS227: Project 1: icsh
 * Name: Abhipob Sethi
 * StudentID: 6480192
 */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/wait.h"
#include "signal.h"
#include "fcntl.h"

#define MAX_CMD_BUFFER 255
#define MAX_BG_JOBS 10

typedef struct
{
    pid_t pid;
    int job_id;
    char command[MAX_CMD_BUFFER];
    int completed;
    int stopped;
} BackgroundJob;

char prev_command[MAX_CMD_BUFFER] = ""; // For the previous command !!
pid_t foreground_pid = -1;
int prev_exit_status = 0;
BackgroundJob bg_jobs[MAX_BG_JOBS];
int num_bg_jobs = 0;

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

void list_jobs()
{ // List Current Jobs
    printf("Job ID\tPID\tStatus\tCommand\n");
    for (int i = 0; i < num_bg_jobs; i++)
    {
        if (!bg_jobs[i].completed)
        {
            int status;
            pid_t result = waitpid(bg_jobs[i].pid, &status, WNOHANG);
            if (result == 0)
            {
                // Process Running
                printf("[%d]\t%d\tRunning\t%s\n", bg_jobs[i].job_id, bg_jobs[i].pid, bg_jobs[i].command);
            }
            else if (result == -1)
            {
                // Error
                printf("[%d]\t%d\tError\t%s\n", bg_jobs[i].job_id, bg_jobs[i].pid, bg_jobs[i].command);
            }
            else
            {
                // Process Complete
                bg_jobs[i].completed = 1;
                prev_exit_status = WEXITSTATUS(status);
                printf("[%d]\t%d\tDone\t%s\n", bg_jobs[i].job_id, bg_jobs[i].pid, bg_jobs[i].command);
            }
        }
    }
}

void foreground_job(int job_id)
{
    int job_index = job_id - 1;
    if (job_index >= 0 && job_index < num_bg_jobs)
    {
        pid_t pid = bg_jobs[job_index].pid;
        if (bg_jobs[job_index].stopped)
            kill(pid, SIGCONT); // Resume if stopped

        foreground_pid = pid;
        int status;
        waitpid(pid, &status, 0);
        if (WIFSTOPPED(status))
        {
            bg_jobs[job_index].stopped = 1;
            printf("Foreground job suspended\n");
        }
        else
        {
            bg_jobs[job_index].completed = 1;
            prev_exit_status = WEXITSTATUS(status);
        }
        foreground_pid = -1; // Reset the foreground process ID
    }
    else
    {
        printf("Invalid job ID\n");
    }
}

void background_job(int job_id)
{ // Execute the suspended job in the background
    int job_index = job_id - 1;
    if (job_index >= 0 && job_index < num_bg_jobs)
    {
        pid_t pid = bg_jobs[job_index].pid;
        if (bg_jobs[job_index].stopped)
        {
            bg_jobs[job_index].stopped = 0;
            bg_jobs[job_index].completed = 0;
            kill(pid, SIGCONT); // Resume if stopped
            printf("[%d]+  Running\t%s\n", bg_jobs[job_index].job_id, bg_jobs[job_index].command);
        }
        else
        {
            printf("Job is not stopped\n");
        }
    }
    else
    {
        printf("Invalid job ID\n");
    }
}

int get_job_id(char *job_id_str)
{
    if (strlen(job_id_str) < 2)
    {
        return -1;
    }
    char *ptr = job_id_str + 1; // Ignore %
    return atoi(ptr);
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
    else if (strcmp(buffer, "jobs") == 0)
    {
        list_jobs();
    }
    else if (strncmp(buffer, "fg ", 3) == 0)
    {
        int job_id = get_job_id(buffer + 3);
        if (job_id > 0)
            foreground_job(job_id);
        else
            printf("Invalid job ID\n");
    }
    else if (strncmp(buffer, "bg ", 3) == 0)
    {
        int job_id = get_job_id(buffer + 3);
        if (job_id > 0)
            background_job(job_id);
        else
            printf("Invalid job ID\n");
    }
    else
    {

        // printf("bad command\n"); Not needed Anymore

        int background_execution = 0; // Flag to indicate background execution
        if (buffer[strlen(buffer) - 1] == '&')
        {
            buffer[strlen(buffer) - 1] = '\0'; // Remove the '&' symbol
            background_execution = 1;
        }

        pid_t pid = fork(); // Used to create a new fork

        if (pid == 0)
        {
            // Child process
            char *command = strtok(buffer, " ");
            char *args[MAX_CMD_BUFFER];
            args[0] = command;

            int i = 1;
            while ((args[i] = strtok(NULL, " "))) // Get args
                i++;

            // Add for Handling I/O Redirection
            char *redirect_type = NULL;
            char *file_name = NULL;

            // Look For </>
            for (int j = 1; j < i; j++)
            {
                if (strcmp(args[j], ">") == 0 || strcmp(args[j], "<") == 0)
                {
                    redirect_type = args[j];
                    file_name = args[j + 1];
                    args[j] = NULL;
                    args[j + 1] = NULL;
                    break;
                }
            }

            if (redirect_type != NULL && file_name != NULL)
            {
                int fd;
                if (strcmp(redirect_type, ">") == 0)
                {
                    // Open the file in write-only mode, create if not exists, truncate if exists
                    fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd == -1)
                    {
                        printf("Failed to open output file: %s\n", file_name);
                        exit(1);
                    }
                    dup2(fd, STDOUT_FILENO); // Redirect stdout to the file
                }
                else if (strcmp(redirect_type, "<") == 0)
                {
                    // Open the file in read-only mode
                    fd = open(file_name, O_RDONLY);
                    if (fd == -1)
                    {
                        printf("Failed to open input file: %s\n", file_name);
                        exit(1);
                    }
                    dup2(fd, STDIN_FILENO); // Redirect stdin to the file
                }

                close(fd); // Close the file descriptor
            }

            execvp(args[0], args); // Execute the command with args
            printf("Failed to execute command: %s\n", command);
            exit(1); // Normal Exit Code
        }
        else if (pid < 0)
        {
            // Error Forking
            printf("Failed to fork a child process\n");
        }
        else
        {
            // Parent process
            if (background_execution)
            {
                // Add the command to background jobs list
                bg_jobs[num_bg_jobs].pid = pid;
                bg_jobs[num_bg_jobs].job_id = num_bg_jobs + 1;
                strncpy(bg_jobs[num_bg_jobs].command, buffer, MAX_CMD_BUFFER);
                bg_jobs[num_bg_jobs].completed = 0;
                bg_jobs[num_bg_jobs].stopped = 0;
                num_bg_jobs++;

                printf("[%d] %d\n", num_bg_jobs, pid);
            }
            else
            {
                // Foreground job
                foreground_pid = pid;
                int status;
                waitpid(pid, &status, 0);
                if (WIFSTOPPED(status))
                {
                    int job_index = num_bg_jobs - 1;
                    bg_jobs[job_index].stopped = 1;
                    printf("[%d]+  Stopped\t%s\n", bg_jobs[job_index].job_id, bg_jobs[job_index].command);
                }
                else
                {
                    prev_exit_status = WEXITSTATUS(status);
                }
                foreground_pid = -1; // Reset the foreground process ID
            }
        }
    }
}

void handle_sigint(int signum)
{
    if (foreground_pid != -1)
        kill(foreground_pid, SIGINT);
}

void handle_sigtstp(int signum)
{
    if (foreground_pid != -1)
    {
        kill(foreground_pid, SIGTSTP);
        foreground_pid = -1; // Reset foreground PID to indicate no foreground job
    }
}

int main(int argc, char *argv[])
{
    char buffer[MAX_CMD_BUFFER];

    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);

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
