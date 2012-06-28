/* COMPILE: boxmuller.c -lm -Wall -O3 -DTEST_MODE
 * -DREPEAT
 */
/* my simple implementation of the marked watershed algorithm */
/* Note: program WILL NOT WORK in endian systems other than intel */

/* Started writing on June 10 2011
 * First successfull test: June 11 2011
 */

#ifdef TEST_MODE
#include <malloc.h>
#endif
#include <stdio.h>


#include "boxmuller.h"	/* for producing normal variables */

/* we have a `boundary queue' which we add pixels on to.
   At every step, we take a pixel off the queue and if it's neighbors
   are below a certain threshold value we push them on the queue.
   It's a bit like flood-fill, if that helps.
   If all of a pixel's neighbors are on the queue, we don't push itself.
   We continue doing this until no more pixels can be pushed.
   Then we increase the threshold value.
   If two different boundary pixels of different marks meet we store it.
*/

/* maximum size of pixel stack */
#define PIX_QUEUE_SIZE 100000

/* check that the queue is integral */
#define CHECK_Q(qidx) { \
	if(qidx >= PIX_QUEUE_SIZE) qidx -= PIX_QUEUE_SIZE; \
	if((nq>=PIX_QUEUE_SIZE)||(nq<0)) return 3; \
}

#define PUSH_Q(x,y,mark) { \
	bqx[qlast] = x; \
	bqy[qlast] = y; \
	bqm[qlast] = mark; \
	qlast++; \
	nq++; \
	CHECK_Q(qlast); \
}

#define POP_Q(x,y,mark) { \
	x = bqx[qfirst]; \
	y = bqy[qfirst]; \
	mark = bqm[qfirst]; \
	qfirst++; \
	nq--; \
	CHECK_Q(qfirst); \
}

/*  in: image.
   out: marked output
     w: width of input (and output)
     h: height of input (and output)
  markx,marky: coordinates for markers.
   nmark: number of markers.
*/ 
/*
 * error codes: 1: marker x,y negative
 *              2: marker x,y too big
 *              3: too many boundary points
 */
/* TODO: maxval instead of 255 */
int watershed(int *in, unsigned char *out, int *dup, int w, int h, int *markx, int *marky, int nmark)
{
	int i;
	
	int bx,by,bm;
	int bidx;
	int newb;
	int nbcount;
	int thresh;
	int curnq;
	int iteration;
	
	int bqx[PIX_QUEUE_SIZE];
	int bqy[PIX_QUEUE_SIZE];
	int bqm[PIX_QUEUE_SIZE];
	int qfirst,qlast;	/* positions in queue */
	int nq;				/* number of items in queue */
	
	/* initialize queue */
	qfirst=0;
	qlast =0;
	nq    =0;
	
	/* first, make sure output is zero */
	for(i=0;i<w*h;i++) out[i]=0;
	for(i=0;i<w*h;i++) dup[i]=-1;

	/* now, pop on the first markers */
	for(i=0;i<nmark;i++){
		/* make sure the marks are within bounds */
		if(markx[i] <  0) return 1;
		if(marky[i] <  0) return 1;
		if(markx[i] >= w) return 2;
		if(marky[i] >= h) return 2;
		
		PUSH_Q(markx[i],marky[i],i+1);	/* i+1 because 0 means 'empty'*/
	}
	
	/* for increasing values of the threshold */
	/* (we increase threshold at bottom, see below) */
	iteration=0;
	for(thresh=0;thresh<256;iteration++){
		
		newb=0;
		//fprintf(stderr,"nq:%d\n",nq);
		/* we can not use nq directly because it is constantly changing */
		curnq = nq;
		for(i=0;i<curnq;i++){
			POP_Q(bx,by,bm);	/* take the next item */
			
			bidx = by*w + bx;
			
			/* this prevents duplicates */
			/* -thresh because we want to revisit them on the next
			 * iteration */
			if(dup[bidx]==iteration) continue;
			dup[bidx] = iteration;
			
			if(in[bidx] > thresh){
				/* still a boundary and not yet `in' the space */
				/* put back in and wait for a better day */
				PUSH_Q(bx,by,bm);
				continue;
			}
			
			/* in the marked space! */
			out[bidx] = bm;
			newb = 1;
			
			/* now do the neighbors */

			/* left */
			if(bx >= 1)
				if(!out[bidx-1]){
					PUSH_Q(bx-1,by,bm);
					nbcount++;
				}
				
			/* right */
			if(bx <= w-2)
				if(!out[bidx+1]){
					PUSH_Q(bx+1,by,bm);
					nbcount++;
				}

			/* top */
			if(by >= 1)
				if(!out[bidx-w]){
					PUSH_Q(bx,by-1,bm);
					nbcount++;
				}
				
			/* bottom */
			if(by <= h-2)
				if(!out[bidx+w]){
					PUSH_Q(bx,by+1,bm);
					nbcount++;
				}
		}	/* i */
		
		/* only continue when there have been no boundary changes */
		if(!newb) thresh++;
		
	}	/* thresh */
	
	return 0;
}

