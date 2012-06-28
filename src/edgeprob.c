/* COMPILE: filters.c blur.c watershed.c boxmuller.c -Wall -lm -lics -ggdb
 */

/* edge tracking using probabilities */

#include <string.h>		/* memcpy() */
#include <stdlib.h>
#include <stdint.h>		/* pix_type */
#include <malloc.h>		/* malloc() */
#include <stdio.h>
#include "libics.h"		/* Lib ICS */
#include "global.h"		/* pix_type */
#include "filters.h"	/* sobel() */
#include "blur.h"		/* blur_cusp() */
#include "watershed.h"	/* watershed() */

#define ICS_CHECK(c) if(c != IcsErr_Ok){printf("Ics_Error = %d\n", c); return c;}

#define DEBUG_PRINT

/* even though PGM supports 16 bit, not all imaging software does */
void export_8bit_pgm(unsigned char *buf, int width, int height, char *filename)
{
    FILE *f;
   
    f = fopen(filename, "wb");
   
    if(!f){
		printf("error: couldn't open %s for output\n", filename);
		return;
	}
	
    fprintf(f,"P5\n");
    fprintf(f,"%d %d\n", width, height);
    fprintf(f,"255\n");
   
    fwrite(buf, 1, width*height,f);
   
    fclose(f);
}

/* load probabilities file */
#define FSCAN_CHECK(s, var) { if(fscanf(f, s, &var) != 1) { fprintf(stderr, "Input error.\n"); return 1; } }
int load_data(FILE *f, int **pin, int *w, int *h)
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
		if(fread(in + i,Bpp,1,f)!=1) {
			fprintf(stderr,"Couldn't load image data.\n");
			return 4;
		}
	}	
	
	*pin = in;
	*w = width;
	*h = height;
	
	return 0;
}

/* convert input images from native 16-bit format to int */
void im2int(pix_type *prev, int *out, int w, int h)
{
	int i;
	for(i=0;i<w*h;i++) out[i]=prev[i];
}

void combine_prob(int *probb, int *newprob, int *out, int w, int h)
{
	int i;
	for(i=0;i<w*h;i++) out[i] = ((probb[i]+2)*(newprob[i]+32));
}

