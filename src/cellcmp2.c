/* COMPILE: framelist.c hausdorff.c svg2.c textutil.c -Wall -O2 -I/usr/include/libxml2 -lxml2 -lm
 */

/* TODO: do valgrind on this
 */
 
#include <stdio.h>
#include <string.h>	/* strcmp() */
#include <malloc.h>
#include <math.h>	/* sqrt() */
#include "point2d.h"
#include "svg2.h"
#include "textutil.h"
#include "hausdorff.h"
#include "framelist.h"

#define MAX_PATHN 4096
#define MAX_IDLEN 256

#define MAX_FRAMES 4096
#define MAX_FILENAME_LEN 1024

float charlie(point2d32f *p, int n)
{
	int i;
	int i2;
	float dx,dy;
	float sum=0;
	for(i=0;i<n;i++){
		i2=i+1;
		if(i2==n) i2=0;
		dx = p[i2].x - p[i].x;
		dy = p[i2].y - p[i].y;
		sum += sqrt(dx*dx + dy*dy);
	}
	return sum;
}

void compare4way(point2d32f *p1, int n1, point2d32f *p2, int n2)
{
	/*
	printf("%f %f %f %f ",
		path_dist_hausdorff(p1,n1,p2,n2)/charlie(p2,n2),
		path_dist_hausdorff(p2,n2,p1,n1)/charlie(p1,n1),
		path_dist_avg      (p1,n1,p2,n2)/charlie(p2,n2),
		path_dist_avg      (p2,n2,p1,n1)/charlie(p1,n1));
	*/
	printf("%f %f %f %f ",
		path_dist_hausdorff(p1,n1,p2,n2),
		path_dist_hausdorff(p2,n2,p1,n1),
		path_dist_avg      (p1,n1,p2,n2),
		path_dist_avg      (p2,n2,p1,n1));
	
}

int main(int argc, char **argv)
{
	int i,j,k;
	FILE *f1,*f2;
	cell_t *cell;
	point2d32f *p2=NULL;
	int n1,pathn2;
	char *id[MAX_CELLPATHS];
	point2d32f *p3[MAX_CELLPATHS];
	int pathn3[MAX_CELLPATHS];
	int *ppathn3[MAX_CELLPATHS];
	int n3;
	int silent;
	int row;
	
	int frames[MAX_FRAMES];
	int nframes;
	int c;
	char filenam[MAX_FILENAME_LEN];
	
	if(argc < 5){
		printf("Usage: cellcmp <framelist> <format.txt> <format.txt> <format.svg> [OPTION]\n");
		printf("\tCompares cell outlines using Hausdorff distance.\n");
		printf("\tFormats must be given in standard C style.\n");
		printf("\tFirst files must be of internal format.\n");
		printf("\tSecond must be of ImageJ format.\n");
		printf("\tuse 'none' (without quotes) if not required\n");
		printf("\tCurrently only loads first path in svg file.\n");
		printf("\tOPTION can be -s (only data) or -r (avg. only; first cell only; print as row)\n");
		return 0;
	}
	
	silent=0;
	row=0;
	if(argc >= 6){
		if(strcmp(argv[5], "-s")==0) silent=1;
		if(strcmp(argv[5], "-r")==0) {silent=1; row=1;}
	}
	
	/* parse frames */
	nframes = parse_framelst(argv[1], frames);
	if(nframes < 0){
		fprintf(stderr,"ERROR: in parse_framelst.\n");
		return 1;
	}
	
	for(c=0; c<nframes; c++){
		/* -------------------------------------------------------------- */
		/* load files into memory */
		
		/* load first file */
		sprintf(filenam, argv[2], frames[c]);
		f1 = fopen(filenam,"r");
		if(!f1){
			fprintf(stderr,"ERROR: couldn't open %s\n", filenam);
			return 1;
		}
		n1 = loadcells(f1, &cell);
		fclose(f1);
		
		/* load second file */
		if(strncmp(argv[3],"none",4)){
			sprintf(filenam, argv[3], frames[c]);
			f2 = fopen(filenam,"r");
			if(!f2){
				fprintf(stderr,"ERROR: couldn't open %s\n", filenam);
				return 1;
			}
			p2 = load_imagej(f2, &pathn2);
			fclose(f2);
		} else {
			pathn2=0;
		}
		
		/* load svg file */
		/* we exploit an interesting property of this that if any member
		 * of p3 is NULL, it is not loaded. Thus only the first path in the
		 * svg file is loaded */

		/* first, get number of paths we'll need */
		sprintf(filenam, argv[4], frames[c]);
		n3 = get_paths_from_svg(filenam, NULL, NULL, NULL, 0);
		if(n3 < 0){
			printf("Error: couldn't load paths from SVG file %s (error %d)\n", filenam, n3);
			return 1;
		}
		
		/* now allocate memory */
		for(i=0;i<n3;i++){
			id[i] = (char *)malloc(MAX_ID_LEN);
			p3[i] = (point2d32f *)malloc(MAX_PATHN*sizeof(point2d32f *));
			ppathn3[i] = pathn3 + i;
			if((!id[i])||(!p3[i])){
				fprintf(stderr,"ERROR: couldn't allocate memory for svg\n");
				return 1;
			}
		}
		
		/* and load the paths */
		n3 = get_paths_from_svg(filenam, id, p3, ppathn3, MAX_PATHN);
		
		/* -------------------------------------------------------------- */
		/* compare */
		for(i=0;i<n3;i++){
			if(!silent) printf("Comparing SVG path '%s'\n", id[i]);
			
			if(row){
				//printf("%g, ", path_dist_avg(cell[0].p[0], cell[0].n[0], p3[i], pathn3[i]) / charlie(cell[0].p[0], cell[0].n[0]));
				printf("%g, ", path_dist_avg(cell[0].p[0], cell[0].n[0], p3[i], pathn3[i]));
				continue;
			}
			
			if(pathn2>0){
				if(!silent) printf("\twith imageJ roi: ");
				compare4way(p2, pathn2, p3[i], pathn3[i]);
				if(!silent) printf("\n");
			}
			
			for(j=0;j<n1;j++){
				if(!silent) printf("\twith cell %d ", j);
				for(k=0;k<cell[j].npaths;k++){
					if(!silent) printf("path #%d: ",k);
					compare4way(cell[j].p[k], cell[j].n[k], p3[i], pathn3[i]);
					if(!silent) printf("\n");
				}
			}
			
			printf("\n");
		}
		
		/* free memory and prepare for next image */
		for(i=0;i<n3;i++){
			free(id[i]);
			free(p3[i]);
		}
		if(strncmp(argv[3],"none",4)){
			free(p2);
		}
		freecells(cell, n1);
	}

	if(row) printf("\n");
	
	return 0;
}
