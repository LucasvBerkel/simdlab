#include <stdlib.h>
#include "combine.h"

/* Original function */

char combine1_descr[] = "combine1: Maximum use of data abstraction";
/* $begin combine1 */
/* Implementation with maximum use of data abstraction */
void combine1(vec_ptr v, data_t *dest)
{
    long i;

    *dest = IDENT;
    for (i = 0; i < vec_length(v); i++) {
        data_t val;
        get_vec_element(v, i, &val);
        /* $begin combineline */
        *dest = *dest OP val;
        /* $end combineline */
    }
}
/* $end combine1 */

/* Best scalar method mentioned in Section 5 of Web Aside OPT:SIMD */

char unroll10x10a_descr[] = "unroll10x10a: Array code, unrolled by 10, Superscalar x10";
void unroll10x10a_combine(vec_ptr v, data_t *dest)
{
    long i;
    long length = vec_length(v);
    long limit = length-9;
    data_t *data = get_vec_start(v);
    data_t acc0 = IDENT;
    data_t acc1 = IDENT;
    data_t acc2 = IDENT;
    data_t acc3 = IDENT;
    data_t acc4 = IDENT;
    data_t acc5 = IDENT;
    data_t acc6 = IDENT;
    data_t acc7 = IDENT;
    data_t acc8 = IDENT;
    data_t acc9 = IDENT;

    /* Combine 10 elements at a time */
    for (i = 0; i < limit; i+=10) {
        acc0 = acc0 OP data[i];   acc1 = acc1 OP data[i+1];
        acc2 = acc2 OP data[i+2]; acc3 = acc3 OP data[i+3];
        acc4 = acc4 OP data[i+4]; acc5 = acc5 OP data[i+5];
        acc6 = acc6 OP data[i+6]; acc7 = acc7 OP data[i+7];
        acc8 = acc8 OP data[i+8]; acc9 = acc9 OP data[i+9];
    }

    /* Finish any remaining elements */
    for (; i < length; i++) {
        acc0 = acc0 OP data[i];
    }
    *dest = ((acc0 OP acc1) OP (acc2 OP acc3)) OP
        ((acc4 OP acc5) OP (acc6 OP acc7)) OP
        (acc8 OP acc9);
}

/* Experiment using GCC support for SSE instructions */

/* $begin simd_vec_sizes */
/* Number of bytes in a vector */
#define VBYTES 32

/* Number of elements in a vector */
#define VSIZE VBYTES/sizeof(data_t)
/* $end simd_vec_sizes */

/* $begin simd_vec_t */
/* Vector data type */
typedef data_t vec_t __attribute__ ((vector_size(VBYTES)));
/* $end simd_vec_t */

/* $begin simd_pack_t */
typedef union {
    vec_t v;
    data_t d[VSIZE];
} pack_t;
/* $end simd_pack_t */

char simd_v1_descr[] = "simd_v1: SSE code, 1*VSIZE-way parallelism";
/* $begin simd_combine-c */
void simd_v1_combine(vec_ptr v, data_t *dest)
{
    long i;
    pack_t xfer;
    vec_t accum;
    data_t *data = get_vec_start(v);
    int cnt = vec_length(v);
    data_t result = IDENT;

    /* Initialize accum entries to IDENT */
    for (i = 0; i < VSIZE; i++) //line:opt:simd:initstart
	xfer.d[i] = IDENT;
    accum = xfer.v;             //line:opt:simd:initend
    /* $end simd_init-c */

    /* Single step until have memory alignment */
    while ((((size_t) data) % VBYTES) != 0 && cnt) {  //line:opt:simd:startstart
	result = result OP *data++; 
	cnt--;                              
    }                              //line:opt:simd:startend

    /* Step through data with VSIZE-way parallelism */
    while (cnt >= VSIZE) {    //line:opt:simd:loopstart
	vec_t chunk = *((vec_t *) data);
	accum = accum OP chunk;
	data += VSIZE;
	cnt -= VSIZE;       
    } //line:opt:simd:loopend

    /* Single-step through remaining elements */
    while (cnt) { //line:opt:simd:loopfinishstart
	result = result OP *data++;
	cnt--;  
    } //line:opt:simd:loopfinishend

    /* Combine elements of accumulator vector */
    xfer.v = accum; //line:opt:simd:finishstart
    for (i = 0; i < VSIZE; i++)
	result = result OP xfer.d[i]; //line:opt:simd:finishend

    /* Store result */
    *dest = result; 
}
/* $end simd_combine-c */


