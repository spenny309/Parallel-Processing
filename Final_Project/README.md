## Final Project: Parallel PageRank in C
### Williams College – CSCI 338: Parallel Processing – Fall 2019
#### 20sdc2

This is the final submission of my final independent project for Parallel Processing, where I have chosen to implement a parallelized PageRank in C. This directory contains a variety of executables, testing materials, and testing outputs.
---
No program in this file takes command-line arguments in order to run. As such, programs simply need to be compiled and then can be run. PageRank implementations require the specified sets of input graphs to already be generated in the directory, otherwise execution of any such program will fail. The graph generating program requires there to already be a directory "graphs/" in the current directory, and the specified subdirectory must exist within "graphs/". the autotest executable can test all of the PageRank executions.

A sample usage:

make --f makeserial
./pr-serial

{will execute serial PageRank on 500 test graphs: graphs 0-99 each in sets 1-5}

---
Contents of this directory:
* Original project proposal – final_proposal.pdf
* Final project paper – Parallelization of the PageRank Algorithm.pdf
* Tiny sample graph – sample.txt
* C Files:
  * Graph generation program – graph-generator.c
  * Serial PageRank – pagerank_serial.c
  * Row-Block PageRank – pagerank_rowblock.c
  * Row-Interleaved PageRank – pagerank_rowint.c
  * Column-Block PageRank – pagerank_colblock.c
  * Column-Interleaved PageRank – pagerank_colint.c
  * First Optimized PageRank – pagerank_opt1.c
  * Second Optimized PageRank – pagerank_opt2.c
  * Final Optimized PageRank – pagerank_opt3.c
* Makefiles:
  * Default makefile, builds Serial PageRank – Makefile
  * Graph generation program – makegraphgen
  * Serial PageRank – makeserial
  * Row-Block PageRank – makerb
  * Row-Interleaved PageRank – makeri
  * Column-Block PageRank – makecb
  * Column-Interleaved PageRank – makeci
  * First Optimized PageRank – makeop1
  * Second Optimized PageRank – makeop2
  * Final Optimized PageRank – makeop3
* Other Executables:
  * Compile and test all implementations – autotest
  * Make all implementations – makeall
  * Clean all implementations – cleanall
* Graphs:
  * Tiny sample test – sample.txt
  * Directory of 5 large graph sets [each set contains 100 large graphs] – graphs/
* Outputs:
  * Output from all test – test_outputs/
  * Images generated from outputs – test_outputs/data_sheet_files/\*.png
