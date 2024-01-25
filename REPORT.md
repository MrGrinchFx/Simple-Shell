#sshell (Simple Shell)

##SUMMARY
The idea of the program is to mimic an actual shell such as git bash that
utilizes the execvp() function in order to make syscalls to the operating
system as well as supporting common shell commands that are built-in.
Depending on the users-commands, the program will execute operations such as
redirection and piping (The main components).

##Implementation
There are several steps of the program that need to be viewed on a higher-level.

1. Be able to parse the command line into distinct arguments.
2. Take the arguments and check the first index for the command (i.e cd, pwd,
   sls, exit, echo, etc.)
3. Take the arguments and create nodes for each command within the command line.

## Parsing

The custom_parser() function is the first function that the command line is
passed into in order to return an array of strings (return type: char\*\*).
In order to determine when the end of the argument has been reached, it is not
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

## cd

The cd command is a one-argument that will take in the route destination with in
the file-system that the user wants to venture into. This is done by using the
function chdir() (change directory) function. If for whatever reason the cd
command does not work, then we will print an error into stderr.

## pwd

The command is a stand-alone command that uses the getcwd() function in order to
print the current working directory of the user. It will return a status code
depending on if there was any error within the function.

## sls

The
