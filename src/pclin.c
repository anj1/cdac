/* COMPILE: -Wall -lm -DTEST_MODE -DDEBUG_PRIN
 */

/* Subroutines for piecewise linear functions */
/* Started August 28 */
/* Aug 31: completed */
/* Sep 5: final bugs quashed */
/* dx: distance to next x point
 *  f: value at that x point.
 * if period==0, function is assumed to be non-periodic.
 * otherise, period is assumed to be sum(dx)
 * and dx[0] is taken to be distance from point n-1, not from origin.
 */
/* second-degree autocorrelation computation */

/*
0-1-2-3-4-5-6-7-8-9-
  a       b     c
dx = {1, 4, 3};
 f = {a, b, c};
 * 
 * Actually, the above is wrong?
 * Should be:
0-1-2-3-4-5-6-7-8-
  a       b     c
*/

/* undef this if ISO C99 is not available */
#define CALC_FT

#include <malloc.h>
#include <stdio.h>
#ifdef CALC_FT
#include <complex.h>	/* for pclin_fourier */
//#include <math.h>		/* exp(), for pclin_fourier */
#endif

#ifndef PI
#define PI 3.141592654
#endif

/* calculation of inner product of piecewise linear functions */
/* calculation proceeds through a series of stops */
/* at each stop, we accumulate x and find f */
/* dx is refreshed at each stop for one of the data sets */
/* we always pick the dataset with minimum dx */
/* for initiation, simply set dx to -offset */
/* Code must be beautiful. */
/* pdx,pf: [n] arrays of offsets and function values,
 * offset: offset at which cross-correlation should be calculated
 * (that is, function only calculates it at one point */
/* offset is from first data, so offset=0.0 means from pdx2[0] */
/* works even when periods aren't the same
 * however, might go into infinite loop */
#define CROSSCORR_STEP(dx1,f1,pdx1,pf1,slope1,i1,n1,dx2,f2,pdx2,pf2,slope2,i2,n2) \
{ \
	dx2 -= dx1; \
	f2 += slope2*dx1; \
	f1 =   pf1[i1%n1]; \
	i1++; \
	i = i1%n1; \
	dx1 = pdx1[i]; \
	slope1 = (pf1[i]-f1)/dx1; \
	if((dx1 < 0.00002) && (dx1 > -0.00002)) {printf("slope overflow\n"); return 0.0;}\
}
float pclin_innerproduct(float *pdx1, float *pf1, int n1, float *pdx2, float *pf2, int n2, float offset)
{
	float dx1,dx2;
	float f1,f2;
	float step;
	float slope1,slope2;
	int i1,i2;
	int inii1; /* values at start */
	/*int inii2 */
	float sum;
	int i;
	
	if(offset<0.0) return 0.0;
	offset+=pdx2[0];	/* reference */
	
	/* find i1,i2 */
	/* i1 starts at 0,
	 * i2 starts at after offset (dx2 must be set accordingly) */
	i1=1;
	dx1=pdx1[1];
	for(i2=0;offset>=0.0;i2++) offset -= pdx2[i2%n2];	/* EDIT: %n2 */
	dx2 = -offset;
	i2--;
	f1 = pf1[0];
	f2 = pf2[(i2-1)%n2];		/* i2 will always be nonzero */
	
	slope1 = (pf1[i1]-f1)/dx1;
	slope2 = (pf2[(i2%n2)]-f2)/pdx2[i2%n2];	/* EDIT: %n2 */

	if(dx1 < 0.0000002) printf("hhhdx1\n");
	if(pdx2[i2%n2] < 0.0000002) printf("hhhpdx2[i2]\n");
	
#ifdef DEBUG_PRINT
	printf("i2: %d dx2: %f slope2:%f\n", i2, dx2, slope2);
#endif

	inii1=i1;
	/*inii2=i2;*/
	
	sum=0.0;
	while(1){
		/* multiply together */
		/* move origin to current spot */
		/* curves are: */
		/* f1 + slope*x = A1
		   f2 + slope*x = A2
		   multiply ---> integrate(A1*A2) from 0 to step */
		step = dx1 < dx2 ? dx1 : dx2;	/* ok, so here's another hack */
		sum += step*(f1*f2 + step*(0.5*(f1*slope2 + f2*slope1) + step*0.3333333*slope1*slope2));

		if(sum > 1000000) printf("khhh\n");
		if(sum == (1.0/0.0)) printf("khhhnan\n");
		
#ifdef DEBUG_PRINT
		printf("i %d %d, f1 f2 slope1 slope2 %f %f %f %f\n", i1,i2,f1, f2, slope1, slope2);
#endif

		/* calculate next stop */
		if(dx1 < dx2){
			CROSSCORR_STEP(dx1,f1,pdx1,pf1,slope1,i1,n1,dx2,f2,pdx2,pf2,slope2,i2,n2)
		} else {
			CROSSCORR_STEP(dx2,f2,pdx2,pf2,slope2,i2,n2,dx1,f1,pdx1,pf1,slope1,i1,n1)
		}

#ifdef DEBUG_PRINT
		printf("step: %f dx1:%f dx2:%f sum: %f\n", step, dx1, dx2, sum);
#endif
		/* reached the end? */
		/* TODO */
		//if(((i1%n1)==inii1) && ((i2%n2)==inii2))	/* make sure we go at least one round */
		if(i1==n1+inii1)
			return sum;
	}
}

