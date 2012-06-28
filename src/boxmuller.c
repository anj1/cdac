/* Box-Muller transform for generating gaussian random variables */

#include <stdlib.h>		/* rand() */
#include <math.h>		/* log(), sqrt(), cos(), sin() */
#include "boxmuller.h"

#ifndef PI
/* PI in float_type-precision */
#define PI 3.141592653589793
#endif

/* returns a single gaussian-distributed value */
float_type box_muller()
{
	float_type u1,u2;
	
	/* +1 to keep floating point exceptions from occuring */
	u1 = ((float_type)(rand()+1))/((float_type)RAND_MAX);
	u2 = ((float_type)(rand()+1))/((float_type)RAND_MAX);
	
	return sqrt(-2.0*log(u1))*cos(2*PI*u2);
}

/* returns a gaussian-distributed coordinate */
void box_muller2(float_type *x, float_type *y)
{
	float_type u1,u2;
	float_type p1;
	
	u1 = ((float_type)rand())/((float_type)RAND_MAX);
	u2 = ((float_type)rand())/((float_type)RAND_MAX);
	
	p1 = sqrt(-2.0*log(u1));
	
	(*x) = p1*cos(2*PI*u2);
	(*y) = p1*sin(2*PI*u2);
}
