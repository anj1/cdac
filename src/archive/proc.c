/* COMPILE: -lics
 */

/* Test to see if we can use the ICS library to open microscope data */

#include <stdint.h>		/* pix_type */
#include <malloc.h>		/* malloc() */
#include <stdio.h>
#include "libics.h"		/* Lib ICS */

#define ICS_CHECK(c) if(c != IcsErr_Ok){printf("Ics_Error = %d\n", c); return c;}

#define SWAP(a,b)  {tmp=a; a=b; b=tmp;}

#define pix_type uint16_t

#define DEBUG_PRINT

/* even though PGM supports 16 bit, not all imaging software does */
void export_8bit_pgm(pix_type *buf, int width, int height, char *filename)
{
	FILE *f;
	char outb;
	pix_type inw;
	int i;
	
	f = fopen(filename, "wb");
	
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

int main(int argc, char **argv)
{

	Ics_Error icserr;
	ICS* ics;
	size_t dims[ICS_MAXDIM];
	int ndims;
	int i;
	Ics_DataType dt;
	
	/* TODO */
	pix_type *prev;	/* previous frame */
	pix_type *next; /* next frame */

	pix_type *tmp;
	
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
	if((!prev)||(!next)){
		printf("Couldn't allocate enough memory.\n");
		return 1;
	}
	
	/* read sequentially from file. */
	/* Each call to IcsGetDataBlock proceeds from the end of the last */
	for(i=0;i<dims[3];i++){
		SWAP(prev, next);	/* swap buffers */
		
		icserr = IcsGetDataBlock(ics, next, dims[1]*dims[2]*2);
		ICS_CHECK(icserr);
		if(i==0) continue;	/* wait for at least two complete frames */
	
		if(!(i % 100)){	/* every 5 hours */
			char fname[32];
			sprintf(fname, "out/frame_%d.pgm", i);
			export_8bit_pgm(next, dims[1], dims[2], fname);
		}
	}

	/* no need to free memory; that's just silly */
	
	IcsClose(ics);
	return 0;
}
