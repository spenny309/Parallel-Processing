/* A program to recursively sum the numbers 1...n
   ./recurse {n}
   Example command-line:

    Input : ./recurse 10
    Output: sum to n: 55
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

//A buffer size to store itoa
#define INT_STRING (sizeof(int)*8 + 1)

//takes parent read pipe and parent write pipe fds as args
void recurse(int read_from_parent, int write_to_parent){
  //read the current value from parent, convert to int
  char n_as_str[INT_STRING];
  int n_as_num;
  read(read_from_parent, n_as_str, INT_STRING);
  close(read_from_parent);
  n_as_num = atoi(n_as_str);

  //base case: n==1, write 1 to parent
  if(n_as_num == 1){
    write(write_to_parent, n_as_str, INT_STRING);
    close(write_to_parent);
    return;
  } else { //otherwise recursively create child and pass {n-1}
    int this_to_child[2];
    int child_to_this[2];
    if(pipe(this_to_child) == -1){
      perror("failed to open pipe\n");
      exit(1);
    }
    if(pipe(child_to_this) == -1){
      perror("failed to open pipe\n");
      close(this_to_child[0]);
      close(this_to_child[1]);
      exit(1);
    }

    pid_t pid = fork();
    if(pid == -1){
      perror("failed to fork\n");
      close(this_to_child[0]);
      close(this_to_child[1]);
      close(child_to_this[0]);
      close(child_to_this[1]);
      exit(1);
    }

    if(pid == 0){ //pass parent pipes to child recurse call
      close(this_to_child[1]);
      close(child_to_this[0]);
      recurse(this_to_child[0], child_to_this[1]);
      return;
    } else {
      close(this_to_child[0]);
      close(child_to_this[1]);

      //write {n-1} to child
      char decr_n[INT_STRING];
      sprintf(decr_n, "%d", n_as_num-1);
      write(this_to_child[1], decr_n, INT_STRING);
      close(this_to_child[1]);

      //read result of recursive unwind from child
      char sum_as_str[INT_STRING];
      int sum_as_int;
      read(child_to_this[0], sum_as_str, INT_STRING);

      //write result from child + {n} to parent
      sum_as_int = atoi(sum_as_str);
      sum_as_int += n_as_num;
      sprintf(sum_as_str, "%d", sum_as_int);
      write(write_to_parent, sum_as_str, INT_STRING);
      close(write_to_parent);

      return;
    }
  }
}

int main(int argc, char **argv){
  if(argc < 2 || argc > 2){
    perror("Please include only one command-line argument, a positive integer n\n");
    exit(1);
  } else {
    char* input = argv[1];

    //Ensure input is a positive integer
    if(strlen(input) == 1 && input[0] == '0'){
      perror("invalid input. must input positive integer n\n");
      exit(1);
    }
    for (int i = 0; i < strlen(input); i++){
      if (input[i] > '9' || input[i] < '0'){
        perror("invalid input. must input positive integer n\n");
        exit(1);
      }
    }

    int parent_to_recurse[2];
    int recurse_to_parent[2];

    if(pipe(parent_to_recurse) == -1){
      perror("failed to open pipe\n");
      exit(1);
    }
    if(pipe(recurse_to_parent) == -1){
      perror("failed to open pipe\n");
      close(parent_to_recurse[0]);
      close(parent_to_recurse[1]);
      exit(1);
    }

    pid_t pid = fork();
    if(pid == -1){
      perror("failed to fork\n");
      close(parent_to_recurse[0]);
      close(parent_to_recurse[1]);
      close(recurse_to_parent[0]);
      close(recurse_to_parent[1]);
      exit(1);
    }

    //child process will call recurse
    if(pid == 0){
      close(parent_to_recurse[1]);
      close(recurse_to_parent[0]);
      recurse(parent_to_recurse[0], recurse_to_parent[1]);

      exit(0);
    } else { //parent process will write to child, and read result
      close(recurse_to_parent[1]);
      close(parent_to_recurse[0]);

      write(parent_to_recurse[1], input, INT_STRING);
      close(parent_to_recurse[1]);

      char result[INT_STRING];
      int sum;

      read(recurse_to_parent[0], result, INT_STRING);
      close(recurse_to_parent[0]);

      sum = atoi(result);
      printf("sum to n: %d\n", sum);

      exit(0);
    }
  }
}
