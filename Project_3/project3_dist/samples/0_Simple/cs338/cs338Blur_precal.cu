// Fully optimized blur code with branch prediction and pre-calculation
/**
 * Copyright 1993-2015 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */


#include <stdio.h>

// For the CUDA runtime routines (prefixed with "cuda_")
#include <cuda_runtime.h>

#include <helper_cuda.h>



////////////////////////////////////////////////////////////////////////////////


#include "jpeglib.h"

#include <string.h>

/*
 * IMAGE DATA FORMATS:
 *
 * The standard input image format is a rectangular array of pixels, with
 * each pixel having the same number of "component" values (color channels).
 * Each pixel row is an array of JSAMPLEs (which typically are unsigned chars).
 * If you are working with color data, then the color values for each pixel
 * must be adjacent in the row; for example, R,G,B,R,G,B,R,G,B,... for 24-bit
 * RGB color.
 */

/* The "frame structure" structure contains an image frame (in RGB or grayscale
 * formats) for passing around the CS338 projects.
 */
typedef struct frame_struct
{
  JSAMPLE *image_buffer;	/* Points to large array of R,G,B-order/grayscale data
                             * Access directly with:
                             *   image_buffer[num_components*pixel + component]
                             */
  JSAMPLE **row_pointers;	/* Points to an array of pointers to the beginning
                             * of each row in the image buffer.  Use to access
                             * the image buffer in a row-wise fashion, with:
                             *   row_pointers[row][num_components*pixel + component]
                             */
  int image_height;		/* Number of rows in image */
  int image_width;		/* Number of columns in image */
  int num_components;	/* Number of components (usually RGB=3 or gray=1) */
} frame_struct_t;
typedef frame_struct_t *frame_ptr;


#ifdef BLOCK
  #define BLOCK_SIZE BLOCK
#else
  //default block_size
  #define BLOCK_SIZE 32.0
#endif

//CHANGE TO UPDATE RADIUS
#define RADIAL_PARAM 0.05f


#define MAXINPUTS 1
#define MAXOUTPUTS 1
frame_ptr input_frames[MAXINPUTS];	/* Pointers to input frames */
frame_ptr output_frames[MAXOUTPUTS];	/* Pointers to output frames */

/* Read/write JPEGs, for program startup & shutdown */
/* YOU SHOULD NOT NEED TO USE THESE AT ALL */
void write_JPEG_file (char * filename, frame_ptr p_info, int quality);
frame_ptr read_JPEG_file (char * filename);

/* Allocate/deallocate frame buffers, USE AS NECESSARY! */
frame_ptr allocate_frame(int height, int width, int num_components);
void destroy_frame(frame_ptr kill_me);

/*
 * write_JPEG_file writes out the contents of an image buffer to a JPEG.
 * A quality level of 2-100 can be provided (default = 75, high quality = ~95,
 * low quality = ~25, utter pixellation = 2).  Note that unlike read_JPEG_file,
 * it does not do any memory allocation on the buffer passed to it.
 */

void write_JPEG_file (char * filename, frame_ptr p_info, int quality)
{
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  FILE * outfile;		/* target file */

  /* Step 1: allocate and initialize JPEG compression object */
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);

  /* Step 2: specify data destination (eg, a file) */
  /* Note: steps 2 and 3 can be done in either order. */

  if ((outfile = fopen(filename, "wb")) == NULL) {
    fprintf(stderr, "ERROR: Can't open output file %s\n", filename);
    exit(1);
  }
  jpeg_stdio_dest(&cinfo, outfile);

  /* Step 3: set parameters for compression */

  /* Set basic picture parameters (not optional) */
  cinfo.image_width = p_info->image_width; 	/* image width and height, in pixels */
  cinfo.image_height = p_info->image_height;
  cinfo.input_components = p_info->num_components; /* # of color components per pixel */
  if (p_info->num_components == 3)
    cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */
  else if (p_info->num_components == 1)
    cinfo.in_color_space = JCS_GRAYSCALE;
  else {
    fprintf(stderr, "ERROR: Non-standard colorspace for compressing!\n");
    exit(1);
  }
  /* Fill in the defaults for everything else, then override quality */
  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

  /* Step 4: Start compressor */
  jpeg_start_compress(&cinfo, TRUE);

  /* Step 5: while (scan lines remain to be written) */
  /*           jpeg_write_scanlines(...); */
  while (cinfo.next_scanline < cinfo.image_height) {
    (void) jpeg_write_scanlines(&cinfo, &(p_info->row_pointers[cinfo.next_scanline]), 1);
  }

  /* Step 6: Finish compression & close output */

  jpeg_finish_compress(&cinfo);
  fclose(outfile);

  /* Step 7: release JPEG compression object */
  jpeg_destroy_compress(&cinfo);
}


