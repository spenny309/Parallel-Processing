#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>

#define THREAD_COUNT 8
#define ERROR_INVARIANT .0000001

const char* directory = "graphs/";
const char* file_name = "graph_";
const char* ext = ".txt";

struct Node
{
  double weight;
  double new_weight;
  double incoming_neighbor_count;
  double outgoing_neighbor_count;
};

double error, parameter;
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

  for(int index = 0 ; index < 100 ; index++){
    printf("working on file: %d\n", index);
    clock_t start, end;
    double clock_count;
    iteration_count = 0;

    start = clock();
    sprintf(input_file, "%s%s%d%s", directory, file_name, index, ext);
    FILE * fp = fopen(input_file, "r");
    if (fp == NULL){
      fprintf(stderr, "ERROR: failed to open edge file!\n");
      exit(-1);
    }

    fscanf(fp, "%d\n", &num_nodes);
    double initial_weight = 1.0 / num_nodes;

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
}

void * page_rank_execute(void *args)
{
  //get current thread number to partition nodes
  long this_thread = (long)args;
  //printf("executing thread: %ld\n", this_thread);

  //CRITICAL: must reset error to 0.0
  pthread_barrier_wait(&loop_barrier);
  //printf("checkpoint on: %ld\n", this_thread);
  error = 0.0;
  pthread_barrier_wait(&loop_barrier);

  double damping = (1.0 - parameter) / num_nodes;

  //printf("setting damping on: %ld\n", this_thread);
  for (long i = this_thread * (num_nodes / THREAD_COUNT) ; i < (this_thread+1) * (num_nodes / THREAD_COUNT) ; i++){
    //printf("trying to access: %ld\ton: %ld\n", i, this_thread);
    node_matrix[i].new_weight = damping;
  }

  //printf("setting new_weight on: %ld\n", this_thread);
  for (long i = this_thread * (num_nodes / THREAD_COUNT) ; i < (this_thread+1) * (num_nodes / THREAD_COUNT) ; i++){
    for (int j = 0 ; j < num_nodes ; j++){
      if(adjacency_matrix[i][j] != 0){
        node_matrix[j].new_weight += damping * (node_matrix[i].weight / node_matrix[i].outgoing_neighbor_count);
      }
    }
  }

  //printf("error and updated weight on: %ld\n", this_thread);
  for(long i = this_thread * (num_nodes / THREAD_COUNT) ; i < (this_thread+1) * (num_nodes / THREAD_COUNT) ; i++){
    pthread_mutex_lock(&error_lock);
    error += fabs(node_matrix[i].new_weight - node_matrix[i].weight);
    pthread_mutex_unlock(&error_lock);
    node_matrix[i].weight = node_matrix[i].new_weight;
  }

  //printf("barrier on: %ld\n", this_thread);
  if (pthread_barrier_wait(&loop_barrier) == PTHREAD_BARRIER_SERIAL_THREAD){
    iteration_count += 1;
  }

  if(error > ERROR_INVARIANT) {
    //printf("recurse on: %ld\n", this_thread);
    page_rank_execute(args);
  } else {
    //printf("exit on: %ld\n", this_thread);
    pthread_exit((void *) 0);
  }
}

void print_page_ranks(struct Node * node_matrix, int num_nodes){
  for(int i = 0; i < num_nodes; i++){
    printf("Node: %d\t -\t Weight: %1.8lf\n", i, node_matrix[i].weight);
  }
  printf("-------------------------------------------------\n");
}
