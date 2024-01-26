# sshell (Simple Shell)

## Summary

The idea of the program is to mimic an actual shell such as git bash that
utilizes the execvp() function in order to make syscalls to the operating
system as well as supporting common shell commands that are built-in.
Depending on the users-commands, the program will execute operations such as
redirection and piping (The main components).

## Implementation

There are several steps of the program that need to be viewed on a higher-level.

1. Be able to parse the command line into distinct arguments.
2. Parse each set of arguments into individual commands.
3. Pipe commands if needed.
4. Redirect the final output if needed.

## Parsing

The custom_parser() function is the first function that the command line is
passed into in order to return an array of strings (return type: char\*\*). In
order to determine when the end of the argument has been reached, it is not
enough to just separate by whitespaces, so each character of the command line
must be visited. The function will parse through the command until it either
reaches a space, >, >>, or | character. (>> is handled by looking at current
character and the character right after.) If any of the meta characters that are
to be included in the array of arguments such as the redirection operator must
have its previous command and arguments. This is done by using a buffer array
that we'll use to record what non-space characters we have encountered thus far,
and whenever we reach any of the "termination" characters, we will take whatever
is in the buffer array and copy it over to the arguments array that we passed
into the function. We also have to include the "termination" character as well,
so we have to also append it to the array of arguments. Once we reach the null
character of the command line, we will break out of the while loop and have to
append any argument in the buffer array, if there is any, and append a NULL to
the array as required by execvp().

## Redirection

Redirection is implemented by checking the last two arguments after the command
line has been parsed. If the second to last argument is a redirect character,
the function assumes that the next argument is the file name, so it opens the
output file and duplicates its file descriptor into stdout.

## cd

The cd command is a one-argument that will take in the route destination with in
the file-system that the user wants to venture into. This is done by using the
function chdir() (change directory) function. If for whatever reason the cd
command does not work, then we will print an error into stderr.

## pwd

The command is a stand-alone command that uses the getcwd() function in order to
print the current working directory of the user. It will return a status code
depending on if there was any error within the function. When using the cd in
conjunction with this command, the current directory of the user is correctly
outputted.

## sls

The sls is a simple stand-alone command that uses the readdir() command in c in
order to read the content of the user's current directory and the files and
folders within it. In order to print the amount of data stored in each file, the
stats library must be included. We traverse the directory looking at each file
making sure that if there are any files that begin with "." or "..", that they
are not to be printed out, since these files are meant to be kept hidden. We
print non-hidden files and also print the size which is return from the stat()
function.

## Piping

In order to implement piping, the parsed commands needed to be put into a data
structure that allowed for separation of individual commands. A linked list was
used for this. Each node in the list contains an array of strings for the
command and its flags as well as a pointer to the next command node and the pid
of the current process. To begin, an array of n-1 pipes was created, where n was
the number of commands. Then, the linked list is iterated through. If the
current command is the first command, the process forks, and the file
descriptors are changed such that the command reads from stdin, and writes to
the first pipe. If the current command is the last command, the process forks,
and the file descriptors are changed such that the command reads from the final
pipe, and outputs to stdout. For any other command, the process forks, and the
file descriptors are changed such that the command reads from the previous pipe,
and writes to the next pipe. After all commands are successfully executed, the
parent process closes all opened file descriptors and then waits for all status
values, setting them in the final status array.

## Error Checking

All parsing errors were done after the custom parser is run, but before the
commands are placed into the linked list data structure. If the error checker
detects an error, all memory is freed and the shell continues to ask for the
next prompt. 

For launching errors, they were checked during the command execution steps. In
the event that a file could not be opened, the cd command will set the status
value of the process to 1, and break. If a command doesn't exist for when execvp
is run, its status value will be collected during the process waiting phase and
then sent to stderr.
