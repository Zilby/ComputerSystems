#include "nush.h"

// Runs a command
void run_command(char* s) {
  // Gets the command if none given
  if(s==0){
    char ss[256];
    printf("nush$ ");
    fgets(ss, 255, stdin);
    ss[strlen(ss)-1]=0;
    s=ss;
  }
  char *args[256];
  char *s1 = s;
  char *s2;
  char *s3;
  // Seperates different commands by ;
  if (s2 = strsep(&s1, ";")) {
    int i = 0;
    while(s3 = strsep(&s2, " ")) {
      if(strcmp(s3,"") != 0){
	args[i]=s3;
	i++;
      }
    }
    // exits at EOF if no arguments were found
    if(i == 0) {
      return;
    }
    args[i] = 0;
    if(strcmp(args[0],"cd") == 0) {
      change_directory(args);
      run_command(s1);
    } else {
      int f=fork();
      if(f) {
	// deals with & operator, set wait to WNOHANG
	if(strcmp(args[i - 1], "&") == 0) {
	  waitpid(f, &f, WNOHANG);
	  tcsetpgrp(STDIN_FILENO, getpid());
	} else {
	  waitpid(f, &f, 0);
	}
	// runs next command
	run_command(s1);
      }else{
	if(strcmp(args[i - 1], "&") == 0) {
	  setpgid(0, 0);
	  // deletes & argument now that it's been used
	  args[i - 1] = 0;
	}
	execute_command(args);
      }
    }
  }
}

void execute_command(char** args) {
  if(args[0] == 0) {
    printf("Invalid number of arguments given\n"); 
    exit(-1);
  }
  // deals with exit command
  if(strcmp(args[0],"exit") == 0) {
    kill(getppid(), SIGUSR1);
    exit(-1); 
  }
  // deals with special operators
  int i = 0;
  int r = 0;
  while(args[i]) {
    if(strcmp(args[i], ">") == 0) {
      r = 1;
      break;
    } else if(strcmp(args[i], "<") == 0) {
      r = 2;
      break;
    } else if(strcmp(args[i], "|") == 0) {
      r = 3;
      break;
    } else if(strcmp(args[i], "&&") == 0) {
      r = 4;
      break;
    } else if(strcmp(args[i], "||") == 0) {
      r = 5;
      break;
    }
    i++;
  }
  if(r) {
    redirect(i,r,args);
  }
  execvp(args[0],args);
  printf("Command not found: %s\n",args[0]);
  exit(-1);
}

void change_directory(char** args) {
  char path[256];
  getcwd(path, sizeof(path));
  if(args[1] == 0) {
    printf("Invalid number of arguments given\n"); 
    return;
  }
  int n = chdir(args[1]);
  if(n == -1) {
    printf("Directory not found: %s\n",args[1]);
  }
  char path2[256];
  getcwd(path2, sizeof(path2));
}

void redirect(int i, int r, char** args) {
  args[i] = 0;
  char *args2[256];
  i++;
  int j = i;
  // gets args past operator
  while(args[i]){
    args2[i-j] = args[i];
    i++;
  }
  if(args[0] == 0 || j == i) {
    printf("Invalid number of arguments given\n"); 
    exit(-1);
  }
  args2[i-j] = 0;
  int std;
  int fd;
  if(r == 1) {
    int f2 = fork();
    std = dup(STDOUT_FILENO);
    fd = open(args2[0], O_WRONLY|O_CREAT|O_TRUNC, 0644 );
    dup2(fd, STDOUT_FILENO);
    if(f2) {
      waitpid(f2, &f2, 0);
      dup2(std, STDOUT_FILENO);
      close(fd);
      exit(-1);
    } else {
      execvp(args[0], args);
      printf("Command not found: %s\n",args[0]);
      exit(-1);
    }
  }
  else if(r == 2) {
    int f2 = fork();
    std = dup(STDIN_FILENO);
    fd = open(args2[0], O_RDONLY, 0644 );
    dup2(fd, STDIN_FILENO);
    if(f2) {
      waitpid(f2, &f2, 0);
      dup2(std, STDIN_FILENO);
      close(fd);
      exit(-1);
    } else { 
      execvp(args[0], args);
      printf("Command not found: %s\n",args[0]);
      exit(-1);
    }
  }
  else if(r == 3) {
    int pipes[2];
    if(pipe(pipes) == -1) {
      perror("Pipe failed");
      exit(-1);
    }
    int f2 = fork();
    if(f2) {
      waitpid(f2, &f2, 0);
      close(pipes[1]);
      dup2(pipes[0], STDIN_FILENO);
      close(pipes[0]);
      execvp(args2[0], args2);
      printf("Command not found: %s\n",args2[0]);
      exit(-1);
    } else {
      close(pipes[0]);
      dup2(pipes[1], STDOUT_FILENO);
      close(pipes[1]);
      execvp(args[0], args);
      printf("Command not found: %s\n",args[0]);
      exit(-1);
    }
  }
  else if(r == 4) {
    int f2 = fork();
    if(f2) {
      waitpid(f2, &f2, 0);
      if(f2 == 0) {
	execvp(args2[0], args2);
      }
      exit(-1);
    } else { 
      execvp(args[0], args);
      exit(-1);
    }
  }
  else if(r == 5) {
    int f2 = fork();
    if(f2) {
      waitpid(f2, &f2, 0);
      if(f2 != 0) {
	execvp(args2[0], args2);
      }
      exit(-1);
    } else { 
      execvp(args[0], args);
      exit(-1);
    }
  }
}
