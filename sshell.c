#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512

int custom_system(char *cmd)
{
    pid_t pid;
    pid = fork();
    char args[] = {cmd, NULL};
    if (pid == 0)
    {
        execv(cmd, args);
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
        charnl;
        int retval;

        /* Print prompt poopy*/
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

        / Remove trailing newline from command line * /
            nl = strchr(cmd, '\n');
        if (nl)
            nl = '\0';

        /*Builtin command */
        if (!strcmp(cmd, "exit"))
        {
            fprintf(stderr, "Bye...\n");
            break;
        }

        /*Regular command change this so its doesn't use system*/
        retval = custom_system(cmd);
        fprintf(stdout, "Return status value for '%s': %d\n",
                cmd, retval);
    }

    return EXIT_SUCCESS;
}