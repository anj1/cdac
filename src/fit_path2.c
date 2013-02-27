/* Created on August 26th, by breaking up motest3 into two files */

/* TODO: sobhist not used, and in filters.c MAX_DIF needs to be made
 * image-independent */
 
/* Oct 19: added a remove_intersect just after deformation, this
 * makes the path much better but I don't yet know why */

/* Oct 22: Located source of noisy-edges errors: using MAX_SEARCH_RAD instead
 * of search_rad (wasn't important until I started calling fit_path with
 * different search radii) */

/* Nov: Added tmpdot, removed abs() in beam.c, to reflect bipolarness */
/* Nov 9: Fixed bug where I wasn't subtracting kappa from tmpdot */

#define pix_type uint16_t

#include <stdint.h>		/* pix_type, int32_t */
#include <malloc.h>
#include <string.h>
#include <math.h>
#include <avl.h>

#include "point2d.h"
#include "path.h"
#include "prec.h"
#include "snake.h"
#include "fit_path.h"
#include "filters.h"
#include "beamdynamic.h"

#define INPUT_MAX 4096

#define MAX_TRIES 1

#ifndef PI
#define PI 3.141592654
#endif

#define ANGLE_THRESHOLD 0.25*PI

#define PATH_ERR_CHK(a,func) \
	if((pathn = a) < 0){ \
		printf("ERR:%s:%d\n",func,pathn); \
		return 30+pathn; \
	}


int cpath_compare(const void *a, const void *b)
{
	float dif;
	
	dif = ((cpath *)a)->eval - ((cpath *)b)->eval;
	if(dif == 0.0){
		/* identical paths? */
		/* if using the forcing method, this situation should almost
		 * never arise */
		return memcmp(((cpath *)a)->p, ((cpath *)b)->p,
		               ((cpath *)a)->n*sizeof(point2d32f));
	}
	if(dif < 0.0) return -1;
	return 1;
}

/* take histogram of sobel gradient */
/* we actually have to take separate histograms for each possible
 * normal vector. However, since the input is isotropic and relatively
 * large, we make the simplifying assumption of rotational symmetry */
/* thus we only take one component of the sobel as input */
void sobel_hist(float *sobhist, int *sob, int size)
{
	int i;
	int x;
	float isize;
	
	for(i=0;i<INPUT_MAX*4;i++) sobhist[i]=0.0;

	for(i=0;i<size;i++){
		x = sob[i];
		sobhist[(x >= 0 ? x : -x)]++;
	}

	isize = 1.0/((float)size);
	for(i=0;i<INPUT_MAX*4;  i++) sobhist[i] = isize*sobhist[i];
	for(i=1;i<INPUT_MAX*4-1;i++){
		sobhist[i] = 0.333333*(sobhist[i-1]+sobhist[i]+sobhist[i+1]);
	}
}

float avg_err(float *perr, int pathn)
{
	int i;
	float sumerr=0;
	
	for(i=0;i<pathn;i++) sumerr+=perr[i];
	
	return sumerr/(float)pathn;
}

void copy_negate(tdot_t *dst, tdot_t *src, int n)
{
	int i;
	for(i=0;i<n;i++) dst[i]=-src[i];
}

/* if the current frame has 'ignore' areas, remove them */
void remove_ignore(float *sobx, float *soby, int w, s_ignore *ignore_area)
{
	int i,x,y;
	int x1,y1,x2,y2;
	
	/* no ignore list in current frame? *return* */
	if(!ignore_area->n) return;

	/* for each ignored area */
	for(i=0;i<(ignore_area->n);i++){
		
		x1 = ignore_area->x1[i];
		y1 = ignore_area->y1[i];
		x2 = ignore_area->x2[i];
		y2 = ignore_area->y2[i];
	
		for(x=x1; x<x2; x++){
			for(y=y1; y<y2; y++){
				sobx[w*y + x] = 0.0;
				soby[w*y + x] = 0.0;
			}
		}
	}
}


