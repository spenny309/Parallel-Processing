20sdc2

# Part 1: Image Blurring
Part one contains an autotest which will compile 4 versions of cs338-prog2.c, with the appropriate definitions for the four methodologies of blurring. These can be specified manually by defining ROW_MAJOR and BLOCK_ORDER. If ROW_MAJOR is not defined, column-major will be used, if BLOCK_ORDER is not defined, interleaved will be used. You must specify a number of runs and a number of processors, as well as an input image and a desired output image file for the program to work. An example usage, using the row_block makefile:

make --f row_block.txt
./prog2_rb 2 8 pictures/peacock.jpg output.jpg

This will compile the code with row_block definitions, then blur the image with 8 processors, and do 2 runs.

# Part 2: Pixel Counting
In this part, we used row-major block division for counting, as the results from part one did not suggest that one of the four methods was significantly better than any of the others. The code for this section also includes an autotest which will compile 5 different versions of the program and run them. The code can be manually defined as desired by compiling with a definition of INDIV_LOCKS, BUCKET_LOCKS, UNI_LOCK or NO_LOCKS. If none are defined, the default is LOCAL HISTOGRAMS, the fastest method with perfect accuracy. An example run is below, using BUCKET_LOCKS:

make --f make_bucket
./prog2_bucket_locks 2 8 pictures/peacock.jpg nulloutput.jpg

This will count the number of each color pixel in peacock, and output those to outputs/output_bucket_locks.txt and outputs/output_bucket_locks.csv.
WARNING: folder: ./outputs/ must exist!

## Results are discussed thoroughly in write_up_final.pdf, and code comments further describe functionality and decision paradigms. Directories for part one and part two also contain more data and outputs from test runs and used for the final write up! :)
