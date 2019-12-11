#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <assert.h>
#include <string.h>

/*
set 1 details:
#define SEED 309
#define GRAPH_FILES 100
#define NODE_MIN 7500
#define NODE_MAX 12500
const char* subdirectory = "set_1/";
*/
/*
set 2 details:
#define SEED 1023
#define GRAPH_FILES 100
#define NODE_MIN 100
#define NODE_MAX 1000
const char* subdirectory = "set_2/";
*/
/*
set 3 details:
#define SEED 42
#define GRAPH_FILES 100
#define NODE_MIN 1000
#define NODE_MAX 2500
const char* subdirectory = "set_3/";
*/
/*
set 4 details:
#define SEED 100
#define GRAPH_FILES 100
#define NODE_MIN 1000
#define NODE_MAX 2500
const char* subdirectory = "set_4/";
*/
/*set 5 details:*/
#define SEED 101
#define GRAPH_FILES 100
#define NODE_MIN 2500
#define NODE_MAX 5000
const char* subdirectory = "set_5/";



const char* directory = "graphs/";
const char* file_name = "graph_";
const char* ext = ".txt";

int main(int argc, char *argv[]){

  int node_range = NODE_MAX - NODE_MIN;
  //testing on sparse graphs is not representative
  long long int edge_min = NODE_MIN << 1;
  long long int edge_max = NODE_MAX * (NODE_MAX - 1);
  int current_file = 0;
  char graph_name[256];

  int node_count, edge_count, edge_out, edge_in, edge_range;

  srand(SEED);

  while(current_file < GRAPH_FILES){
    node_count = (rand() % node_range) + NODE_MIN;
    edge_range = (node_count * (node_count - 1)) - (node_count << 1);
    edge_count = (rand() % edge_range) + edge_min;

    // TODO : Add error checking
    sprintf(graph_name, "%s%s%s%d%s", directory, subdirectory, file_name, current_file++, ext);
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
