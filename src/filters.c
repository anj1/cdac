/* COMPILE: -lm -O2 -DTEST_MODE
 */

/* August 3 2011: Fixed nasty outyo[x] bug */

/* Oct 19: Fixed a FUCKING bug that had been causing all this pain for
 * me: mixing up outxo[] and outyo[].
 * MOTHERFUCKER */
 
#include <stdint.h>
#include <math.h>		/* exp, sqrt */
#include <malloc.h>
#include "global.h"

#ifdef TEST_MODE
#include <stdio.h>
#endif

/* does some checks */
#define SAFE_MODE

#define MAX_DIF 4096*4096

#ifndef INT_MIN
#define INT_MIN -0x80000000
#endif

/* fixed bug in fast_highpass that was causing errors to accumulate.
 * (out's edges weren't being set) */
 
/* create a 1d gaussian kernel with standard deviation sd */
/* the values will be less than max */
/* example:  2 1 0 */
void gaussian1d(int *out, int n, double sd, double max)
{
	int i;
	double di;
	
	for(i=0;i<=(n/2);i++){
		di = (double)i;
		out[i] = max*exp(-(di*di)/(sd*sd));
		if(i>0) out[n-i] = out[i];
	}
}

/* tells the sum of the elements of a 1d gaussian transformation */
/* useful for normalization after blurring */ 
int sumgaus1d(double sd, double max){
	int flt[256];
	int i;
	int sum;
	
	gaussian1d(flt, 256, sd, max);
	
	sum=0;
	for(i=0;i<256;i++) sum+=flt[i];
	return sum;
}


/* shift by (xo,yo) and compare (prev - next)^2 */
/* TODO: make it faster */
void shift_cmp(pix_type *prev, pix_type *next, int *out, int width, int height, int xo, int yo, int shift)
{
	int x,y;
	int dif;
	int index;
	
	index=0;
	for(y=0;y<height;y++){
		for(x=0;x<width;x++){
			if(((x+xo)<0)||((x+xo)>=width)||((y+yo)<0)||((y+yo)>=width)){
				/* Out of bounds */
				out[index] = MAX_DIF;	/* set large (because 0=similar) */
			} else {
				dif = prev[index] - next[index + xo + yo*width];
				out[index] = dif*dif >> shift;
			}
			
			index++;
		}
	}
}

/* apply horizontal filter to line of data */
/* does not apply to edges (non-wrapping) */
void filterh(int *ker, int nker, int *in, int *out, int width)
{
	int i,x;
	int sum;
	int *ino;
	
	for(x=(nker/2);x<(width-(nker/2));x++){
		sum=0;
		ino = in + x;
		for(i=0;       i<=(nker/2);i++) sum += ker[i]*ino[i];
		for(i=(nker-1);i> (nker/2);i--) sum += ker[i]*ino[i-nker];
		out[x]=sum;
	}
}

/* apply horizontal filter to vertical line of data */
/* does not apply to edges (non-wrapping) */
void filterv(int *ker, int nker, int *in, int *out, int width, int height)
{
	int i,c,y;
	int sum;
	int *ino,*outo;
	
	ino  = in;
	outo = out;
	for(y=(nker/2);y<(height-(nker/2));y++){
		sum=0;
		
		outo = out + y*width;
		ino  = in + y*width;
		
		c=0;
		for(i=0;i<=(nker/2);i++){
			sum += ker[i]*ino[c];
			if(i>0) sum += ker[nker-i]*ino[-c];
			c+=width;
		}

		*outo =sum;
		
		/*outo += width;
		ino  += width;*/
	}
}

/* apply a filter kernel to an image, horizontally */
void filterh2d(int *ker, int nker, int *in, int *out, int width, int height)
{
	int y;
	
	for(y=0;y<height;y++){
		filterh(ker,nker,in+width*y,out+width*y,width);
	}
}

/* apply a filter kernel to an image, vertically */
void filterv2d(int *ker, int nker, int *in, int *out, int width, int height)
{
	int x;
	
	for(x=0;x<width;x++){
		filterv(ker,nker,in+x,out+x,width,height);
	}
}

/* the number of kernel points per standard deviation */
#define KER_LEN_SD   3.0
#define MAX_KER_LEN 30