/*
 * read_JPEG_file reads the contents of a JPEG into an image buffer, which
 * is automatically allocated after the size of the image is determined.
 * We want to return a frame struct on success, NULL on error.
 */

frame_ptr read_JPEG_file (char * filename)
{
  /* This struct contains the JPEG decompression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   */
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  FILE * infile;		/* source file */
  frame_ptr p_info;		/* Output frame information */

  //  JSAMPLE *realBuffer;
  //  JSAMPLE **buffer;		/* Output row buffer */
  //  int row_stride;		/* physical row width in output buffer */

  /* Step 1: allocate and initialize JPEG decompression object */
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);

  /* Step 2: open & specify data source (eg, a file) */
  if ((infile = fopen(filename, "rb")) == NULL) {
    fprintf(stderr, "ERROR: Can't open input file %s\n", filename);
    exit(1);
  }
  jpeg_stdio_src(&cinfo, infile);

  /* Step 3: read file parameters with jpeg_read_header() */
  (void) jpeg_read_header(&cinfo, TRUE);

  /* Step 4: use default parameters for decompression */

  /* Step 5: Start decompressor */
  (void) jpeg_start_decompress(&cinfo);

  /* Step X: Create a frame struct & buffers and fill in the blanks */
  fprintf(stderr, "  Opened %s: height = %d, width = %d, c = %d\n",
      filename, cinfo.output_height, cinfo.output_width, cinfo.output_components);
  p_info = allocate_frame(cinfo.output_height, cinfo.output_width, cinfo.output_components);

  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */
  while (cinfo.output_scanline < cinfo.output_height) {
    (void) jpeg_read_scanlines(&cinfo, &(p_info->row_pointers[cinfo.output_scanline]), 1);
  }

  /* Step 7: Finish decompression */
  (void) jpeg_finish_decompress(&cinfo);

  /* Step 8: Release JPEG decompression object & file */
  jpeg_destroy_decompress(&cinfo);
  fclose(infile);

  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
   */

  /* And we're done! */
  return p_info;
}


/*
 * allocate/destroy_frame allocate a frame_struct_t and fill in the
 *  blanks appropriately (including allocating the actual frames), and
 *  then destroy them afterwards.
 */

frame_ptr allocate_frame(int height, int width, int num_components)
{
  int row_stride;		/* physical row width in output buffer */
  int i;
  frame_ptr p_info;		/* Output frame information */

  /* JSAMPLEs per row in output buffer */
  row_stride = width * num_components;

  /* Basic struct and information */
  if ((p_info = (frame_struct_t*)malloc(sizeof(frame_struct_t))) == NULL) {
    fprintf(stderr, "ERROR: Memory allocation failure\n");
    exit(1);
  }
  p_info->image_height = height;
  p_info->image_width = width;
  p_info->num_components = num_components;

  /* Image array and pointers to rows */
  if ((p_info->row_pointers = (JSAMPLE**)malloc(sizeof(JSAMPLE *) * height)) == NULL) {
    fprintf(stderr, "ERROR: Memory allocation failure\n");
    exit(1);
  }
  if ((p_info->image_buffer = (JSAMPLE*)malloc(sizeof(JSAMPLE) * row_stride * height)) == NULL) {
    fprintf(stderr, "ERROR: Memory allocation failure\n");
    exit(1);
  }
  for (i=0; i < height; i++)
  	p_info->row_pointers[i] = & (p_info->image_buffer[i * row_stride]);

  /* And send it back! */
  return p_info;
}

void destroy_frame(frame_ptr kill_me)
{
	free(kill_me->image_buffer);
	free(kill_me->row_pointers);
	free(kill_me);
}


void usage()
{
  fprintf(stderr, "ERROR: Need to specify input file and then output file\n");
  exit(1);
}


