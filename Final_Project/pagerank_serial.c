#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <assert.h>
#include <string.h>

#define NUM_RUNS 50

const char* directory = "graphs/";
const char* file_name = "graph_";
const char* ext = ".txt";
double error, parameter;

struct Node
{
  double weight;
  //unsigned char visited;
  double incoming_neighbor_count;
  double outgoing_neighbor_count;
};

void page_rank_execute(struct Node * node_matrix, int ** adjacency_matrix, int num_runs, int num_nodes, double error, double parameter);
void print_page_ranks(struct Node * node_matrix, int num_nodes);

int main(int argc, char *argv[])
{
  error = .1;
  parameter = .85;
  int num_nodes;
  char input_file[256];

  for(int index = 0 ; index < 100 ; index++){
    clock_t start, end;
    double clock_count;

    start = clock();
    sprintf(input_file, "%s%s%d%s", directory, file_name, index, ext);
    //UPDATE fopen to take different files
    FILE * fp = fopen(input_file, "r");
    if (fp == NULL){
      fprintf(stderr, "ERROR: failed to open edge file!\n");
      exit(-1);
    }

    fscanf(fp, "%d\n", &num_nodes);
    printf("NUM NODES: %d\n", num_nodes);
    double initial_weight = 1.0 / num_nodes;

    //initialize node matrix with initial_weight before processing
    struct Node * node_matrix;
    node_matrix = (struct Node *)malloc(sizeof(struct Node) * num_nodes);
    if (node_matrix == NULL){
      fprintf(stderr, "ERROR: failed to malloc node matrix!\n");
      exit(-1);
    }
    printf("MALLOC'd NODE MATRIX\n");

    for (int i = 0 ; i < num_nodes ; i++){
      (node_matrix[i]).weight = initial_weight;
      //(node_matrix[i]).visited = 0;
      (node_matrix[i]).outgoing_neighbor_count = 0.0;
      (node_matrix[i]).incoming_neighbor_count = 0.0;
    }

    int ** adjacency_matrix;
    adjacency_matrix = (int **)malloc(num_nodes * sizeof(int*));

    printf("MALLOC'd ADJ MATRIX\n");

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

    printf("MALLOC'd ADJ SUBS\n");

    //for Nodes i, j, adjacency_matrix[i][j] is 0 if no edge, 1 if edge from i --> j
    //Set default value to 0
    for (int i = 0 ; i < num_nodes ; i++){
      for (int j = 0 ; j < num_nodes ; j++){
        adjacency_matrix[i][j] = 0;
      }
    }

    printf("FINISHED ACCESSING ADJ MATR1\n");

    int out;
    int in;
    while(fscanf(fp, "%d %d\n", &out, &in) != EOF){
      adjacency_matrix[out ][in] = 1;
      node_matrix[out].outgoing_neighbor_count += 1.0;
      node_matrix[in].incoming_neighbor_count += 1.0;
    }

    printf("FINISHED ACCESSING ADJ MATR + NODE MATR\n");

    if (fclose(fp) == EOF){
      fprintf(stderr, "ERROR: failed to close edge file!\n");
      exit(-1);
    }

    //run the PageRank algorithm, and store the error from each run
    page_rank_execute(node_matrix, adjacency_matrix, NUM_RUNS, num_nodes, error, parameter);

    for(int i = NUM_RUNS - 1 ; i >= 0 ; i++){
      free(adjacency_matrix[i]);
    }
    free(adjacency_matrix);
    free(node_matrix);

    end = clock();
    clock_count = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Time used on file %d:\t%lf", index, clock_count);
  }
}

void page_rank_execute(struct Node * node_matrix, int ** adjacency_matrix, int num_runs, int num_nodes, double error, double parameter)
{
  //print_page_ranks(node_matrix, num_nodes);
  printf("Begin execute\n");
  if(num_runs == 0){
    printf("exiting!\n");
    return;
  }

  double damping = (1.0 - parameter) / num_nodes;

  struct Node * updated_matrix;
  updated_matrix = (struct Node *)malloc(num_nodes * sizeof(struct Node));
  printf("mallocing node matr\n");
  if (updated_matrix == NULL){
    fprintf(stderr, "ERROR: failed to malloc node matrix!\n");
    exit(-1);
  }

  for (int i = 0 ; i < num_nodes ; i++){
    updated_matrix[i].weight = damping;
    updated_matrix[i].outgoing_neighbor_count = node_matrix[i].outgoing_neighbor_count;
    updated_matrix[i].incoming_neighbor_count = node_matrix[i].incoming_neighbor_count;
  }
  printf("intiialized new node matr\n");

  for (int i = 0 ; i < num_nodes ; i++){
    for (int j = 0 ; j < num_nodes ; j++){
      if(adjacency_matrix[i][j] != 0){
        updated_matrix[j].weight += damping * (node_matrix[i].weight / node_matrix[i].outgoing_neighbor_count);
      }
    }
  }

  printf("finished PGE\n");
  page_rank_execute(updated_matrix, adjacency_matrix, num_runs - 1, num_nodes, error, parameter);
  printf("Trying to free: %d\tnodes: %d!\n", num_runs, num_nodes);
  free(updated_matrix);
}

void print_page_ranks(struct Node * node_matrix, int num_nodes){
  for(int i = 0; i < num_nodes; i++){
    printf("Node: %d\t -\t Weight: %1.8lf\n", i, node_matrix[i].weight);
  }
  printf("-------------------------------------------------\n");
}
