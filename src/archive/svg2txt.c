/* COMPILE: svg.c -Wall -O2 -I/usr/include/libxml2 -lxml2
 */

/* Opens an svg and prints out the path data */
/* one line for x, one line for y */

#include <stdio.h>
#include "point2d.h"
#include "svg.h"

#define MAX_POINTS 0x4000

int main(int argc, char **argv)
{
	FILE *f;
	point2d32f p[MAX_POINTS];
	int n;
	int i;
	
	if(argc < 2){
		fprintf(stderr,"Usage: svg2txt <file.svg>\n");
		return 0;
	}
	
	f = fopen(argv[1], "r");
	if(!f){
		fprintf(stderr,"Couldn't open %s\n", argv[1]);
		return 1;
	}
	
	if(get_path_from_svg(argv[1], "cell", p, &n, MAX_POINTS)){
		printf("error: couldn't load svg file %s\n", argv[1]);
		return 2;
	}
	
	for(i=0;i<n;i++) printf("%f ", p[i].x);
	printf("\n");
	for(i=0;i<n;i++) printf("%f ", p[i].y);
	printf("\n");
	
	fclose(f);
	
	return 0;
}
