/*
 * cs338-prog2.c
 * Created by Lance Hammond and Kunle Olukotun for CS315a at Stanford University
 * This file provides the "shell" for the first programming assignment
 * for PA1.  It reads in one or more JPEG files at the start, leaving
 * them in buffers that the students can manipulate, and then can compress
 * one or more output files so that results may be compared with input.
 * Currently, the earlier input(s) are input files, and the later ones
 * are output files.
 */

/******************** CONTROLLABLE PARAMETERS *******************/

/* Define this if you want to set the # of runs from the command line.
 * This will be a numerical argument, the first one after the command.
 * Also, feel free to define NO_FIRST if you want to discard the performance
 * of the first run (most likely).
 */
#define USE_NUM_RUNS
/* #define NO_FIRST */

/* Define this if you want to set the # of processors from the command line.
 * This will be a numerical argument, either right after the command or
 * right after the # of runs.
 */
#define USE_NUM_PROCS

/* The following parameters adjust the tolerance of the timing-correction
 * code (0.1 = 10% tolerance, etc.) and the number of out-of-tolerance
 * runs the system will attempt before giving up.  The test delay parameter
 * should be set to check tolerances for at least a couple of seconds.
 */
#define MAX_TOLERANCE_ERRORS	4
#define TOLERANCE				0.1
#define TEST_DELAY				100000000

/* The number of files can be adjusted with the following parameters: */

/* For PA1, you'll need to set this to 1 for Problems 1 & 2, 2 for P.3 */
#define NUM_INPUTS		1
/* If NUM_INPUTS == 0, use these to control a variable # of inputs: */
/*   Not actually necessary for PA1, but could be helpful */
#define MIN_INPUTS		1
#define MAX_INPUTS		1

/* NUM_OUTPUTS is always fixed.  For PA1, 1 for P.1 & 3, 0 for P.2 */
#define NUM_OUTPUTS		1

/* A quality level of 2-100 can be provided (default = 75, high quality = ~95,
 * low quality = ~25, utter pixellation = 2).  This controls the degree of
 * compression at the output.
 */
#define OUT_QUALITY		75

/************************* END OF PARAMETERS *******************/

#ifdef USE_NUM_RUNS
	#define RUNS_COMMAND	1
#else
	#define RUNS_COMMAND	0
#endif

#ifdef USE_NUM_PROCS
	#define RUNS_PROCS		1
#else
	#define RUNS_PROCS		0
#endif

#define BLUR_PCT .05

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include "jpeglib.h"
#include <string.h>
#include <errno.h>

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

/* These arrays of frame structure pointers point to all of the frames that
 * are read at the beginning of the program or written at the end.  Note
 * that you can simply move input frame pointers to output frame pointers if
 * you do "in-place" transformations (leaving output data right in the input
 * buffers).
 */
frame_ptr input_frames[MAX_INPUTS];	/* Pointers to input frames */
frame_ptr output_frames[NUM_OUTPUTS];	/* Pointers to output frames */
int num_procs;		/* Number of processors, for parallel use */

unsigned long rHist[256];
unsigned long gHist[256];
unsigned long bHist[256];
unsigned long sHist[768];

#if defined(INDIV_LOCKS)
#define LOCKS_ON 1
pthread_mutex_t redLock[256];
pthread_mutex_t greenLock[256];
pthread_mutex_t blueLock[256];
pthread_mutex_t sumLock[768];
int lock_count = 256;
char * output_txt = "outputs/output_indiv_locks.txt";
char * output_csv = "outputs/output_indiv_locks.csv";

#elif defined(BUCKET_LOCKS)
#define LOCKS_ON 1
pthread_mutex_t redLock[8];
pthread_mutex_t greenLock[8];
pthread_mutex_t blueLock[8];
pthread_mutex_t sumLock[24];
int lock_count = 8;
char * output_txt = "outputs/output_bucket_locks.txt";
char * output_csv = "outputs/output_bucket_locks.csv";

