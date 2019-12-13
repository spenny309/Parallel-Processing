#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>

#define THREAD_COUNT 8
#define ERROR_INVARIANT .00000000001

const char* directory = "graphs/";
const char* subdirectory = "set_";
const char* file_name = "/graph_";
const char* ext = ".txt";

struct Node
{
  long double weight;
  long double new_weight;
  double incoming_neighbor_count;
  double outgoing_neighbor_count;
};

long double error, parameter;
int num_nodes, iteration_count;
struct Node * node_matrix;
int ** adjacency_matrix;
pthread_barrier_t loop_barrier;
pthread_mutex_t error_lock;

void * page_rank_execute(void * args);
void print_page_ranks(struct Node * node_matrix, int num_nodes);

int main(int argc, char *argv[])
{
  long pthread;
  pthread_t thread_IDs[THREAD_COUNT];

  //initialize a barrier to wait for THREAD_COUNT threads to be used in each loop
  pthread_barrier_init(&loop_barrier, NULL, THREAD_COUNT);
  pthread_mutex_init(&error_lock, NULL);

  error = 0.0;
  parameter = .85;
  char input_file[256];

  printf("executing pagerank\n");
  for(int set_num = 1; set_num < 6; set_num++){
    printf("starting set %d\n", set_num);
    clock_t set_start, set_end;
    set_start = clock();
    for(int index = 0 ; index < 100 ; index++){
      clock_t start, end;
      double clock_count;
      iteration_count = 0;

      start = clock();
      sprintf(input_file, "%s%s%d%s%d%s", directory, subdirectory, set_num, file_name, index, ext);
      FILE * fp = fopen(input_file, "r");
      if (fp == NULL){
        fprintf(stderr, "ERROR: failed to open edge file!\n");
        exit(-1);
      }

      fscanf(fp, "%d\n", &num_nodes);
      long double initial_weight = 1.0 / num_nodes;

      //initialize node matrix with initial_weight before processing
      //printf("creating node_matrix\n");
      node_matrix = (struct Node *)malloc(sizeof(struct Node) * num_nodes);
      if (node_matrix == NULL){
        fprintf(stderr, "ERROR: failed to malloc node matrix!\n");
        exit(-1);
      }

      //printf("initializing node_matrix\n");
      for (int i = 0 ; i < num_nodes ; i++){
        (node_matrix[i]).weight = initial_weight;
        (node_matrix[i]).outgoing_neighbor_count = 0.0;
        (node_matrix[i]).incoming_neighbor_count = 0.0;
      }

      //printf("creating adjacency_matrix\n");
      adjacency_matrix = (int **)malloc(num_nodes * sizeof(int*));
      if (adjacency_matrix == NULL){
        fprintf(stderr, "ERROR: failed to malloc adjacency matrix!\n");
        exit(-1);
      }

      //printf("malloc for adjacency_matrix\n");
      for(int i = 0 ; i < num_nodes ; i++){
        adjacency_matrix[i] = (int *)malloc(num_nodes * sizeof(int));
        if (adjacency_matrix[i] == NULL){
          fprintf(stderr, "ERROR: failed to malloc adjacency matrix!\n");
          exit(-1);
        }
      }

      //for Nodes i, j, adjacency_matrix[i][j] is 0 if no edge, 1 if edge from i --> j
      //Set default value to 0
      //printf("filling adjacency_matrix\n");
      for (int i = 0 ; i < num_nodes ; i++){
        for (int j = 0 ; j < num_nodes ; j++){
          adjacency_matrix[i][j] = 0;
        }
      }

      int out;
      int in;
      while(fscanf(fp, "%d %d\n", &out, &in) != EOF){
        if(adjacency_matrix[out][in] == 0){
          adjacency_matrix[out][in] = 1;
          node_matrix[out].outgoing_neighbor_count += 1.0;
          node_matrix[in].incoming_neighbor_count += 1.0;
        }
      }

      if (fclose(fp) == EOF){
        fprintf(stderr, "ERROR: failed to close edge file!\n");
        exit(-1);
      }

      //run the PageRank algorithm, and store the error from each run
      //printf("NODE COUNT: %d\n", num_nodes);
      for(long thread = 0 ; thread < THREAD_COUNT ; thread++){
        //printf("creating thread: %ld\n", thread);
        pthread_create(&thread_IDs[thread], NULL, page_rank_execute, (void*) thread);
      }

      for(long thread = 0 ; thread < THREAD_COUNT ; thread++){
        pthread_join(thread_IDs[thread], NULL);
      }

      for(int i = 0 ; i < num_nodes ; i++){
        free(adjacency_matrix[i]);
      }
      free(adjacency_matrix);
      free(node_matrix);

      end = clock();
      double time = end - start;
      clock_count = ((double) (end - start)) / CLOCKS_PER_SEC;
      printf("Time used on file %d:\t%lf\t%lf\titers: %d\n", index, time, clock_count, iteration_count);
    }
    set_end = clock();
    double time = set_end - set_start;
    double clock_count = ((double) (set_end - set_start)) / CLOCKS_PER_SEC;
    printf("Time used on set %d:\t%lf\t%lf\n", set_num, time, clock_count);
  }
}