void normalize_prob(int *inout, int w, int h)
{
	int i;
	int max;
	max=0;
	for(i=0;i<w*h;i++) if(inout[i]>max) max=inout[i];
	max = (max/255)+1;
	for(i=0;i<w*h;i++) inout[i] /= max;
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
	int startframe,endframe;
	int *tmp;
	int *probb;
	int *prob;
	int *imsob;
	FILE *f;
	int w,h;
	int err;
	/* unsigned char *wsh; */
	int markx[2],marky[2];
	unsigned char *wsh;
	
    if(argc < 2){
        printf(" usage: edgeprob <filename>.ics <prob.pgm> [startframe [endframe]] [-e cutoff]\n");
        printf(" example: edgeprob livecell.ics livecell1.pgm 30 35 e\n");
        printf(" center of cell should be marked with single dot of intensity 128\n");
        printf(" does all frames if start frame not specified\n");
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
    
	/* number of frames specified */
	startframe = 0;
	endframe = dims[3];
	if(argc >= 4){
		startframe = strtol(argv[3],NULL,0);
		if(startframe > dims[3]) startframe = dims[3];
		if(startframe < 0){
			printf("Start frame number must be non-negative.\n");
			return 1;
		}
		if(argc >= 5){
			endframe = strtol(argv[4],NULL,0);
			if(endframe < startframe){
				printf("End frame number must be greater than start.\n");
				return 2;
			}
			if(endframe > dims[3]) endframe = dims[3];
		}
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
    prev  = (pix_type *)     malloc(dims[1]*dims[2]*sizeof(pix_type));
    buf   = (unsigned char *)malloc(dims[1]*dims[2]*sizeof(unsigned char));
    tmp   = (int *)          malloc(dims[1]*dims[2]*sizeof(int));
    probb = (int *)          malloc(dims[1]*dims[2]*sizeof(int));
    prob  = (int *)          malloc(dims[1]*dims[2]*sizeof(int));
    imsob = (int *)          malloc(dims[1]*dims[2]*sizeof(int));
    wsh   = (unsigned char *)malloc(dims[1]*dims[2]*sizeof(unsigned char));
    if((!prev)||(!buf)||(!tmp)||(!probb)||(!prob)||(!imsob)||(!wsh))
    {
        printf("Couldn't allocate enough memory.\n");
        return 1;
    }
   
   	/* load probabilities */
   	f = fopen(argv[2],"r");
   	if(!f){
		printf("Error: could not open file %s\n",argv[2]);
		return 10;
	}
	if((err = load_data(f, &prob, &w, &h))){
		printf("Error: %s: incorrect or corrupt pgm file.\n",argv[2]);
		return 11;
	}
	if((w!=dims[1])||(h!=dims[2])){
		printf("Error: Size mismatch. ICS data are %ldx%ld but probability data are %dx%d\n",
		       dims[1],dims[2],w,h);
		return 9;
	}
	fclose(f);
	
	/* obtain marker from prob image */
	markx[0]=10;       marky[0]=10;
	markx[1]=markx[0]; marky[1]=marky[0];
	for(j=0;j<dims[2];j++){
		for(i=0;i<dims[1];i++){
			if(prob[j*dims[1] + i]==128){
				markx[1]=i;
				marky[1]=j;
			}
		}
	}
	if((markx[0]==markx[1])&&(marky[1]==marky[0])){
		printf("Error: marker not found in prob image.\n");
		printf("(should be single pixel with intensity 128)\n");
		return 8;
	}
	
	printf("tracking...\n");
	
    /* read sequentially from file. */
    /* Each call to IcsGetDataBlock proceeds from the end of the last */
    for(i=0;i<endframe;i++){
        icserr = IcsGetDataBlock(ics, prev, dims[1]*dims[2]*2);
        ICS_CHECK(icserr);
			
        if(i>=startframe){
			
			/* blur probabilities */
			memcpy(probb, prob, sizeof(int)*dims[1]*dims[2]);
			blur_diffuse(probb, tmp, dims[1], dims[2]);
			blur_diffuse(tmp, probb, dims[1], dims[2]);
			blur_cusp(probb, tmp, dims[1], dims[2], 210);
			blur_diffuse(probb, tmp, dims[1], dims[2]);
			blur_diffuse(tmp, probb, dims[1], dims[2]);

			/* come up with new probabilities */
			im2int(prev, tmp, dims[1], dims[2]);
			sobel(tmp, imsob, dims[1], dims[2]);
			/* convert to probability */
			for(j=0;j<dims[1]*dims[2];j++){
				imsob[j] >>= 4;
				if(imsob[j]>255) imsob[j]=255;
			}
			
			/* mix them up */
			combine_prob(probb, imsob, prob, dims[1], dims[2]);
			normalize_prob(prob, dims[1], dims[2]);

			/* watershed to 'collapse' probabilities */
			/*watershed(prob, wsh, tmp, dims[1], dims[2], markx, marky, 2);
			for(j=0;j<dims[1]*dims[2];j++){
				wsh[j] = (wsh[j]==2 ? 8 : 0);
			}*/
			watershed_rep(prob, buf, tmp, dims[1], dims[2], markx, marky, 2, wsh, 8, 24.0);
			center_of_mass(wsh, dims[1], dims[2], 8, markx, marky);
			hardedge(wsh, buf, dims[1], dims[2]);
			
			/* now combine the existing probabilities with the ones from hardedge */
			for(j=0;j<dims[1]*dims[2];j++){
				if(buf[j]==255) prob[j]=(3*255+prob[j])/4;
			}
			
			for(j=0;j<dims[1]*dims[2];j++)
				buf[j]=prob[j];
			
			printf("frame %d\n", i);
			sprintf(fname, "prob_%04d.png", i);
			export_8bit_pgm(buf, dims[1], dims[2], "tmp.pgm");
			sprintf(cmdlin, "pnmtopng -compression 1 < tmp.pgm > %s", fname);
			system(cmdlin);

		}
    }

    /* no need to free memory; that's just silly */
   
    IcsClose(ics);
    return 0;
}
