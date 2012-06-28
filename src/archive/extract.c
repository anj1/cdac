/* COMPILE: histnorm.c -lm -lics -O2
 */

/* extraction of microscope data to PNG */

/* mplayer/mencoder "mf:///dev/shm/micro/frame_*.png" -mf w=800:h=600:fps=10:type=png -ovc x264 -x264encopts subq=5 -o /home/alireza/m1_13_20.avi */

#define pix_type uint16_t

#include <stdint.h>        /* pix_type */
#include <malloc.h>        /* malloc() */
#include <stdio.h>
#include "libics.h"        /* Lib ICS */
#include "histnorm.h"		/* histogram normalization primitives */

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
	int emin,emax;
	
    if(argc < 2){
        printf("usage: extract <filename>.ics [startframe [endframe]] [-e cutoff]\n");
        printf("the -e option enhances the contrast using histogram analysis.\n");
        printf("TODO: enabled by default, cutoff=16\n");
        printf("example: extract livecell.ics 30 35 e\n");
        printf("extracts all frames if start frame not specified\n");
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
	if(argc >= 3){
		startframe = strtol(argv[2],NULL,0);
		if(startframe > dims[3]) startframe = dims[3];
		if(startframe < 0){
			printf("Start frame number must be non-negative.\n");
			return 1;
		}
		if(argc >= 4){
			endframe = strtol(argv[3],NULL,0);
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
    prev = malloc(dims[1]*dims[2]*sizeof(pix_type));
    buf = malloc(dims[1]*dims[2]*sizeof(unsigned char));
    if((!prev)||(!buf))
    {
        printf("Couldn't allocate enough memory.\n");
        return 1;
    }
   
	printf("tracking...\n");
	
    /* read sequentially from file. */
    /* Each call to IcsGetDataBlock proceeds from the end of the last */
    for(i=0;i<endframe;i++){
        icserr = IcsGetDataBlock(ics, prev, dims[1]*dims[2]*2);
        ICS_CHECK(icserr);
			
        if(i>=startframe){
			printf("frame %d\n", i);
			
			//for(j=0;j<dims[1]*dims[2];j++)
				//buf[j] = prev[j]; >> 4;
			
			/* normalize all images to first output frame */
			if(i==startframe) edr(prev, dims[1], dims[2], &emin, &emax);
			
			normalize(prev, buf, dims[1], dims[2], emin, emax);
			
			sprintf(fname, "frame_%04d.png", i);
			
			export_8bit_pgm(buf, dims[1], dims[2], "tmp.pgm");
			
			//alib_savebitmap(fname, buf, dims[1], dims[2], 8, 1);

			sprintf(cmdlin, "pnmtopng -compression 1 < tmp.pgm > %s", fname, fname);
			system(cmdlin);
		}
    }

    /* no need to free memory; that's just silly */
   
    IcsClose(ics);
    return 0;
}
