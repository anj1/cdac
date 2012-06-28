/* COMPILE: filters.c watershed.c boxmuller.c blur.c -Wall -lm -lics -O3
 */

/* follow nuclei around */

#include <stdint.h>        /* pix_type */
#include <stdlib.h>			/* strtol() */
#include <malloc.h>        /* malloc() */
#include <stdio.h>
#include "libics.h"        /* Lib ICS */
#include "global.h"
#include "filters.h"
#include "blur.h"
#include "watershed.h"

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
    fprintf(f,"%d\n%d\n", width, height);
    fprintf(f,"255\n");
   
    fwrite(buf, 1, width*height, f);
   
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
    /* TODO */
    pix_type *prev;    /* previous frame */
	unsigned char *buf; /* 8-bit buffer */
	int startframe,endframe;
	int *tmp1,*tmp2,*var,*sob;
	int xmark[2],ymark[2];
	unsigned char *wsh;
	FILE *comf;
	char fname[64],cmdlin[128];
	
    if(argc < 4){
        printf("usage: nucfollow <filename>.ics <x> <y> [startframe [endframe]] [-e cutoff]\n");
        printf("(x,y) is the coordinate of the nucleus at first iteration.\n");
        printf("example: nucfollow livecell.ics 400 300 30\n");
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
    
    /* marker */
    xmark[0] = 10;
    ymark[0] = 10;
    xmark[1] = strtol(argv[2],NULL,0);
    ymark[1] = strtol(argv[3],NULL,0);
    
	/* number of frames specified */
	startframe = 0;
	endframe = 1;
	if(argc >= 5){
		startframe = strtol(argv[4],NULL,0);
		if(startframe > dims[3]) startframe = dims[3];
		if(startframe < 0){
			printf("Start frame number must be non-negative.\n");
			return 1;
		}
		if(argc >= 6){
			endframe = strtol(argv[5],NULL,0);
			if(endframe < startframe){
				printf("End frame number must be greater than start.\n");
				return 2;
			}
			if(endframe > dims[3]) endframe = dims[3];
		} else {
			endframe = startframe+1;
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
    tmp1 = (int *)malloc(dims[1]*dims[2]*sizeof(int));
    tmp2 = (int *)malloc(dims[1]*dims[2]*sizeof(int));
    var  = (int *)malloc(dims[1]*dims[2]*sizeof(int));
    sob  = (int *)malloc(dims[1]*dims[2]*sizeof(int));
    wsh  = (unsigned char*)malloc(dims[1]*dims[2]*sizeof(int));
    if((!prev)||(!buf)||(!tmp1)||(!tmp2)||(!var)||(!sob)||(!wsh))
    {
        printf("Couldn't allocate enough memory.\n");
        return 1;
    }
   
    /* open CoM file for output */
    comf = fopen("comf.txt","w");
    if(!comf){
		printf("error: couldn't open comf.txt to write center of mass data!\n");
		return 22;
	}
	fprintf(comf,"frame x y\n");	/* legend */
	
	printf("tracking...\n");
	
    /* read sequentially from file. */
    /* Each call to IcsGetDataBlock proceeds from the end of the last */
    for(i=0;i<endframe;i++){
        icserr = IcsGetDataBlock(ics, prev, dims[1]*dims[2]*2);
        ICS_CHECK(icserr);
		
		if(i <startframe) continue;
		
		/* we are at the desired frame sequence. */
		
		/* variance filter */
		printf("frame %d\n", i);
		
		printf(" Variance filter...\n");
#if 0
		if(variance(prev,var,tmp1,tmp2,dims[1],dims[2],1.0,4.0,12))
			printf("Variance filter error!\n");
#endif
		fast_variance(prev, var, tmp1, dims[1], dims[2]);
		
		printf(" Cusp blur...\n");
		blur_cusp(var,tmp1,dims[1],dims[2],230);
		/*printf("Diffusion blur...\n");
		blur_diffuse(var,tmp2,dims[1],dims[2]);
		blur_diffuse(tmp2,var,dims[1],dims[2]);
		*/
		int maxo;
		maxo=0;
		for(j=0;j<dims[1]*dims[2];j++) if(var[j]>maxo) maxo=var[j];
		printf(" maxo: %d\n", maxo);
		
		printf(" Sobel edge detection...\n");
		sobel(var, sob, dims[1], dims[2]);
		
		/* reduce the output of sobel before watershed;
		 * this greatly reduces processing time */
		/* TODO: infer this from the actual histographic data */
		for(j=0;j<dims[1]*dims[2];j++) sob[j] /= 10;
		for(j=0;j<dims[1]*dims[2];j++) if(sob[j]>255) sob[j]=255;
		
		printf(" watershed assembly...\n");
		if(watershed_rep(sob, wsh, tmp1, dims[1], dims[2], xmark, ymark, 2, buf, 8, 8.0)){
			printf(" error during watershed!\n");
			return 1;
		}
		
		/* print center of mass to file */
		fprintf(comf,"%d %d %d\n", i, xmark[1], ymark[1]);
		
		/* modify mark */
		if(center_of_mass(buf, dims[1], dims[2], 8, xmark+1, ymark+1)){
			printf(" zero identified regions!\n aborting.\n");
		    break;	/* abort so we can at least see the results */
		}
		
		/* output superimposed image */
		for(j=0;j<dims[1]*dims[2];j++)
			wsh[j] = prev[j] >> 4;
		
		hardedge(buf, wsh, dims[1], dims[2]);
		sprintf(fname,"vesw_sup_%04d.pgm",i);
		export_8bit_pgm(wsh, dims[1], dims[2], fname);
		sprintf(cmdlin,"pnmtopng < %s > vesw_sup_%04d.png\nrm %s\n", fname, i, fname);
		if(system(cmdlin)){
			printf("Error during NetPbm invoke!\n");
			return 55;
		}
		
    }

	buf[ymark[1]*dims[1] + xmark[1]]=255-buf[ymark[1]*dims[1] + xmark[1]];	/* mark next CoM */
	export_8bit_pgm(buf, dims[1], dims[2], "vesw.pgm");
		
	for(j=0;j<dims[1]*dims[2];j++) buf[j] = var[j]/64;
	export_8bit_pgm(buf, dims[1], dims[2], "ve.pgm");

	for(j=0;j<dims[1]*dims[2];j++) buf[j] = sob[j]/8;
	export_8bit_pgm(buf, dims[1], dims[2], "ves.pgm");
	
	fclose(comf);
	
    /* no need to free memory; that's just silly */
   
    IcsClose(ics);
    return 0;
}