/* perform watershed transform many times, averaging over them */
/* at each iteration, randomizes marker positions and adds a gradient */
/* out, dup and rep are strictly outputs */
int watershed_rep(int *in, unsigned char *out, int *dup, int w, int h, int *markx, int *marky, int nmark, unsigned char *rep, int nrep, double rrad)
{
	int markrx[256],markry[256];	/* randomized marks */
	double rx,ry;
	int c,i;
	
	/* first, make sure repetitions are zero */
	for(i=0;i<w*h;i++) rep[i]=0;
	
	for(c=0;c<nrep;c++){
		/* alter marker positions */
		for(i=0;i<nmark;i++){
			box_muller2(&rx,&ry);
			markrx[i] = markx[i] + (int)(rx*rrad);
			markry[i] = marky[i] + (int)(ry*rrad);
		}
		
		/* set dup to zero */
		for(i=0;i<w*h;i++) dup[i]=0;
		
		/* do watershed */
		/* we don't care if marks are negative or whatnot due to randomization*/
		if(watershed(in, out, dup, w, h, markrx, markry, nmark)==3) return 3;
		
		/* count */
		for(i=0;i<w*h;i++){
			if(out[i]==2) rep[i]++;
		}
	}
	
	return 0;
}

/* load markers for watershed transform from file */
int load_markers(char *markfile, int *markx, int *marky, int *nmark)
{
	FILE *f;
	
	f = fopen(markfile, "r");
	if(!f){
		fprintf(stderr, "Couldn't load %s\n", markfile);
		return 1;
	}
	
	(*nmark)=0;
	while(!feof(f)){
		if(fscanf(f, "%d ", markx + (*nmark))!=1) continue;
		if(fscanf(f, "%d ", marky + (*nmark))!=1) return 2;	/* unmatched pair */	
		(*nmark)++;
	}
	
	fclose(f);
	return 0;
}

/* given the number of repetitions that the watershed assembly has
 * found, computes the center of mass to obtain the marker for
 * the next iteration */
/* of course, the implicit assumption is made that the nucleus contains
 * no holes */
/* iter: maximum number of iterations. Regions with less than half
 * of iter are ignored */
int center_of_mass(unsigned char *rep, int w, int h, int iter, int *markx, int *marky)
{
	int sumx,sumy;
	int sumw;
	unsigned char *repo;
	int x,y;
	int iter2;
	
	iter2 = iter/4;
	
	sumx=0; sumy=0; sumw=0;
	sumx+=markx[0];
	sumy+=marky[0];
	sumw+=1;
	
	for(y=0;y<h;y++){
		repo = rep + y*w;
		for(x=0;x<w;x++){
			if(repo[x]<iter2) continue;
			sumx += x*((int)repo[x]);
			sumy += y*((int)repo[x]);
			sumw += repo[x];
		}
	}
	/* if sumw zero, major error */
	if(!sumw) return 1;
	
	(*markx) = sumx / sumw;
	(*marky) = sumy / sumw;
	
	return 0;
}

#ifdef TEST_MODE

#define FSCAN_CHECK(s, var) { if(fscanf(stdin, s, &var) != 1) { fprintf(stderr, "Input error.\n"); return 1; } }