char simd_v2_descr[] = "simd_v2: SSE code, 2*VSIZE-way parallelism";
void simd_v2_combine(vec_ptr v, data_t *dest)
{
    long i;
    pack_t xfer;
    vec_t accum0, accum1;
    data_t *data = get_vec_start(v);
    int cnt = vec_length(v);
    data_t result = IDENT;

    /* Initialize to accum IDENT */
    for (i = 0; i < VSIZE; i++)
	xfer.d[i] = IDENT;
    accum0 = xfer.v;
    accum1 = xfer.v;

    /* Single step until have memory alignment */
    while ((((size_t) data) % VBYTES) != 0 && cnt) {
	result = result OP *data++; 
	cnt--;                              
    }                              //line:opt:simd:startend

    while (cnt >= 2*VSIZE) {
	vec_t chunk0 = *((vec_t *) data);
	vec_t chunk1 = *((vec_t *) (data+VSIZE));
	accum0 = accum0 OP chunk0;
	accum1 = accum1 OP chunk1;
	data += 2*VSIZE;
	cnt -= 2*VSIZE;
    }
    while (cnt) {
	result = result OP *data++;
	cnt--;
    }
    xfer.v = accum0 OP accum1;
    for (i = 0; i < VSIZE; i++)
	result = result OP xfer.d[i];
    *dest = result;
}

/* Solution Web Aside Problem 1 */

char simd_v4_descr[] = "simd_v4: SSE code, 4*VSIZE-way parallelism";
void simd_v4_combine(vec_ptr v, data_t *dest)
{
    long i;
    pack_t xfer;
    data_t *data = get_vec_start(v);
    int cnt = vec_length(v);
    data_t result = IDENT;

    /* Create 4 accumulators and initialize elements to IDENT */
    vec_t accum0, accum1, accum2, accum3;
    for (i = 0; i < VSIZE; i++)
	xfer.d[i] = IDENT;
    accum0 = xfer.v; accum1 = xfer.v;
    accum2 = xfer.v; accum3 = xfer.v;
    
    /* Single step until have memory alignment */
    while ((((size_t) data) % VBYTES) != 0 && cnt) {
	result = result OP *data++; 
	cnt--;                              
    }                              //line:opt:simd:startend

    /* $begin simd_v4_loop-c */
    /* 4 * VSIZE x 4 * VSIZE unrolling */
    while (cnt >= 4*VSIZE) {
	vec_t chunk0 = *((vec_t *) data);
	vec_t chunk1 = *((vec_t *) (data+VSIZE));
	vec_t chunk2 = *((vec_t *) (data+2*VSIZE));
	vec_t chunk3 = *((vec_t *) (data+3*VSIZE));
	accum0 = accum0 OP chunk0;
	accum1 = accum1 OP chunk1;
	accum2 = accum2 OP chunk2;
	accum3 = accum3 OP chunk3;
	data += 4*VSIZE;
	cnt -= 4*VSIZE;
    }
    /* $end simd_v4_loop-c */

    while (cnt) {
	result = result OP *data++;
	cnt--;
    }

    /* $begin simd_v4_accum-c */
    /* Combine into single accumulator */
    xfer.v = (accum0 OP accum1) OP (accum2 OP accum3);

    /* Combine results from accumulators within vector */
    for (i = 0; i < VSIZE; i++)
	result = result OP xfer.d[i];
    /* $end simd_v4_accum-c */
    *dest = result;
}