float pclin_mean(float *dx, float *f, int n, float periodic)
{
	int i;
	float sum;
	float x;
	float f1,f2;
	float thisdx;
	/*  */
	
	x = 0.0;
	sum = 0.0;
	f1=f[0];
	for(i=1;i<n;i++){
		f2 = f[i];
			
		thisdx = dx[i];
		
		/* calculate sum */
		sum += 0.5*(f1 + f2)*thisdx;
		
		x += thisdx;
		f1 = f2;
	}
	/* finish up */
	if(periodic) {sum += 0.5*(f[n-1] + f[0])*dx[0]; x+=dx[0];}
	//printf("mean: sm: x: %f %f\n", sum,x);
	return sum / x;
}

/* subtract dc component of signal */
void pclin_zerodc(float *pdx, float *pf, int n)
{
	int i;
	float mean;
	
	mean = pclin_mean(pdx, pf, n, 1);
	for(i=0;i<n;i++) pf[i] -= mean;
}

void pclin_crosscorr(float *out, float step, int n, float *pdx1, float *pf1, int n1, float *pdx2, float *pf2, int n2)
{
	int i;
	float x;
	
	pclin_zerodc(pdx1, pf1, n1);
	pclin_zerodc(pdx2, pf2, n2);
	
	x=0.0;
	for(i=0;i<n;i++){
		out[i] = pclin_innerproduct(pdx1, pf1, n1, pdx2, pf2, n2, x);
		x += step;
	}	
}

void pclin_export_crosscorr(char *filename, float step, float *pdx1, float *pf1, int n1, float *pdx2, float *pf2, int n2)
{
	FILE *f;
	int i;
	float len;
	float x;
	
	f = fopen(filename,"a");
	if(!f) return;
	
	/* obtain length */
	len=0.0;
	for(i=0;i<n2;i++) len += pdx2[i];	/* n2 since offse is alied o series 2 */
	
	x=0.0;
	while(x<len){
		fprintf(f, "%f ", pclin_innerproduct(pdx1, pf1, n1, pdx2, pf2, n2, x));
		x+=step;
	}
	fprintf(f,"\n");
	
	fclose(f);	
}

void pclin_export_crosscorr_int(char *filename, float step, float *pdx1, int *pf1, int n1, float *pdx2, int *pf2, int n2)
{
	int i;
	float *g,*h;
	
	g = (float *)malloc(n1 * sizeof(float));
	if(!g) return;
	h = (float *)malloc(n2 * sizeof(float));
	if(!h) {free(g); return;}
	
	for(i=0;i<n1;i++) g[i] = pf1[i];
	for(i=0;i<n2;i++) h[i] = pf2[i];
	
	pclin_export_crosscorr(filename, step, pdx1, g, n1, pdx2, h, n2);
	
	free(g);
	free(h);
}

