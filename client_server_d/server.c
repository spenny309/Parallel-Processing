#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <time.h>

#define FILE_PRE "server_to_client_"
#define PATH_SIZE (strlen("server_to_client_") + 1)
#define PID_STRLEN (sizeof(pid_t)*8 + 1)
#define MAX_RESULT_LENGTH 255

extern int errno;

int count(int file_descriptor){
  printf("trying to count\n");
  int result = 1;
  char current_char[1];

  read(file_descriptor, current_char, 1);
  printf("char: %c\n", current_char[0]);
  read(file_descriptor, current_char, 1);
  printf("char: %c\n", current_char[0]);
  
  while(current_char[0] != EOF){
    if(current_char[0] == '\n'){
      result += 1;
    }
    read(file_descriptor, current_char, 1);
    printf("char: %c", current_char[0]);
  }
  printf("word count:  %d\n", result);
  return result;
}

int open_word_files(int file_array[], int file_word_counts[]){
  int valid_open;
  printf("begin open\n");
  
  valid_open = open("verb.txt", O_RDONLY);
  printf("opened verb: %d\n", valid_open);
  if(valid_open == -1){
    perror("failed to open verb.txt\n");
    return 1;
  }else{
    printf("verb.txt ");
    file_array[0] = valid_open;
    file_word_counts[0] = count(valid_open);
  }
  printf("finished verb count\n\n");

  valid_open = open("preposition.txt", O_RDONLY);
  printf("opened prep: %d\n", valid_open);
  if(valid_open == -1){
    perror("failed to open preposition.txt\n");
    return 1;
  }else{
    printf("preposition.txt ");
    file_array[1] = valid_open;
    file_word_counts[1] = count(valid_open);
  }
  printf("finished prep count\n\n");
  
  valid_open = open("adjective.txt", O_RDONLY);
  printf("opened adj:  %d\n", valid_open);
  if(valid_open == -1){
    perror("failed to open adjective.txt\n");
    return 1;
  }else{
    printf("adjective.txt ");
    file_array[2] = valid_open;
    file_word_counts[2] = count(valid_open);
  }
  printf("finished adj  count\n\n");

  valid_open = open("noun.txt", O_RDONLY);
  printf("opened noun: %d\n", valid_open);
  if(valid_open == -1){
    perror("failed to open noun.txt\n");
    return 1;
  }else{
    printf("noun.txt ");
    file_array[3] = valid_open;
    file_word_counts[3] = count(valid_open);
  }
  printf("finished noun count\n\n");

  return 0;
}

int main(int argc, char **argv){
  printf("start\n");
  int word_files[4];
  int word_counts[4];
  if (open_word_files(word_files, word_counts) != 0){
    perror("could not open word files\n");
    exit(1);
  }

  if(argc != 1){
    perror("invalid input. to run: ./server");
    exit(1);
  }
  int fifo_check = mkfifo("client_to_server_fifo", S_IRWXU);
  if (fifo_check == -1 && errno != EEXIST){
    perror("failed to create new fifo");
    exit(1);
  }
  int client_to_server = open("client_to_server_fifo", O_RDONLY);
  if(client_to_server == -1){
    perror("could not open or create fifo\n");
    exit(1);
  }

  close(client_to_server);

  

  return 0;
}
/*
  char result[MAX_RESULT_LENGTH];
  char child_ID_str[PID_STRLEN];
  srand(time(0));

  while(1){
    if (read(client_to_server, result, MAX_RESULT_LENGTH) == 0){
      break;
    } else if (read(client_to_server, child_ID_str, PID_STRLEN) == 0){
      break;
    }

    //fifo name: server_to_client_{PROCESS_ID}
    char server_to_client_ID[PID_STRLEN + PATH_SIZE];
    strcat(server_to_client_ID, FILE_PRE);
    strcat(server_to_client_ID, child_ID_str);

    fifo_check = mkfifo(server_to_client_ID, S_IRWXU);
    if (fifo_check == -1 && errno != EEXIST){
      perror("failed to create new fifo");
      exit(1);
    }
    int server_to_client = open(server_to_client_ID, O_WRONLY);
    if(fd == -1){
      perror("could not open or create fifo\n");
      exit(1);
    }

    int num = rand() % //# of words in file;

    //RNG
    //build result
    //write result to s_t_c fifo


  }
  close(client_to_server);



  char result[MAX_RESULT_LENGTH];
  read(server_to_client, result, MAX_RESULT_LENGTH);
  printf("%s", result);

  close(server_to_client);
  exit(0);
} */
