#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <assert.h>
#include <string.h>

#define ERROR_INVARIANT .00000000001

const char* directory = "graphs/";
const char* subdirectory = "set_";
const char* file_name = "/graph_";
const char* ext = ".txt";

long double error, parameter;
int num_nodes, iteration_count;
struct Node * node_matrix;
int ** adjacency_matrix;

struct Node
{
  long double weight;
  long double new_weight;
  double incoming_neighbor_count;
  double outgoing_neighbor_count;
};

void page_rank_execute();
void print_page_ranks();

int main(int argc, char *argv[])
{
  parameter = .85;
  iteration_count = 0;
  error = 0.0;
  char input_file[256];

  printf("executing pagerank\n");
  for(int set_num = 1 ; set_num < 6 ; set_num++){
    printf("starting set %d\n", set_num);
    clock_t set_start, set_end;
    set_start = clock();
    for(int index = 0 ; index < 100 ; index++){
      clock_t start, end;
      double clock_count;

      start = clock();
      sprintf(input_file, "%s%s%d%s%d%s", directory, subdirectory, set_num, file_name, index, ext);
      FILE * fp = fopen(input_file, "r");
      if (fp == NULL){
        fprintf(stderr, "ERROR: failed to open edge file!\n");
        exit(-1);
      }

      fscanf(fp, "%d\n", &num_nodes);
      double initial_weight = 1.0 / num_nodes;

      //initialize node matrix with initial_weight before processing
      node_matrix = (struct Node *)malloc(sizeof(struct Node) * num_nodes);
      if (node_matrix == NULL){
        fprintf(stderr, "ERROR: failed to malloc node matrix!\n");
        exit(-1);
      }

      for (int i = 0 ; i < num_nodes ; i++){
        (node_matrix[i]).weight = initial_weight;
        (node_matrix[i]).outgoing_neighbor_count = 0.0;
        (node_matrix[i]).incoming_neighbor_count = 0.0;
      }

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
      page_rank_execute();

      for(int i = 0 ; i < num_nodes ; i++){
        free(adjacency_matrix[i]);
      }
      free(adjacency_matrix);
      free(node_matrix);

      end = clock();
      double time = end - start;
      clock_count = ((double) (end - start)) / CLOCKS_PER_SEC;
      printf("Time used on file %d:\t%lf\t%lf\n", index, time, clock_count);
    }
    set_end = clock();
    double time = set_end - set_start;
    double clock_count = ((double) (set_end - set_start)) / CLOCKS_PER_SEC;
    printf("Time used on set %d:\t%lf\t%lf\n", set_num, time, clock_count);
  }
}

void page_rank_execute()
{
  //print_page_ranks(node_matrix, num_nodes);
  iteration_count += 1;
  printf("executing PR: %d\n", iteration_count);
  error = 0.0;
  double damping = (1.0 - parameter) / num_nodes;
  long double local_error = 0.0;

  for (int i = 0 ; i < num_nodes ; i++){
    node_matrix[i].new_weight = damping;
  }

  for (int i = 0 ; i < num_nodes ; i++){
    for (int j = 0 ; j < num_nodes ; j++){
      if(adjacency_matrix[j][i] != 0){
        node_matrix[i].new_weight += parameter * (node_matrix[j].weight / node_matrix[j].outgoing_neighbor_count);
      }
    }
  }

  for (int i = 0 ; i < num_nodes ; i++){
    local_error = fabsl(node_matrix[i].new_weight - node_matrix[i].weight);
    error = error > local_error ? error : local_error;
    node_matrix[i].weight = node_matrix[i].new_weight;
  }

  printf("error: %Lf\n", error);

  if(error > ERROR_INVARIANT){
    page_rank_execute();
  }
}

void print_page_ranks(){
  for(int i = 0; i < num_nodes; i++){
    printf("Node: %d\t -\t Weight: %1.8Lf\n", i, node_matrix[i].weight);
  }
  printf("-------------------------------------------------\n");
}
