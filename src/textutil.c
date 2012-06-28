
/* Utilities for loading cell structures from plaintext files */

#include <stdio.h>
#include <stdlib.h>		/* exit() */
#include <string.h>		/* strncmp() */
#include "point2d.h"
#include "textutil.h"

/** robie files **/

void loadoutline(FILE *f, point2d32f **pp, float **pphase, int *pn)
{
	point2d32f *p;
	float *phase;
	int n;
	int i;
	
	/* read number of elements in outline */
	if(fscanf(f,"%d\n",&n)!=1){
		fprintf(stderr,"ERROR: couldn't read number of vertices.\n");
		exit(1);
	}
	
	/* allocate memory for elements */
	p     = (point2d32f *)malloc(n*sizeof(point2d32f));
	phase = (float     *)malloc(n*sizeof(float));
	if((!p)||(!phase)){
		fprintf(stderr,"ERROR: couldn't allocate %d vertices.\n", n);
		exit(1);
	}	
	
	/* read elements */
	for(i=0;i<n;i++){
		if(fscanf(f,"%f %f %f", &p[i].x, &p[i].y, &phase[i])!=3){
			fprintf(stderr,"ERROR: couldn't read %d vertices.\n",n);
			exit(1);
		}
	}
	
	/* return */
	*pp = p;
	*pphase = phase;
	*pn = n;
}

void freeoutline(point2d32f *p)
{
	free(p);
}

/* loads a cell from standard text file */
/* input: pointer to single cell structure */
void loadcell(FILE *f, cell_t *c)
{
	int i;
	
	/* load id */
	if(fscanf(f,"%s\n",c->id)!=1){
		fprintf(stderr,"Couldn't load cell id.\n");
		exit(1);
	}
	
	/* load number of paths */
	if(fscanf(f,"%d\n", &c->npaths)!=1){
		fprintf(stderr,"ERROR: couldn't load number of paths.\n");
		exit(1);
	}
	
	if(c->npaths > MAX_CELLPATHS){
		fprintf(stderr,"ERROR: MAX_CELLPATHS: %d\n", MAX_CELLPATHS);
		exit(1);
	}
	
	for(i=0;i<c->npaths;i++) loadoutline(f,c->p+i,c->phase+i,c->n+i);
}

void freecell(cell_t *c)
{
	int i;
	for(i=0;i<c->npaths;i++) freeoutline(c->p[i]);
}

/* loads a standard text file */
/* input: array of cell structures */
/* (allocated inside function) */
/* returns number of cells loaded */
int loadcells(FILE *f, cell_t **pc)
{
	int i;
	int n;
	cell_t *c;
	char magic[7];
	int iswrong;
	
	/* Verify with magic number (should be 'robie') */
	if(fscanf(f,"%s\n",magic)!=1){
		fprintf(stderr,"ERROR: couldn't read magic string.\n");
		exit(1);
	}
	
	iswrong=1;
	if(magic[5]==0){
		if(strncmp(magic,"robie",5)==0) iswrong=0;
	}
	if(iswrong){
		fprintf(stderr,"ERROR: file type is incorrect.\n");
		exit(1);
	}
	
	/* read number of elements in outline */
	if(fscanf(f,"%d\n",&n)!=1){
		fprintf(stderr,"ERROR: couldn't read number of cells.\n");
		exit(1);
	}
	
	/* allocate memory for cells */
	c = (cell_t *)malloc(n*sizeof(cell_t));
	if(!c){
		fprintf(stderr,"ERROR: couldn't allocate %d cells.\n", n);
		exit(1);
	}
	
	for(i=0;i<n;i++) loadcell(f, c+i);
	
	*pc = c;
	return n;
}

/* frees memory associated with cell structures */
void freecells(cell_t *c, int n)
{
	int i;
	for(i=0;i<n;i++) freecell(c+i);
	free(c);
}

/** Other file formats **/

/* returns pointer to array of coordinates. Also returns number
 * of elements read */
#define BLOCK_SIZE 256
point2d32f *load_imagej(FILE *f, int *pn)
{
	char h[5];
	int nb;
	float x,y,z,xcal,ycal;
	int n=0,nused=0;
	point2d32f *path=NULL;
	
	/* ignore first line */
	if(fscanf(f, "%s %s %s %s %s %s\n",h,h,h,h,h,h)!=6){
		fprintf(stderr,"ERROR: load_imagej: invalid header.\n");
		exit(1);
	}
	
	while(fscanf(f,"%d %f %f %f %f %f",&nb,&x,&y,&z,&xcal,&ycal)==6){
		/* bump up size if initial size exceeded */
		if(nused==n){
			n += BLOCK_SIZE;
			path = (point2d32f *)realloc(path, n*sizeof(point2d32f));
			if(!path){
				fprintf(stderr,"load_imagej: ERROR: couldn't allocate enough memory.\n");
				exit(1);
			}
		}
		//printf("nb: %d\n", nb);
		path[nused].x = x;
		path[nused].y = y;
		nused++;
	}
	
	/* fscanf failed, what was the reason? */
	if(feof(f)){
		*pn = nused;
		return path;
	} else {
		fprintf(stderr,"ERROR: load_imagej: Invalid format.\n");
		exit(1);
	}
}

/* like the above, but with interface similar to loadcells so that
 * programs don't have to write two interfaces */
int loadcells_imagej(FILE *f, cell_t **pc)
{
	cell_t *c;
	
	/* allocate memory for cell */
	c = (cell_t *)malloc(sizeof(cell_t));
	if(!c){
		fprintf(stderr,"ERROR: couldn't allocate cell struct.\n");
		exit(1);
	}
	
	c->npaths=1;
	c->p[0] = load_imagej(f, c->n+0);
	
	strcpy(c->id, "auto");
	
	*pc = c;
	return 1;
}
