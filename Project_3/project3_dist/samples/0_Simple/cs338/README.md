## Project 3 : Final submission
#### 20sdc2

This is my final submission for Project 3 for CS338. There are 3 associated files taking advantage of a CUDA device to parallelize an image blurring task.

These files are:
* cs338Blur_unopt.cu    (Part 1)
* cs338Blur_branch.cu   (Part 2)
* cs338Blur_precal.cu   (Part 3)

WARNING: The Makefile, and variations of the Makefile (make1, make2, make4, ..., make32) included in this directory can only be used on a file named cs338Blur.cu. Therefore, in order to use one of the files, you must issue the following command before trying to compile it:

$ cp cs338Blur_{version}.cu cs338Blur.cu

This will create a copy of the desired version into a file named cs338Blur.cu. Next, a user can compile with the following (NOTE: make will default to block dimensions of 32 threads by 32 threads).

* $ make
* Output: cs338Blur.o && cs338Blur*

A user can also specify a makefile that will determine the block dimensions of the program, with the options of: 1, 2, 4, 8, 16, or 32.

* $ make --f make{dimension}
* Output: cs338Blur_{dimension}.o && cs338Blur_{dimension}*

The program may then be run as follows, to blur an input jpeg, and store the result in an output jpeg:

* $ ./cs338Blur {input.jpg} {output.jpg}
OR:
* $ ./cs338Blur_{dimension} {input.jpg} {output.jpg}

Also included in this directory is an autotest executable which will run all of the tests performed for the final paper of this assignment. Autotest will create a file, output.txt, delete its contents, then compile and execute 1, 2, 4, 8, 16, and 32 thread-dimensioned block versions of the unoptimized, branch-optimized, and fully-optimized code on 3 different images: twotone.jpg, tree.jpg, and redtree.jpg. The executables are then cleaned up from the filesystem.

The radial parameter of each file is defined as .05. If a user wants to change this value, they must update the defined value of RADIAL_PARAM.

Also included in this directory is a final paper, final outputs (output and nohup), a directory p2_distribute from Professor Kelly Shaw, which contains the 3 test images used by autotest.
