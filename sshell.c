#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define CMDLINE_MAX 512
#define CWD_MAX 1024
#define MAX_BYTES 32

char **whitespace_delimiter(char *cmd)
{
    char *argument = strtok(cmd, " ");
    char **args = malloc(MAX_BYTES);
    int count = 2;
    while (argument != NULL)
    {
        args = realloc(args, count * MAX_BYTES);
        args[count - 2] = argument;
        argument = strtok(NULL, " ");
        count++;
    }
    args = realloc(args, count * MAX_BYTES);
    args[count + 1] = NULL;
    return args;
}

void redirect(char **args)
{
    int count = 0; // Retrieve num of args
    while (args[count] != NULL)
    {
        count++;
    }

    for (int i = 0; i < count; i++)
    { // Find the first redirect symbol (Assuming there is only one redirect)
        if (strcmp(args[i], ">") == 0 && i != count-1)
        {
            int fileDesc = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0777); // create file descriptor
            /*Add Error Checking here*/
            dup2(fileDesc, STDOUT_FILENO); // duplicate to stdout
            close(fileDesc);               // remove original descriptor
            args[i] = NULL;                // set ">" as NULL
            args[i + 1] = NULL;            // set "filename.txt" to null
            break;
        }
        else if (strcmp(args[i], ">>") == 0 && i != count-1)
        {
            int fileDesc = open(args[i + 1], O_WRONLY | O_APPEND, 0777); // create file descriptor
            /*Add Error Checking here*/
            dup2(fileDesc, STDOUT_FILENO); // duplicate to stdout
            close(fileDesc);               // remove original descriptor
            args[i] = NULL;                // set ">" as NULL
            args[i + 1] = NULL;            // set "filename.txt" to null
            break;
        }
    }
}

void cd_command(char **args)
{
    int status = chdir(args[1]);
    if (status != 0)
    {
        perror("cd");
    }
}

void pwd_command()
{
    char cwd[CWD_MAX];
    char *status = getcwd(cwd, sizeof(cwd));
    if (status == NULL)
    {
        perror("pwd");
    }
    printf("%s\n", cwd);
}

int custom_system(char **args)
{
    /*CD COMMAND*/
    if (!strcmp(args[0], "cd"))
    {
        cd_command(args);
        return 0;
    }
    /*PWD COMMAND*/
    if (!strcmp(args[0], "pwd"))
    {
        pwd_command();
        return 0;
    }

    pid_t pid;
    pid = fork();
    
    if (pid == 0)
    {
        /*Child Process*/
        execvp(args[0], args);
        perror("execv");
        exit(1);
    }
    else if (pid > 0)
    {
        /*Parent Process*/
        int status;
        waitpid(pid, &status, 0);
        // printf("Child returned %d\n", WEXITSTATUS(status));
    }
    else
    {
        // Error Returned
        perror("fork");
        exit(1);
    }
    free(args);
    return 0;
}

int main(void)
{
    char cmd[CMDLINE_MAX];

    while (1)
    {
        char *nl;
        int retval;

        /* Print prompt */
        printf("sshell@ucd$ ");
        fflush(stdout);

        /* Get command line */
        fgets(cmd, CMDLINE_MAX, stdin);

        /* Print command line if stdin is not provided by terminal */
        if (!isatty(STDIN_FILENO))
        {
            printf("%s", cmd);
            fflush(stdout);
        }

        /* Remove trailing newline from command line */
        nl = strchr(cmd, '\n');
        if (nl)
            *nl = '\0';

        /* Builtin commands */
        if (!strcmp(cmd, "exit"))
        {
            fprintf(stderr, "Bye...\n");
            break;
        }

        char **args = whitespace_delimiter(cmd);

        /* Save output redirection */
        int saved_out = dup(1);
        redirect(args);

        /* Execute command*/
        retval = custom_system(args);
        /* Restore output redirection */
        dup2(saved_out, 1);
        close(saved_out);
        fprintf(stdout, "+ completed '%s' [%d]\n", cmd, retval);
    }

    return EXIT_SUCCESS;
}