/* A program to count exact occurences of {search-term} in {file}
   ./count {search-term} {file}
   Example command-line:

    Input : ./count file test1.txt
    Output: count: 3
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

//assume words encountered won't exceed MAX_WORD_SIZE
#define MAX_WORD_SIZE 255

//bit size of int (32, 64) + 1 ('\0') for itoa conversion
#define INT_STRING sizeof(int)*8

void compare(int read_pipe, char* match_me, int return_pipe){
  char current_word[MAX_WORD_SIZE];
  int return_count = 0;
  //max int bits + null byte
  char return_string[INT_STRING];

  //strcmp each word with match_me, increment count
  while(read(read_pipe, current_word, MAX_WORD_SIZE) != 0){
    if (strcmp(current_word, match_me) == 0){
      return_count += 1;
    }
  }

  //write count to ancestor
  sprintf(return_string, "%d", return_count);
  write(return_pipe, return_string, INT_STRING);

  return;
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

  if(pid == 0){ //child process passes pipe information to compare
    close(child2_to_child3[1]);
    compare(child2_to_child3[0], match_me, return_pipe);
    close(child2_to_child3[0]);
    return;
  } else { //parent strips punctuation and writes to child
    close(child2_to_child3[0]);
    char current_word[MAX_WORD_SIZE];

    //read word-by-word from to_lower pipe
    while(read(read_pipe, current_word, MAX_WORD_SIZE) != 0){
      int old_index = 0;
      int new_index = 0;
      //for each char of word, remove punctuation in place
      while(current_word[old_index] != '\0'){
        if(!ispunct(current_word[old_index])){
          current_word[new_index] = current_word[old_index];
          new_index++;
        }
        old_index++;
      }
      //terminate new word in place
      current_word[new_index] = '\0';
      write(child2_to_child3[1], current_word, MAX_WORD_SIZE);
    }
    close(child2_to_child3[1]);
    wait(NULL);
    return;
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

  if (pid == 0){ //child process passes pipe information to strip_punctuation
    close(child1_to_child2[1]);
    strip_punctuation(child1_to_child2[0], match_me, return_pipe);
    close(child1_to_child2[0]);
    exit(0);
  } else {  //parent sets tolower and passes to child
    close(child1_to_child2[0]);
    char current_word[MAX_WORD_SIZE];

    //read word-by-word from parent pipe (main)
    while(read(read_pipe, current_word, MAX_WORD_SIZE) != 0){
      int i = 0;
      //for each char of word, set tolower in place
      while(current_word[i] != '\0'){
        current_word[i] = tolower(current_word[i]);
        i++;
      }
      //write tolower{word} to child
      write(child1_to_child2[1], current_word, MAX_WORD_SIZE);
    }
    close(child1_to_child2[1]);
    wait(NULL);
    return;
  }
}


int main(int argc, char** argv){
  //error handling, pipe openings, filel opening
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

    if(pid == 0){ //child processes {search_term}, and calls to_lower
      close(parent_to_child1[1]);
      close(child3_to_parent[0]);

      //strip punctuation and set tolower our search term for later comparison
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
    } else { // main process sends each word to the pipe
      close(parent_to_child1[0]);
      close(child3_to_parent[1]);

      char current_word[MAX_WORD_SIZE];
      char curr;

      while((curr = fgetc(fp)) != EOF) {
        //build each word char-by-char
        int index = 0;
        while(curr != EOF && curr != ' ' && curr != '\n'){
          current_word[index] = curr;
          curr = fgetc(fp);
          index++;
        }
        current_word[index] = '\0';

        //write word to child
        write(parent_to_child1[1], current_word, MAX_WORD_SIZE);
      }

      close(parent_to_child1[1]);
      wait(NULL);

      //get result from compare pipe
      char output_str[INT_STRING];
      int output_int = 0;
      read(child3_to_parent[0], output_str, INT_STRING);
      output_int = atoi(output_str);
      printf("count: %d\n", output_int);

      close(child3_to_parent[0]);
      fclose(fp);
      exit(0);
    }
  }
}
