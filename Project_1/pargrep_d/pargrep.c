/* A program to print lines containing {search-term} in {files...}
   ./pargrep {search-term} {file_1} {file_2} ...
   Example command-line:

    Input : ./pargrep file test1.txt
    Output:
          test1.txt: file
          test1.txt: wait just kidding im adding morefilestuff here
          test1.txt: a long line with the word file in it
          test1.txt: end of file
          test1.txt: wow so many files!
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

//predefined max size of internal string buffer
#define BUFFER_SIZE 16383

void search_in_file(FILE *fp, char* search_term, char* file_name){
  char* output = (char*)malloc(sizeof(char) * BUFFER_SIZE);
  if(!output){
    perror("failed to malloc internal buffer\n");
    exit(1);
  }
  output[0] = '\0';

  //search file line by line, looking for strstr match
  while(!feof(fp)){
    char* buffer = NULL;
    size_t n = 0;
    getline(&buffer, &n, fp);
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