/* gaussian blurs an image with standard deviation sd */
/* automagically guesses the right kernel size to use */
int gaussian_blur(int *in, int *out, int *tmp, int width, int height, double sd)
{
	int nker;
	int ker[MAX_KER_LEN];
	
	/* compute size of kernel */
	nker = KER_LEN_SD*2.0*sd;
	if(nker > MAX_KER_LEN) return 1;

	/* prepare kernel */
	/* input: 12 bits, dif:13 dif^2:26  shifted:18 ker:26 kerlen:30 */
	gaussian1d(ker, nker, sd, 255);
	
	/* horizontal filter */
	filterh2d(ker, nker, in, tmp,  width, height);
	
	/* vertical filter (notice in and out) */
	filterv2d(ker, nker, tmp, out, width, height);
	
	return 0;
}

/**uses all the above functions to detect motion.
 * inputs:
 *  w,h: width and height of frame
 *  prev[w*h]: 'previous' frame (the current frame)
 *  next[w*h]: next frame
 *  sd: standard deviation of gaussian window
 *  shift: inaccuracy (must be nonzero otherwise risk integer overflow)
 *  maxx: maximum x-direction to search for i.e. [-x,x]
 *  maxy: maximum y-direction to search for i.e. [-y,y]
 * 
 * outputs:
 *  mx[w*h]: x component of detected motion vector
 *  my[w*h]: y component of detected motion vector
 *  conf[w*h]: confidence of motion vector
 */ 
int motion_detect(pix_type *prev, pix_type *next,
                   int w, int h,
                   double sd, int shift,
                   char maxx, char maxy,
                   char *mx, char *my, float *conf)
{
	int x,y;
	int i;
	float tmpf;
	
	/*  * memory:
	 *  dif [w*h]: stores differences
	 *  difb[w*h]: stores blurred difference
	 *  difw[w*h]: stores current (winning; minimum) blurred difference
	 *  difa[w*h]: stores average (for computing of confidence)
	 *  tmp [w*h]: temporary storage
	 */
	int *dif,*difb,*difw,*tmp,*difa;
	
	/* before anything, allocate memory */
	dif  = (int *)malloc(w*h*sizeof(int));
	difb = (int *)malloc(w*h*sizeof(int));
	difw = (int *)malloc(w*h*sizeof(int));
	tmp  = (int *)malloc(w*h*sizeof(int));
	difa = (int *)malloc(w*h*sizeof(int));
	if((!dif)||(!difb)||(!difw)||(!tmp)||(!difa))
		return 1;
		
	/* clear motion vector first */
	for(i=0;i<w*h;i++) mx[i]=0;
	for(i=0;i<w*h;i++) my[i]=0;
	
	/* clear winning blurred differences */
	for(i=0;i<w*h;i++) difw[i] = 0x7FFFFFFF;	/* TODO */
	for(i=0;i<w*h;i++) difa[i] = 0;
	
	/* for each possible direction of motion */
	for(y=-maxy;y<=maxy;y++){
		for(x=-maxx;x<=maxx;x++){
			
			/* create differences */
			shift_cmp(prev, next, dif, w, h, x, y, shift);
			
			/* blur differences (has the effect of gaussian windowing) */
			gaussian_blur(dif, difb, tmp, w, h, sd);
			
			/* update motion vector */
			/* simple greedy winner-take all */
#if 0
			if((x==0)&&(y==0)){
				for(i=0;i<w*h;i++)
					difb[i]-=0x600000;	/* (0,0) has an advantage */
			}
#endif
			for(i=0;i<w*h;i++){
				/* update averages */
				difa[i] += difb[i] >> 8;
				
				/* new minimum? */
				if(difb[i] < difw[i]){
					mx[i] = x;
					my[i] = y;
					difw[i] = difb[i];
				}
			}
		}
	}
	
	/* compute difference confidences */
	for(i=0;i<w*h;i++){
		if(difw[i]==0){	/* we have a divide by zero */
			conf[i]=1.0;
		} else {
			tmpf = difw[i];
			tmpf = tmpf*(2*maxx + 1)*(2*maxy + 1);
			tmpf = tmpf*0.0039;
			tmpf = tmpf / ((float)difa[i]);
			if(tmpf<0.0) tmpf=0.0;
			conf[i] = 1.0 - tmpf;
		}
	}
	
	free(difa);
	free(tmp);
	free(difw);
	free(difb);
	free(dif);
	
	return 0;
}