char simd_v8_descr[] = "simd_v8: SSE code, 8*VSIZE-way parallelism";
void simd_v8_combine(vec_ptr v, data_t *dest)
{
    long i;
    pack_t xfer;
    vec_t accum0, accum1, accum2, accum3, accum4, accum5, accum6, accum7;
    data_t *data = get_vec_start(v);
    int cnt = vec_length(v);
    data_t result = IDENT;

    /* Initialize to accum IDENT */
    for (i = 0; i < VSIZE; i++)
	xfer.d[i] = IDENT;
    accum0 = xfer.v;
    accum1 = xfer.v;
    accum2 = xfer.v;
    accum3 = xfer.v;
    accum4 = xfer.v;
    accum5 = xfer.v;
    accum6 = xfer.v;
    accum7 = xfer.v;
    
    /* Single step until have memory alignment */
    while ((((size_t) data) % VBYTES) != 0 && cnt) {
	result = result OP *data++; 
	cnt--;                              
    }                              //line:opt:simd:startend

    while (cnt >= 8*VSIZE) {
	vec_t chunk0 = *((vec_t *) data);
	vec_t chunk1 = *((vec_t *) (data+VSIZE));
	vec_t chunk2 = *((vec_t *) (data+2*VSIZE));
	vec_t chunk3 = *((vec_t *) (data+3*VSIZE));
	vec_t chunk4 = *((vec_t *) (data+4*VSIZE));
	vec_t chunk5 = *((vec_t *) (data+5*VSIZE));
	vec_t chunk6 = *((vec_t *) (data+6*VSIZE));
	vec_t chunk7 = *((vec_t *) (data+7*VSIZE));
	accum0 = accum0 OP chunk0;
	accum1 = accum1 OP chunk1;
	accum2 = accum2 OP chunk2;
	accum3 = accum3 OP chunk3;
	accum4 = accum4 OP chunk4;
	accum5 = accum5 OP chunk5;
	accum6 = accum6 OP chunk6;
	accum7 = accum7 OP chunk7;
	data += 8*VSIZE;
	cnt -= 8*VSIZE;
    }
    while (cnt) {
	result = result OP *data++;
	cnt--;
    }
    xfer.v = (accum0 OP accum1) OP (accum2 OP accum3);
    xfer.v = xfer.v OP (accum4 OP accum5) OP (accum6 OP accum7);
    for (i = 0; i < VSIZE; i++)
	result = result OP xfer.d[i];
    *dest = result;
}

char simd_v10_descr[] = "simd_v10: SSE code, 10*VSIZE-way parallelism";
void simd_v10_combine(vec_ptr v, data_t *dest)
{
    long i;
    pack_t xfer;
    vec_t accum0, accum1, accum2, accum3, accum4, accum5, accum6, accum7, accum8, accum9;
    data_t *data = get_vec_start(v);
    int cnt = vec_length(v);
    data_t result = IDENT;

    /* Initialize to accum IDENT */
    for (i = 0; i < VSIZE; i++)
	xfer.d[i] = IDENT;
    accum0 = xfer.v;
    accum1 = xfer.v;
    accum2 = xfer.v;
    accum3 = xfer.v;
    accum4 = xfer.v;
    accum5 = xfer.v;
    accum6 = xfer.v;
    accum7 = xfer.v;
    accum8 = xfer.v;
    accum9 = xfer.v;
    
    /* Single step until have memory alignment */
    while ((((size_t) data) % VBYTES) != 0 && cnt) {
	result = result OP *data++; 
	cnt--;                              
    }                              //line:opt:simd:startend

    while (cnt >= 10*VSIZE) {
	vec_t chunk0 = *((vec_t *) data);
	vec_t chunk1 = *((vec_t *) (data+VSIZE));
	vec_t chunk2 = *((vec_t *) (data+2*VSIZE));
	vec_t chunk3 = *((vec_t *) (data+3*VSIZE));
	vec_t chunk4 = *((vec_t *) (data+4*VSIZE));
	vec_t chunk5 = *((vec_t *) (data+5*VSIZE));
	vec_t chunk6 = *((vec_t *) (data+6*VSIZE));
	vec_t chunk7 = *((vec_t *) (data+7*VSIZE));
	vec_t chunk8 = *((vec_t *) (data+8*VSIZE));
	vec_t chunk9 = *((vec_t *) (data+9*VSIZE));
	accum0 = accum0 OP chunk0;
	accum1 = accum1 OP chunk1;
	accum2 = accum2 OP chunk2;
	accum3 = accum3 OP chunk3;
	accum4 = accum4 OP chunk4;
	accum5 = accum5 OP chunk5;
	accum6 = accum6 OP chunk6;
	accum7 = accum7 OP chunk7;
	accum8 = accum8 OP chunk8;
	accum9 = accum9 OP chunk9;
	data += 10*VSIZE;
	cnt -= 10*VSIZE;
    }
    while (cnt) {
	result = result OP *data++;
	cnt--;
    }
    xfer.v = (accum0 OP accum1) OP (accum2 OP accum3);
    xfer.v = xfer.v OP (accum4 OP accum5) OP (accum6 OP accum7);
    xfer.v = xfer.v OP (accum8 OP accum9);
    for (i = 0; i < VSIZE; i++)
	result = result OP xfer.d[i];
    *dest = result;
}

