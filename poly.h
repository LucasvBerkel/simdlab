#ifdef FLOAT
typedef float data_t;
#define DATA_NAME "Float"
#endif

#ifdef DOUBLE
typedef double data_t;
#define DATA_NAME "Double"
#endif


#ifdef EXTEND
typedef long double data_t;
#define DATA_NAME "Extended"
#endif

#ifdef INT
typedef int data_t;
#define DATA_NAME "Integer"
#endif

#ifdef LONG
/* $begin typedefint */
typedef long data_t;
/* $end typedefint */ 
#define DATA_NAME "Long"
#endif

#ifdef CHAR
typedef char data_t;
#define DATA_NAME "Char"
#endif

#define OP_NAME "Poly"

#include "vec.h"

/* Declaration of a poly routine */
/* Source vector, destination location */ 
typedef void (*poly)(vec_ptr, long, data_t, data_t *);

/* Add combining routine to list of programs to measure */
void add_poly(poly f, poly fc, char *description);

/* Flag combiner for logging, giving bounds for fast and slow cases */
/* Can only log one combiner at a time */
void log_poly(poly f, double fast_cpe, double slow_cpe);

/* Called by main to register the set of transposition routines to benchmark */
void register_polys(void);


