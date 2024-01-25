#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>

#define CMDLINE_MAX 512
#define CWD_MAX 1024
#define MAX_ARG_SIZE 32
#define MAX_NUM_ARGS 16

struct Command_Node
{
    char **args;
    struct Command_Node *next;
    pid_t pid;
} Command_Node;

void appendNode(struct Command_Node **head, char **args)
{
    struct Command_Node *newNode = (struct Command_Node *)malloc(sizeof(Command_Node));
    if (newNode == NULL)
    {
        return;
    }
    newNode->args = args;
    newNode->next = NULL;
    if (*head == NULL)
    {
        *head = newNode;
        return;
    }
    struct Command_Node *current = *head;
    while (current->next != NULL)
    {
        current = current->next;
    }
    current->next = newNode;
}

struct Command_Node *createLinkedList(char **args)
{
    struct Command_Node *head = NULL;
    int commandSize = 0;
    int i = 0;
    while (args[i - 1] != NULL)
    {
        if (!args[i] || strcmp(args[i], "|") == 0)
        {
            // Create a new subarray with an additional NULL element
            char **newArgs = (char **)malloc((commandSize + 1) * sizeof(char *));
            // Copy elements from the original subarray
            for (int j = 0; j < commandSize; ++j)
            {
                newArgs[j] = args[i - commandSize + j];
            }
            newArgs[commandSize] = NULL;

            // Append the node with the new subarray
            appendNode(&head, newArgs);
            commandSize = 0;
        }
        else
        {
            commandSize += 1;
        }
        i += 1;
    }

    return head;
}

int getNumCommands(struct Command_Node *head)
{
    int num = 0;
    struct Command_Node *curr = head;
    while (curr)
    {
        num++;
        curr = curr->next;
    }
    return num;
}

/*TO DOOOOO: PLS MAKE SURE THAT WE CHECK IF BUFFER IS EMPTY WHEN WE ENCOUNTER THE KEY CHAR TO DETERMINE IF WE EXECUTE*/
char **custom_parser(char *cmd)
{
    char buffer[MAX_ARG_SIZE + 1];
    char **args = malloc(sizeof(char *));
    int pointer = 0;  // index of cmd
    int count = 0;    // index of buffer
    int argCount = 0; // number of arguments

    while (cmd[pointer] != '\0')
    {
        if (count != 0 && cmd[pointer] == ' ')
        { // check if buffer is empty
            buffer[count] = '\0';
            args = realloc(args, (argCount + 1) * sizeof(char *));
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
                args = realloc(args, (argCount + 1) * sizeof(char *));
                args[argCount] = malloc(MAX_ARG_SIZE); // allocate memory for buffer
                strcpy(args[argCount], buffer);        // copy over content
                argCount++;
                memset(buffer, '\0', sizeof(buffer)); // clear buffer
            }
            args = realloc(args, (argCount + 1) * sizeof(char *));
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
                args = realloc(args, (argCount + 1) * sizeof(char *));
                args[argCount] = malloc(MAX_ARG_SIZE); // allocate memory for buffer
                strcpy(args[argCount], buffer);        // copy over content
                argCount++;
                memset(buffer, '\0', sizeof(buffer)); // clear buffer
            }
            args = realloc(args, (argCount + 1) * sizeof(char *));
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
                args = realloc(args, (argCount + 1) * sizeof(char *));
                args[argCount] = malloc(MAX_ARG_SIZE); // allocate memory for buffer
                strcpy(args[argCount], buffer);        // copy over content
                argCount++;
                memset(buffer, '\0', sizeof(buffer)); // clear buffer
            }
            args = realloc(args, (argCount + 1) * sizeof(char *));
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
    if (count != 0)
    {
        args = realloc(args, (argCount + 1) * sizeof(char *));
        args[argCount] = malloc(MAX_ARG_SIZE);
        buffer[count] = '\0';
        strcpy(args[argCount], buffer);
        argCount++;
    }

    args = realloc(args, (argCount + 1) * sizeof(char *));
    args[argCount] = NULL;
    return args;
}

