#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512

char **whitespace_delimiter(char *cmd)
{
    char *argument = strtok(cmd, " ");
    char **args = malloc(1 * sizeof(char *));
    int count = 0;
    while (argument != NULL)
    {
        args = realloc(args, 1 * sizeof(char *));
        args[count] = argument;
        count++;
        argument = strtok(NULL, " ");
    }
    return args;
}

int custom_system(char *cmd)
{
    pid_t pid;
    pid = fork();
    char *args[];

    char **test = whitespace_delimiter(cmd);

    test = realloc()

        if (pid == 0)
    {
        execvp(test[0], args);
        perror("execv");
        exit(1);
    }
    else if (pid > 0)
    {
        int status;
        waitpid(pid, &status, 0);
        printf("Child returned %d\n", WEXITSTATUS(status));
    }
    else
    {
        perror("fork");
        exit(1);
    }
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
