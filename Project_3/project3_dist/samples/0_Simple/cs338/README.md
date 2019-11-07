## Project 3 : Check Point
#### 20sdc2

This is my check-in submission for Project 3, Part 1 for CS338. The associated file, cs338Blur.cu will blur an input image, taking advantage of a CUDA device to parallelize the blurring task. To run this code, do the follow in the command line:

* $ make
* $ ./cs338Blur {input.jpg} {output.jpg}

This program yields a blurred output image based on the input.jpg supplied.

This program is a work in progress. Updates should be made to allow for user-specified blur-radius parameters, and there is an issue causing me to need to hard-code the k-value in a certain location to 3, which would impact the efficiency (but not functionality) of the program on a greyscale image.

The definition of BLOCK_SIZE is currently set to 32. This defines the block-dimension of our kernel (it is a BLOCK_SIZE by BLOCK_SIZE square), and this necessarily influences the grid dimensions (which is also a square of sides of ceil(max_of_width_and_height) / BLOCK_SIZE)
