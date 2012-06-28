/* COMPILE: pclin.c -Wall -O2 -lm
 */
 
/* perform fourier transform on cell boundary */

#include <stdlib.h> /* strtof() */
#include <stdio.h>
#include <malloc.h>	/* realloc() */
#include <ctype.h>	/* isspace() */
#include <complex.h>
#include <math.h>	/* sqrt() */
#include "pclin.h"

#ifndef PI
#define PI 3.141592654
#endif


/* this doesn't need to be a stiff maximum */
#define LINE_SIZE 8192

typedef struct ext_vec
{
	float *d;
	size_t n;		/* number of elements in use */
	size_t nallocd;	/* number of elements allocated */
}ext_vec;

void init_vector(ext_vec *v)
{
	v->d = NULL;
	v->n = 0;
	v->nallocd = 0;
}

int resize_vector(ext_vec *v, size_t n)
{
	if(n > v->nallocd){
		v->d = realloc(v->d, n * sizeof(float));
		if(!v->d){
			fprintf(stderr,"Error: couldn't allocate enough memory.\n");
			return 1;
		}
		v->nallocd = n;
	}
	v->n = n;
	return 0;
}

#if 0
void calc_lens(ext_vec *t, ext_vec *x, ext_vec *y)
{
	int i;
	float x1,y1,x2,y2,tlen;
	float dx,dy;
	float nrm;
	
	x1=x->d[x->n-1];
	y1=y->d[y->n-1];
	tlen=0;
	for(i=0;i<x->n;i++){
		x2=x->d[i];
		y2=y->d[i];
		
		dx = x2-x1;
		dy = y2-y1;
		t->d[i] = 1.0; //sqrt(dx*dx + dy*dy);
		tlen += t->d[i];
		
		x1=x2;
		y1=y2;
	}
	
	nrm = 2*PI/tlen;
	for(i=0;i<x->n;i++) t->d[i] *= nrm;
}
#endif

/* First of all, the phases are absolute, but for piecewise-linear
 * representation we need them to be relative.
 * But there's more.
 * the phases from robie are kind of weird in that the first one can be
 * 0.0, and they need not necessarily sum to 2*PI. Thus, they must be
 * modified to piecewise-linear format.
 * So, we first offset so that the final point is on 2*PI,
 * then we must shift the whole thing (see shift_left)
 */
void correct_phases(ext_vec *t)
{
	float offset;
	float *d;
	float prevt,tmpt;
	int n;
	int i;
	
	d = t->d;
	n = t->n;
	
	offset = 2*PI - d[n-1];
	prevt=d[0];
	d[0]+=offset;	/* shift the whole thing */
	for(i=1;i<n;i++){
		tmpt = d[i];
		d[i] = d[i]-prevt;
		prevt = tmpt;
	}
	
}

void shift_left(ext_vec *a)
{
	float tmpd;
	int i;
	tmpd = a->d[0];
	for(i=0;i<(a->n)-1;i++){
		a->d[i]=a->d[i+1];
	}
	a->d[a->n-1]=tmpd;
}

int parseline(ext_vec *x, ext_vec *y, ext_vec *t, char *lin)
{
	int i;
	int c;
	int inspace;
	char *tmplin;
	
	/* get number of pairs in line */
	c=0; inspace=0;
	for(i=0;lin[i]!=0;i++) if(isspace(lin[i])&&(!inspace)) {c++; inspace=1;} else inspace=0;
	
	/* allocate memory */
	if(resize_vector(x,c)) return 1;
	if(resize_vector(y,c)) return 2;
	if(resize_vector(t,c)) return 3;
	
	/* read values */
	tmplin=lin;
	for(i=0;i<c;i++){
		x->d[i] = strtof(tmplin ,&tmplin);
		
		if(tmplin[0]!=','){
			fprintf(stderr,"Error: x and y values must be separated by comma\n");
			return 4;
		}
		tmplin++;
		
		y->d[i] = strtof(tmplin,&tmplin);
		
		if(tmplin[0]!=';'){
			fprintf(stderr,"Error: coordinates and phase must be separated by semicolon\n");
			return 5;
		}
		tmplin++;
		
		t->d[i] = strtof(tmplin,&tmplin);
	}
	
	return 0;
}

