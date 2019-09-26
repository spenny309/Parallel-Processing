# Parallel Processing
## Exercises with processes

This repository includes 5 programs, covering 4 tasks. These include: pargrep.c, count.c, recurse.c, client.c, and server.c.

### Parallel Grep

The pargrep.c program parallelizes the system call grep. Therefore, given a search-term and a number of files, the program will create one child process per file which seeks for, and prints, each line of its file containing the search-term.

This program can be compiled by {} or by entering the following command in the command line:
  - gcc -g -o pargrep pargrep.c

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

This program can be compiled by {} or by entering the following command in the command line:
  - gcc -g -o count count.c

This program can then be executed as follows:
  - ./count {search-term} {file}

An example execution:
  - Input:
    - ./count file test1.txt
  - Output:
    - count: 3
