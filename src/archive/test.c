/* COMPILE: -lics
 */

/* Test to see if we can use the ICS library to open microscope data */

#include <stdint.h>
#include <stdio.h>
#include "libics.h"		/* Lib ICS */

#define ICS_CHECK(c) if(c != IcsErr_Ok){printf("Ics_Error = %d\n", c); return c;}

#define pix_type uint16_t

#define DEBUG_PRINT

int main(int argc, char **argv)
{

	Ics_Error icserr;
	ICS* ics;
	size_t dims[ICS_MAXDIM];
	int ndims;
	int i;
	Ics_DataType dt;
	
	/* TODO */
	pix_type *data;
	
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

	/* allocate space before reading */
	/* Only allocate for one frame; allows images with many frames
	 * to be loaded without much memory, unlike some programs
	 * (I'm looking at you, ics_opener/ImageJ ! */
	data = malloc(dims[1]*dims[2]*sizeof(pix_type));
	if(!data){
		printf("Couldn't allocate enough memory.\n");
		return 1;
	}
	


	IcsClose(ics);

	return 0;
}