bool redirect(char **args)
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
            if (fileDesc == -1)
            {
                fprintf(stderr, "Error: cannot open output file");
                return true;
            }
            dup2(fileDesc, STDOUT_FILENO); // duplicate to stdout
            close(fileDesc);               // remove original descriptor
            args[i] = NULL;                // set ">" as NULL
            args[i + 1] = NULL;            // set "filename.txt" to null
            break;
        }
        else if (strcmp(args[i], ">>") == 0 && i != count - 1)
        {
            int fileDesc = open(args[i + 1], O_WRONLY | O_APPEND | O_CREAT, 0777); // create file descriptor
            if (fileDesc == -1)
            {
                fprintf(stderr, "Error: cannot open output file");
                return true;
            }
            dup2(fileDesc, STDOUT_FILENO); // duplicate to stdout
            close(fileDesc);               // remove original descriptor
            args[i] = NULL;                // set ">" as NULL
            args[i + 1] = NULL;            // set "filename.txt" to null
            break;
        }
    }
    return false;
}

int cd_command(char **args)
{
    int status = chdir(args[1]);
    if (status != 0)
    {
        fprintf(stderr, "Error: cannot cd into directory\n");
        return 1;
    }
    return 0;
}

int pwd_command()
{
    char cwd[CWD_MAX];
    char *status = getcwd(cwd, sizeof(cwd));
    if (status == NULL)
    {
        perror("pwd");
        return 1;
    }
    printf("%s\n", cwd);
    return 0;
}
int sls_command()
{
    DIR *directory;
    struct dirent *directoryContent;
    directory = opendir("."); // open current directory
    struct stat st;

    while ((directoryContent = readdir(directory)) != NULL)
    {
        if (!strcmp(directoryContent->d_name, ".") || !strcmp(directoryContent->d_name, ".."))
        {
        }
        else
        {
            printf("%s ", directoryContent->d_name);
            if (stat(directoryContent->d_name, &st) == 0)
            {
                printf("(%ld bytes)\n", st.st_size); // in bytes
            }
        }
    }
    closedir(directory);
    return 0;
}

void closeFDS(int fds[][2], int numCommands)
{
    for (int i = 0; i < numCommands - 1; i++)
    {
        close(fds[i][0]);
        close(fds[i][1]);
    }
}

bool isBuiltIn(char *command)
{
    return (!strcmp(command, "cd") || !strcmp(command, "sls") || !strcmp(command, "pwd"));
}

void custom_system(struct Command_Node *head, int numCommands, int returnValues[])
{
    // create array of pipes;
    int fds[numCommands - 1][2];
    for (int i = 0; i < numCommands - 1; i++)
    {
        pipe(fds[i]);
    }

    struct Command_Node *curr = head;
    int currProcess = 0;
    while (curr)
    {
        /*CD COMMAND*/
        if (!strcmp(curr->args[0], "cd"))
        {
            returnValues[currProcess] = cd_command(curr->args);
            curr = curr->next;
            currProcess++;
            continue;
        }
        /*PWD COMMAND*/
        if (!strcmp(curr->args[0], "pwd"))
        {
            returnValues[currProcess] = pwd_command();
            curr = curr->next;
            currProcess++;
            continue;
        }
        if (!strcmp(curr->args[0], "sls"))
        {
            returnValues[currProcess] = sls_command();
            curr = curr->next;
            currProcess++;
            continue;
        }

        if (currProcess == 0)
        { // first command
            if (!(curr->pid = fork()))
            {
                dup2(fds[currProcess][1], STDOUT_FILENO); /* Replace stdout with pipe */
                closeFDS(fds, numCommands);
                execvp(curr->args[0], curr->args);
                exit(1);
            }
        }
        else if (currProcess == numCommands - 1)
        { // last command
            if (!(curr->pid = fork()))
            {
                dup2(fds[currProcess - 1][0], STDIN_FILENO); /* Replace stdin with pipe */
                closeFDS(fds, numCommands);
                execvp(curr->args[0], curr->args); /* Child #2 becomes command2 */
                exit(1);
            }
        }
        else
        {
            if (!(curr->pid = fork()))
            {                                                // in between commands
                dup2(fds[currProcess - 1][0], STDIN_FILENO); /* Replace stdin with previous pipe */
                dup2(fds[currProcess][1], STDOUT_FILENO);    /* Replace stdout with next pipe */
                closeFDS(fds, numCommands);
                execvp(curr->args[0], curr->args); /* Child #2 becomes command2 */
                exit(1);
            }
        }
        curr = curr->next;
        returnValues[currProcess] = 0;
        currProcess++;
    }
    /*close all fds*/
    closeFDS(fds, numCommands);
    /*wait for all children to finish*/
    int currWaiting = 0;
    while (head)
    {
        int status;
        waitpid(head->pid, &status, 0);
        if (!isBuiltIn(head->args[0]) && status)
        {
            returnValues[currWaiting] = 1;
            fprintf(stderr, "Error: command not found\n");
        }
        head = head->next;
        currWaiting++;
    }
}

