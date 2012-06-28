/* COMPILE: filters.c -O2 -lm -lics
 */

/* detect areas with movement */

#include <stdint.h>        /* pix_type */
#include <malloc.h>        /* malloc() */
#include <stdio.h>
#include "libics.h"        /* Lib ICS */
#include "global.h"
#include "filters.h"

#define ICS_CHECK(c) if(c != IcsErr_Ok){printf("Ics_Error = %d\n", c); return c;}

#define SWAP(a,b)  {tmp=a; a=b; b=tmp;}

#define DEBUG_PRINT

void export_12bit_pgm(pix_type *buf, int width, int height, char *filename)
{
    FILE *f;
    char outb;
    pix_type inw;
    int i;
   
    f = fopen(filename, "wb");
   
    if(!f){
		printf("error: couldn't open %s for output\n", filename);
		return;
	}
	
    fprintf(f,"P5\n");
    fprintf(f,"%d %d\n", width, height);
    fprintf(f,"255\n");
   
    for(i=0;i<width*height;i++){
        inw = buf[i] >> 4;
        outb = inw;
        fwrite(&outb,1,1,f);
    }
   
    fclose(f);
}

void export_12bit_moved_pgm(pix_type *buf, int *moved, int width, int height, char *filename)
{
    FILE *f;
    char outb;
    pix_type inw;
    int i;
   
    f = fopen(filename, "wb");
   
    if(!f){
		printf("error: couldn't open %s for output\n", filename);
		return;
	}
	
    fprintf(f,"P5\n");
    fprintf(f,"%d %d\n", width, height);
    fprintf(f,"255\n");
   
    for(i=0;i<width*height;i++){
        inw = buf[i] >> 4;
        outb = inw;
        if(moved[i] < 60000000) outb >>= 1;
        fwrite(&outb,1,1,f);
    }
   
    fclose(f);
}

int main(int argc, char **argv)
{

    Ics_Error icserr;
    ICS* ics;
    size_t dims[ICS_MAXDIM];
    int ndims;
    int i;
    Ics_DataType dt;
   
    /* TODO */
    pix_type *prev;    /* previous frame */
    pix_type *next; /* next frame */
    pix_type *tmp;
   
	int *dif,*difb,*dtmp;
	
    if(argc < 2){
        printf("usage: test <filename>.ics\n");
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
    prev = (pix_type *)malloc(dims[1]*dims[2]*sizeof(pix_type));
    next = (pix_type *)malloc(dims[1]*dims[2]*sizeof(pix_type));
    dif  = (int *)malloc(dims[1]*dims[2]*sizeof(int));
    difb = (int *)malloc(dims[1]*dims[2]*sizeof(int));
    dtmp = (int *)malloc(dims[1]*dims[2]*sizeof(int));
    if((!prev)||(!next)||(!dif)||(!difb)||(!dtmp))
    {
        printf("Couldn't allocate enough memory.\n");
        return 1;
    }
   
    /* read sequentially from file. */
    /* Each call to IcsGetDataBlock proceeds from the end of the last */
    for(i=0;i<2;i++){
        SWAP(prev, next);    /* swap buffers */
       
        icserr = IcsGetDataBlock(ics, next, dims[1]*dims[2]*2);	/* TODO: *2 */
        ICS_CHECK(icserr);
        if(i==0) continue;    /* wait for at least two complete frames */
      
		printf("frame %d\n", i);
		
		/* detect any motion. Areas that haven't changed significantly
		 * are discarded. Areas that have changed are kept */
		shift_cmp(prev,next,dif,dims[1],dims[2],0,0,8);
		gaussian_blur(dif, difb, dtmp, dims[1], dims[2], 4.0);
		
		export_12bit_pgm(prev, dims[1], dims[2], "out/prev.pgm");
		export_12bit_pgm(next, dims[1], dims[2], "out/next.pgm");
		//export_8bit_pgm(my, dims[1], dims[2], "out/motion.pgm");
		export_12bit_moved_pgm(next,difb,dims[1],dims[2],"out/moved.pgm");
    }

    /* no need to free memory; that's just silly */
   
    IcsClose(ics);
    return 0;
}
