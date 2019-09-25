#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFER_SIZE 16383

void search_in_file(FILE *fp, char* search_term, char* file_name){
  int current_line = 0;
  char* output = (char*)malloc(sizeof(char) * BUFFER_SIZE);
  output[0] = '\0';

  //search file line by line, looking for strstr match
  while(!feof(fp)){
    char* buffer = NULL;
    size_t n = 0;
    ssize_t line_length = getline(&buffer, &n, fp);
    if (ferror(fp)){
      perror("error reading");
      exit(1);
    }
    //add each line with a match to our internal buffer
    if (strstr(buffer, search_term) != NULL){
      strcat(output, file_name);
      strcat(output, ": ");
      strcat(output, buffer);
    }
  }

  //print internal buffer and return
  printf("%s", output);
  free(output);
  return;
}

int main(int argc, char **argv){
  switch(argc) {
    case 0:
    case 1:
    printf("to use: ./pargrep term file_1 file_2 ...\n");
    exit(1);
    default:
    //fork once per file specified in argument
    for (int i = 2; i < argc; i++){
      pid_t pid = fork();
      if (pid == 0){
        //each child process will search its respective file
        FILE *fp = fopen(argv[i], "r");
        if (fp == NULL) {
          perror("cannot find/open file");
          exit(1);
        }
        search_in_file(fp, argv[1], argv[i]);
        fclose(fp);
        exit(0);
      }
    }
  }
  //wait for each child
  for (int i = 2; i < argc; i++) {
    wait(NULL);
  }
  exit(0);
}
