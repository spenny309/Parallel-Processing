#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
/*
int compare(int read_pipe, char* match_me, int return_pipe){
  exit(0);
}


int strip_puntuation(int read_pipe, char* match_me, int return_pipe){
  int child2_to_child3[2];
  if(pipe(child2_to_child3) == -1){
    perror("failed to open pipe\n");
    exit(1);
  }

  pid_t pid = fork();
  if(pid == -1){
    perror("failed to fork\n");
    exit(1);
  }

  if(pid == 0){
    close(child2_to_child3[1]);
    close(read_pipe);
    compare(child2_to_child3[0], match_me);
    exit(0);
  } else {
    close(child2_to_child3[0]);
    char* current_word
    current_word = (char*)malloc(255 * sizeof(char));

    //read from read_pipe
    //remove punct as in main
    //write to child2_to_child3

    close(read_pipe);
    close(child2_to_child3[1]);
    free(current_word);
    exit(0);
  }
}
*/


int to_lower(int read_pipe, char* match_me, int return_pipe){
  int child1_to_child2[2];
  if(pipe(child1_to_child2) == -1){
    perror("failed to open pipe\n");
    exit(1);
  }

  pid_t pid = fork();
  if(pid == -1){
    perror("failed to fork\n");
    exit(1);
  }

  if (pid == 0){
    close(child1_to_child2[1]);
    close(read_pipe);
    //strip_punctuation(child1_to_child2[0], match_me);
    exit(0);
  } else {
    close(child1_to_child2[0]);
    char current_word[255];
    while(read(read_pipe, current_word, 255) != 0){
      int i = 0;
      while(current_word[i] != '\0'){
        current_word[i] = tolower(current_word[i]);
        i++;
      }
      write(child1_to_child2[1], current_word, i);
      printf("word: %s\n", current_word);
    }
    close(read_pipe);
    close(child1_to_child2[1]);
    exit(0);
  }
}


int main(int argc, char** argv){
  if(argc < 3 || argc > 3){
    printf("to run: ./count [term] [file]\n");
    exit(1);
  } else {
    FILE *fp = fopen(argv[2], "r");
    if (fp == NULL) {
      perror("cannot find/open file\n");
      exit(1);
    }
    int parent_to_child1[2];
    if(pipe(parent_to_child1) == -1){
      perror("failed to open pipe\n");
      exit(1);
    }
    int child3_to_parent[2];
    if(pipe(child3_to_parent) == -1){
      perror("failed to open pipe\n");
      exit(1);
    }
    pid_t pid = fork();
    if(pid == -1){
      perror("failed to fork\n");
      exit(1);
    }

    int result = 0;

    if(pid == 0){
      //won't write to pipe
      close(parent_to_child1[1]);
      close(child3_to_parent[0]);

      //strip punctuation and set tolower our search term
      char* search_term = argv[1];
      int new_index = 0;
      for(int old_index = 0; old_index < strlen(search_term); old_index++){
        if(!ispunct(search_term[old_index])) {
          search_term[new_index] = tolower(search_term[old_index]);
          new_index++;
        }
      }
      search_term[new_index] = '\0';

      to_lower(parent_to_child1[0], search_term, child3_to_parent[1]);
      exit(0);
    } else {
      /* main process sends each word to the pipe */
      //won't read from pipe
      close(parent_to_child1[0]);
      close(child3_to_parent[1]);
      char current_word[255];
      char curr;

      while((curr = fgetc(fp)) != EOF) {
        int index = 0;
        while(curr != EOF && curr != ' ' && curr != '\n'){
          current_word[index] = curr;
          curr = fgetc(fp);
          index++;
        }
        current_word[index] = '\0';
        write(parent_to_child1[1], current_word, 255);
      }
      //cleanup
      close(parent_to_child1[1]);
      close(child3_to_parent[0]);
      fclose(fp);
    }
  }
}