#ifdef CALC_FT
/* obtain fourier coefficient of periodic, piecewise-linear function */
/* inputs:
 *  pdx,pf,n: piecewise linear function dx, values, and length
 *  w: desired angular frequency
 * returns fourier coefficient as complex number (ISO C99).
 * If the input is a complex function, the following can be called twice
 * with the second coefficient multiplied by _Complex_I and added to the
 * first (see pclin_fourier_trans_cplx).
 * It is assumed that the length of the x-axis is 2*pi. If it is not, it
 * should be normalized to 2*pi before calling this function.
 */
float complex pclin_fourier(float *pdx, float *pf, int n, float w)
{
	int i;
	float x1,x2;
	float f1,f2;
	float slope,offset;
	
	float complex sum;
	float complex alpha;
	float complex invalpha;
	
	x1 = 0.0;
	sum = 0.0;
	f1=pf[n-1];

	/* different behavior for zero w */
	if(w==0.0){
		for(i=0;i<n;i++){
			sum += 0.5*pdx[i]*(f1 + pf[i]);
			x1 += pdx[i];
			f1 = pf[i];
		}
		return sum;
	}
	
	/* regular behavior for nonzero w */
	alpha = -_Complex_I*w;
	invalpha = 1/alpha;
	for(i=0;i<n;i++){
		f2 =     pf[i];
		x2 = x1+pdx[i];
		
		/* calculate product of fourier wave with function */
		/* first, calculate slope and offset of curve */
		slope=(f2-f1)/(x2-x1);
		offset=f1-slope*x1;
		
		/* now use formula (refer to pclin_fourier.tex) */
		sum += cexpf(alpha*x2)*(slope*(x2-invalpha)+offset) -
		       cexpf(alpha*x1)*(slope*(x1-invalpha)+offset);
		
		f1 = f2;
		x1 = x2;
	}
	
	return sum/alpha;
}

/* takes the fourier transform (calls the above m times)
 */
void pclin_fourier_trans_real(float complex *out, int m, float *pdx, float *pf, int n)
{
	int i;
	int idx;
	
	for(i=1;i<m;i++){
		idx = i < 0 ? 2*m+i : i;
		if(i==0){
			out[idx] = (pclin_mean(pdx,pf,n,1))*2*PI;
		}
		out[idx] = pclin_fourier(pdx,pf,n,(float)i);
	}
}

/* like the above, but operates on complex functions */
/* real values given in pr and imaginary ones in pi */
/* out is assumed to be allocated for 2*m, not m */
void pclin_fourier_trans_cplx(float complex *out, int m, float *pdx, float *pr, float *pi, int n)
{
	int i;
	int idx;
	
	for(i=-m+1;i<=m;i++){
		idx = i < 0 ? 2*m+i : i;
		if(i==0){
			out[idx] = (pclin_mean(pdx,pr,n,1) +
			            pclin_mean(pdx,pi,n,1)*_Complex_I)*2*PI;
			continue;
		}
		out[idx] = pclin_fourier(pdx,pr,n,(float)(+i)) +
		           pclin_fourier(pdx,pi,n,(float)(+i))*_Complex_I;
	}
}
#endif

#ifdef TEST_MODE

#define TEST_N 4

//float dx[] = {1.0, 3.0, 1.0, 3.0};
float dx[] = {1.0, 1.0, 1.0, 1.0};
float  f[] = {5.0, 1.0, -5.0,0.0};

void test_ft()
{
	int i;
	float complex ft[16];
	
	for(i=0;i<4;i++) dx[i] *= (2*PI/4.0);
	
	pclin_fourier_trans_real(ft,16,dx,f,4);
	
	for(i=0;i<16;i++){
		printf("%f + i*%f, ", creal(ft[i]), cimag(ft[i]));
	}
	printf("\n");
}

void test1()
{
	float offset;
	
	printf("mean: %f\n", pclin_mean((float *)dx, (float *)f, TEST_N, 1));
	
	//offset = 8.00;
	//offset = 2.275;
	
	for(offset=0.0;offset<8.0;offset+=0.2){
		printf("%f: %f\n", offset, pclin_innerproduct(dx, f, 4, dx, f, 4, offset));
	}	
}

int main()
{
	
	test_ft();
	
	return 0;
}

#endif
