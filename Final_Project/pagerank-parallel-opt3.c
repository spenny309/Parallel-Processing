/*
  Parallel program to execute PageRank on many sets of input files.
  This version of the program uses a row-block method in parallelization,
  which leads to a receiving-node based division of labor per thread.
  One benefit of such an organization is that we don't need to lock receiving nodes,
  as we know each one will only be updated by one thread.

  This method was the fastest in initial tests, but lost out to the serial version
  as the number of threads increased, due to high overhead, and the need for a lot of
  locking and barriers.

  This version implements all three major optimizations to try to
  increase the efficiency. the first two optimizations come from opt1, and include
  the optimization from opt2. We also include the minor optimization from opt2 of
  precalculating the damping value outside of each thread
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>

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

  //OPTIMIZATION: include contribution field
  long double contribution;

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

/*
OPTIMIZATION: Locks and barriers no longer needed with this optimization
pthread_barrier_t loop_barrier;
pthread_mutex_t error_lock;
*/

//OPTIMIZATION: pre-calculate dampening value outside of each thread
double damping;

//OPTIMIZATION: pre-calculate node range per thread
int * thread_node_range;

void * page_rank_execute(void * args);
void print_page_ranks();

/*
  The main program will initialize the required data structures, then execute
  and time sets of PageRank graphs provided by the specified path.
  The current organization runs PageRank on 5 sets of 100 unique graphs each.
*/
int main(int argc, char *argv[])
{
  //Execute PageRank with the given number of threads (2, 4, 8, and 16)
  for(thread_count = 2 ; thread_count <= 16 ; thread_count <<= 1){
    //Initialize thread_count number of threads
    long pthread;
    pthread_t thread_IDs[thread_count];

    //OPTIMIZATION: Allocate thread_node_range using thread_count
    thread_node_range = (int *)malloc(sizeof(int) * (thread_count+1));
    if(thread_node_range == NULL){
      fprintf(stderr, "ERROR: Failed to allocated node range array!\n");
      exit(-1);
    }

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
          exit(-1);
        }

        //OPTIMIZATION: Set thread node ranges based on num_nodes and thread_count
        thread_node_range[thread_count] = num_nodes;
        for(int i = 0 ; i < thread_count ; i++){
          thread_node_range[i] = i * ((thread_count+num_nodes) / thread_count);
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

        //OPTIMIZATION: calculating contribution for first iteration
        for(int i = 0 ; i < num_nodes ; i++){
          (node_array[i]).contribution = PARAMETER * (node_array[i].weight / node_array[i].outgoing_neighbor_count);
        }

        //Time the PageRank execution
        start = clock();

        error = 1.0;
        //OPTIMIZATION: pre-calculate damping value
        damping = (1.0 - PARAMETER) / num_nodes;

        //OPTIMIZATION: Calculate iteration, error and weight <-- new_weight sequentially,
        //  to avoid heavy locks and barriers within the page_rank_execute function.
        while(error > ERROR_INVARIANT){
          //track how many iterations the program runs for
          iteration_count += 1;

          //Run the PageRank algorithm on thread_count threads, passing thread_num as args
          for(long thread = 0 ; thread < thread_count ; thread++){
            pthread_create(&thread_IDs[thread], NULL, page_rank_execute, (void*) thread);
          }
          //Wait for all threads to finish execution and return
          for(long thread = 0 ; thread < thread_count ; thread++){
            pthread_join(thread_IDs[thread], NULL);
          }

          //Calculate error from this iteration
          //Update weight to be new_weight for next iteration
          error = 0.0;
          for(int i = 0 ; i < num_nodes ; i++){
            error = error > fabsl(node_array[i].new_weight - node_array[i].weight) ? error : fabsl(node_array[i].new_weight - node_array[i].weight);
            node_array[i].weight = node_array[i].new_weight;

            //OPTIMIZATION: calculating contribution for next iteration
            node_array[i].contribution = PARAMETER * (node_array[i].new_weight / node_array[i].outgoing_neighbor_count);;
          }
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
    free(thread_node_range);
  }
}

/*
  Function to execute PageRank in Parallel. args is a pointer the thread number
  for the currently executing thread. PageRank runs in parallel until convergence,
  using the ERROR_INVARIANT.
  OPTIMIZATION: page_rank_execute is now very simple, and simply calculates the new_weight for each node
                no longer requires any locks, barriers, etc. Which may reduce heavy
                overhead
*/
void * page_rank_execute(void *args)
{
  //get current thread number to partition nodes
  long this_thread = (long)args;

  //set new_weight to damping (random) factor before calculating neighbor contributions
  //OPTIMIZATION: use pre-calculate thread_node_range for values
  for (int i = thread_node_range[this_thread] ; i < thread_node_range[this_thread + 1] ; i++){
    node_array[i].new_weight = damping;
  }

  //add neighbor contributions to new_weight
  //row-block ordering
  //OPTIMIZATION: use pre-calculate thread_node_range for values
  for (int i = thread_node_range[this_thread] ; i < thread_node_range[this_thread + 1] ; i++){
    for (int j = 0 ; j < num_nodes ; j++){
      if(adjacency_matrix[j][i] != 0){
        //OPTIMIZATION: using pre-calculated contribution instead of re-calculating value
        node_array[i].new_weight += node_array[j].contribution;
      }
    }
  }
}

/*
  Simple function to print out current state of node_array
  Recommended to only use on very tiny test files.
*/
void print_page_ranks(){
  for(int i = 0; i < num_nodes; i++){
    printf("Node: %d\t -\t Weight: %1.8Lf\n", i, node_array[i].weight);
  }
  printf("-------------------------------------------------\n");
}