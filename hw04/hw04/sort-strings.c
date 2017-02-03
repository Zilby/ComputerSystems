#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char** read_words(int nn) {
  char** ys = (char**)malloc(nn * sizeof(char*)); // sbrk
  char y[80];
  int i = 0;

  while(fgets(y, 80, stdin)) {
    ys[i] = malloc(sizeof(char) * 85);
    strcpy(ys[i], y);
    i++;
  }
  
  return ys;
}

int compare(const void * a, const void * b) {
  return strcmp(*(const char**)a, *(const char**)b);
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    printf("Usage:\n\t./sort-strings [N]\n");
    return 1;
  }

  int words = atol(argv[1]); 
  char** ys = read_words(words);
  qsort(ys, words, sizeof(char*), compare);
  
  for(int i = 0; i < words; i++) {
    printf("%s", ys[i]);
    free(ys[i]);
  }

  free(ys);

  return 0;
}