char simd_v12_descr[] = "simd_v12: SSE code, 12*VSIZE-way parallelism";
void simd_v12_combine(vec_ptr v, data_t *dest)
{
    long i;
    pack_t xfer;
    vec_t accum0, accum1, accum2, accum3, accum4, accum5, accum6, accum7;
    vec_t accum8, accum9, accum10, accum11;
    data_t *data = get_vec_start(v);
    int cnt = vec_length(v);
    data_t result = IDENT;

    /* Initialize to accum IDENT */
    for (i = 0; i < VSIZE; i++)
	xfer.d[i] = IDENT;
    accum0 = xfer.v;
    accum1 = xfer.v;
    accum2 = xfer.v;
    accum3 = xfer.v;
    accum4 = xfer.v;
    accum5 = xfer.v;
    accum6 = xfer.v;
    accum7 = xfer.v;
    accum8 = xfer.v;
    accum9 = xfer.v;
    accum10 = xfer.v;
    accum11 = xfer.v;

    /* Single step until have memory alignment */
    while ((((size_t) data) % VBYTES) != 0 && cnt) {
	result = result OP *data++; 
	cnt--;                              
    }                              //line:opt:simd:startend

    while (cnt >= 12*VSIZE) {
	vec_t chunk0 = *((vec_t *) data);
	vec_t chunk1 = *((vec_t *) (data+VSIZE));
	vec_t chunk2 = *((vec_t *) (data+2*VSIZE));
	vec_t chunk3 = *((vec_t *) (data+3*VSIZE));
	vec_t chunk4 = *((vec_t *) (data+4*VSIZE));
	vec_t chunk5 = *((vec_t *) (data+5*VSIZE));
	vec_t chunk6 = *((vec_t *) (data+6*VSIZE));
	vec_t chunk7 = *((vec_t *) (data+7*VSIZE));
	vec_t chunk8 = *((vec_t *) (data+8*VSIZE));
	vec_t chunk9 = *((vec_t *) (data+9*VSIZE));
	vec_t chunk10 = *((vec_t *) (data+10*VSIZE));
	vec_t chunk11 = *((vec_t *) (data+11*VSIZE));
	accum0 = accum0 OP chunk0;
	accum1 = accum1 OP chunk1;
	accum2 = accum2 OP chunk2;
	accum3 = accum3 OP chunk3;
	accum4 = accum4 OP chunk4;
	accum5 = accum5 OP chunk5;
	accum6 = accum6 OP chunk6;
	accum7 = accum7 OP chunk7;
	accum8 = accum8 OP chunk8;
	accum9 = accum9 OP chunk9;
	accum10 = accum10 OP chunk10;
	accum11 = accum11 OP chunk11;
	data += 12*VSIZE;
	cnt -= 12*VSIZE;
    }
    while (cnt) {
	result = result OP *data++;
	cnt--;
    }
    xfer.v = (accum0 OP accum1) OP (accum2 OP accum3);
    xfer.v = xfer.v OP (accum4 OP accum5) OP (accum6 OP accum7);
    xfer.v = xfer.v OP (accum8 OP accum9) OP (accum10 OP accum11);
    for (i = 0; i < VSIZE; i++)
	result = result OP xfer.d[i];
    *dest = result;
}

/* Same idea, but use different associativity to get parallelism */
char simd_v2a_descr[] = "simd_v2a: SSE code, 2*VSIZE-way parallelism, reassociate";
void simd_v2a_combine(vec_ptr v, data_t *dest)
{
    long i;
    pack_t xfer;
    vec_t accum;
    data_t *data = get_vec_start(v);
    int cnt = vec_length(v);
    data_t result = IDENT;

    /* Initialize accum to IDENT */
    for (i = 0; i < VSIZE; i++)
	xfer.d[i] = IDENT;
    accum = xfer.v;

    /* Single step until have memory alignment */
    while ((((size_t) data) % VBYTES) != 0 && cnt) {
	result = result OP *data++; 
	cnt--;                              
    }                              //line:opt:simd:startend

    while (cnt >= 2*VSIZE) {
	vec_t chunk0 = *((vec_t *) data);
	vec_t chunk1 = *((vec_t *) (data+VSIZE));
	accum = accum OP (chunk0 OP chunk1);
	data += 2*VSIZE;
	cnt -= 2*VSIZE;
    }
    while (cnt) {
	result = result OP *data++;
	cnt--;
    }
    xfer.v = accum;
    for (i = 0; i < VSIZE; i++)
	result = result OP xfer.d[i];
    *dest = result;
}