/* smooths motion vectors */
void smooth_mv(char *in, char *out, float *conf, int w, int h)
{
	int i,j,c;
	
	for(j=1;j<h-1;j++){
		c = j*w + 1;
		for(i=1;i<w-1;i++){
			if(conf[c]<0.95){
				out[c] = (2*in[c] + in[c-1] + in[c+1] + in[c+w] + in[c-w]) / 6;
			}
			c++;
		}
	}
}

/* applies 2d square transform filter to local part of image */
/* returns INT_MIN (essentially -infinity) for edges */
int apply_filter(pix_type *in, int w, int h, int x, int y, int *gabor, int ngab)
{
	int ngab2;
	int sum;
	int i,j;
	
	int *gabo;
	pix_type *ino;
	
	ngab2 = ngab/2;
	
	/* check for bounds */
	if(x < ngab2) return INT_MIN;
	if(y < ngab2) return INT_MIN;
	if((w-x) <= ngab2) return INT_MIN;
	if((h-y) <= ngab2) return INT_MIN;
	
	/* apply */
	sum=0;
	gabo=gabor;
	for(j=0;j<ngab;j++){
		ino = in + ((j+y-ngab2)*w + (x-ngab2));
		for(i=0;i<ngab;i++){
			sum += ((int)ino[i])*(*gabo);
			gabo++;
		}
	}
	
	return sum;
}

/* performs edge filter on image */
void fast_highpass(int *in, int *out, int w, int h)
{
	int sum;
	int x,y;
	int *ino00,*ino0,*ino1,*ino2,*ino3;
	int *outo;
	
	for(y=2;y<h-2;y++){
		ino00= in + y*w - w - w;
		ino0 = in + y*w - w;
		ino1 = in + y*w;
		ino2 = in + y*w + w;
		ino3 = in + y*w + w + w;
		outo = out + y*w;
		
		for(x=2;x<w-2;x++){
			
			sum  = ino00[x] + ino3[x] + ino1[x-2] + ino1[x+2];
			sum += 3*(ino0[x-1] + ino0[x+1] + ino2[x-1] + ino2[x+1]);
			sum += 4*(ino0[x] + ino1[x-1] + ino1[x+1] + ino2[x]);
			sum -= 32*(ino1[x]);
			outo[x] = sum >> 3;
		}
		
		outo[0]=0;   outo[1]=0;	    /* this prevents errors from accumulating */
		outo[w-1]=0; outo[w-2]=0;
	}
	
	/* this prevents errors from accumulating */
	for(x=0;x<w*2;x++)       out[x]=0;
	for(x=w*(h-2);x<w*h;x++) out[x]=0;
}

/* performs variance filtering on input */
/* tmp1, tmp2 are given rather than allocated; to speed up multiple
 * calls to routine */
 /* shift should be the number of important input bits */
int variance(pix_type *in, int *out, int *tmp1, int *tmp2, int w, int h, double sd1, double sd2, int shift)
{
	int i;
	int dif;
	int ndiv;
	int nmul;
	int err;
	int idx;
	
	/* transfer pixels to integers for blurring */
	for(i=0;i<w*h;i++) tmp1[i] = in[i];

	/* compute normalization factor */
	ndiv = sumgaus1d(sd1, 255);
	ndiv *= ndiv;
	ndiv >>= 10;	/* because we want the input to second gaussian blur to have 12 bits max */
	ndiv++;			/* to keep div-by-zero errors from occuring */
	ndiv <<= shift;
	//printf("ndiv: %d\n", ndiv);
	
	if((err = gaussian_blur(tmp1, out, tmp2, w, h, sd1))) return err;

	/* square of difference */
	idx = (h/2)*w + (w/2);
	nmul = out[idx]/in[idx];

	for(i=0;i<w*h;i++){
		dif = in[i];
		dif = (dif*nmul - out[i]) / (ndiv/2);
		if(dif<0) printf("dif:%d\n",dif);
		tmp2[i] = (dif*dif);
	}
	
	if((err = gaussian_blur(tmp2, out, tmp1, w, h, sd2))) return err;
	
	return 0;
}

