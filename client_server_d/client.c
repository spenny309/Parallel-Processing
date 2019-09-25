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

int main(int argc, char **argv){
  if(argc != 2){
    perror("invalid input. to run: ./client {proper noun}");
    exit(1);
  }
  int fifo_check = mkfifo("client_to_server_fifo", S_IRWXU);
  if (fifo_check == -1 && errno != EEXIST){
    perror("failed to create new fifo");
    exit(1);
  }
  int client_to_server = open("client_to_server_fifo", O_WRONLY);
  if(client_to_server == -1){
    perror("could not open or create fifo\n");
    exit(1);
  }

  pid_t this_process = getpid();
  char file_post[PID_STRLEN];
  sprintf(file_post, "%d", this_process);

  write(client_to_server, argv[1], strlen(argv[1]) + 1);
  write(client_to_server, process_as_str, strlen(process_as_str) + 1);
  close(client_to_server);

  //fifo name: server_to_client_{PROCESS_ID}
  char server_to_client_ID[PID_STRLEN + PATH_SIZE];
  strcat(server_to_client_ID, FILE_PRE);
  strcat(server_to_client_ID, file_post);

  fifo_check = mkfifo(server_to_client_ID, S_IRWXU);
  if (fifo_check == -1 && errno != EEXIST){
    perror("failed to create new fifo");
    exit(1);
  }
  int server_to_client = open(server_to_client_ID, O_RDONLY);
  if(server_to_client == -1){
    perror("could not open or create fifo\n");
    exit(1);
  }

  char result[MAX_RESULT_LENGTH];
  read(server_to_client, result, MAX_RESULT_LENGTH);
  printf("%s", result);

  close(server_to_client);
  exit(0);
}
