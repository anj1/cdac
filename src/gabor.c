/* COMPILE: -lm -DTEST_MODE
 */

#include <math.h>

#ifdef TEST_MODE
#include <stdio.h>
#endif

#ifndef PI
/* PI in double-precision */
#define PI 3.141592653589793
#endif

/* creates an nxn gabor kernel */
/* freq=2.0 gives 1,-1,1,...
 * aspect < 1.0 stretches, aspect > 1.0 squashes
 * multiplying aspect by X gives same result as dividing by X
 * and dividing sd by sqrt(X), but in the perpendicular direction */
int gabor(
	int *out,			/* n x n array, output */
	int n,
	double freq,		/* frequency of periodic component. Typical=2 */
	double angle,		/* angle of whole transformation; 0=vertical */
	double phase,		/* phase offset of periodic component */
	double sd,			/* standard deviation of gaussian */
	double aspect,		/* spatial aspect ratio */
	int s,				/* 1:sine 0:cosine */
	int max)			/* maximum integer value of result */
{
	int x,y;
	double fx,fy;
	double fxp,fyp;		/* rotated fx,fy */
	double cn;
	double g;			/* gauss component */
	double fmax;
	
	fmax = max;
	
	/* centroid */
	cn = (((double)n)/2.0)-0.5;
	
	aspect *= aspect;	/* we need aspect^2 */
	sd     *= sd;		/* we need sd^2 */
	
	if((s!=1)&&(s!=0)) return 1;	/* error in value */
	
	for(y=0;y<n;y++){
		for(x=0;x<n;x++){
			fx=x; fx-=cn;
			fy=y; fy-=cn;
			
			fxp =  fx*cos(angle) + fy*sin(angle);
			fyp = -fx*sin(angle) + fy*cos(angle);
			
			g = exp(-(fxp*fxp + aspect*fyp*fyp)/(2*sd*sd));
			if(s==1){
				out[y*n + x] = g*sin(2*PI*fxp/freq + phase)*fmax;
			} else {	/* eliminated all other possibilities above */
				out[y*n + x] = g*cos(2*PI*fxp/freq + phase)*fmax;
			}
		}
	}
	
	return 0;
}

/* sometimes the coefficients do not add up to 1.
 * this function helps in solving that */
void sum_gabor(int *gabor, int n)
{
	int sum;
	int i;
	
	sum=0;
	for(i=0;i<n*n;i++) sum+=gabor[i];

#ifdef TEST_MODE
	printf("sum:%d\n",sum);
#endif
}

#if 0
/* applies gabor kernel gab to image in */
void apply_gabor(int *out, int *in, int w, int h, int *gabor, int ngab)
{
	int x,y;
	int i,j;
	int c;
	int ngab2;
	int disp;
	int curgab;
	
	ngab2 = ngab/2;
	
	for(i=0;i<w*h;i++) out[i]=0;
	
	for(j=0;j<ngab;j++){
		for(i=0;i<ngab;i++){
			disp = (j-ngab2)*w + (i-ngab2);
			curgab = gabor[j*ngab + i];
			
			for(y=ngab2;y<h-ngab2;y++){
				c = y*w + ngab2;
				for(c=;c<w-ngab;x++){
					out[c] += curgab*in[
				}
			}
		}
	}
	
}
#endif

#ifdef TEST_MODE

#define SIZ 5

void printest(int *a, int n)
{
	int i,j;
	
	for(j=0;j<n;j++){
		for(i=0;i<n;i++){
			printf("% 5d ", a[j*n + i]);
		}
		printf("\n");
	}
}

int main()
{
	int gab[SIZ*SIZ];
	int apl[SIZ*SIZ];
	int i;

	gabor(gab, SIZ, 2.0, PI*0.25, 0.0, 1.0*0.707, 0.5, 0, 255);
	sum_gabor(gab, SIZ);
	printest(gab, SIZ);
	
	gabor(gab, SIZ, 2.0, PI*0.25, 0.0, 1.0*0.707, 0.5, 1, 255);
	sum_gabor(gab, SIZ);
	printest(gab, SIZ);
	
	/* test application */
	for(i=0;i<SIZ*SIZ;i++) apl[i]=0;
	apl[3*SIZ + 3]=1;
	printf("apply_gabor: %d\n", apply_gabor(apl, SIZ, SIZ, SIZ/2, SIZ/2, gab, SIZ));
	printest(apl, SIZ);
	
	return 0;
}

#endif /* TEST_MODE */