int load_data(int **pin, int *w, int *h)
{
	int type,width,height,maxval,Bpp; /* pgm parameters */
	int *in;
	int i;
	
	/* first, header */
	FSCAN_CHECK("P%d",type);
	if(type!=5) { fprintf(stderr, "invalid pam type (%d)\n", type); return 2; }
	FSCAN_CHECK("%d",width);
	FSCAN_CHECK("%d",height);
	FSCAN_CHECK("%d\n",maxval);
	//for(i=0;newline!=0x0A;i++) if(fread(&newline,1,1,stdin)!=1) return 5;	/* search for newline */
	
	/* determine bytes to read per pixel */
	Bpp=4;
	if(maxval<0x1000000) Bpp=3;
	if(maxval<  0x10000) Bpp=2;
	if(maxval<    0x100) Bpp=1;
	
	/* now, allocate room for data */
	in = (int *)malloc(width * height * sizeof(int));
	if(!in){
		fprintf(stderr, "Couldn't allocate memory for %d x %d input.\n", width, height);
		return 3;
	}
	
	/* load data */
	for(i=0;i<width*height-1;i++){
		in[i]=0;	/* for when Bpp < 4 */
		if(fread(in + i,Bpp,1,stdin)!=1) {
			fprintf(stderr,"Couldn't load image data.\n");
			return 4;
		}
	}	
	
	*pin = in;
	*w = width;
	*h = height;
	
	return 0;
}

int write_data(unsigned char *out, int width, int height)
{
	printf("P5\n%d %d\n%d\n", width, height, 255);
	if(fwrite(out, 1, width*height, stdout)!=width*height){
		fprintf(stderr,"Couldn't write output to stdout.\n");
		return 1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	int width,height; /* pgm parameters */
	int markx[256];	/* 256 because char */
	int marky[256];
	int nmark;
	
	int *in;
	unsigned char *out;
	int *dup;	/* for keeping track of duplicates */
#ifdef REPEAT
	unsigned char *rep;	/* number of repetitions for which pixel falls into region */
#endif
	int err;
	
	/* check for correct format */
	if(argc<2){
		printf("Usage: watershed <marker file>\n");
		printf("Takes (pgm) image from stdin.\n");
		printf("Example marker file:\n");
		printf("\t10 100\n");
		printf("\t20 50\n");
		return 0;
	}
	
	/* load marker file */
	if((err = load_markers(argv[1], markx, marky, &nmark))){
		fprintf(stderr,"Error during marker reading.\n");
		return err;
	} 
	
	/* load pgm from standard input */
	if((err = load_data(&in, &width, &height))){
		fprintf(stderr,"Error during image data load.\n");
		return err;
	}
	
	//printf("first pixels: %d %d\n", in[0], in[1]);
	/* prepare output */
	out = (unsigned char  *)malloc(width * height * sizeof(char));
	dup = (int *)malloc(width * height * sizeof(int));
	if(!out){fprintf(stderr,"couldn't allocate space for output.\n");return 10;}
	if(!dup){fprintf(stderr,"couldn't allocate space for output.\n");return 10;}
#ifdef REPEAT
	rep = (unsigned char *)malloc(width * height * sizeof(int));
	if(!rep){fprintf(stderr,"couldn't allocate space for output.\n");return 10;}
#endif
	
	switch(watershed(in, out, dup, width, height, markx, marky, nmark)){
	case 1: fprintf(stderr,"ERROR: Mark negative.\n"); 				break;
	case 2: fprintf(stderr,"ERROR: Mark out of bounds.\n"); 		break;
	case 3: fprintf(stderr,"ERROR: Too many boundary points.\n");	break;
	default:;
	}
	
	/* write output */
#ifndef REPEAT
	int i;
	/* make marks visible */
	for(i=0;i<nmark;i++){
		out[marky[i]*width + markx[i]] = 0;
	}
	/* do output */
	write_data(out, width, height);
#endif

#ifdef REPEAT
	watershed_rep(in, out,dup, width, height, markx, marky,nmark, rep, 16, 1.5);
	write_data(rep, width, height);
#endif
	
	return 0;
}

#endif