void * page_rank_execute(void *args)
{
  //get current thread number to partition nodes
  long this_thread = (long)args;
  //printf("executing thread: %ld\n", this_thread);
  long double local_max_error = 0.0;
  //CRITICAL: must reset error to 0.0
  printf("check 1 on thread: %ld\n", this_thread);
  pthread_barrier_wait(&loop_barrier);
  //printf("checkpoint on: %ld\n", this_thread);
  error = 0.0;
  printf("check 2 on thread: %ld\n", this_thread);
  pthread_barrier_wait(&loop_barrier);

  double damping = (1.0 - parameter) / num_nodes;
  printf("data:\tdamp: %lf\tpara: %Lf\tnumN: %d\n", damping, parameter, num_nodes);
  //printf("setting damping on: %ld\n", this_thread);
  printf("range for thread %ld:\t %5ld\t-----\t %5ld\n", this_thread, (this_thread * (THREAD_COUNT+num_nodes) / THREAD_COUNT), ((1+this_thread) * (THREAD_COUNT+num_nodes) / THREAD_COUNT));
  for (int i = (this_thread * (THREAD_COUNT+num_nodes) / THREAD_COUNT) ; i < ((1+this_thread) * (THREAD_COUNT+num_nodes) / THREAD_COUNT) && i < num_nodes ; i++){
    //printf("trying to access: %ld\ton: %ld\n", i, this_thread);
    node_matrix[i].new_weight = damping;
  }

  //printf("setting new_weight on: %ld\n", this_thread);
  for (int i = (this_thread * (THREAD_COUNT+num_nodes) / THREAD_COUNT) ; i < ((1+this_thread) * (THREAD_COUNT+num_nodes) / THREAD_COUNT) && i < num_nodes ; i++){
    for (int j = 0 ; j < num_nodes ; j++){
      if(adjacency_matrix[j][i] != 0){
        node_matrix[i].new_weight += parameter * (node_matrix[j].weight / node_matrix[j].outgoing_neighbor_count);
      }
    }
  }

  printf("check 3 on thread: %ld\n", this_thread);
  //wait until all of the new weights are calculated before updated old weights
  pthread_barrier_wait(&loop_barrier);

  //printf("error and updated weight on: %ld\n", this_thread);
  for(int i = (this_thread * (THREAD_COUNT+num_nodes) / THREAD_COUNT) ; i < ((1+this_thread) * (THREAD_COUNT+num_nodes) / THREAD_COUNT) && i < num_nodes ; i++){
    local_max_error = local_max_error > fabsl(node_matrix[i].new_weight - node_matrix[i].weight) ? local_max_error : fabsl(node_matrix[i].new_weight - node_matrix[i].weight);
    node_matrix[i].weight = node_matrix[i].new_weight;
  }

  pthread_mutex_lock(&error_lock);
  if(local_max_error > error){
    error = local_max_error;
  }
  pthread_mutex_unlock(&error_lock);
  //printf("barrier on: %ld\n", this_thread);
  printf("check 4 on thread: %ld\n", this_thread);
  if (pthread_barrier_wait(&loop_barrier) == PTHREAD_BARRIER_SERIAL_THREAD){
    //printf("iter: %d\terr: %1.14Lf\n", iteration_count, error);
    iteration_count += 1;
  }

  if(error > ERROR_INVARIANT) {
    //printf("recurse on: %ld\n", this_thread);
    printf("error: %1.14Lf\n", error);
    page_rank_execute(args);
  } else {
    //printf("exit on: %ld\n", this_thread);
    pthread_exit((void *) 0);
  }
}

void print_page_ranks(struct Node * node_matrix, int num_nodes){
  for(int i = 0; i < num_nodes; i++){
    printf("Node: %d\t -\t Weight: %1.8Lf\n", i, node_matrix[i].weight);
  }
  printf("-------------------------------------------------\n");
}
