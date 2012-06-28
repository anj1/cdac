/* COMPILE: framelist.c textutil.c compressimg.c pgm_util.c -Wall -lm -O2
 */

/* splice together paths, create an svg file with image as background */
/* written, in entirety (along with textutil.c) on November 8th */

#define pix_type uint16_t

#include <stdint.h>		/* uint16_t (pix_type) */
#include <stdio.h>
#include <stdlib.h>		/* atol(), exit(), malloc() */
#include <string.h>		/* memcpy() */
#include <math.h>		/* floor(), ceil() */
#include "point2d.h"
#include "pgm_util.h"
#include "compressimg.h"
#include "textutil.h"
#include "framelist.h"

#define MAX_FRAMES 2048

#define NAME_LEN 1024

#ifndef INFINITY
#define INFINITY 1000000000
#endif

char usagestr[] =
"<imagefmt> <textfmt> <frames> <ncols> <outname> [infmt]\n";

/* get global bounding box */
rect_t get_boundingbox(cell_t **c, int nframes, int *n)
{
	int i,j,k,l;
	cell_t *curcell;
	point2d32f *curp;
	rect_t r;
	
	point2d32f topleft;
	point2d32f btmright;
	
	/* set initial values to ones that will surely change */
	topleft.x  = +INFINITY;
	topleft.y  = +INFINITY;
	btmright.x = -INFINITY;
	btmright.y = -INFINITY;
	
	for(i=0;i<nframes;i++){		/* over all frames */
		for(j=0;j<n[i];j++){		/* and all cells */
			curcell = c[i] + j;
			for(k=0;k<curcell->npaths;k++){	/* and all outlines */
				curp = curcell->p[k];
				for(l=0;l<curcell->n[k];l++){	/* and all vertices */
					if(curp[l].x < topleft.x ) topleft.x  = curp[l].x;
					if(curp[l].y < topleft.y ) topleft.y  = curp[l].y;
					if(curp[l].x > btmright.x) btmright.x = curp[l].x;
					if(curp[l].y > btmright.y) btmright.y = curp[l].y;
				}
			}
		}
	}
	
	r.topleft  = topleft;
	r.btmright = btmright;
	return r;
}

