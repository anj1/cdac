/* COMPILE: filters.c gabor.c -O3 -lm -lics
 */

/* line following algorithm using gabor transformation */

#include <stdint.h>        /* pix_type */
#include <math.h>			/* sqrt() */
#include <malloc.h>        /* malloc() */
#include <stdio.h>
#include "libics.h"        /* Lib ICS */
#include "global.h"
#include "gabor.h"
#include "filters.h"

#define ICS_CHECK(c) if(c != IcsErr_Ok){printf("Ics_Error = %d\n", c); return c;}

#define DEBUG_PRINT

#ifndef PI
/* PI in double-precision */
#define PI 3.141592653589793
#endif

/* even though PGM supports 16 bit, not all imaging software does */
void export_8bit_pgm(unsigned char *buf, int width, int height, char *filename)
{
    FILE *f;
   
    f = fopen(filename, "wb");
   
    if(!f){
		printf("error: couldn't open %s for output\n", filename);
		return;
	}
	
    fprintf(f,"P5");
    fprintf(f,"%d %d\n", width, height);
    fprintf(f,"255\n");
   
    fwrite(buf, 1, width*height,f);
   
    fclose(f);
}

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

/* out: confidence level for each pixel being part of a contiguous line */
int linefollow(pix_type *in, unsigned char *out, int w, int h, double sd)
{
	double angle;
	int ntrans;	/* number of gabor transformations */
	int *gabr,*gabro;
	int *gabi,*gabio;
	int *tmp;
	int ngab;
	int i,x,y;
	int tmpout;
	int tmpr,tmpi;
	
	ngab = 9; //6.0*sd;
	
	/* the larger sd is, the more transformations we have to consider */
	ntrans = 4.0*sd;
	gabr = (int *)malloc((ntrans+1)*ngab*ngab*sizeof(int));	/* ntrans+1 for floating-point errors TODO */
	gabi = (int *)malloc((ntrans+1)*ngab*ngab*sizeof(int));
	if(gabr==NULL) return 1;
	if(gabi==NULL) return 1;
	
	/* first create gabor transformations */
	gabro=gabr;
	gabio=gabi;
	for(angle=0;angle<PI;angle+=PI/((double)ntrans)){
		printf("creating transform with angle %g pi radians\n",angle/PI);
		//gabor(gabo, ngab, 2.0/sd, angle+(0.5*PI/((double)ntrans)), 0.0, sd, 1.0, 1, 255);
		gabor(gabro, ngab, 2.0/sd, angle, 0.0, sd, 1.0, 1, 255);
		gabor(gabio, ngab, 2.0/sd, angle, 0.0, sd, 1.0, 0, 255);
		gabro += ngab*ngab;
		gabio += ngab*ngab;
	}
	
	/* now allocate space for transformation outputs */
	tmp = (int *)malloc(w*h*ntrans*sizeof(int));
	if(tmp==NULL){ free(gabr); free(gabi); return 2; }
	
	for(i=0;i<ntrans;i++){
#ifdef DEBUG_PRINT
		printf("computing transform %d out of %d\n", i+1, ntrans);
#endif
		printest(gabr+i*ngab*ngab,ngab);
		printf("\n");
		printest(gabi+i*ngab*ngab,ngab);
		
		for(y=0;y<h;y++){
			for(x=0;x<w;x++){
				tmpr = apply_filter(in,w,h,x,y,gabr+i*ngab*ngab,ngab) >> 5;
				tmpi = apply_filter(in,w,h,x,y,gabi+i*ngab*ngab,ngab) >> 5;
				if((tmpr>65536)||(tmpi>65536)){
					printf("Overflow error.\n");
					return 5;
				}
				tmp[i*w*h + y*w + x] = sqrt((double)(tmpr*tmpr + tmpi*tmpi));
			}
		}
	}

#if 0
	for(x=0;x<h*w;x++){
		out[x]=0;
		for(i=1;i<ntrans;i++){
			if(tmp[i*w*h + x] > tmp[x]){
				tmp[x]=tmp[i*w*h + x];
				out[x]=32*i;
			}
		}
	}
#endif
	for(x=0;x<h*w;x++){
		tmpout=0;
		for(i=0;i<6;i++){
			tmpout += tmp[i*w*h + x];
		}
		out[x]=(tmpout>>7);
	}

	free(tmp);
	free(gabr);
	free(gabi);
	
	return 0;
}

int main(int argc, char **argv)
{

    Ics_Error icserr;
    ICS* ics;
    size_t dims[ICS_MAXDIM];
    int ndims;
    int i,j;
    Ics_DataType dt;
	char fname[32];
	char cmdlin[256];
    /* TODO */
    pix_type *prev;    /* previous frame */
	unsigned char *buf; /* 8-bit buffer */
	size_t nframe;
	int emin,emax;
	int err;
	
    if(argc < 3){
        printf("usage: linefollow <filename>.ics <frame number>\n");
        printf("example: linefollow livecell.ics 30 e\n");
        return 0;
    }
	
    /* Open file */
    icserr = IcsOpen(&ics, argv[1], "r");
    ICS_CHECK(icserr);

#ifdef DEBUG_PRINT
    printf("ics file loaded.\n");
#endif

    /* Sniff out file; make sure it is compatible with our prog */
    icserr = IcsGetLayout(ics, &dt, &ndims, dims);
    ICS_CHECK(icserr);
    if(dt != Ics_uint16){
        printf("Sorry, image modes other than 16-bit (unsigned) ");
        printf("not supported at the moment.\n");
        return 0;
    }
    
	/* select desired frame */
	nframe = strtol(argv[2],NULL,0);
	if(nframe < 0){
		printf("Start frame number must be non-negative.\n");
		return 1;
	}
	if(nframe >= dims[3]){
		printf("Error: frame %ld is not in image set!\n", nframe);
		printf("Only %ld frames present.\n", dims[3]);
		return 2;
	}
	
#ifdef DEBUG_PRINT
    printf("dimensions: ");
    for(i=0;i<ndims;i++)
        printf("%ld ", dims[i]);
    printf("data type: %d \n", dt);
#endif

    /* TODO: warning if dimensions look non-standard */
   
    /* allocate space before reading */
    /* Only allocate for one frame; allows images with many frames
     * to be loaded without much memory, unlike some programs
     * (I'm looking at you, ics_opener/ImageJ ! */
    prev = malloc(dims[1]*dims[2]*sizeof(pix_type));
    buf = malloc(dims[1]*dims[2]*sizeof(unsigned char));
    if((!prev)||(!buf))
    {
        printf("Couldn't allocate enough memory.\n");
        return 1;
    }
   
	printf("tracking...\n");
	
    /* read sequentially from file until we reach desired frame number */
    /* Each call to IcsGetDataBlock proceeds from the end of the last */
    for(i=0;i<=nframe;i++){
        icserr = IcsGetDataBlock(ics, prev, dims[1]*dims[2]*2);
        ICS_CHECK(icserr);
    }

	/* desired frame reached */
	if((err=linefollow(prev, buf, dims[1], dims[2], 1.5))){
		printf("Error %d during line-following procedure\n", err);
		return err;
	}

	export_8bit_pgm(buf, dims[1], dims[2], "imganal/gaborwin.pgm");
	system("pnmtopng < imganal/gaborwin.pgm > imganal/gaborwin.png\n");
	system("rm imganal/gaborwin.pgm\n");
    /* no need to free memory; that's just silly */
   
    IcsClose(ics);
    return 0;
}
