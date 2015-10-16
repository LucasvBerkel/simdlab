#include <stdlib.h>
#include "poly.h"

/* Original function */

char polycheck_descr[] = "polycheck: Traditional execution";
/* $begin polycheck */
/* Implementation with maximum use of data abstraction */
void polycheck(vec_ptr a, long degree, data_t x, data_t *dest)
{
    long i;
    data_t val, result;
    get_vec_element(a, degree, &result);
    for (i = degree - 1; i >= 0; i--)
    {
        get_vec_element(a, i, &val);
        /* $begin combineline */
	       result = val + x*result;
        /* $end combineline */
    }
}
/* $end polycheck */

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

char opt_poly_descr[] = "simd_v2: optimized poly";
void opt_poly(vec_ptr a, long degree, data_t x, data_t *dest)
{
    long int i;
    pack_t xfer;
    vec_t accum0, accum1;
    // data_t *data = get_vec_start(a);
    int cnt = vec_length(a);
    data_t result = 0;

    while(cnt >= VSIZE)
    {
        accum0 = *((vec_t *) a);
        int limit = cnt - VSIZE;
        for(i = cnt - 1; i > limit; i--)
        {
            set_vec_element(&accum1, i, x^cnt);
        }
        xfer.v = accum0 * powers.v;
        for (i = 0; i < VSIZE; i++)
            result += xfer.d[i];
        a   += VSIZE;
        cnt -= VSIZE;
    }

    // TODO remainder

    *dest = result;
}

void register_polys(void)
{
    add_poly(polycheck, polycheck, polycheck_descr);

    add_poly(opt_poly, polycheck, opt_poly_descr);

    log_poly(opt_poly, 0.16, 0.24);
}
