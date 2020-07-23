#ifndef COMMANDS_INCLUDED
#define COMMANDS_INCLUDED

#include "process.h"

void catchSIGINT(int signo);
void catchSIGTSTP(int);
int shellForeground(char **);
void shellBackground(char **, BackArr *, int);
void promptUser(BackArr *);
int runCommand(char *, int, BackArr *);
void exitCustom(BackArr *);
void cdCustom(char *);
char *trimWhiteSpace(char *);
void statusCustom(int);
char **parseCommand(char *, int *);
int checkState(BackArr *);
char *getPath();
int redirectOutput(char **, int);
int redirectInput(char **, int);
int backgroundRedirect(int, int);

#endif
