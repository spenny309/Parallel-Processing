#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
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
  if(fd == -1){
    perror("could not open or create fifo\n");
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
}
