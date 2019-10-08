/* A client program which provides {Proper-noun} to a server
   Receives a randomly generated sentence from server, of the form:
    {Proper-noun} {verb} {preposition} the {adjective} {noun}!

   ./client {Proper-noun}
*/

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

//used to build private server_to_client fifo
#define FILE_PRE "server_to_client_"
#define PATH_SIZE (strlen("server_to_client_") + 1)
#define PID_STRLEN (sizeof(pid_t)*8 + 1)

//assume result is shorter than 255 chars
#define MAX_RESULT_LENGTH 255

extern int errno;

int main(int argc, char **argv){
  //error checking, fifo generation, etc.
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

  //get ID, for client and server to create private return fifo
  pid_t this_process = getpid();
  char file_post[PID_STRLEN];
  sprintf(file_post, "%d", this_process);

  //write {Proper-noun} and ID to server
  write(client_to_server, argv[1], MAX_RESULT_LENGTH);
  write(client_to_server, file_post, PID_STRLEN);
  close(client_to_server);

  //fifo name: server_to_client_{PROCESS_ID}
  char server_to_client_ID[PID_STRLEN + PATH_SIZE];
  strcat(server_to_client_ID, FILE_PRE);
  strcat(server_to_client_ID, file_post);

  //Create/open private return fifo
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

  //read and print result from server
  char result[MAX_RESULT_LENGTH];
  read(server_to_client, result, MAX_RESULT_LENGTH);
  printf("%s\n", result);

  close(server_to_client);

  //remove private fifo from system before exiting
  char remove_fifo[strlen(server_to_client_ID) + 4];
  remove_fifo[0] = '\0';
  strcat(remove_fifo, "rm ");
  strcat(remove_fifo, server_to_client_ID);
  system(remove_fifo);

  exit(0);
}
