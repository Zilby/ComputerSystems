#include "nush.h"

int f = 0;

static void sighandler(int signo) {
  // contrl-c or exit command
  if (signo == SIGUSR1 || signo == SIGINT) {
    // ensures new line for ctrl c
    if(signo != SIGUSR1) {
      printf("\n");
    }
    printf("exit\n");
    exit(-1);
  } 
}

int main(int argc, char* argv[]){
  // Can use while loop to make it not end at EOF
  //while(1) {
  signal(SIGINT, sighandler);
  signal(SIGUSR1, sighandler);
  // accepts script file as first command
  if(argc == 2) {
    char* file = argv[1];
    chmod(file, S_IRWXU);
    struct stat buffer;
    system(argv[1]);
  } else {
    run_command(0);
  }
  // This line gives errors, but it looks nice if it says
  // exit when exiting on EOF
  //printf("exit\n");
}
