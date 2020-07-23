#define _POSIX_C_SOURCE 200809L
#define MAX_ARG 512
#define MAX_LEN 2048

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include "commands.h"
#include "process.h"

int main(int argc, char **argv)
{
    // singal handler for SIGINT
    struct sigaction SIGINT_action = {{0}};
    SIGINT_action.sa_handler = catchSIGINT;
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = SA_RESTART;
    sigaction(SIGINT, &SIGINT_action, NULL);

    // signal handler for SIGTSTP
    struct sigaction SIGTSTP_action = {{0}};
    SIGTSTP_action.sa_handler = catchSIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    int status = 0;
    char *input = NULL;
    size_t inputSize = MAX_LEN;

    BackArr *v;
    v = newBackArr(4); // create a new back array

    // start up the shell and continue forever, until
    // someone terminates it with the exit command
    do
    {
        promptUser(v);
        getline(&input, &inputSize, stdin);

        // do nothing when first char is # or blank
        if (input[0] != '#' && input[0] != '\n')
        {
            status = runCommand(input, status, v);
        }
    } while (1);

    return 0;
}
