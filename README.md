ID: 20sdc2

# Parallel Processing
## Exercises with processes

This repository includes 5 programs, covering 4 tasks. These include: pargrep.c, count.c, recurse.c, client.c, and server.c. Compilation of all programs assumes your pwd is the directoy associated with the program (e.g. to compile parallel grep with the below instruction, pwd must be /pargrep_d/)

### Parallel Grep

The pargrep.c program parallelizes the system call grep. Therefore, given a search-term and a number of files, the program will create one child process per file which seeks for, and prints, each line of its file containing the search-term.

This program can be compiled by using make or by entering the following command in the command line:
  - gcc -g -Wall -o pargrep pargrep.c

This program can then be executed as follows:
  - ./pargrep {search-term} {file 1} {file 2} ...

An example execution:
  - Input:
    - ./pargrep file test1.txt
  - Output:
    - test1.txt: file
    - test1.txt: wait just kidding im adding morefilestuff here
    - test1.txt: a long line with the word file in it
    - test1.txt: end of file
    - test1.txt: wow so many files!

### Count

The count.c program parallelizes the process of counting the number of times {search-term} occurs in {file}. Capitalization and punctuation are ignored during this search, but the lexicographical comparison must be exact.

This program can be compiled by using make or by entering the following command in the command line:
  - gcc -g -Wall -o count count.c

This program can then be executed as follows:
  - ./count {search-term} {file}

An example execution:
  - Input:
    - ./count file test1.txt
  - Output:
    - count: 3

### Recurse

The recurse.c program parallelizes the summation of the whole numbers from 1 to {input-number}. A separate process is created for each numbers from 1 to {input-number} in computing this sum.

This program can be compiled by using make or by entering the following command in the command line:
  - gcc -g -Wall -o recurse recurse.c

This program can then be executed as follows:
  - ./recurse {input-number}

An example execution:
  - Input:
    - ./recurse 10
  - Output:
    - The sum of 1 to 10 is 55

### Client-Server

The client.c and server.c programs parallelize the random generation of sentences (i.e. Mad Libs). The client will send {Proper-noun} to the server, and the server will return a sentence of the following form to the client:
  - {Proper-noun} {verb} {preposition} the {adjective} {noun}!

NOTE: server.c includes a definition of IDLE_TIME, currently set to 3 (seconds). After the server processes a message, it will wait IDLE_TIME before trying to read from the client-server FIFO again. IDLE_TIME after server has read a message, if there is no new message for server to read, the server will close. IDLE_TIME can be adjusted or removed as desired by the user.

These programs can be compiled by using make or by entering the following commands in the command line:
  - gcc -g -Wall -o client client.c
  - gcc -g -Wall -o server server.c

These programs can then be executed as follows (order of execution does not matter, but recall the above note about server closing IDLE_TIME after its last message received!):
  - ./client {Proper-noun}
  - ./server

An example execution:
  - Input:
    - ./server
    - ./client Kelly
  - Output {printed by client}:
    - Kelly steered across the pink cucumber!