#elif defined(UNI_LOCK)
#define LOCKS_ON 1
pthread_mutex_t redLock[1];
pthread_mutex_t greenLock[1];
pthread_mutex_t blueLock[1];
/*
	As with the INDIV_LOCKS sumLock which has 2 superfluous locks, code
	consistency/maths is more simple with num(sumLocks) == 3 * num(redLocks)
*/
pthread_mutex_t sumLock[3];
int lock_count = 1;
char * output_txt = "outputs/output_big_lock.txt";
char * output_csv = "outputs/output_big_lock.csv";

#elif defined(NO_LOCKS)
int lock_count = 0;
char * output_txt = "outputs/output_no_locks.txt";
char * output_csv = "outputs/output_no_locks.csv";

#else
int lock_count = 0;
char * output_txt = "outputs/output_local_hists.txt";
char * output_csv = "outputs/output_local_hists.csv";
struct local_histogram {
	long processor_number;
	unsigned long local_r_hist[256];
	unsigned long local_g_hist[256];
	unsigned long local_b_hist[256];
	unsigned long local_s_hist[768];
};

#endif

/* Function prototypes */

/* Your code */
void *CS338_row_seq(void *proc_num);
void CS338_function();

/* Read/write JPEGs, for program startup & shutdown */
/* YOU SHOULD NOT NEED TO USE THESE AT ALL */
void write_JPEG_file (char * filename, frame_ptr p_info, int quality);
frame_ptr read_JPEG_file (char * filename);

/* Allocate/deallocate frame buffers, USE AS NECESSARY! */
frame_ptr allocate_frame(int height, int width, int num_components);
void destroy_frame(frame_ptr kill_me);


/******************** START OF ASSIGNMENT SECTION *******************/

/* While you're free to tweak other parts of this file, most of your
 * new code should be located here (or in another file, if it's large).
 */

//row-major order
void *CS338_row_seq(void *local_hist_or_proc_num){
	int i, j, k;
	frame_ptr from;
	output_frames[0] = from = input_frames[0];
	long thread_num = -1;
	int r, g, b;

	#if defined(INDIV_LOCKS) || defined(BUCKET_LOCKS) || defined(UNI_LOCK) || defined(NO_LOCKS)
	thread_num = (long)local_hist_or_proc_num;

	#else
	struct local_histogram *local_data = (struct local_histogram*) local_hist_or_proc_num;
	thread_num = (*local_data).processor_number;

	#endif

	#if defined(INDIV_LOCKS) || defined(BUCKET_LOCKS) || defined(UNI_LOCK)
	//for all height and width from radius...
	for(i = thread_num * (from->image_height / num_procs); i < (1 + thread_num) * (from->image_height / num_procs); i++){
		for(j = 0; j < from->image_width; j++){

			r = from->row_pointers[i][j*3];
			pthread_mutex_lock(&redLock[r % lock_count]);
			rHist[r]++;
			pthread_mutex_unlock(&redLock[r % lock_count]);

			g = from->row_pointers[i][1 + j*3];
			pthread_mutex_lock(&greenLock[g % lock_count]);
			gHist[g]++;
			pthread_mutex_unlock(&greenLock[g % lock_count]);

			b = from->row_pointers[i][2 + j*3];
			pthread_mutex_lock(&blueLock[b % lock_count]);
			bHist[b]++;
			pthread_mutex_unlock(&blueLock[b % lock_count]);

			pthread_mutex_lock(&sumLock[(r+g+b) % lock_count]);
			sHist[r+g+b]++;
			pthread_mutex_unlock(&sumLock[(r+g+b) % lock_count]);
		}
	}

	pthread_exit((void*)0);

	#elif defined(NO_LOCKS)
	//for all height and width from radius...
	for(i = thread_num * (from->image_height / num_procs); i < (1 + thread_num) * (from->image_height / num_procs); i++){
		for(j = 0; j < from->image_width; j++){
			r = from->row_pointers[i][j*3];
			g = from->row_pointers[i][1 + j*3];
			b = from->row_pointers[i][2 + j*3];
			rHist[r]++;
			gHist[g]++;
			bHist[b]++;
			sHist[r+g+b]++;
		}
	}

	pthread_exit((void*)0);

	#else
	//for all height and width from radius...
	printf("incrementing local histograms\n");
	for(i = thread_num * (from->image_height / num_procs); i < (1 + thread_num) * (from->image_height / num_procs); i++){
		for(j = 0; j < from->image_width; j++){
			r = from->row_pointers[i][j*3];
			g = from->row_pointers[i][1 + j*3];
			b = from->row_pointers[i][2 + j*3];
			(*local_data).local_r_hist[r]++;
			(*local_data).local_g_hist[g]++;
			(*local_data).local_b_hist[b]++;
			(*local_data).local_s_hist[r + g + b]++;
		}
	}

	pthread_exit((void *) local_data);

	#endif

	pthread_exit((void*)-1);
}

