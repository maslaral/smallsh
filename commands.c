#define _POSIX_C_SOURCE 200809L
#define _POSIX_SOURCE

#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "commands.h"

int allowBackground = 1;

struct BackArr
{
    pid_t *process;
    int size;
    int capacity;
};

/*************************************************
Function: catchSIGINT()
Description: catches the SIGINT signal and prints
out to the signal that terminated the running 
process
*************************************************/
void catchSIGINT(int signo)
{
    printf("\nterminated by signal %d\n", signo);
    fflush(stdout);
}

/*************************************************
Function: catchSIGTSTP()
Description: catches the SIGSTP signal and prints
the description, and flips the allow background
flag
*************************************************/
void catchSIGTSTP(int signo)
{
    if (allowBackground)
    {
        char *message = "\nEntering foreground-only mode (& is now ignored)\n: ";
        write(STDOUT_FILENO, message, 52);
        allowBackground = 0; // no background process can be run
    }
    else
    {
        char *message = "\nExiting foreground-only mode\n: ";
        write(STDOUT_FILENO, message, 32);
        allowBackground = 1; // background processes can be run
    }

    fflush(stdout);
}

/*************************************************
Function: shellForeground()
Description: immediately runs non-built-in
commands in the foreground of the shell
*************************************************/
int shellForeground(char **args)
{
    pid_t childPid;
    int childStatus;

    switch (childPid = fork())
    {
    case -1: // handle error creating fork
        perror("smallsh");
        fflush(stdout);
        exit(1);
        break;
    case 0: // handle error with command
        if (execv(args[0], args) == -1)
        {
            perror("smallsh");
            fflush(stdout);
            exit(1);
        };
        break;
    default: // if no errors, fork process
        do
        {
            childPid = waitpid(childPid, &childStatus, WUNTRACED);
        } while (!WIFEXITED(childStatus) && !WIFSIGNALED(childStatus));
    }

    return childStatus; // return child status for output
};

/*************************************************
Function: shellBackground()
Description: runs non-built-in commands in the
background of the shell, adds the PIDs to an array,
and redirects output to the standard output
*************************************************/
void shellBackground(char **args, BackArr *v, int stdOut)
{
    pid_t childPid;

    childPid = fork();

    if (childPid == -1) // handle error creating fork
    {
        perror("smallsh");
        fflush(stdout);
        exit(1);
    }
    else if (childPid == 0)
    {
        if (execvp(args[0], args) == -1) // handle error with command
        {
            perror("smallsh");
            fflush(stdout);
        }
    }

    addBackArr(v, childPid); // add pid to array to keep track
    dup2(stdOut, 1);         // reset standard output so can print the background pid
    printf("background pid is %d\n", childPid);
    fflush(stdout);
}

/*************************************************
Function: promptUser()
Description: simple function that first calls
the checkState function, and then prints the prompt
indicator ":"
*************************************************/
void promptUser(BackArr *v)
{
    if (checkState(v))
    {
        printf(": ");
        fflush(stdout);
    }
}

/*************************************************
Function: checkState()
Description: checks the state of each PID in the
background array to determine if they are complete,
if so then prints out a status message prior to 
the next prompt
*************************************************/
int checkState(BackArr *v)
{
    int childStatus;

    // loops through each pid in the background array
    for (int i = 0; i < v->size; i++)
    {
        // if the process is completed, print the message and exit value
        if (waitpid(v->process[i], &childStatus, WNOHANG))
        {
            if (WIFEXITED(childStatus)) // exited
            {
                printf("background pid %d is done: exit value %d\n", v->process[i], WEXITSTATUS(childStatus));
            }
            else // terminated
            {
                printf("background pid %d is done: terminated by signal %d\n", v->process[i], WTERMSIG(childStatus));
            }
            fflush(stdout);
            removeBackArr(v, v->process[i]); // remove from process array
        }
    }
    return 1;
}

