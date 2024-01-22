#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include <fcntl.h>

#define CMDLINE_MAX 512
#define CWD_MAX 1024
#define MAX_ARG_SIZE 32
#define MAX_NUM_ARGS 16

// char **whitespace_delimiter(char *cmd)
// {

//     char *argument = strtok(cmd, " ");
//     char **args = malloc(MAX_BYTES);
//     int count = 2;
//     while (argument != NULL)
//     {
//         args = realloc(args, count * MAX_BYTES);
//         args[count - 2] = argument;
//         argument = strtok(NULL, " ");
//         count++;
//     }
//     args = realloc(args, count * MAX_BYTES);
//     args[count + 1] = NULL;
//     return args;
// }

/*TO DOOOOO: PLS MAKE SURE THAT WE CHECK IF BUFFER IS EMPTY WHEN WE ENCOUNTER THE KEY CHAR TO DETERMINE IF WE EXECUTE*/
char **custom_parser(char *cmd)
{
    char buffer[MAX_ARG_SIZE + 1];
    char **args = malloc(MAX_NUM_ARGS * sizeof(char *));
    int pointer = 0;  // index of cmd
    int count = 0;    // index of buffer
    int argCount = 0; // number of arguments
    while (cmd[pointer] != '\0')
    {
        if (count != 0 && cmd[pointer] == ' ')
        { // check if buffer is empty
            buffer[count] = '\0';
            args[argCount] = malloc(MAX_ARG_SIZE); // allocate memory for buffer
            strcpy(args[argCount], buffer);        // copy over buffer content
            argCount++;
            memset(buffer, '\0', sizeof(buffer)); // clear the buffer
            count = 0;
        }
        else if (cmd[pointer] == '|')
        {
            if (count != 0)
            { // check if buffer is empty
                buffer[count] = '\0';
                args[argCount] = malloc(MAX_ARG_SIZE); // allocate memory for buffer
                strcpy(args[argCount], buffer);        // copy over content
                argCount++;
                memset(buffer, '\0', sizeof(buffer)); // clear buffer
            }
            args[argCount] = malloc(MAX_ARG_SIZE); // allocate memory for |
            args[argCount] = "|";                  // add "|" to array
            argCount++;
            count = 0;
        }
        else if (cmd[pointer] == '>' && cmd[pointer + 1] == '>')
        {
            if (count != 0)
            { // check if buffer is empty
                buffer[count] = '\0';
                args[argCount] = malloc(MAX_ARG_SIZE); // allocate memory for buffer
                strcpy(args[argCount], buffer);        // copy over content
                argCount++;
                memset(buffer, '\0', sizeof(buffer)); // clear buffer
            }
            args[argCount] = malloc(MAX_ARG_SIZE); // allocate memory for >>
            args[argCount] = ">>";                 // add ">" to array
            argCount++;
            count = 0; // set buffer back to 0
            pointer++; // move onto next cmd char
        }
        else if (cmd[pointer] == '>')
        {
            if (count != 0)
            { // check if buffer is empty
                buffer[count] = '\0';
                args[argCount] = malloc(MAX_ARG_SIZE); // allocate memory for buffer
                strcpy(args[argCount], buffer);        // copy over content
                argCount++;
                memset(buffer, '\0', sizeof(buffer)); // clear buffer
            }
            args[argCount] = malloc(MAX_ARG_SIZE); // allocate memory for >
            args[argCount] = ">";                  // add ">" to array
            argCount++;
            count = 0;
        }
        else if (!isspace(cmd[pointer]))
        {
            buffer[count] = cmd[pointer];
            count++;
        }
        pointer++;
    }
    /*WRITE CODE THAT WILL HANDLE ADDING THE LAST ARG AND NULL*/
    args[argCount] = malloc(MAX_ARG_SIZE);
    strcpy(args[argCount], buffer);
    argCount++;
    args = realloc(args, (argCount + 1) * sizeof(char *));
    args[argCount] = NULL;
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
        if (strcmp(args[i], ">") == 0 && i != count - 1)
        {
            int fileDesc = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0777); // create file descriptor
            /*Add Error Checking here*/
            dup2(fileDesc, STDOUT_FILENO); // duplicate to stdout
            close(fileDesc);               // remove original descriptor
            args[i] = NULL;                // set ">" as NULL
            args[i + 1] = NULL;            // set "filename.txt" to null
            break;
        }
        else if (strcmp(args[i], ">>") == 0 && i != count - 1)
        {
            int fileDesc = open(args[i + 1], O_WRONLY | O_APPEND | O_CREAT, 0777); // create file descriptor
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
        return 1;
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

        char **args = custom_parser(cmd);

        /* Save output redirection */
        int saved_out = dup(1);
        redirect(args);

        /* Execute command*/
        retval = custom_system(args);
        /* Restore output redirection */

        if (retval == 1)
        { // Command not Found
            printf("Error: command not found\n");
        }
        dup2(saved_out, 1);
        close(saved_out);
        fprintf(stdout, "+ completed '%s' [%d]\n", cmd, retval);
    }

    return EXIT_SUCCESS;
}