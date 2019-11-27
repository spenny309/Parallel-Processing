#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <assert.h>
#include <string.h>

#define SEED 309
#define GRAPH_FILES 100
#define NODE_MIN 1000
#define NODE_MAX 5000

const char* directory = "graphs/";
const char* file_name = "graph_";
const char* ext = ".txt";

int main(int argc, char *argv[]){

  int node_range = NODE_MAX - NODE_MIN;
  //testing on sparse graphs is not representative
  int edge_min = NODE_MIN << 1;
  int edge_max = NODE_MAX * (NODE_MAX - 1);
  int current_file = 0;
  char graph_name[256];

  int node_count, edge_count, edge_out, edge_in, edge_range;

  srand(SEED);

  while(current_file < GRAPH_FILES){
    node_count = (rand() % node_range) + NODE_MIN;
    edge_range = (node_count * (node_count - 1)) - (node_count << 1);
    edge_count = (rand() % edge_range) + edge_min;

    // TODO : Add error checking
    sprintf(graph_name, "%s%s%d%s", directory, file_name, current_file++, ext);
    FILE * fp = fopen(graph_name, "w+");

    fprintf(fp, "%d\n", node_count);
    for(int i = 0 ; i < edge_count ; i++){
      edge_out = rand() % node_count;
      while((edge_in = (rand() % node_count)) == edge_out){}
      fprintf(fp, "%d %d\n", edge_out, edge_in);
    }
    fclose(fp);
  }
}
