/* COMPILE: svg2.c -Wall -O2 -I/usr/include/libxml2 -lxml2
 */
 
#include <stdio.h>
#include <string.h>	/* strcmp() */
#include <malloc.h>
#include "point2d.h"
#include "svg2.h"

#define MAX_PATHS 256
#define MAX_PATHN 4096
#define MAX_ID_LEN 256

void export_paths_text(FILE *f, point2d32f **pp, int *pathn, char **id, int n)
{
	int i,k;
	point2d32f *p;
	
	/* magic text */
	fprintf(f,"robie\n");
	
	/* treat each outline as belonging to a different cell
	 * (the idea is that the user will rarely draw a cell twice */
	 
	/* list of cells */
	fprintf(f,"%d\n",n);
	for(i=0;i<n;i++){
		
		/* outline for each cell */
		fprintf(f,"%s\n",id[i]);
		fprintf(f,"%d\n",1);
			
		/* individual outline */
		p = pp[i];
		fprintf(f,"%d\n",pathn[i]);
		for(k=0;k<pathn[i];k++){
			fprintf(f,"%f %f\n",p[k].x,p[k].y);
		}
	}
	
	fclose(f);
}

int main(int argc, char **argv)
{
	int i;
	char *id[MAX_PATHS];
	point2d32f *p3[MAX_PATHS];
	int pathn3[MAX_PATHS];
	int *ppathn3[MAX_PATHS];
	int n3;
	
	if(argc < 2){
		printf("Usage: svgtotext <filename.svg>\n");
		printf("\tConverts list of paths from svg to text format.\n");
		return 0;
	}
	
	/* load svg file */

	/* first, get number of paths we'll need */
	n3 = get_paths_from_svg(argv[1], NULL, NULL, NULL, 0);
	if(n3 < 0){
		printf("Error: couldn't load paths from SVG file %s (error %d)\n", argv[1], n3);
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
	n3 = get_paths_from_svg(argv[1], id, p3, ppathn3, MAX_PATHN);

	/* -------------------------------------------------------------- */
	/* export */
	export_paths_text(stdout, p3, pathn3, id, n3);
	
	
	return 0;
}
