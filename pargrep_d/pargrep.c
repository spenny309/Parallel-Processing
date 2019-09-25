#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void search_in_file(FILE *fp, char* search_term, char* file_name){
  int current_line = 0;
  int buff_size = 2;
  int curr_size;
  char* output = (char*)malloc(sizeof(char) * buff_size);
  output[0] = '\0';
//search file line by line, looking for strstr match
  while(!feof(fp)){
    char* buffer = NULL;
    size_t n = 0;
    ssize_t line_length = getline(&buffer, &n, fp);
    curr_size += line_length;

    if (ferror(fp)){
      perror("error reading");
      exit(1);
    }
    if (strstr(buffer, search_term) != NULL){
      while(curr_size > buff_size){
        buff_size *= 2;
        output = (char*)realloc(output, buff_size);
      }

      strcat(output, file_name);
      strcat(output, ": ");
      strcat(output, buffer);
      strcat(output, "\n");
      /*
      printf("%s: ", file_name);
      fwrite(buffer, 1, line_length, stdout);
      if(line_length > 0 && buffer[line_length - 1] != '\n') {
        printf("\n");
      }*/
    }
    free(buffer);
  }
  printf("%s", output);
  free(output);
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
