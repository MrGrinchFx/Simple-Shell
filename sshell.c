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
#include <errno.h>

#define CMDLINE_MAX 512
#define CWD_MAX 1024
#define MAX_ARG_SIZE 32
#define MAX_NUM_ARGS 16

struct commandNode
{
    char **args;
    struct commandNode *next;
    pid_t pid;
} commandNode;

void appendNode(struct commandNode **head, char **args)
{
    struct commandNode *newNode = (struct commandNode *)malloc(sizeof(commandNode));
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
    struct commandNode *current = *head;
    while (current->next != NULL)
    {
        current = current->next;
    }
    current->next = newNode;
}

struct commandNode *createLinkedList(char **args)
{
    struct commandNode *head = NULL;
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

int getNumCommands(struct commandNode *head)
{
    int num = 0;
    struct commandNode *curr = head;
    while (curr)
    {
        num++;
        curr = curr->next;
    }
    return num;
}

char **customParser(char *cmd)
{
    char buffer[MAX_ARG_SIZE + 1];        // allocate space for 16 chars and one null character
    char **args = malloc(sizeof(char *)); // allocate an initial memory for a string.
    int pointer = 0;                      // index of cmd
    int count = 0;                        // index of buffer
    int argCount = 0;                     // number of arguments

    while (cmd[pointer] != '\0') // while its not the end of the command line
    {
        if (count != 0 && cmd[pointer] == ' ') // if buffer is not empty and not a white-space
        {
            buffer[count] = '\0';                                  // add a null char to buffer
            args = realloc(args, (argCount + 1) * sizeof(char *)); // create an extra space in the args array
            args[argCount] = malloc(MAX_ARG_SIZE);                 // allocate memory for a string in the array
            strcpy(args[argCount], buffer);                        // copy over buffer content
            argCount++;
            memset(buffer, '\0', sizeof(buffer)); // clear the buffer
            count = 0;                            // reset the size of the buffer
        }
        else if (cmd[pointer] == '|') // if it s pipe argument
        {
            if (count != 0) // check if buffer is empty
            {
                buffer[count] = '\0';                                  // add a null char to buffer
                args = realloc(args, (argCount + 1) * sizeof(char *)); // create an extra space in the args array
                args[argCount] = malloc(MAX_ARG_SIZE);                 // allocate memory for buffer
                strcpy(args[argCount], buffer);                        // copy over content
                argCount++;
                memset(buffer, '\0', sizeof(buffer)); // clear buffer
            }
            args = realloc(args, (argCount + 1) * sizeof(char *)); // allocate memory for an extra index in the args array.
            args[argCount] = malloc(MAX_ARG_SIZE);                 // allocate memory for "|" string
            args[argCount] = "|";                                  // add "|" to array
            argCount++;
            count = 0; // reset the buffer size
        }
        else if (cmd[pointer] == '>' && cmd[pointer + 1] == '>') // check current char and next char for append operator.
        {
            if (count != 0)
            {                                                          // check if buffer is empty
                buffer[count] = '\0';                                  // append a null char to the end of buffer.
                args = realloc(args, (argCount + 1) * sizeof(char *)); // allocate a new index in the buffer.
                args[argCount] = malloc(MAX_ARG_SIZE);                 // allocate memory for buffer
                strcpy(args[argCount], buffer);                        // copy over content
                argCount++;
                memset(buffer, '\0', sizeof(buffer)); // clear buffer
            }
            args = realloc(args, (argCount + 1) * sizeof(char *)); // allocate memory for a new index in args
            args[argCount] = malloc(MAX_ARG_SIZE);                 // allocate memory for >>
            args[argCount] = ">>";                                 // add ">" to array
            argCount++;
            count = 0; // set buffer size back to 0
            pointer++; // move onto next cmd char
        }
        else if (cmd[pointer] == '>') // check if truncate operator
        {
            if (count != 0)
            {                                                          // check if buffer is empty
                buffer[count] = '\0';                                  // append a null char to buffer.
                args = realloc(args, (argCount + 1) * sizeof(char *)); // allocate a new index in args.
                args[argCount] = malloc(MAX_ARG_SIZE);                 // allocate memory for buffer
                strcpy(args[argCount], buffer);                        // copy over content
                argCount++;
                memset(buffer, '\0', sizeof(buffer)); // clear buffer
            }
            args = realloc(args, (argCount + 1) * sizeof(char *)); // allocate new index in args
            args[argCount] = malloc(MAX_ARG_SIZE);                 // allocate memory for >
            args[argCount] = ">";                                  // add ">" to array
            argCount++;
            count = 0; // reset buffer size
        }
        else if (!isspace(cmd[pointer])) // if its not a space, then it's a letter or non-meta char symbol.
        {
            buffer[count] = cmd[pointer]; // set the buffer char with the command line char.
            count++;
        }
        pointer++;
    }
    if (count != 0) // if buffer is not empty
    {
        args = realloc(args, (argCount + 1) * sizeof(char *)); // allocate memory for new index for the last argument
        args[argCount] = malloc(MAX_ARG_SIZE);                 // allocate memory in the index for a string.
        buffer[count] = '\0';                                  // append null char to the buffer.
        strcpy(args[argCount], buffer);                        // copy over the string.
        argCount++;
    }

    args = realloc(args, (argCount + 1) * sizeof(char *)); // allocate new index in args for NULL argument
    args[argCount] = NULL;                                 // set it to Null.
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

int cdCommand(char **args)
{
    int status = chdir(args[1]); // returns 0 if successful.
    if (status != 0)
    {
        fprintf(stderr, "Error: cannot cd into directory\n");
        return 1;
    }
    return 0;
}

int pwdCommand()
{
    char cwd[CWD_MAX];
    char *status = getcwd(cwd, sizeof(cwd)); // returns a status with type char*.
    if (status == NULL)
    {
        perror("pwd");
        return 1;
    }
    printf("%s\n", cwd);
    return 0;
}
int slsCommand()
{
    DIR *directory;
    struct dirent *directoryContent;
    directory = opendir("."); // open current directory
    struct stat statistics;

    while ((directoryContent = readdir(directory)) != NULL)
    {
        if (!strcmp(directoryContent->d_name, ".") || !strcmp(directoryContent->d_name, ".."))
        {
        }
        else
        {
            printf("%s ", directoryContent->d_name);
            if (stat(directoryContent->d_name, &statistics) == 0) // stat returns 0 if successfully retrieved statistics.
            {
                printf("(%ld bytes)\n", statistics.st_size); // in bytes
            }
        }
    }
    closedir(directory); // close the directory after finishing looking at it.
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

void customSystem(struct commandNode *head, int numCommands, int returnValues[])
{
    // create array of pipes;
    int fds[numCommands - 1][2];
    for (int i = 0; i < numCommands - 1; i++)
    {
        pipe(fds[i]);
    }

    struct commandNode *curr = head;
    int currProcess = 0;
    while (curr)
    {
        /*CD COMMAND*/
        if (!strcmp(curr->args[0], "cd"))
        {
            returnValues[currProcess] = cdCommand(curr->args);
            curr = curr->next;
            currProcess++;
            continue;
        }
        /*PWD COMMAND*/
        if (!strcmp(curr->args[0], "pwd"))
        {
            returnValues[currProcess] = pwdCommand();
            curr = curr->next;
            currProcess++;
            continue;
        }
        if (!strcmp(curr->args[0], "sls"))
        {
            returnValues[currProcess] = slsCommand();
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
                if (errno == 2)
                {
                    fprintf(stderr, "Error: command not found\n");
                }
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
                if (errno == 2)
                {
                    fprintf(stderr, "Error: command not found\n");
                }
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
                if (errno == 2)
                {
                    fprintf(stderr, "Error: command not found\n");
                }
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
    int status = 0;
    while (head)
    {
        status = 0;
        waitpid(head->pid, &status, 0);
        if (status)
        {
            returnValues[currWaiting] = WEXITSTATUS(status);
        }
        head = head->next;
        currWaiting++;
    }
}

void freeLinkedList(struct commandNode *head)
{
    struct commandNode *current = head;
    struct commandNode *nextNode;
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
        if (strcmp(cmd, "") == 0) // check if the command line is empty before parsing
        {
            continue;
        }
        char **args = customParser(cmd); // parse the command line
        if (errorCheck(args))            // check for errors in arguments
        {
            free(args);
            continue;
        }
        int saved_out = dup(1); // save the stdout for later restoration.
        if (redirect(args))
        {
            free(args);
            continue;
        }
        struct commandNode *head = createLinkedList(args);
        int numCommands = getNumCommands(head);
        int retvals[numCommands];

        /* Execute command*/
        customSystem(head, numCommands, retvals);

        /*restore output redirection*/
        dup2(saved_out, 1);
        close(saved_out);

        fprintf(stderr, "+ completed '%s' ", cmd);
        for (int i = 0; i < numCommands; i++) // print error code of all commands
        {
            fprintf(stderr, "[%d]", retvals[i]);
        }
        fprintf(stderr, "\n");
        /*Free all allocated memory*/
        free(args);
        freeLinkedList(head);
    }
    return EXIT_SUCCESS;
}
