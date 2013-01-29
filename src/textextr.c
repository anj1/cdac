/* COMPILE: textutil.c -Wall -O2
 */

#include <string.h>	/* strcmp() */
#include <stdio.h>
#include "point2d.h"
#include "textutil.h"

#define USAGESTR "Usage: textextr <input.txt> <id> [h|v]\n"

int main(int argc, char **argv)
{
	int i,idx;
	
	char *inpath;
	char *id;
	FILE *f;
	
	int horiz;
	
	cell_t *c;
	int nc;

	if(argc < 3){
		fprintf(stderr,USAGESTR);
		return 0;
	}
	
	inpath = argv[1];
	id     = argv[2];
	horiz  = 1;	/* default behavior */
	if(argc>=4){
		if(strncmp(argv[3],"v",1)==0) horiz=0;
	}
	
	f = fopen(inpath,"r");
	if(!f){
		fprintf(stderr,"Error: couldn't open file %s\n", inpath);
		return 1;
	}
	
	nc = loadcells(f, &c);
	if(nc < 0){
		fprintf(stderr,"Couldn't parse file.\n");
		return 2;
	}
	
	fclose(f);
	
	for(i=0;i<nc;i++){
		if(strcmp(c[i].id,id)==0) break;
	}
	idx=i;

	for(i=0;i<c[idx].n[0];i++){
		if(horiz){
			printf("%f,%f;%f ",c[idx].p[0][i].x,c[idx].p[0][i].y,c[idx].phase[0][i]);
		} else {
			printf("%f %f %f\n",c[idx].p[0][i].x,c[idx].p[0][i].y,c[idx].phase[0][i]);
		}
	}
	printf("\n");
	
	
	return 0;
}