/*************************************************
Function: runCommand()
Description: this function coordinates the running
of commands, including deciding if a built in 
command, checking if background or foreground, and
replacing the instances of $$ with the pid 
*************************************************/
int runCommand(char *c, int prevStatus, BackArr *v)
{
    char **args;
    char **temp;
    int status = prevStatus;
    int stdoutCopy = dup(STDOUT_FILENO);
    int stdinCopy = dup(STDIN_FILENO);
    c = trimWhiteSpace(c);
    int outRedirect = 0; // flag that output has been redirected
    int inRedirect = 0;  // flag that input has been redirected

    // first checks if the command is a built in one
    if (!strcmp(c, "exit"))
    {
        exitCustom(v);
    }
    else if (strstr(c, "cd") != NULL)
    {
        cdCustom(c);
    }
    else if (strstr(c, "status") != NULL)
    {
        statusCustom(prevStatus);
    }
    else // not a built in command
    {
        int *numptr = malloc(sizeof(int *));
        args = parseCommand(c, numptr); // parsing the command
        int numArgs = *numptr;          // save the number of arguments

        for (int i = 0; i < numArgs; i++)
        {
            // if identify an output redirection operator
            if (*args[i] == '>')
            {
                // call function to redirect the output
                if (!redirectOutput(args, i))
                {
                    // now remove that redirect from the command
                    args[i] = args[i + 2];
                    i += 2;
                    numArgs -= 2;

                    // set flag that the output has been redirected
                    outRedirect = 1;
                }
            }
        }

        for (int i = 0; i < numArgs; i++)
        {
            // if identify an input redirection operator
            if (*args[i] == '<')
            {
                // call function to redirect the input
                if (!redirectInput(args, i))
                {
                    // now remove that redirect from the command
                    args[i] = args[i + 2];
                    i += 2;
                    numArgs -= 2;

                    // set flag that the input has been redirected
                    inRedirect = 1;
                }
            }
        }

        for (int i = 0; i < numArgs; i++)
        {
            // check if the argument contains $$
            if (strstr(args[i], "$$") != NULL)
            {
                // save the pid to a variable pidStr
                char pidStr[6];
                sprintf(pidStr, "%d", getpid());

                // replace the $$ value with the pidStr variable
                for (int j = 0; args[i][j]; j++)
                {
                    if (args[i][j] == '$' && args[i][j + 1] == '$')
                    {
                        args[i][j] = '\0';
                        strcat(args[i], pidStr);
                    }
                }
            }
        }

        // allocate a temporary string
        temp = malloc(numArgs * sizeof(char **));

        // if the last argument is &, then run background
        if (*args[numArgs - 1] == '&')
        {
            // remove the & from the command
            for (int j = 0; j < numArgs - 1; j++)
            {
                temp[j] = args[j];
            }

            if (allowBackground)
            {
                // if the background command hasn't been redirected
                // backgroundRedirect() will redirect to /dev/null
                backgroundRedirect(outRedirect, inRedirect);
                // run the command in the background
                shellBackground(temp, v, stdoutCopy);
            }
            else
            {
                // if background is not allowed, then just run it
                // in the foreground
                checkState(v);
                status = shellForeground(temp);
            }
        }

        // otherwise, run foreground
        else
        {
            checkState(v);
            status = shellForeground(args);
        }
    }

    // redirect back to the standard input and output
    dup2(stdoutCopy, 1);
    dup2(stdinCopy, 0);

    return status;
}

/*************************************************
Function: trimWhiteSpace()
Description: removes the white space at the beginning
and end of a command from the user
*************************************************/
char *trimWhiteSpace(char *c)
{
    char *end;

    while (isspace((unsigned char)*c))
        c++;

    if (*c == 0)
    {
        return c;
    }

    end = c + strlen(c) - 1;
    while (end > c && isspace((unsigned char)*end))
        end--;

    end[1] = '\0';

    return c;
}

/*************************************************
Function: exitCustom()
Description: first deletes all the background
processes, and then exits the shell
*************************************************/
void exitCustom(BackArr *v)
{
    // killing all background process in array
    for (int i = 0; i < v->size; i++)
    {
        kill(v->process[i], SIGKILL);
    }

    // freeing memory of array
    deleteBackArr(v);

    // killing the entire shell
    kill(0, SIGKILL);
    exit(0);
}

/*************************************************
Function: cdCustom()
Description: built-in cd function that implements
the cd functionality
*************************************************/
void cdCustom(char *c)
{
    int length = strlen(c);
    char dir[length - 3];

    // if no argument then change to home
    if (!strcmp(c, "cd"))
    {
        chdir(getenv("HOME"));
    }
    else
    {
        // get the directory from the command
        for (int i = 3; i <= strlen(c); i++)
        {
            dir[i - 3] = c[i];
        }

        if (strstr(dir, "$$") != NULL)
        {
            char pidStr[6];
            sprintf(pidStr, "%d", getpid());

            for (int i = 0; i < strlen(dir); i++)
            {
                if (dir[i] == '$' && dir[i + 1] == '$')
                {
                    dir[i] = '\0';
                    strcat(dir, pidStr);
                }
            }
        }

        // if directory doesn't exist display explanation
        if (chdir(dir) == -1)
        {
            printf("cd: no such file or directory: %s\n", dir);
            fflush(stdout);
        }
    }
}

