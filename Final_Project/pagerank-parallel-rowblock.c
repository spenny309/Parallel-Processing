/*
  Parallel program to execute PageRank on many sets of input files.
  This version of the program uses a row-block method in parallelization,
  which leads to a receiving-node based division of labor per thread.
  One benefit of such an organization is that we don't need to lock receiving nodes,
  as we know each one will only be updated by one thread.
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>

//Allow user to specify number of THREADS, or default to 8
/*
#ifdef THREADS
  #define thread_count THREADS
#else
  #define thread_count 8
#endif
*/

//Default error invariant for PageRank algorithm
#define ERROR_INVARIANT .000000000001
//Default dampening parameter for sequential browsing in PageRank algorithm: 85% seq, 15% random
#define PARAMETER 0.85

//Input file constants
const char* directory = "graphs/";
const char* subdirectory = "set_";
const char* file_name = "/graph_";
const char* ext = ".txt";

/*
  Node structure used implicitly for nodes in PageRank connectivity graph;
  stored in node_array. Maintains count of neighbors in each direction,
  Generation k weight when building Generation k+1 new_weight.
*/
struct Node
{
  long double weight;
  long double new_weight;
  double incoming_neighbor_count;
  double outgoing_neighbor_count;
};

//global value for current error
long double error;
//Store node count on current graph and number of iterations PageRank takes to converge
int num_nodes, iteration_count, thread_count;
//Store num_nodes in array for fast access and updates
struct Node * node_array;
//Adjacency matrix which stores edge links between nodes in graph,
//where [i][j] == 1 implies i --> j
int ** adjacency_matrix;
//barrier to maintain consistency within each generation of algorithm
pthread_barrier_t loop_barrier;
//lock to prevent current error race conditions
pthread_mutex_t error_lock;

void * page_rank_execute(void * args);
void print_page_ranks(struct Node * node_array, int num_nodes);

/*
  The main program will initialize the required data structures, then execute
  and time sets of PageRank graphs provided by the specified path.
  The current organization runs PageRank on 5 sets of 100 unique graphs each.
*/
int main(int argc, char *argv[])
{
  for(thread_count = 2 ; thread_count <= 16 ; thread_count <<= 1){
    //Initialize thread_count number of threads
    long pthread;
    pthread_t thread_IDs[thread_count];
    //initialize a barrier to wait for thread_count threads to be used in each loop
    pthread_barrier_init(&loop_barrier, NULL, thread_count);
    //Initialize lock to prevent error race conditions
    pthread_mutex_init(&error_lock, NULL);

    //Set error to default value
    error = 0.0;

    //Reasonable space to store an input file name.
    char input_file[256];

    printf("executing pagerank with %d threads\n", thread_count);
    //Execute PageRank over the range of given sets
    for(int set_num = 1; set_num < 6; set_num++){
      printf("starting set %d\n", set_num);
      //Initialize time structures and record start time for this set
      clock_t set_start, set_end;
      set_start = clock();
      //Execute PgaeRank over the range of given file numbers
      for(int index = 0 ; index < 100 ; index++){
        //Initialize time structures and set default values
        clock_t start, end;
        double clock_count;
        long long int edge_count = 0;
        iteration_count = 0;

        //Generate current file's  name
        sprintf(input_file, "%s%s%d%s%d%s", directory, subdirectory, set_num, file_name, index, ext);
        //Open file
        FILE * fp = fopen(input_file, "r");
        if (fp == NULL){
          fprintf(stderr, "ERROR: failed to open edge file!\n");
          exit(-1);
        }

        //Get num_nodes from first line of file
        if (fscanf(fp, "%d\n", &num_nodes) == -1){
          fprintf(stderr, "ERROR: failed to read from file!\n");
        }
        //Each node starts with weight: 1 / N
        long double initial_weight = 1.0 / num_nodes;

        //initialize node array to hold num_nodes Nodes
        node_array = (struct Node *)malloc(sizeof(struct Node) * num_nodes);
        if (node_array == NULL){
          fprintf(stderr, "ERROR: failed to malloc node array!\n");
          exit(-1);
        }
        //set initial weight for all nodes to be 1 / N
        for (int i = 0 ; i < num_nodes ; i++){
          (node_array[i]).weight = initial_weight;
          (node_array[i]).outgoing_neighbor_count = 0.0;
          (node_array[i]).incoming_neighbor_count = 0.0;
        }

        //initialize adjacency matrix to hold num_nodes X num_nodes edges (ints)
        adjacency_matrix = (int **)malloc(num_nodes * sizeof(int*));
        if (adjacency_matrix == NULL){
          fprintf(stderr, "ERROR: failed to malloc adjacency matrix!\n");
          exit(-1);
        }
        for(int i = 0 ; i < num_nodes ; i++){
          adjacency_matrix[i] = (int *)malloc(num_nodes * sizeof(int));
          if (adjacency_matrix[i] == NULL){
            fprintf(stderr, "ERROR: failed to malloc adjacency matrix!\n");
            exit(-1);
          }
        }

        //for Nodes i, j, adjacency_matrix[i][j] is 0 if no edge, 1 if edge from i --> j
        //Set default value to 0
        for (int i = 0 ; i < num_nodes ; i++){
          for (int j = 0 ; j < num_nodes ; j++){
            adjacency_matrix[i][j] = 0;
          }
        }

        int out;
        int in;
        //Scan from fp line-by-line to read edge data; set new edges to 1 in adjacency_matrix
        //Increment outoging and incoming neighbor counts appropriately
        while(fscanf(fp, "%d %d\n", &out, &in) != EOF){
          if(adjacency_matrix[out][in] == 0){
            adjacency_matrix[out][in] = 1;
            node_array[out].outgoing_neighbor_count += 1.0;
            node_array[in].incoming_neighbor_count += 1.0;
            edge_count++;
          }
        }

        //Cleanup
        if (fclose(fp) == EOF){
          fprintf(stderr, "ERROR: failed to close edge file!\n");
          exit(-1);
        }

        //Time the PageRank execution
        start = clock();

        //Run the PageRank algorithm on thread_count threads, passing thread_num as args
        for(long thread = 0 ; thread < thread_count ; thread++){
          pthread_create(&thread_IDs[thread], NULL, page_rank_execute, (void*) thread);
        }
        //Wait for all threads to finish execution and return
        for(long thread = 0 ; thread < thread_count ; thread++){
          pthread_join(thread_IDs[thread], NULL);
        }
        //Time the PageRank execution
        end = clock();

        //Free data structures related to the current graph
        for(int i = 0 ; i < num_nodes ; i++){
          free(adjacency_matrix[i]);
        }
        free(adjacency_matrix);
        free(node_array);

        //Print information relating to this execution
        double time = end - start;
        clock_count = ((double) (end - start)) / CLOCKS_PER_SEC;
        printf("set: %d\tfile: %3d\tn: %5d\te: %12Ld\t%10.0lf\t%4.6lf\ti: %d\n", set_num, index, num_nodes, edge_count, time, clock_count, iteration_count);
      }
      //Time PageRank execution on this set
      set_end = clock();

      //Print information relating to the execution of this set
      double time = set_end - set_start;
      double clock_count = ((double) (set_end - set_start)) / CLOCKS_PER_SEC;
      printf("TOTAL SET: %d\t%10.0lf\t%4.6lf\n", set_num, time, clock_count);
    }
  }
}