/* Makes sure values match in the two images*/
void checkResults(frame_ptr f1, frame_ptr f2)
{
  int i, j, k;

  if(f1->image_height != f2->image_height && f1->image_width != f2->image_width
		&& f1->num_components != f2->num_components){
	fprintf(stderr, "Dimensions do not match\n");
	exit(1);
  }

  for (i=0; i < f1->image_height; i++){
    for (j=0; j < f1->image_width; j++){
      for (k=0; k < f1->num_components; k++){
		JSAMPLE j1 = f1->row_pointers[i][(f1->num_components)*j+k];
		JSAMPLE j2 = f2->row_pointers[i][(f2->num_components)*j+k];
		if(j1 != j2){
			fprintf(stderr, "Values do not match at (%d, %d, %d) \n", i, j, k);
			fprintf(stderr, "from %d\n", j1);
			fprintf(stderr, "to %d\n", j2);
			exit(1);
		}
      }
    }
  }

}

void runKernel(frame_ptr result);



/*
 * This is just a helper method. It should call runKernel to set up and
 * invoke the kernel.  It should then also call the uniprocessor version
 * of your blurring code (which does not need to be optimized) and
 * check for correctness of your kernel code.
 */
void
runTest( int argc, char** argv)
{

  frame_ptr from = input_frames[0];
  // Allocate frame for kernel to store its results into
  output_frames[0] = allocate_frame(from->image_height, from->image_width, from->num_components);

  // call kernel
  runKernel(output_frames[0]);

  // TODO : {easy} - invoke uniprocessor version and check results of kernel
                   //to uniprocessor version

}



/**
 * CUDA Kernel Device code
 * This is code for blurring a single pixel
 *
*/
    //VERSION 3: Block Branching approach with Pre-calculated values
__global__ void cs338Blur(unsigned char* from, unsigned char* to, int r,
  int height, int width, int k, int * weight_matrix, long pre_calculated_divisor)
  {
    long col = (blockIdx.x * blockDim.x + threadIdx.x);
    long row = (blockIdx.y * blockDim.y + threadIdx.y);
    //If current pixel is invalid, do nothing {col && row cann never be < 0, so no need to check}
    if(col >= width || row >= height) {
      return;
    }
    long this_pixel = (row * width * k) + col * k;
    // Wastes space, but still works on greyscale images
    long blurred_pixels[3] = { 0 };
    int col_neighbor;
    int row_neighbor;
    int curr_dimension;
    int current_neighbor;
    int min_of_height_and_width = min(height, width);
    long weight_divisor = pre_calculated_divisor;
    //If we're in an edge case, use boundary checking, else assume we have r+ neighbors in each directions
    if((blockIdx.x * blockDim.x) < r || ((1 + blockIdx.x) * blockDim.x) > width - r || (blockIdx.y * blockDim.y) < r || ((1 + blockIdx.y) * blockDim.y) > height - r){
      int local_weight;
      weight_divisor = 0;
      //For this pixel, find all valid neighbors and calculate weights and values
      for(row_neighbor = (1 + row - r) ; row_neighbor < row + r ; row_neighbor++){
        for(col_neighbor = (1 + col - r) ; col_neighbor < col + r ; col_neighbor++){
          //Check bounds to ensure validity
          if(row_neighbor >= 0 && col_neighbor >= 0 && row_neighbor < height && col_neighbor < width){
            //Weight adjustment based on abs distance from this_pixel
            local_weight = (r - abs(row - row_neighbor)) * (r - abs(col - col_neighbor));
            weight_divisor += local_weight;
            //current_neighbor = location of R value in RGB
            current_neighbor = (row_neighbor * width * k) + (col_neighbor * k);
            for(curr_dimension = 0 ; curr_dimension < k ; curr_dimension++) {
              blurred_pixels[curr_dimension] += from[current_neighbor + curr_dimension] * local_weight;
            }
          }
        }
      }
    } else {
      //No need for bounds checks in this else case
      for(row_neighbor = (1 + row - r) ; row_neighbor < row + r ; row_neighbor++){
        for(col_neighbor = (1 + col - r) ; col_neighbor < col + r ; col_neighbor++){
          //current_neighbor = location of R value in RGB
          current_neighbor = (row_neighbor * width * k) + (col_neighbor * k);
          for(curr_dimension = 0 ; curr_dimension < k ; curr_dimension++) {
            //use pre-calculated weight matrix to determine weight of current neighbor on blur of current pixel
            blurred_pixels[curr_dimension] += from[current_neighbor + curr_dimension] * weight_matrix[(abs(row - row_neighbor) * r) + abs(col - col_neighbor)];
          }
        }
      }
    }

    //Check for divide by 0 errors {should NEVER trip unless error}
    if(weight_divisor == 0){
      return;
    }
    //Calculate blurred pixel values
    for(curr_dimension = 0 ; curr_dimension < k ; curr_dimension++) {
      to[this_pixel + curr_dimension] = (unsigned char) (blurred_pixels[curr_dimension] / weight_divisor);
    }
    return;
  }

  // UNCOMMENT FOR TEST OUTPUT TXT
  // // Insert this function before main
  // void kelly_write_file(char *fname)
  // {
  //   char *name = (char*)malloc(strlen(fname) + 5);
  //   strcpy(name, fname);
  //   strcat(name, ".out");
  //
  //   FILE *file = fopen(name, "w");
  //   if(file == 0){
  //     printf("Unable to open %s\n", name);
  //   }
  //   else{
  //     int i, j, k;
  //     frame_ptr to;
  //
  //     to = output_frames[0];
  //
  //     for (i=0; i < to->image_height; i++){
  //       for (j=0; j < to->image_width; j++){
  //         for (k=0; k < to->num_components; k++){
  //                   fprintf(file, "%d ", to->row_pointers[i][(to->num_components)*j+k]);
  //         }
  //       }
  //       fprintf(file, "\n");
  //     }
  //   }
  //   fclose(file);
  //   free(name);
  // }

