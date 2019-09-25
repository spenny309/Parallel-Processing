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

int find_word(char word_buffer[], int file, int word_in_file){
  char temp_buffer[MAX_RESULT_LENGTH];
  int words_encountered = 0;

  while(words_encountered < word_in_file){
    int j = read(file, temp_buffer, 1);
    if (j == 0){
      perror("reached EOF without reaching word_in_file\n");
      return 1;
    }
    for (int i = 0; i < j; i++){
      if(temp_buffer[i] == '\n'){
        words_encountered += 1;
      }
    }
  }

  int final_read = read(file, temp_buffer, MAX_RESULT_LENGTH);
  for(int i = 0; i < final_read; i++){
    if(temp_buffer[i] == '\n'){
      temp_buffer[i] = '\0';
      break;
    }
  }

  strcpy(word_buffer, temp_buffer);
  lseek(file, 0, SEEK_SET);
  return 0;
}

int count(int file_descriptor){
  int result = 1;
  char current_chars[MAX_RESULT_LENGTH];
  int j;

  while(j = read(file_descriptor, current_chars, MAX_RESULT_LENGTH)){
    for(int i = 0; i < j; i++) {
      if(current_chars[i] == '\n'){
        result += 1;
      }
    }
  }
  lseek(file_descriptor, 0, SEEK_SET);
  return result;
}

int open_word_files(int file_array[], int file_word_counts[]){
  int valid_open;

  valid_open = open("verb.txt", O_RDONLY);
  if(valid_open == -1){
    perror("failed to open verb.txt\n");
    return 1;
  }else{
    file_array[0] = valid_open;
    file_word_counts[0] = count(valid_open);
  }

  valid_open = open("preposition.txt", O_RDONLY);
  if(valid_open == -1){
    perror("failed to open preposition.txt\n");
    return 1;
  }else{
    file_array[1] = valid_open;
    file_word_counts[1] = count(valid_open);
  }

  valid_open = open("adjective.txt", O_RDONLY);
  if(valid_open == -1){
    perror("failed to open adjective.txt\n");
    return 1;
  }else{
    file_array[2] = valid_open;
    file_word_counts[2] = count(valid_open);
  }

  valid_open = open("noun.txt", O_RDONLY);
  if(valid_open == -1){
    perror("failed to open noun.txt\n");
    return 1;
  }else{
    file_array[3] = valid_open;
    file_word_counts[3] = count(valid_open);
  }

  return 0;
}

int main(int argc, char **argv){
  int spot = 0;
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

  int word_files[4];
  int word_counts[4];
  if (open_word_files(word_files, word_counts) != 0){
    perror("could not open word files\n");
    exit(1);
  }

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
    server_to_client_ID[0] = '\0';
    strcat(server_to_client_ID, FILE_PRE);
    strcat(server_to_client_ID, child_ID_str);

    fifo_check = mkfifo(server_to_client_ID, S_IRWXU);
    if (fifo_check == -1 && errno != EEXIST){
      perror("failed to create new fifo");
      exit(1);
    }
    int server_to_client = open(server_to_client_ID, O_WRONLY);
    if(server_to_client == -1){
      perror("could not open or create fifo\n");
      exit(1);
    }

    char output_words[4][MAX_RESULT_LENGTH];

    int num_verb = rand() % word_counts[0]; //# of words in file;
    int num_prep = rand() % word_counts[1];
    int num_adjc = rand() % word_counts[2];
    int num_noun = rand() % word_counts[3];

    find_word(output_words[0], word_files[0], num_verb);
    find_word(output_words[1], word_files[1], num_prep);
    find_word(output_words[2], word_files[2], num_adjc);
    find_word(output_words[3], word_files[3], num_noun);

    strcat(result, " ");
    strcat(result, output_words[0]);
    strcat(result, " ");
    strcat(result, output_words[1]);
    strcat(result, " the ");
    strcat(result, output_words[2]);
    strcat(result, " ");
    strcat(result, output_words[3]);
    strcat(result, "!");

    write(server_to_client, result, MAX_RESULT_LENGTH);
    close(server_to_client);

    sleep(3);
  }

  close(client_to_server);

  exit(0);
}