/*
Initialize locks for our histograms, then
call a method to count pixel colours, then
output this data to an OutputFile
*/
void CS338_function(){

	int i = 0;
	long pthread;
	pthread_t thread_IDs[num_procs];
	FILE * txt_output;
	FILE * csv_output;

	//memset histograms to 0
	printf("sizeof rHist: %lu\nexpected size: 256 * sizeof(u int)\n", sizeof(rHist));
	memset(rHist, 0, sizeof(rHist));
	memset(gHist, 0, sizeof(gHist));
	memset(bHist, 0, sizeof(bHist));
	memset(sHist, 0, sizeof(sHist));

	//initialize locks
#ifdef LOCKS_ON
		for(i = 0; i < lock_count; i++){
			if (pthread_mutex_init(&redLock[i], NULL) != 0){
				perror("failed to initialize a red lock");
				exit(1);
			}
			if (pthread_mutex_init(&greenLock[i], NULL) != 0){
				perror("failed to initialize a green lock");
				exit(1);
			}
			if (pthread_mutex_init(&blueLock[i], NULL) != 0){
				perror("failed to initialize a blue lock");
				exit(1);
			}
			if (pthread_mutex_init(&sumLock[i], NULL) != 0){
				perror("failed to initialize a sum lock");
				exit(1);
			}
			if (pthread_mutex_init(&sumLock[i + lock_count], NULL) != 0){
				perror("failed to initialize a sum lock");
				exit(1);
			}
			if (pthread_mutex_init(&sumLock[i + 2 * lock_count], NULL) != 0){
				perror("failed to initialize a sum lock");
				exit(1);
			}
		}
#endif

	//Create num_procs threads to count pixels in row-major order
	for(long thread = 0; thread < num_procs; thread++){
		struct local_histogram *this_hist;
		(*this_hist).processor_number = thread;
		memset((*this_hist).local_r_hist, 0, sizeof((*this_hist).local_r_hist));
		memset((*this_hist).local_g_hist, 0, sizeof((*this_hist).local_g_hist));
		memset((*this_hist).local_b_hist, 0, sizeof((*this_hist).local_b_hist));
		memset((*this_hist).local_s_hist, 0, sizeof((*this_hist).local_s_hist));
		pthread_create(&thread_IDs[thread], NULL, CS338_row_seq, (void*)this_hist);
	}

	//Recall threads
	#if defined(INDIV_LOCKS) || defined(BUCKET_LOCKS) || defined(UNI_LOCK) || defined(NO_LOCKS)
	for(long come_back = 0; come_back < num_procs; come_back++){
		pthread_join(thread_IDs[come_back], NULL);
	}

	#else
	void * retval;
	for(long come_back = 0; come_back < num_procs; come_back++){
		pthread_join(thread_IDs[come_back], retval);
		printf("rejoining\n");
		struct local_histogram *this_proc_data = (struct local_histogram*) retval;
		printf("casting\n");

		for (i=0; i < 256; i++){
			rHist[i] += (*this_proc_data).local_r_hist[i];
			gHist[i] += (*this_proc_data).local_g_hist[i];
			bHist[i] += (*this_proc_data).local_b_hist[i];
			sHist[i] += (*this_proc_data).local_s_hist[i];
			sHist[i + 256] += (*this_proc_data).local_s_hist[i + 256];
			sHist[i + 512] += (*this_proc_data).local_s_hist[i + 512];
		}
	}
		// for (i=0; i < 256; i++){
		// 	rHist[i] += ((unsigned long*)retval[0])[i];
		// 	gHist[i] += ((unsigned long*)retval[1])[i];
		// 	bHist[i] += ((unsigned long*)retval[2])[i];
		// 	sHist[i] += ((unsigned long*)retval[3])[i];
		// 	sHist[i + 256] += ((unsigned long*)retval[3])[i + 256];
		// 	sHist[i + 512] += ((unsigned long*)retval[3])[i + 512];
		// }
	#endif

	/*
	WARNING: fopen "w" overwrites old output files if they already exist
	*/
	txt_output = fopen(output_txt, "w");
	csv_output = fopen(output_csv, "w");

	//Output histogram results
	for (i=0; i < 256; i++){
		fprintf(txt_output, "%3d: R:%8lu G:%8lu B:%8lu S0:%8lu S1:%8lu S2:%8lu\n", i, rHist[i], gHist[i], bHist[i], sHist[i], sHist[i+256], sHist[i+512]);
		fprintf(csv_output, "%8lu, %8lu, %8lu, %8lu, %8lu, %8lu\n", rHist[i], gHist[i], bHist[i], sHist[i], sHist[i + 256], sHist[i + 512]);
	}

	fclose(txt_output);
	fclose(csv_output);

	return;
}