/**
 * Host main routine
 */
int
main(int argc, char **argv)
{

  if(argc < 3){
    usage();
    exit(1);
  }

  // Load input file
  input_frames[0] = read_JPEG_file(argv[1]);

  // Do the actual work including calling CUDA kernel
  runTest(argc, argv);

  // Write output file
  write_JPEG_file(argv[2], output_frames[0], 75);

  // UNCOMMENT FOR TEST OUTPUT TXT
  //frame_ptr compare_to_me = read_JPEG_file(argv[3]);
  //kelly_write_file(argv[3]);

  return 0;
}

//********************************************************************************************************************************************

// This sets up GPU device by allocating the required memory and then
// calls the kernel on GPU. (You might choose to add/remove arguments.)
// It's currently set up to use the global variables and write its
// final results into the specified argument.
void
runKernel(frame_ptr result)
{
  frame_ptr from = input_frames[0];
  int picture_height = from->image_height;
  int picture_width = from->image_width;
  int picture_components = from->num_components;
  long array_size_for_memory = picture_width * picture_height * picture_components * sizeof(char);
  int * weight_matrix;
  long pre_calculated_divisor = 0;
  int max_of_width_and_height = (picture_height > picture_width) ? picture_height : picture_width;
  int radius = ceil(max_of_width_and_height * RADIAL_PARAM);
  cudaEvent_t start, stop;
  if (cudaEventCreate(&start) != cudaSuccess){
    fprintf(stderr, "ERROR: Failed to create CUDA start\n");
    exit(1);
  }
  if (cudaEventCreate(&stop) != cudaSuccess){
    fprintf(stderr, "ERROR: Failed to create CUDA stop\n");
    exit(1);
  }

  //Allocate one dimensional array for input picture pixels
  unsigned char *image_as_one_dimensional_array;
  image_as_one_dimensional_array = (unsigned char*)malloc(array_size_for_memory);
  if (image_as_one_dimensional_array == NULL){
    fprintf(stderr, "ERROR: Memory allocation failure\n");
    exit(1);
  }

  //Allocate one dimensional array for output picture pixels
  JSAMPLE *output_as_one_dimensional_array;
  output_as_one_dimensional_array = (unsigned char*)malloc(array_size_for_memory);
  if (output_as_one_dimensional_array == NULL){
    fprintf(stderr, "ERROR: Memory allocation failure\n");
    exit(1);
  }

  //Fill input array with picture pixels (row major), and set output array to 255 [white]
  int offset = 0;
  for(int i = 0 ; i < picture_height ; i++){
    for(int j = 0 ; j < picture_width ; j++){
      for(int k = 0 ; k < picture_components ; k++){
        offset = (i * picture_width * picture_components) + (j * picture_components) + k;
        image_as_one_dimensional_array[offset] = from->row_pointers[i][(j * picture_components) + k];
        output_as_one_dimensional_array[offset] = 255;
      }
    }
  }

  //Allocate device memory and transfer input data and output array
  unsigned char* d_image_as_one_dimensional_array;
  unsigned char* d_output_as_one_dimensional_array;
  if (cudaMalloc((void **) &d_image_as_one_dimensional_array, array_size_for_memory) != cudaSuccess){
    fprintf(stderr, "ERROR: CUDA memory allocation failure\n");
    exit(1);
  }
  if (cudaMemcpy(d_image_as_one_dimensional_array, image_as_one_dimensional_array, array_size_for_memory, cudaMemcpyHostToDevice) != cudaSuccess){
    fprintf(stderr, "ERROR: CUDA memory copy failure\n");
    exit(1);
  }

  if (cudaMalloc((void **) &d_output_as_one_dimensional_array, array_size_for_memory) != cudaSuccess){
    fprintf(stderr, "ERROR: CUDA memory allocation failure\n");
    exit(1);
  }
  if (cudaMemcpy(d_output_as_one_dimensional_array, output_as_one_dimensional_array, array_size_for_memory, cudaMemcpyHostToDevice) != cudaSuccess){
    fprintf(stderr, "ERROR: CUDA memory copy failure\n");
    exit(1);
  }

  //Allocate weight matrix for pre-calculations of inner-pixels
  int weight_matrix_size = sizeof(int) * (radius * radius);
  weight_matrix = (int *)calloc(1, weight_matrix_size);
  if (weight_matrix == NULL){
    fprintf(stderr, "ERROR: Memory allocation failure\n");
    exit(1);
  }
  //Pre-calculate divisor and weight_matrix via simple maths
	for (int i = 0; i < radius; i++){
		for (int j = 0; j < radius; j++){
      weight_matrix[(i*radius) + j] = (radius - i) * (radius - j);
      if (i > 0 && j > 0) { //the 4* covers the 4 quadrant equivalents of i,j
        pre_calculated_divisor += 4 * ((radius - i) * (radius - j));
      } else if (i > 0 || j > 0) { //the 2* covers the 2 axes equivalents of i,j
        pre_calculated_divisor += 2 * ((radius - i) * (radius - j));
      } else { // the 1* covers the one origin at i,j = 0,0
        pre_calculated_divisor += (radius - i) * (radius - j);
      }
		}
	}

  //Create a device copy of weight matrix
  int* d_weight_matrix;
  if (cudaMalloc((void **) &d_weight_matrix, weight_matrix_size) != cudaSuccess){
    fprintf(stderr, "ERROR: CUDA memory allocation failure\n");
    exit(1);
  }
  if (cudaMemcpy(d_weight_matrix, weight_matrix, weight_matrix_size, cudaMemcpyHostToDevice) != cudaSuccess){
    fprintf(stderr, "ERROR: CUDA memory copy failure\n");
    exit(1);
  }

  //Kernel invocation with dimensionality
  dim3 dim_grid(ceil(picture_width / BLOCK_SIZE), ceil(picture_height / BLOCK_SIZE), 1);
  dim3 dim_block(BLOCK_SIZE, BLOCK_SIZE, 1);

  cudaEventRecord(start);
  cs338Blur<<<dim_grid, dim_block>>>(d_image_as_one_dimensional_array, d_output_as_one_dimensional_array, radius, picture_height, picture_width, picture_components, d_weight_matrix, pre_calculated_divisor);
  cudaEventRecord(stop);

  //Collect results with Device to Host memcpy
  if (cudaMemcpy(output_as_one_dimensional_array, d_output_as_one_dimensional_array, array_size_for_memory, cudaMemcpyDeviceToHost) != cudaSuccess){
    fprintf(stderr, "ERROR: CUDA memory copy failure\n");
    exit(1);
  }

  cudaEventSynchronize(stop);

  float milliseconds = 0;
  cudaEventElapsedTime(&milliseconds, start, stop);

  //Transform into 2D array
  //Fill output image with pixels from cudaMemcpy
  for(int i = 0 ; i < picture_height ; i++){
    for(int j = 0 ; j < picture_width ; j++){
      for(int k = 0 ; k < picture_components ; k++){
        offset = (i * picture_width * picture_components) + (j * picture_components) + k;
        result->row_pointers[i][(j * picture_components) + k] = output_as_one_dimensional_array[offset];
      }
    }
  }

  printf("Kernal runtime: %10.2f milliseconds\t\tBlock size: %2.1f\n", milliseconds, BLOCK_SIZE);
  free(weight_matrix);
  free(image_as_one_dimensional_array);
  free(output_as_one_dimensional_array);
  cudaFree(d_weight_matrix);
  cudaFree(d_image_as_one_dimensional_array);
  cudaFree(d_output_as_one_dimensional_array);
}