void freeLinkedList(struct Command_Node *head)
{
    struct Command_Node *current = head;
    struct Command_Node *nextNode;
    while (current != NULL)
    {
        nextNode = current->next;
        free(current->args);
        free(current);
        current = nextNode;
    }
}

bool errorCheck(char **args)
{
    // check for too many args
    int numArgs = 0;
    while (args[numArgs] != NULL)
    {
        numArgs++;
    }
    // printf("%d %s\n", numArgs, args[numArgs-1]);
    if (numArgs > 16)
    {
        fprintf(stderr, "Error: too many process arguments\n");
        return true;
    }
    // check for missing command: first or last arg is a pipe or first arg is a redirect
    if (strcmp(args[0], "|") == 0 || strcmp(args[0], ">") == 0 || strcmp(args[0], ">>") == 0 || strcmp(args[numArgs - 1], "|\0") == 0)
    {
        fprintf(stderr, "Error: missing command\n");
        return true;
    }
    // check for no output file: last arg is a redirect
    if (strcmp(args[numArgs - 1], ">") == 0 || strcmp(args[numArgs - 1], ">>") == 0)
    {
        fprintf(stderr, "Error: no output file\n");
        return true;
    }
    // check for misplaced redirect
    for (int i = 0; i < numArgs; i++)
    {
        if (((!strcmp(args[i], ">") || !strcmp(args[i], ">>")) && i != numArgs - 2))
        {
            fprintf(stderr, "Error: mislocated output redirection\n");
            return true;
        }
    }
    return false;
}

int main(void)
{
    char cmd[CMDLINE_MAX];

    while (1)
    {
        char *nl;

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
            fprintf(stderr, "+ completed '%s' [0]\n", cmd);
            break;
        }
        if (strcmp(cmd, "") == 0)
        {
            continue;
        }
        char **args = custom_parser(cmd);
        if (errorCheck(args))
        {
            free(args);
            continue;
        }
        int saved_out = dup(1);
        if (redirect(args))
        {
            free(args);
            continue;
        }
        struct Command_Node *head = createLinkedList(args);
        int numCommands = getNumCommands(head);
        int retvals[numCommands];

        /* Execute command*/
        custom_system(head, numCommands, retvals);

        /*restore output redirection*/
        dup2(saved_out, 1);
        close(saved_out);

        fprintf(stderr, "+ completed '%s' ", cmd);
        for (int i = 0; i < numCommands; i++)
        {
            fprintf(stderr, "[%d]", retvals[i]);
        }
        fprintf(stderr, "\n");
        free(args);
        freeLinkedList(head);
    }

    return EXIT_SUCCESS;
}