/* Solution Web Aside Problem 2 */

char simd_v4a_descr[] = "simd_v4a: SSE code, 4*VSIZE-way parallelism, reassociate";
void simd_v4a_combine(vec_ptr v, data_t *dest)
{
    long i;
    pack_t xfer;
    vec_t accum;
    data_t *data = get_vec_start(v);
    int cnt = vec_length(v);
    data_t result = IDENT;

    /* Initialize accum to IDENT */
    for (i = 0; i < VSIZE; i++)
	xfer.d[i] = IDENT;
    accum = xfer.v;

    /* Single step until have memory alignment */
    while ((((size_t) data) % VBYTES) != 0 && cnt) {
	result = result OP *data++; 
	cnt--;                              
    }                              //line:opt:simd:startend

    /* $begin simd_v4a_loop-c */
    while (cnt >= 4*VSIZE) {
	vec_t chunk0 = *((vec_t *) data);
	vec_t chunk1 = *((vec_t *) (data+VSIZE));
	vec_t chunk2 = *((vec_t *) (data+2*VSIZE));
	vec_t chunk3 = *((vec_t *) (data+3*VSIZE));
	accum = accum OP
	    ((chunk0 OP chunk1) OP (chunk2 OP chunk3));
	data += 4*VSIZE;
	cnt -= 4*VSIZE;
    }
    /* $end simd_v4a_loop-c */

    while (cnt) {
	result = result OP *data++;
	cnt--;
    }
    xfer.v = accum;
    for (i = 0; i < VSIZE; i++)
	result = result OP xfer.d[i];
    *dest = result;
}

char simd_v8a_descr[] = "simd_v8a: SSE code, 8*VSIZE-way parallelism, reassociate";
void simd_v8a_combine(vec_ptr v, data_t *dest)
{
    long i;
    pack_t xfer;
    vec_t accum;
    data_t *data = get_vec_start(v);
    int cnt = vec_length(v);
    data_t result = IDENT;

    /* Initialize accum to IDENT */
    for (i = 0; i < VSIZE; i++)
	xfer.d[i] = IDENT;
    accum = xfer.v;

    /* Single step until have memory alignment */
    while ((((size_t) data) % VBYTES) != 0 && cnt) {
	result = result OP *data++; 
	cnt--;                              
    }                              //line:opt:simd:startend

    while (cnt >= 8*VSIZE) {
	vec_t chunk0 = *((vec_t *) data);
	vec_t chunk1 = *((vec_t *) (data+VSIZE));
	vec_t chunk2 = *((vec_t *) (data+2*VSIZE));
	vec_t chunk3 = *((vec_t *) (data+3*VSIZE));
	vec_t chunk4 = *((vec_t *) (data+4*VSIZE));
	vec_t chunk5 = *((vec_t *) (data+5*VSIZE));
	vec_t chunk6 = *((vec_t *) (data+6*VSIZE));
	vec_t chunk7 = *((vec_t *) (data+7*VSIZE));
	accum = accum OP
	    (((chunk0 OP chunk1) OP (chunk2 OP chunk3))
	     OP
	     ((chunk4 OP chunk5) OP (chunk6 OP chunk7)));
	data += 8*VSIZE;
	cnt -= 8*VSIZE;
    }
    while (cnt) {
	result = result OP *data++;
	cnt--;
    }
    xfer.v = accum;
    for (i = 0; i < VSIZE; i++)
	result = result OP xfer.d[i];
    *dest = result;
}


void register_combiners(void)
{
    add_combiner(combine1, combine1, combine1_descr);

    add_combiner(unroll10x10a_combine, combine1, unroll10x10a_descr);

    add_combiner(simd_v1_combine, combine1, simd_v1_descr);
    add_combiner(simd_v2_combine, combine1, simd_v2_descr);
    add_combiner(simd_v4_combine, combine1, simd_v4_descr);
    add_combiner(simd_v8_combine, combine1, simd_v8_descr);
    add_combiner(simd_v10_combine, combine1, simd_v10_descr);
    add_combiner(simd_v12_combine, combine1, simd_v12_descr);
    add_combiner(simd_v2a_combine, combine1, simd_v2a_descr);
    add_combiner(simd_v4a_combine, combine1, simd_v4a_descr);
    add_combiner(simd_v8a_combine, combine1, simd_v8a_descr);
    log_combiner(simd_v8a_combine, 0.16, 0.24);
}







