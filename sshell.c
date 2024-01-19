#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512

char **whitespace_delimiter(char *cmd)
{
    char *argument = strtok(cmd, " ");
    char **args = malloc(32);
    int count = 2;
    while (argument != NULL)
    {
        args = realloc(args, count * 32);
        args[count-2] = argument;
        argument = strtok(NULL, " ");
        count++;
    }
    args = realloc(args, count * 32);
    args[count+1] = NULL;
    return args;
}

int custom_system(char *cmd)
{
    char** args = whitespace_delimiter(cmd);
    pid_t pid;
    pid = fork();

    if (pid == 0)
    {
        execvp(args[0], args);
        perror("execv");
        exit(1);
    }
    else if (pid > 0)
    {
        int status;
        waitpid(pid, &status, 0);
        //printf("Child returned %d\n", WEXITSTATUS(status));
    }
    else
    {
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

        /* Print prompt poopy */
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

        /* Builtin command */
        if (!strcmp(cmd, "exit"))
        {
            fprintf(stderr, "Bye...\n");
            break;
        }

        /* Regular command change this so its doesn't use system*/
        retval = custom_system(cmd);
        fprintf(stdout, "+ completed '%s' [%d]\n",
                cmd, retval);
    }

    return EXIT_SUCCESS;
}
