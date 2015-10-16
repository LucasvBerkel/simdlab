#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "cpe.h"
#include "poly.h"

#define SHORT 0
#if SHORT 
#define ASIZE 31
#else
#define ASIZE 973
#endif

/* Keep track of a number of different combining programs */
#define MAX_BENCHMARKS 100

static struct {
    poly cfunct;
    poly checkfunct;
    char *description;
    double cpe;
} benchmarks[MAX_BENCHMARKS];

static long benchmark_count = 0;

static long current_benchmark = 0;

static vec_ptr data;
static data_t poly_result;

/* Used to make sure code doesn't get optimized away */
volatile data_t sink; 

/* Log especially fast or slow cases */
poly log_poly_fun = NULL;
double log_slow_cpe = 0.0;
double log_fast_cpe = 1000.0;
char *log_name = "benchmark-log.txt";
char *log_fast_name = "benchmark-log-fast.txt";
char *log_slow_name = "benchmark-log-slow.txt";

static void setup()
{
    long i;
    data = new_vec(ASIZE);
    /* Initialize array  */
    for (i = 0; i < ASIZE; i++) 
	set_vec_element(data, i, (data_t) (i+1));
//      set_vec_element(data, i, (data_t) (random() & 0x1) ? -1 : 1);

    sink = (data_t) 0;
}

void run(long cnt) {
    
    set_vec_length(data, cnt);
    data_t x = (data_t) (random() & 0xFF); 
    benchmarks[current_benchmark].cfunct(data, cnt, x, &poly_result);
}
     
/* Perform test of combination function */
static void run_test(long bench_index) {
    double cpe;
    char *description = benchmarks[bench_index].description;
    data_t good_result;
    FILE *logfile = NULL;
    current_benchmark = bench_index;
    setup();

    if (benchmarks[bench_index].cfunct == log_poly_fun) {
	logfile = fopen(log_name, "w");
	if (!logfile) {
	    fprintf(stderr, "Failed to open log file\n");
	    exit(1);
	}
    }
    cpe = find_cpe_full(run, ASIZE, 200, logfile, RAN_SAMPLE, 0.3, 2);
    if (logfile) {
	fclose(logfile);
	if (cpe <= log_fast_cpe) {
	    if (rename(log_name, log_fast_name)) {
		fprintf(stderr, "Couldn't rename fast cpe file\n");
		exit(1);
	    }
	}

	if (cpe >= log_slow_cpe) {
	    if (rename(log_name, log_slow_name)) {
		fprintf(stderr, "Couldn't rename slow cpe file\n");
		exit(1);
	    }
	}
    }
    data_t x = (data_t) (random() & 0xFF);
    long len = vec_length(data); 
    benchmarks[bench_index].cfunct(data, len, x, &poly_result);
    benchmarks[bench_index].checkfunct(data, len, x,  &good_result);
    if (poly_result != good_result) {
	printf("Function %s, Should be %ld, Got %ld\n",
	       description, (long) good_result, (long) poly_result);
    }
    benchmarks[current_benchmark].cpe = cpe;
    /* print results */
    /* Column Heading */
    printf("%s %s %s:\n", DATA_NAME, OP_NAME, description);
    printf("%.2f cycles/element\n", cpe);
}

void add_poly(poly f, poly fc, char *description) {
    benchmarks[benchmark_count].cfunct = f;
    benchmarks[benchmark_count].checkfunct = fc;
    benchmarks[benchmark_count].description = description;
    benchmark_count++;
}

void log_poly(poly f, double fast_cpe, double slow_cpe) {
    log_poly_fun = f;
    log_fast_cpe = fast_cpe;
    log_slow_cpe = slow_cpe;
}

int main()
{
    long i;
    register_polys();
    for (i = 0; i < benchmark_count; i++) {
	run_test(i);
    }
    return 0;
}

