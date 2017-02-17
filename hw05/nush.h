#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

void run_command(char* s);
void execute_command(char** args);
void change_directory(char** args);
void redirect(int i, int r, char** args);