void fast_variance(pix_type *in, int *out, int *tmp1, int w, int h)
{
	int i;
	int dif;
	
	/* transfer pixels to integers for blurring */
	for(i=0;i<w*h;i++) tmp1[i] = in[i];

	/* high-pass filter */
	fast_highpass(tmp1, out, w, h);
	
	/* square (take variance) */
	for(i=0;i<w*h;i++){
		dif = out[i] >> 2;
		out[i] = dif*dif;
	}
	
	/* the final blurring step is up to the rest of the program */
}

/* performs sobel edge filter on image, returning both components */
/* inmax: maximum value of input pixels */
/* 5-star */
void sobel2cf(float *in, float *outx, float *outy, int w, int h)
{
	float gd1,gd2;
	int x,y;
	float *ino0,*ino1,*ino2;
	float *outxo,*outyo;
	
	/* be a clean citizen */
	for(x=0;x<w;x++)       outx[x]=0.0;
	for(x=0;x<w;x++)       outy[x]=0.0;
	for(x=w*h-w;x<w*h;x++) outx[x]=0.0;
	for(x=w*h-w;x<w*h;x++) outy[x]=0.0;
	
	/* memory-efficient implementation */
	for(y=1;y<h-1;y++){
		ino0  = in + y*w - w;
		ino1  = in + y*w;
		ino2  = in + y*w + w;
		outxo = outx + y*w;
		outyo = outy + y*w;
		
		/* be a clean citizen */
		outxo[0]=0;	outxo[w-1]=0;
		outyo[0]=0; outyo[w-1]=0;
		
		for(x=1;x<w-1;x++){
			gd1 = ino2[x+1] - ino0[x-1];
			gd2 = ino2[x-1] - ino0[x+1];
			
			outxo[x] = (gd1 - gd2 - 2.0*ino1[x-1] + 2.0*ino1[x+1]);
			outyo[x] = (gd1 + gd2 - 2.0*ino0[x]   + 2.0*ino2[x]);
		}
	}
}

/* overlays out with the edges of in */
void hardedge(unsigned char *in, unsigned char *out, int w, int h)
{
	int i;
	
	//for(i=0;i<w*h;i++) out[i]=0;
	
	for(i=w;i<w*(h-1);i++){
		/* strictly greater! */
		if((in[i]>in[i-1])||(in[i]>in[i+1])||(in[i]>in[i-w])||(in[i]>in[i+w])){
			if(in[i] >= in[i+1]){
				if(in[i] >= in[i-w]){
					if(in[i] >= in[i+w]){
						out[i]=255;
					}
				}
			}
		}
	}
}

/* distort image *in according to motion vectors vecx,vecy. acc is used
 * for temporary working data. Uses the explicit method. */
void distort_vec_expl(unsigned char *in, unsigned char *out, float *vecx, float *vecy, int *acc, int width, int height)
{
	int i,j;
	unsigned char *ino;
	float *vecxo,*vecyo;
	int destidx;
	int vecxi,vecyi;
	
	for(i=0;i<width*height;i++) acc[i]=0;
	for(i=0;i<width*height;i++) out[i]=0;
	
	for(j=0;j<height;j++){
		ino = in + j*width;
		vecxo = vecx + j*width;
		vecyo = vecy + j*width;
		for(i=0;i<width;i++){
			vecxi = i + (int)(vecxo[i]);
			vecyi = j + (int)(vecyo[i]);
			
			/* make sure within bounds */
			if(vecxi < 0) continue;
			if(vecyi < 0) continue;
			if(vecxi >= width) continue;
			if(vecyi >= height) continue;
			
			if((vecxi + width*vecyi) < 0 ) return;
			if((vecxi + width*vecyi) >= width*height ) return;
			
			destidx = vecxi + width*vecyi;
			acc[destidx] += ino[i]; 
			/* use out for temporary storage */
			out[destidx]++;
		}
	}
	
	for(i=0;i<width*height;i++){
		if(out[i]==0){
			out[i]=in[i];
		} else {
			out[i]=acc[i]/out[i];
		}
	}
}