void export_svg(FILE *svg, cell_t **c, int *frames, int nframes, int *n, int ncols, int nrows, point2d32f topleft, int dx, int dy, char *imagename, int start)
{
	int i,j,k,l;
	cell_t *curcell;
	point2d32f *curp;
	int col,row;
	
	char xmlhead[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>";

	fprintf(svg,"%s\n",xmlhead);
	fprintf(svg,"<svg id=\"svg3119\" ");
	fprintf(svg,"xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\" ");
	fprintf(svg,"xmlns=\"http://www.w3.org/2000/svg\" ");
	fprintf(svg,"height=\"%d\" width=\"%d\" ", nrows*dy, ncols*dx);
	fprintf(svg,"version=\"1.1\" ");
	fprintf(svg,"xmlns:cc=\"http://creativecommons.org/ns#\" ");
	fprintf(svg,"xmlns:dc=\"http://purl.org/dc/elements/1.1/\" ");
	fprintf(svg,"xmlns:xlink=\"http://www.w3.org/1999/xlink\" ");
	fprintf(svg,">\n");
	
	/* export background image first */
	/* otherwise will cover outlines */
	if(imagename){
		fprintf(svg,"<image ");
		fprintf(svg,"x=\"0\" y=\"0\" height=\"%d\" width=\"%d\" ", nrows*dy, ncols*dx);
		fprintf(svg,"xlink:href=\"%s\" ", imagename);
		fprintf(svg,"/>\n");
	}
	
	/* export numbers */
	i=0;
	fprintf(svg,"<g id=\"numbers\">\n");	/* group nums for ez edit */
	for(row=0;row<nrows;row++){
		for(col=0;col<ncols;col++){
			fprintf(svg,"\t<text x=\"%d\" y=\"%d\"",col*dx+8,row*dy+30);
			fprintf(svg," font-size=\"28\" font-family=\"Arial\" ");
			fprintf(svg,"fill=\"white\" >");
			fprintf(svg,"%d",frames[i]-frames[0]+start);
			fprintf(svg,"</text>\n");
			i++;
		}
	}
	fprintf(svg,"</g>\n");
	
	/* now export cells */
	col=0;
	row=0;
	for(i=0;i<nframes;i++){		/* over all frames */
		for(j=0;j<n[i];j++){		/* and all cells */
			curcell = c[i] + j;
			for(k=0;k<curcell->npaths;k++){	/* and all outlines */
			
				/* get current path */
				curp = curcell->p[k];
				
				/* start path; remember that since coordinates are
				 * relative, only first coordinate needs to be shifted*/
				fprintf(svg,"<path id=\"%s\" d=\"m %f,%f ",
				  curcell->id,
				  curp[0].x-topleft.x+(float)(dx*col),
				  curp[0].y-topleft.y+(float)(dy*row));
				
				for(l=1;l<curcell->n[k];l++){	/* and all vertices */
					fprintf(svg,"%f,%f ",curp[l].x-curp[l-1].x,curp[l].y-curp[l-1].y);
				}
				fprintf(svg,"z \" ");
					/* tell it how to render it */
				fprintf(svg,"stroke=\"#FFFFFF\" stroke-miterlimit=\"4\" stroke-linejoin=\"round\" stroke-dasharray=\"none\" stroke-width=\"3.0\" fill=\"none\"/>\n");
			}
		}
		col++;
		if(col==ncols){
			col=0;
			row++;
		}
	}
	
	fprintf(svg,"</svg>\n");
}

void copy_region(unsigned char *out, unsigned char *in, int xout, int yout, int xin, int yin, int dx, int dy, int outw, int inw)
{
	int j;
	unsigned char *outrow,*inrow;
	
	for(j=0;j<dy;j++){
		outrow = out + (j+yout)*outw;
		inrow  = in  + (j+yin )*inw;
		memcpy(outrow+xout,inrow+xin,dx);
	}
}

/* cut area out of input images then tile into output image */
void tileimage(unsigned char *out, unsigned char **in, int w, int nrows, int ncols, int x1, int y1, int dx, int dy)
{
	int col,row;
	
	for(row=0;row<nrows;row++){
		for(col=0;col<ncols;col++){
			copy_region(out, in[row*ncols+col], col*dx, row*dy, x1, y1, dx, dy, ncols*dx, w);
		}
	}
}



int main(int argc, char **argv)
{
	/* 'global' variables */
	int nframes,ncols,nrows;
	char *imagefmt,*textfmt,*outname,*framelst;
	char textname[NAME_LEN];
	char imagname[NAME_LEN];
	char svgname [NAME_LEN];
	int frames[MAX_FRAMES];

	int i;
	FILE *f;
	FILE *svg;
	
	/* array of array of cell structures, one array for each file */
	cell_t *cell[MAX_FRAMES];		
	int ncell[MAX_FRAMES];
	
	/* bounding box variables */
	rect_t bb;
	char prompt;
	int x1,y1,x2,y2;
	
	/* image-related variables */
	size_t w,h;
	int maxval;
	size_t thisw,thish;
	int thismaxval;
	unsigned char *buf[MAX_FRAMES];
	unsigned char *buftile;
	char *bgname=NULL;
	
	int isimagej=0;
	int start=0;
	
	/*** Step 0: Preparation ***/
	
	if(argc < 6){
		fprintf(stderr,"Usage: robie-present %s\n", usagestr);
		fprintf(stderr,"\tBoth format strings must have %%d\n");
		fprintf(stderr,"\tUse 'none' (without quotes) for imgfmt if ");
		fprintf(stderr,"none present\n");
		fprintf(stderr,"<frames> can either be range (like 30-40) or ");
		fprintf(stderr,"list (like 10,15,19  or  10-20,22-24)\n");
		fprintf(stderr,"if [infmt], assumes ABSnake format.\n");
		fprintf(stderr,"if [infmt]=absnake, starts from 1.\n");
		fprintf(stderr,"\tExample: ");
		fprintf(stderr,"robie-present frame_%%04d.pgm sup_%%04d.txt\n");
		return 0;
	}
	
	/* get command-line arguments */
	imagefmt	= argv[1];
	textfmt		= argv[2];
	framelst    = argv[3];
	outname		= argv[5];
	ncols		= atol(argv[4]);
	if(argc==7){
		isimagej=1;
		if(strcmp(argv[6],"absnake")==0) start=1;
		if(strcmp(argv[6],"robie")==0) {start=1; isimagej=0;}
	}
	
	/* get list of frames */
	nframes = parse_framelst(framelst, frames);
	if(nframes < 0){
		fprintf(stderr,"ERROR: in parse_framelst.\n");
		return 1;
	}
	
	/* make sure we can tile the frames */
	if(nframes % ncols){
		fprintf(stderr,
		  "Error: %d frames cannot be divided into %d columns.\n",
		  nframes,ncols);
		  return 1;
	}
	nrows = nframes / ncols;
	
	/* step 1: load all outlines into memory */
	if(nframes > MAX_FRAMES){
		fprintf(stderr,"ERROR: why are you trying to tile so many ");
		fprintf(stderr,"(%d) frames together?\n", MAX_FRAMES);
		return 1;
	}
	
	/*** Step 1: Load all contours and calculate bounding box ***/
	
	for(i=0;i<nframes;i++){
		/* get name of file we're trying to open */
		sprintf(textname,textfmt,frames[i]);
		
		/* open it */
		f = fopen(textname,"r");
		if(!f){
			fprintf(stderr,"ERROR: couldn't open %s\n", textname);
			return 1;
		}
		
		if(isimagej){
			ncell[i] = loadcells_imagej(f, cell+i);
		} else {
			ncell[i] = loadcells(f,cell+i);
		}
	
		fclose(f);
	}
	
	bb = get_boundingbox(cell,nframes,ncell);
	printf("%f,%f-%f,%f\n",bb.topleft.x,bb.topleft.y,bb.btmright.x,bb.btmright.y);

	printf("Is bounding box OK? [y/n]):");
	if(scanf("%c",&prompt)!=1){
		fprintf(stderr,"ERROR: premature ending.\n");
		return 1;
	}
	
	if(prompt!='y'){
		printf("\nEnter new bounding box (space separated only):\n");
		if(scanf("%f %f %f %f",&bb.topleft.x,&bb.topleft.y,&bb.btmright.x,&bb.btmright.y)!=4){
			fprintf(stderr,"ERROR: incorrect bounding box.\n");
			return 1;
		}
	}
	printf("\n");

	/* obtain integer version of bounding box */
	x1 = floor(bb.topleft.x);
	y1 = floor(bb.topleft.y);
	x2 = ceil (bb.btmright.x);
	y2 = ceil (bb.btmright.y);
	
	/*** Step 2: Load images, tile, and convert to jpeg ***/ 
	
	if(strcmp(imagefmt,"none")==0) goto skipimage;
		
	/* get size of images
	 * (all images must be of same dimensions) */
	sprintf(imagname,imagefmt,frames[0]);
	if(peek_pgm(&w,&h,&maxval,imagname)){
		fprintf(stderr,"ERROR: couldn't read header for %s\n",imagname);
		return 1;
	}
	
	if(maxval > 256){
		fprintf(stderr,"ERROR: only 8-bit pgms, please.\n");
		return 1;
	}
	
	/* now allocate memory */
	for(i=0;i<nframes;i++){
		buf[i] = (unsigned char *)malloc(w*h);
		if(!buf[i]){
			fprintf(stderr,"ERROR: couldn't allocate enough memory for images.\n");
			return 1;
		}
	}
	
	/* and load the images */
	for(i=0;i<nframes;i++){
		/* get name of file we're trying to open */
		sprintf(imagname,imagefmt,frames[i]);

		if(import_pgm_8bit(buf[i],&thisw,&thish,&thismaxval,imagname)){
			fprintf(stderr,"ERROR: couldn't load file %s\n",imagname);
			return 1;
		}

		if((thisw != w) || (thish != h) || (thismaxval != maxval)){
			fprintf(stderr,"ERROR: images must be identical.\n");
			return 1;
		}
	}
	
	/* allocate memory for tiled version */
	buftile = (unsigned char *)malloc((x2-x1)*ncols*(y2-y1)*nrows);
	if(!buftile){
		fprintf(stderr,"ERROR: couldn't allocate memory for tiling operation.\n");
		return 1;
	}
	
	/* tile */
	tileimage(buftile,buf,w,nrows,ncols,x1,y1,x2-x1,y2-y1);
	
	/* convert to jpeg */
	bgname = malloc(NAME_LEN);
	if(!bgname){fprintf(stderr,"malloc\n"); return 1;}
	sprintf(bgname,"%s.jpg",outname);
	export_to_compressed(bgname,buftile,ncols*(x2-x1),nrows*(y2-y1));
	
skipimage:

	/*** Step 3: export svg ***/
	sprintf(svgname,"%s.svg",outname);
	svg = fopen(svgname,"w");
	if(!svg){
		fprintf(stderr,"ERROR: couldn't open output file.\n");
		return 1;
	}
	
	export_svg(svg, cell, frames, nframes, ncell, ncols, nrows, bb.topleft, x2-x1, y2-y1, bgname, start);
	
	fclose(svg);
	
	return 0;
}
