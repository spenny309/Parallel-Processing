#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void search_in_file(FILE *fp, char* search_term, char* file_name){
  int current_line = 0;
  //printf("entering while\n");
  while(!feof(fp)){
    //printf("in while\n");
    char* buffer = NULL;
    size_t n = 0;
    ssize_t line_length = getline(&buffer, &n, fp);
    //printf("GOT LINE\n");
    if (ferror(fp)){
      //printf("ERROR\n");
      perror("error reading");
      exit(1);
    }
    //printf("LINE: %s\n", buffer);
    //fwrite(buffer, 1, line_length, stdout);
    //printf("\n");
    if (strstr(buffer, search_term) != NULL){
      //printf("found a match\n");
      printf("%s: ", file_name);
      fwrite(buffer, 1, line_length, stdout);
      if(line_length > 0 && buffer[line_length - 1] != '\n') {
	printf("\n");
      }
    }
    free(buffer);
  }
}

int main(int argc, char **argv){
  switch(argc) {
  case 0:
  case 1:
    printf("to use: ./pargrep term file_1 file_2 ...\n");
    exit(1);
  default:
    //printf("entering for loop\n");
    for (int i = 2; i < argc; i++){
      printf("forking\n");
      pid_t pid = fork();
      //printf("i am process: %d\n", pid);
      if (pid == 0){
	FILE *fp = fopen(argv[i], "r");
	if (fp == NULL) {
	  perror("cannot find/open file");
	  exit(1);
	}
	//printf("searching in file: %s for %s\n", argv[i], argv[1]);
	search_in_file(fp, argv[1], argv[i]);
	fclose(fp);
	printf("finishing fork\n");
	exit(0);
      }
    }
  }
  for (int i = 2; i < argc; i++) {
    //printf("waiting\n");
    wait(NULL);
  }
  exit(0);
}