void * page_rank_execute(void *args)
{
  //get current thread number to partition nodes
  long this_thread = (long)args;
  long double local_max_error = 0.0;
  //CRITICAL: must reset error to 0.0
  pthread_barrier_wait(&loop_barrier);
  //printf("checkpoint on: %ld\n", this_thread);
  error = 0.0;
  pthread_barrier_wait(&loop_barrier);

  double damping = (1.0 - PARAMETER) / num_nodes;

  for (int i = (this_thread * (thread_count+num_nodes) / thread_count) ; i < ((1+this_thread) * (thread_count+num_nodes) / thread_count) && i < num_nodes ; i++){
    //printf("trying to access: %ld\ton: %ld\n", i, this_thread);
    node_array[i].new_weight = damping;
  }

  //printf("setting new_weight on: %ld\n", this_thread);
  for (int i = (this_thread * (thread_count+num_nodes) / thread_count) ; i < ((1+this_thread) * (thread_count+num_nodes) / thread_count) && i < num_nodes ; i++){
    for (int j = 0 ; j < num_nodes ; j++){
      if(adjacency_matrix[j][i] != 0){
        node_array[i].new_weight += PARAMETER * (node_array[j].weight / node_array[j].outgoing_neighbor_count);
      }
    }
  }

  //wait until all of the new weights are calculated before updated old weights
  pthread_barrier_wait(&loop_barrier);

  for(int i = (this_thread * (thread_count+num_nodes) / thread_count) ; i < ((1+this_thread) * (thread_count+num_nodes) / thread_count) && i < num_nodes ; i++){
    local_max_error = local_max_error > fabsl(node_array[i].new_weight - node_array[i].weight) ? local_max_error : fabsl(node_array[i].new_weight - node_array[i].weight);
    node_array[i].weight = node_array[i].new_weight;
  }

  pthread_mutex_lock(&error_lock);
  if(local_max_error > error){
    error = local_max_error;
  }
  pthread_mutex_unlock(&error_lock);

  if (pthread_barrier_wait(&loop_barrier) == PTHREAD_BARRIER_SERIAL_THREAD){
    iteration_count += 1;
  }

  if(error > ERROR_INVARIANT) {
    page_rank_execute(args);
  } else {
    pthread_exit((void *) 0);
  }
}

void print_page_ranks(struct Node * node_array, int num_nodes){
  for(int i = 0; i < num_nodes; i++){
    printf("Node: %d\t -\t Weight: %1.8Lf\n", i, node_array[i].weight);
  }
  printf("-------------------------------------------------\n");
}