#ifdef FAST_AND_LOOSE
#warning if downsampling causes errors turn off FAST_AND_LOOSE
#endif

/* down-samples image by 2 */
void downsamplex2(int *out, int *in, int w, int h, int outw)
{
	int i,j;
	int *ino1,*ino2;
	int *outo;
	int outp;
	
	for(j=0;j<h;j+=2){
		ino1 = in  + j*w;
		ino2 = ino1 + w;
		outo = out + (j/2)*outw;
		for(i=0;i<w;i+=2){
#ifdef FAST_AND_LOOSE
			outp = (ino1[i]+ino2[i]+ino1[i+1]+ino2[i+1])>>2;
#else
			outp = (((ino1[i  ]>>1)+(ino2[i  ]>>1))>>1)
			     + (((ino1[i+1]>>1)+(ino2[i+1]>>1))>>1); 
#endif
			*(outo) = outp;
			outo++;
		}
	}
}

unsigned int block_variance(int *in, int w, int h, int pitch)
{
	int i,j;
	unsigned int avg,var;
	int dif;
	int *ino;
	avg=0;
	for(j=0;j<h;j++){
		ino = in + j*pitch;
		for(i=0;i<w;i++){
			avg+=ino[i];
		}
	}
	avg /= (h*w);

	var=0;
	for(j=0;j<h;j++){
		ino = in + j*pitch;
		for(i=0;i<w;i++){
			dif = ino[i] - avg;
			var = var + (dif*dif);
		}
	}
	return var/(h*w);
}

/* subdivides an image into blocks, takes the variance of each
 * block, returns average variance */
/* if variance of block is larger than 2 times running average,
 * discards it (this is to separate noisy blocks from signal blocks) */
int noise_variance(int *in, int w, int h, int block)
{
	int x,y;
	unsigned int thisvar,var;
	int c;
	c=0;
	var=0;
	for(x=0;x<w;x+=block){
		for(y=0;y<h;y+=block){
			thisvar = block_variance(in + x + y*w, block, block, w);
			if(thisvar<0){
				printf("warning!\n");
				continue;
			}
			//if(block==16) printf("c:%d thisvar: %d var:%d\n",c,thisvar,var);
			/*if((c==0)||(thisvar <= (2*var)/c)){
				var += thisvar;
				c++;
			}*/
			if((c==0)||(thisvar < var)) var=thisvar;
			c++;
		}
	}
	//return var/c;
	return var;
}

#ifdef TEST_MODE

#define SX 12
#define SY 12

void printest(int *a, int w, int h)
{
	int x,y;
	
	for(y=0;y<h;y++){
		for(x=0;x<w;x++){
			printf("%7d ", a[y*h + x]);
		}
		printf("\n");
	}
}

int main()
{
	int i;
	
	/* gaussian blur test */
	int in [SX*SY];
	int out[SX*SY];
	int tmp[SX*SY];
	
	for(i=0;i<SX*SY;i++) in [i]=0;
	for(i=0;i<SX*SY;i++) tmp[i]=0;
	for(i=0;i<SX*SY;i++) out[i]=0;
	
	in[78] = 10;
	
	gaussian_blur(in, out, tmp, SX, SY, 1.0);
	
	printest(out, SX, SY);
	
	pix_type prev[SX*SY];
	pix_type next[SX*SY];
	
	/* shift_cmp test */
	for(i=0;i<SX*SY;i++) prev[i]=0;
	for(i=0;i<SX*SY;i++) next[i]=0;
	prev[78] = 255;
	next[66] = 255;
	
	shift_cmp(prev, next, in, SX, SY, 0, 0, 8);
	
	printf("\n");
	printest(in, SX, SY);
	
	int difw[SX*SY];
	char mx[SX*SY];
	char my[SX*SY];
	float conf[SX*SY];
	
	motion_detect(prev, next, SX, SY, 1.0, 8, 3, 3, mx, my, conf);

	printf("(mx,my)[index]: %d,%d\n", mx[78],my[78]);
	printf("(mx,my)[index]: %d,%d\n", mx[75],my[75]);
	
	return 0;
}

#endif
