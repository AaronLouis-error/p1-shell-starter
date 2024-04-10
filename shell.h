#ifndef SHELL_H
#define SHELL_H

#include <assert.h>  // assert
#include <fcntl.h>   // O_RDWR, O_CREAT
#include <stdbool.h> // bool
#include <stdio.h>   // printf, getline
#include <stdlib.h>  // calloc
#include <string.h>  // strcmp
#include <unistd.h>  // execvp

#define MAXLINE 80
#define PROMPT "osh> "

#define RD 0
#define WR 1

int main();

int child(char **args);
void doCommand(char **args, int start, int end, bool waitfor);
int doPipe(char **args, int pipei, int start);
bool parse(char **args, int start, int *end);
char **tokenize(char *line);
bool processLine(char *line);
int interactiveShell();
int runTests();
bool equal(char *a, char *b);
int fetchline(char **line);
void asciiArt();

const int MAX_COMMAND_LENGTH = 100;
const int MAX_ARGS = 10;
const int MAX_ARG_LENGTH = 100;
const int BUF_SIZE = 1024;

#endif