void test1()
{
	ext_vec x;
	ext_vec y;
	ext_vec t;
	int i;
	
	init_vector(&x);
	init_vector(&y);
	init_vector(&t);
	
	resize_vector(&x,4);
	resize_vector(&y,4);
	resize_vector(&t,4);
	
	x.d[0]=0.0;
	x.d[1]=1.0;
	x.d[2]=2.0;
	x.d[3]=3.0;
	
	y.d[0]=0.5;
	y.d[1]=1.5;
	y.d[2]=2.5;
	y.d[3]=3.5;
	
	t.d[0]=0.1*PI/2.0;
	t.d[1]=1.0*PI/2.0;
	t.d[2]=2.0*PI/2.0;
	t.d[3]=3.0*PI/2.0;
	
	correct_phases(&t);
	
	for(i=0;i<4;i++) printf("%f ",t.d[i]/(2.0*PI));
	printf("\n");
	
	shift_left(&t);
	
	for(i=0;i<4;i++) printf("%f ",t.d[i]/(2.0*PI));
}

int main(int argc, char **argv)
{
	int i,j;
	int ncoefs;
	int nlines;
	
	float complex *ftcoef;
	
	int bytes_read;
	size_t nbytes = LINE_SIZE;
	char *my_string;
	
	ext_vec x;
	ext_vec y;
	ext_vec t;
	
	init_vector(&x);
	init_vector(&y);
	init_vector(&t);
	
	if(argc < 2){
		fprintf(stderr,"Usage: pclin_ft <number of coefs>\n");
		fprintf(stderr,"Takes input from stdin, first line must be nu");
		fprintf(stderr,"mber of lines, and each line a list of x,y\n");
		return 0;
	}
	/* number of POSITIVE coefficients (we also have negative coefficients) */
	ncoefs = atoi(argv[1]);
	ftcoef = (float complex *)malloc(2 * ncoefs * sizeof(float complex));
	if(!ftcoef){
		fprintf(stderr,"Error: couldn't alloc enough memory for out\n");
		return 4;
	}
	
	/* read number of lines */
	if(scanf("%d\n",&nlines)!=1){
		fprintf(stderr,"Error: first line must be number of lines.\n");
		return 1;
	}
	
	/* print out data for matlab/octave */
	printf("# Created by pclin_ft, by Alireza Nejati.\n");
	printf("# name: ft\n");
	printf("# type: complex matrix\n");
	printf("# rows: %d\n", nlines);
	printf("# columns: %d\n", 2*ncoefs);
	
	/* initialize */
	my_string = (char *) malloc (nbytes + 1);
	
	/* read every line */
	for(i=0;i<nlines;i++){
		bytes_read = getline (&my_string, &nbytes, stdin);

		if (bytes_read == -1) {
			fprintf(stderr,"ERROR! Couldn't read line from stdin\n");
			return 2;
		} else {
			/* get data */
			if(parseline(&x,&y,&t,my_string)){
				fprintf(stderr,"Error: couldn't parse line %d\n",i);
				return 3;
			}

#if 0
			/* calculate phases */
			resize_vector(&t,x.n);
			calc_lens(&t,&x,&y);
#endif
		
			/* see explanation above correct_phases() */
			/* TODO: might need special conditions for when the first
			 * phase is not 0.0 (such as inserting an extra point),
			 * because it is our assumption that the last point is
			 * always on 2*PI */
			correct_phases(&t);
			shift_left(&x);
			shift_left(&y);
			shift_left(&t);
			
			/* calculate fourier transform */
			pclin_fourier_trans_cplx(ftcoef, ncoefs, t.d, x.d, y.d, x.n);
			
			for(j=0;j<2*ncoefs;j++){
				printf("(%f,%f)", creal(ftcoef[j]), cimag(ftcoef[j]));
				if(j!=2*ncoefs-1) printf(" ");
			}
			printf("\n");
		}
	}
	return 0;
}
