#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

void compare(int read_pipe, char* match_me, int return_pipe){
  char current_word[255];
  int return_count = 0;
  //max int bits + null byte
  char return_string[33];

  while(read(read_pipe, current_word, 255) != 0){
    if (strcmp(current_word, match_me) == 0){
      return_count += 1;
    }
  }

  sprintf(return_string, "%d", return_count);
  write(return_pipe, return_string, 33);

  exit(0);
}


void strip_punctuation(int read_pipe, char* match_me, int return_pipe){
  int child2_to_child3[2];
  if(pipe(child2_to_child3) == -1){
    perror("failed to open pipe\n");
    close(read_pipe);
    close(return_pipe);
    exit(1);
  }

  pid_t pid = fork();
  if(pid == -1){
    perror("failed to fork\n");
    close(read_pipe);
    close(return_pipe);
    exit(1);
  }

  if(pid == 0){
    close(child2_to_child3[1]);
    compare(child2_to_child3[0], match_me, return_pipe);
    close(child2_to_child3[0]);
    exit(0);
  } else {
    close(child2_to_child3[0]);
    char current_word[255];

    while(read(read_pipe, current_word, 255) != 0){
      int old_index = 0;
      int new_index = 0;
      while(current_word[old_index] != '\0'){
        if(!ispunct(current_word[old_index])){
          current_word[new_index] = current_word[old_index];
          new_index++;
        }
        old_index++;
      }
      current_word[new_index] = '\0';
      write(child2_to_child3[1], current_word, 255);
    }
    close(child2_to_child3[1]);
    wait(NULL);
    exit(0);
  }
}



void to_lower(int read_pipe, char* match_me, int return_pipe){
  int child1_to_child2[2];
  if(pipe(child1_to_child2) == -1){
    perror("failed to open pipe\n");
    close(read_pipe);
    close(return_pipe);
    exit(1);
  }

  pid_t pid = fork();
  if(pid == -1){
    perror("failed to fork\n");
    close(read_pipe);
    close(return_pipe);
    exit(1);
  }

  if (pid == 0){
    close(child1_to_child2[1]);
    strip_punctuation(child1_to_child2[0], match_me, return_pipe);
    close(child1_to_child2[0]);
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
      write(child1_to_child2[1], current_word, 255);
    }
    close(child1_to_child2[1]);
    wait(NULL);
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
      close(parent_to_child1[0]);
      close(parent_to_child1[1]);
      exit(1);
    }
    pid_t pid = fork();
    if(pid == -1){
      perror("failed to fork\n");
      close(parent_to_child1[0]);
      close(parent_to_child1[1]);
      close(child3_to_parent[0]);
      close(child3_to_parent[1]);
      exit(1);
    }

    if(pid == 0){
      //won't write to pipe
      close(parent_to_child1[1]);
      //will never read this
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

      close(parent_to_child1[0]);
      close(child3_to_parent[1]);

      exit(0);
    } else {
      /* main process sends each word to the pipe */

      //won't read from pipe
      close(parent_to_child1[0]);
      //will never write to this
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
      wait(NULL);

      char output_str[33];
      int output_int = 0;
      read(child3_to_parent[0], output_str, 33);
      output_int = atoi(output_str);
      printf("count: %d\n", output_int);

      close(child3_to_parent[0]);
      fclose(fp);
      exit(0);
    }
  }
}
