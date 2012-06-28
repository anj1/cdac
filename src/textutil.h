#ifndef TEXTUTIL_H
#define TEXTUTIL_H

#define MAX_CELLPATHS 256
#define MAX_ID_LEN 1024

typedef struct cell_t{
	char id[MAX_ID_LEN];
	int npaths;
	point2d32f *p[MAX_CELLPATHS];
	float *phase[MAX_CELLPATHS];
	int n[MAX_CELLPATHS];
}cell_t;

typedef struct rect_t{
	point2d32f topleft;
	point2d32f btmright;
}rect_t;

void loadoutline(FILE *f, point2d32f **pp, float **pphase, int *pn);
void loadcell(FILE *f, cell_t *c);
int loadcells(FILE *f, cell_t **pc);
void freecells(cell_t *c, int n);

point2d32f *load_imagej(FILE *f, int *pn);
int loadcells_imagej(FILE *f, cell_t **pc);

#endif /* TEXTUTIL_H */
