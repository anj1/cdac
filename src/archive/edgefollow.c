/* COMPILE: svg.c -Wall -O2 -I/usr/include/libxml2 -lxml2
 */
/* requires libxml2-dev to compile */

/* first written June 30 2011 */

//#include "svg.h"

#include <stdio.h>

int get_path_from_svg(char *path, char *id, float **outx, float **outy, int *n, int *nmax);

int main(int argc, char **argv)
{
	float *x,*y;
	int n,nmax;
	int err;
	
	if(argc < 3){
		printf("usage: edgefollow <imageseries.ics> <outline.svg>\n");
		printf("in svg, outline must have id=\"cell\" field.\n");
		printf("by Alireza Nejati\n");
		return 1;
	}
	
	if((err=get_path_from_svg(argv[2],"cell",&x,&y,&n,&nmax))){
		printf("Error %d on get_path_from_svg(\"%s\"...\n",err,argv[2]);
		return err;
	}
		

#if 0
	int i;
	for(i=0;i<n;i++){
		printf("(%g,%g)\n",x[i],y[i]);
	}
#endif
	
	return 0;
}
