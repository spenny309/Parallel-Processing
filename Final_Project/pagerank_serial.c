#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <assert.h>
#include <string.h>

#define N_NODES 10
#define NUM_RUNS 50
double error, parameter;

struct Node
{
  double weight;
  //unsigned char visited;
  double incoming_neighbor_count;
  double outgoing_neighbor_count;
};

void page_rank_execute(struct Node node_matrix[], int adjacency_matrix[][N_NODES], int num_runs, double error, double parameter);
void print_page_ranks(struct Node node_matrix[]);

int main(int argc, char *argv[])
{
  error = .1;
  parameter = .85;
  double initial_weight = 1.0 / N_NODES;

  //initialize node matrix with initial_weight before processing
  struct Node node_matrix[N_NODES];
  for (int i = 0 ; i < N_NODES ; i++){
    (node_matrix[i]).weight = initial_weight;
    //(node_matrix[i]).visited = 0;
    (node_matrix[i]).outgoing_neighbor_count = 0.0;
    (node_matrix[i]).incoming_neighbor_count = 0.0;
  }

  //for Nodes i, j, adjacency_matrix[i][j] is 0 if no edge, 1 if edge from i --> j
  int adjacency_matrix[N_NODES][N_NODES];
  for (int i = 0 ; i < N_NODES ; i++){
    for (int j = 0 ; j < N_NODES ; j++){
      adjacency_matrix[i][j] = 0;
    }
  }

  //Read input to fill in adjacency_matrix
  FILE * fp = fopen("sample.txt", "r");
  if (fp == NULL){
    fprintf(stderr, "ERROR: failed to open edge file!\n");
    exit(-1);
  }

  int out;
  int in;
  while(fscanf(fp, "%d %d\n", &out, &in) != EOF){
    adjacency_matrix[out - 1][in - 1] = 1;
    node_matrix[out-1].outgoing_neighbor_count += 1.0;
    node_matrix[in-1].incoming_neighbor_count += 1.0;
  }

  if (fclose(fp) == EOF){
    fprintf(stderr, "ERROR: failed to close edge file!\n");
    exit(-1);
  }

  //run the PageRank algorithm, and store the error from each run
  page_rank_execute(node_matrix, adjacency_matrix, NUM_RUNS, error, parameter);
}

void page_rank_execute(struct Node node_matrix[], int adjacency_matrix[][N_NODES], int num_runs, double error, double parameter)
{
  print_page_ranks(node_matrix);
  if(num_runs == 0){
    return;
  }

  double damping = (1.0 - parameter) / N_NODES;

  struct Node updated_matrix[N_NODES];
  for (int i = 0 ; i < N_NODES ; i++){
    updated_matrix[i].weight = damping;
    updated_matrix[i].outgoing_neighbor_count = node_matrix[i].outgoing_neighbor_count;
    updated_matrix[i].incoming_neighbor_count = node_matrix[i].incoming_neighbor_count;
  }

  for (int i = 0 ; i < N_NODES ; i++){
    for (int j = 0 ; j < N_NODES ; j++){
      if(adjacency_matrix[i][j] != 0){
        updated_matrix[j].weight += damping * (node_matrix[i].weight / node_matrix[i].outgoing_neighbor_count);
      }
    }
  }
  page_rank_execute(updated_matrix, adjacency_matrix, num_runs - 1, error, parameter);
}

void print_page_ranks(struct Node node_matrix[]){
  for(int i = 0; i < N_NODES; i++){
    printf("Node: %d\t -\t Weight: %1.8lf\n", i, node_matrix[i].weight);
  }
  printf("-------------------------------------------------\n");
}
