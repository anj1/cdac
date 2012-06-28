/* COMPILE: vis_motion.c filters.c -O2 -lm -lics
 */

/* motion detection on microscope data */

#include <stdint.h>        /* pix_type */
#include <malloc.h>        /* malloc() */
#include <stdio.h>
#include "libics.h"        /* Lib ICS */
#include "global.h"
#include "point2d.h"
#include "filters.h"
#include "vis_motion.h"	/* export_motion */

#define ICS_CHECK(c) if(c != IcsErr_Ok){printf("Ics_Error = %d\n", c); return c;}

#define SWAP(a,b)  {tmp=a; a=b; b=tmp;}

#define DEBUG_PRINT

/* even though PGM supports 16 bit, not all imaging software does */
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

void export_8bit_pgm(char *buf, int width, int height, char *filename)
{
    FILE *f;
    unsigned char outb;
    int i;
   
    f = fopen(filename, "wb");
   
    if(!f){
		printf("error: couldn't open %s for output\n", filename);
		return;
	}
	
    fprintf(f,"P5\n%d %d\n", width, height);
    fprintf(f,"255\n");
   
    for(i=0;i<width*height;i++){
		outb = 128 + 8*buf[i];
        fwrite(&outb,1,1,f);
    }
   
    fclose(f);
}

void export_float_pgm(float *a, int width, int height, char *filename)
{
    FILE *f;
    unsigned char outb;
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
		outb = 255.0 * a[i];
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
   
	int *dif,*difb,*difw,*dtmp;
	char *mx,*my,*mt;
	float *conf;
	
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
    prev = malloc(dims[1]*dims[2]*sizeof(pix_type));
    next = malloc(dims[1]*dims[2]*sizeof(pix_type));
    mx   = malloc(dims[1]*dims[2]*sizeof(char));
    my   = malloc(dims[1]*dims[2]*sizeof(char));
    mt   = malloc(dims[1]*dims[2]*sizeof(char));
    conf = malloc(dims[1]*dims[2]*sizeof(float));
    if((!prev)||(!next)||(!mx)||(!my)||(!mt)||(!conf))
    {
        printf("Couldn't allocate enough memory.\n");
        return 1;
    }
   
    /* read sequentially from file. */
    /* Each call to IcsGetDataBlock proceeds from the end of the last */
    for(i=0;i<2;i++){
        SWAP(prev, next);    /* swap buffers */
       
        icserr = IcsGetDataBlock(ics, next, dims[1]*dims[2]*2);
        ICS_CHECK(icserr);
        if(i==0) continue;    /* wait for at least two complete frames */
      
		printf("frame %d\n", i);
		motion_detect(prev, next, dims[1], dims[2], 1.0, 8, 10, 10, mx,my,conf);
#if 0	
		smooth_mv(mx, mt, conf, dims[1], dims[2]);
		smooth_mv(mt, mx, conf, dims[1], dims[2]);
		smooth_mv(my, mt, conf, dims[1], dims[2]);
		smooth_mv(mt, my, conf, dims[1], dims[2]);
		smooth_mv(my, mt, conf, dims[1], dims[2]);
		smooth_mv(mt, my, conf, dims[1], dims[2]);
#endif	
		export_12bit_pgm(prev, dims[1], dims[2], "out/prev.pgm");
		export_12bit_pgm(next, dims[1], dims[2], "out/next.pgm");
		//export_8bit_pgm(my, dims[1], dims[2], "out/motion.pgm");
		export_motion(mx,my,mt,conf,dims[1],dims[2],"out/move.pgm");
		export_float_pgm(conf, dims[1], dims[2], "out/conf.pgm");
		
#if 0
        if(!(i % 100)){    /* every 5 hours */
            char fname[32];
            sprintf(fname, "out/frame_%d.pgm", i);
            export_12bit_pgm(next, dims[1], dims[2], fname);
        }
#endif
    }

    /* no need to free memory; that's just silly */
   
    IcsClose(ics);
    return 0;
}