/*************************************************
Function: statusCustom()
Description: built-in status function that implements
the status functionality
*************************************************/
void statusCustom(int status)
{
    if (WIFEXITED(status)) // if exited
    {
        printf("exit value %d\n", WEXITSTATUS(status));
    }
    else // if terminated via signal
    {
        printf("terminated by signal %d\n", WTERMSIG(status));
    }
    fflush(stdout);
}

/*************************************************
Function: parseCommand()
Description: parses the command line arguments in
order to properly run the commands
*************************************************/
char **parseCommand(char *c, int *num)
{
    int buf = 64;
    int pos = 0;

    char **tokens = malloc(buf * sizeof(char *));
    char *token;
    char *path = getPath();

    token = strtok(c, " ");

    // assigning token to the tokens array, reallocating
    // if greater than the buffer
    while (token != NULL)
    {
        tokens[pos] = token;
        pos++;

        if (pos >= buf)
        {
            buf *= 2;
            tokens = realloc(tokens, buf * sizeof(char *));
        }

        token = strtok(NULL, " ");
    }

    // setting last to null
    tokens[pos] = NULL;
    *num = pos;

    // if doesn't contain the path, then add that to the
    // command
    if (strstr(tokens[0], path) == NULL)
    {
        char *addPrefix = malloc(strlen(tokens[0]) + strlen(path) + 1);
        strcpy(addPrefix, path);
        strcat(addPrefix, tokens[0]);
        tokens[0] = addPrefix;
    };

    return tokens;
};

/*************************************************
Function: getPath()
Description: function that gets the value from 
the path envrionment variable
*************************************************/
char *getPath()
{
    const char *path = getenv("PATH");
    char *temp;
    int i = 0;

    while (path[i] != ':')
    {
        i++;
    }

    temp = malloc(i * sizeof(char *));

    // copying until it gets to the first
    // ":" value
    for (int j = 0; j < i; j++)
    {
        temp[j] = path[j];
    }

    temp[i] = '/';
    return temp;
}

/*************************************************
Function: redirectOutput()
Description: redirects the output to file passed
as an argument
*************************************************/
int redirectOutput(char **args, int index)
{
    int file;

    // opening the file
    file = open(args[index + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file == -1) // handles file open error
    {
        printf("%s: no such file or directory\n", args[index + 1]);
        return 1;
    }

    // sets the new stdout
    int result = dup2(file, 1);
    if (result == -1) // handles error with the new stdout
    {
        perror("dup2");
        fflush(stdout);
        return 1;
    }

    return 0;
}

/*************************************************
Function: redirectInput()
Description: redirects teh input to file passed
as an argument
*************************************************/
int redirectInput(char **args, int index)
{
    int file;

    // opening the file
    file = open(args[index + 1], O_RDONLY);
    if (file == -1) // handles file open error
    {
        printf("cannot open %s for input\n", args[index + 1]);
        return 1;
    }

    // sets the new stdin
    int result = dup2(file, 0);
    if (result == -1) // handles error with the new stdin
    {
        perror("dup2");
        fflush(stdout);
        return 1;
    }

    return 0;
}

/*************************************************
Function: backgroundRedirect()
Description: function redirects background processes
to the /dev/nukll output according to specification -
the function is passed two flags, if a redirect
has already happened, then don't need to go to
/dev/null
*************************************************/
int backgroundRedirect(int out, int in)
{
    int file;

    // has the arg already been redirected?
    if (out == 0)
    {
        file = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (file == -1)
        {
            perror("open()");
            fflush(stdout);
            return 1;
        }

        int result = dup2(file, 1);
        if (result == -1)
        {
            perror("dup2");
            fflush(stdout);
            return 1;
        }
    }

    // has the arg already been redirected?
    if (in == 0)
    {
        file = open("/dev/null", O_RDONLY);
        if (file == -1)
        {
            perror("open()");
            fflush(stdout);
            return 1;
        }

        int result = dup2(file, 0);
        if (result == -1)
        {
            perror("dup2");
            fflush(stdout);
            return 1;
        }
    }
    return 0;
}