/******************** END OF ASSIGNMENT SECTION *******************/


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

  JSAMPLE *realBuffer;
  JSAMPLE **buffer;		/* Output row buffer */
  int row_stride;		/* physical row width in output buffer */

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
  if ((p_info = malloc(sizeof(frame_struct_t))) == NULL) {
    fprintf(stderr, "ERROR: Memory allocation failure\n");
    exit(1);
  }
  p_info->image_height = height;
  p_info->image_width = width;
  p_info->num_components = num_components;

  /* Image array and pointers to rows */
  if ((p_info->row_pointers = malloc(sizeof(JSAMPLE *) * height)) == NULL) {
    fprintf(stderr, "ERROR: Memory allocation failure\n");
    exit(1);
  }
  if ((p_info->image_buffer = malloc(sizeof(JSAMPLE) * row_stride * height)) == NULL) {
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

/* This routine sets the CPU-time vs. real-time correction factor */
double get_correction(int initial)
{
	clock_t start_clock, end_clock;	/* Clock for getting time "reference" */
	struct timeval start_time, end_time;	/* Time before/after "reference" */
	static double cpu_time;
	double real_time, correction;
	volatile int i, sum;

	/* Record start times */
	start_clock = clock();
	gettimeofday(&start_time, NULL);

	/* Delay awhile while holding the CPU */
	for (i=0; i < TEST_DELAY; i++)
		sum += 1;

	/* Get end times */
	end_clock = clock();
	gettimeofday(&end_time, NULL);

	/* Calculate the correction factor */
	if (initial)	/* Assume that CPU time doesn't change for later calls */
		cpu_time = ((double) (end_clock-start_clock)) / CLOCKS_PER_SEC;
	real_time = ((double)(end_time.tv_sec) + (double)(end_time.tv_usec)/1000000.0)
		- ((double)(start_time.tv_sec) + (double)(start_time.tv_usec)/1000000.0);
	correction = cpu_time / real_time;
	fprintf(stderr, "testing: CPU %10.3f vs. Real %10.3f: correction factor = %10.6f\n",
		cpu_time, real_time, correction);
	return correction;
}

/* The shell's main routine.  Consists of several steps:
 *	1> Reads JPEGs using first command-line parameter(s)
 *  2> Records the process's elapsed time
 *  3> Lets the user play with the images
 *  4> Checks the elapsed time again, prints out the difference
 *  5> Writes JPEGs out using last command-line parameter(s)
 */
int main(int argc, char *argv[])
{
	int i;
	int num_inputs;	/* Number of JPEG input-output files */
	int num_runs, run, discards; /* Number of runs, for multi-run code */
	clock_t start_clock, end_clock;	/* Clock for getting time "reference" */
	double run_time, total_time; /* Amount of time we've run */
	double correction, new_correction; /* CPU-vs.-real time correction */
	struct timeval start_time, end_time;	/* Time before/after user code */

	/* Step 0A: Check & process command line */
	if (NUM_INPUTS > 0)
	{
		if (argc != 1 + NUM_INPUTS + NUM_OUTPUTS + RUNS_COMMAND + RUNS_PROCS) {
			fprintf(stderr, "ERROR: Wrong number of inputs\n");
			exit(1);
		}
	}
	else
	{
		if (argc < 1 + MIN_INPUTS + NUM_OUTPUTS + RUNS_COMMAND + RUNS_PROCS) {
			fprintf(stderr, "ERROR: Too few input filenames\n");
			exit(1);
		}
		if (argc > 1 + MAX_INPUTS + NUM_OUTPUTS + RUNS_COMMAND + RUNS_PROCS) {
			fprintf(stderr, "ERROR: Too many input filenames\n");
			exit(1);
		}
	}
	num_inputs = argc - 1 - NUM_OUTPUTS - RUNS_COMMAND - RUNS_PROCS;

	#ifdef USE_NUM_RUNS
		num_runs = atoi(argv[1]);
		if (num_runs < 1) num_runs = 1;
		if (num_runs > 10) num_runs = 10; /* Change if you like LOTS of runs */
		fprintf(stderr, "Making %d runs . . .\n", num_runs);
	#endif
	#ifdef USE_NUM_PROCS
		num_procs = atoi(argv[1 + RUNS_COMMAND]);
		if (num_procs < 1) num_procs = 1;
		if (num_procs > 16) num_procs = 16;
		fprintf(stderr, "Using %d processors . . .\n", num_procs);
	#endif

	/* Step 1: Get some JPEGs into memory, uncompressed */
	for (i=0; i < num_inputs; i++)
		input_frames[i] = read_JPEG_file(argv[i+1+RUNS_COMMAND+RUNS_PROCS]);

	/* Loop over multiple runs, if desired */
	total_time = 0.0;
	correction = get_correction(1);	/* Extra one at start to warm up */
	correction = get_correction(1);
	run = discards = 0;
	#ifdef USE_NUM_RUNS
	while ((run < num_runs) && (discards < MAX_TOLERANCE_ERRORS))
	#endif
	{
		/* Step 2: Record elapsed time */
		start_clock = clock();
		gettimeofday(&start_time, NULL);

		/* Step 3: Call a user function! */
		CS338_function();

		/* Step 4: Check & print elapsed time */
		gettimeofday(&end_time, NULL);
		end_clock = clock();
		run_time = ((double)(end_time.tv_sec) + (double)(end_time.tv_usec)/1000000.0)
			- ((double)(start_time.tv_sec) + (double)(start_time.tv_usec)/1000000.0);

		/* Measure correction factor (parallel threads must be blocked) */
		new_correction = get_correction(0);
		if ((new_correction > (1.0-TOLERANCE)*correction) &&
			(new_correction < (1.0+TOLERANCE)*correction))
		{
			fprintf(stderr, "%d. ELAPSED  TIME = %20.3f sec, corrected to %20.3f sec\n",
				run, run_time, run_time*correction);
			total_time += run_time*correction;
			#ifdef NO_FIRST
				if (run == 0)
				{
					fprintf(stderr, "  . . . first run discarded\n");
					total_time = 0.0;
				}
			#endif
			run++;
		}
		else
		{
			fprintf(stderr, "X. ELAPSED    TIME = %20.3f sec, discarding: correction varying too much\n",
				run_time);
			discards++;
		}
		fprintf(stderr, "   TOTAL CPU TIME = %20.3f sec\n",
			((double) (end_clock-start_clock)) / CLOCKS_PER_SEC);
		correction = new_correction;
	}

	/* Print out final, average time, if desired */
	#ifdef USE_NUM_RUNS
		#ifdef NO_FIRST
			fprintf(stderr, "AVERAGE CORRECTED ELAPSED TIME = %20.3f seconds\n",
				total_time / ((double)(run - 1)));
		#else
			fprintf(stderr, "AVERAGE CORRECTED ELAPSED TIME = %20.3f seconds\n",
				total_time / ((double)run));
		#endif
	#endif

	/* Step 5: Write JPEGs out from memory buffers */
	for (i=0; i < NUM_OUTPUTS; i++)
		write_JPEG_file(argv[argc - NUM_OUTPUTS + i], output_frames[i], OUT_QUALITY);
}
