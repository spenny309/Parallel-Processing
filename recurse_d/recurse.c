
int main(int argc, char **argv){
  if(argc < 2 || argc > 2){
    perror("Please include only one command-line argument, a positive integer n\n");
    exit(1);
  }else{
    int pipe_one[2];
    if(pipe(pipe_one) == -1){
      perror("failed to open pipe\n");
      exit(1);
    }
    pid_t pid = fork();
    if(pid == -1){
      perror("failed to fork\n");
      exit(1);
    }

    char* input = argv[1];

    for (int i = 0; i < strlen(input); i++){
      if (input[i] > '9' || input[i] < '0'){
	perror("invalid input. must input positive integer n\n");
	exit(1);
      }
    }
    
    if(pid == 0){
      /* write n to pipe */
      /* call recursive function with pipe */
      /* recursive function will fork and call itself with n-1 */
    }else{
      /* wait for pipe result? */
      wait(NULL);
    }

  }
}