int fit_path (float *img, int w, int h, float *sobx, float *soby, float *sobhist, spath *s, int search_rad, float normal_drift, float seg_length, int blockw, float kappa, int low_conf_rm, s_ignore *ignore_area)
{
	int pathn;
	avl_tree_t *avl;
	avl_node_t *this;
	int i,j;
	cpath *path,*path1,*path2;
	cpath *p;
	cpath tmp;	/* for path smoothing */

	/* temporary version of tdot and extr */
	tdot_t tmpdot[MAX_PATHN*(2*MAX_SEARCH_RAD-1)*(2*MAX_SEARCH_RAD-1)];
	 
	/** Image preparation **/
	sobel2cf(img,sobx,soby,w,h);
	if(ignore_area) remove_ignore(sobx,soby,w,ignore_area);	/* remove debris areas */
	
	/** AVL tree preparation (for online sort) **/
	avl = avl_alloc_tree(&cpath_compare, NULL);
	
	/** Creation of new paths **/
	/* We take elements off of p->path, and add them to our AVL
	 * tree. We allocate memory for each new path, and then at the
	 * end we copy the AVL tree to p->path and implicitly free
	 * all the memory */
	for(i=0;i<s->npath;i++){
#ifdef DEBUG_PRINT
		printf("input %d\n", i);
#endif
		p = s->path + i;
		pathn = p->n;
		
		/* smooth path before anything else */
		path_smooth(tmp.p,p->p,pathn,1,1.0);
		path_smooth(p->p,tmp.p,pathn,1,1.0);
	
		/* prepare path */
		PATH_ERR_CHK(path_simplify_smart2(p->p,p->phase,0.5*seg_length,pathn,ANGLE_THRESHOLD,1),"simplify_path");
		PATH_ERR_CHK(path_subdivide2(p->p,p->phase,seg_length,pathn,MAX_PATHN,1),"subdivide_path");
		
		pathn = remove_intersect(p->p, p->phase, pathn, 1);
		if(low_conf_rm){
			pathn = remove_lowweight(p->p, p->perr, p->phase, pathn, 1);
		}
		/* to get ready for next step, make sure not self-intersections */
		snake_init_extrude_bounds  (s->bminus,s->bplus,search_rad,pathn);
		snake_refine_extrude_bounds(p->p,s->bminus,s->bplus,pathn,1);
		
		path_precalc_normals(s->norml,p->p,pathn,search_rad,1);

		snake_trans_dot(sobx,soby,w,p->p,pathn,s->tdot,s->norml,s->bminus,s->bplus,search_rad,1);
		//export_int_matlab(p->tdot,pathn*(2*MAX_SEARCH_RAD-1)*(2*MAX_SEARCH_RAD-1),"tdot.m");
		copy_negate(tmpdot, s->tdot, pathn*(2*search_rad-1)*(2*search_rad-1));
		
		tdot_subtract_energy(s->tdot,pathn,search_rad,1,kappa,0.0);
		tdot_subtract_energy(tmpdot,pathn,search_rad,1,kappa,0.0);
		
		/* we now have everything in place to develop a series of
		 * paths */

		for(j=0;j<MAX_TRIES;j++){
			
			//printf(" output %d\n", j);

			path1 = (cpath *)malloc(sizeof(cpath));	/* free() is done by avl lib */
			path2 = (cpath *)malloc(sizeof(cpath));	/* free() is done by avl lib */

			/* create new extrusion */
			/* for negative polarity */
			/* NOTE: NOTE: snake_trace_bellmanford modifies tdot! */
			snake_trace_bellmanford(path1->extr,pathn,s->tdot,search_rad,1);
			path1->opts.kappa  = kappa;
			extr_to_error(path1->perr,path1->extr,pathn,s->tdot,search_rad,1);
			path1->eval = avg_err(path1->perr, pathn);
#ifdef DEBUG_PRINT
			printf("Average along path (-'ve): %f ", path1->eval);
#endif
			/* for positive polarity */
			snake_trace_bellmanford(path2->extr,pathn,tmpdot,search_rad,1);
			path2->opts.kappa  = kappa;
			extr_to_error(path2->perr,path2->extr,pathn,tmpdot,search_rad,1);
			path2->eval = avg_err(path2->perr, pathn);
#ifdef DEBUG_PRINT
			printf("(+'ve): %f\n", path2->eval);
#endif
			/* out of positive and negative polarities, choose best one */
			if(path2->eval > path1->eval){
				free(path1);
				path = path2;
			} else {
				free(path2);
				path = path1;
			}
			
			snake_deform2_int(p->p,path->p,s->norml,path->extr,pathn,search_rad,1);
			if(low_conf_rm){
				pathn = remove_lowweight(path->p, path->perr, path->phase, pathn, 1);
			}
			pathn = remove_intersect(path->p, path->phase, pathn, 1);
			

			memcpy(path->phase, p->phase, sizeof(float)*pathn);

			path->n = pathn;

#if 0
			s->path[0]=*path;
			s->npath=1;
			avl_free_tree(avl);
			return 0;
#endif
		
			/* add to AVL tree */
			avl_insert(avl, path);

			/* remove smaller extra entries from tree */
			while(avl_count(avl) > MAX_PATHS){
				free(avl_delete_node(avl, avl->head));
			}

		}
	}

	/** AVL tree dissolution **/
	this = avl->tail;

	for(i=0;i<MAX_PATHS;i++){
		memcpy(s->path + i,this->item,sizeof(cpath));
#ifdef DEBUG_PRINT
		printf("eval: %f\n", s->path[i].eval);
#endif
		this = this->prev;
		if(!this) {i++; break; }	/* PRESSURE_POINT */
	}
	s->npath = i;	/* sometimes we have 15 instead of 16 (dup) */
	avl_free_tree(avl);

	return 0;
}

/* take average of many different extrusions (for autocorrelation computation) */
int extr_avg(int *extr, spath *s)
{
	int n;
	int i,j;
	cpath *this;
	
	n = s->path[0].n;
	
	/* blank slate */
	for(i=0;i<n;i++) extr[i]=0;
	
	/* add */
	for(i=0;i<s->npath;i++){
		this = s->path + i;
		if(n!=this->n) return 1;	/* number of points must agree */
		for(j=0;j<n;j++) extr[j] += this->extr[j];
	}
	
	/* divide by n */
	for(j=0;j<n;j++) extr[j] /= s->npath;
	
	return 0;
	
}
