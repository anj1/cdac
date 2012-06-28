/* cell snake: snake algorithm for cells */

/* the basic premise is that both the direction and magnitude of the
 * edge are important. Edges are assumed to only extrude or erode;
 * not to turn (at least not in a single frame). */

/* created on July 28 */

/* Aug 04 2011: Fixed STUPID STUPID bj/fj bug that was causing me so many
 * erroneous results. (in trace_max) */
/* Aug 07 2011: bugfix: I was calling normal2d with reversed parameters!
 * damn I'm dumb */
/* Sep 30 2011: Fixed bug in extr_to_error where extrusion wasn't being
 * offset to w. Yes you're dumb */
 
#define DEBUG_PRINT

/* hard-coded parameters for keeping path from self-intersecting */
#define SELF_DIST 		3.0
#define SLOPE_CUTOFF 	0.2
//#define MIN_RAD 		1.0
#define MIN_RAD			0.0

#include <stdlib.h>	/* qsort() */
#include <malloc.h>	/* malloc(), free() */
#include <math.h>	/* fabs(), sqrt(), exp() */
#include <stdint.h>	/* int32_t */
#include <string.h>	/* memcpy() */
#ifdef DEBUG_PRINT
#include <stdio.h>
#endif
#include "point2d.h"
#include "path.h"	/* normal2d() */
#include "matrix.h"	/* mul_mat() */
#include "isqrt.h"	/* isqrt[] */
#include "prec.h"
#include "beam.h"

#ifndef PI
#define PI 3.141592653589793
#endif

#ifndef ALIB_ZERO
#define ALIB_ZERO 0.001
#endif

#define CALC_PREV_NEXT \
	i2=i+1;\
	i0=i-1;\
	if(!closed){\
		if(i==0)i0=i;\
		if(i==pathn-1)i2=i;\
	}\
	if(i2>=pathn)i2-=pathn;\
	if(i0< 0)i0+=pathn;

#define ISQR2PI  0.3989422804

/* inputs:
 * 	dot: result of taking dot product of edge normal and sobel gradient,
 *  hist:histogram of probabilities for each pixel level
 *  sd:  standard deviation for distribution of gaussian noise */

/* deform snake according to given extrusion vector */
void snake_deform(point2d32f *path1, point2d32f *path2, float *extr, int pathn, int closed)
{
	int i;
	int i0,i2;
	point2d32f norml;
	
	for(i=0;i<pathn;i++){
		
		CALC_PREV_NEXT
		
		/* get normal vector */
		norml = normal2d(path1[i0],path1[i2]);
		
		/* extrude along normal */
		path2[i].x = path1[i].x + extr[i]*norml.x;
		path2[i].y = path1[i].y + extr[i]*norml.y;
	}
}

/* like above, but with precalculated normals */
void snake_deform2(point2d32f *path1, point2d32f *path2, point2d32f *nrm, float *extr, int pathn, int rad, int closed)
{
	int i;
	int iextr;
	
	for(i=0;i<pathn;i++){

		/* get normal vector */
		iextr = (int)extr[i];
		iextr = (iextr >= 0 ? iextr : -iextr);
		
		/* extrude along normal */
		path2[i].x = path1[i].x + extr[i]*nrm[iextr + i*rad].x;
		path2[i].y = path1[i].y + extr[i]*nrm[iextr + i*rad].y;
	}
}

/* like above, but with integer extrusion level */
void snake_deform2_int(point2d32f *path1, point2d32f *path2, point2d32f *nrm, int *extr, int pathn, int rad, int closed)
{
	int i;
	int iextr;
	
	for(i=0;i<pathn;i++){

		/* get normal vector */
		iextr = (extr[i] >= 0 ? extr[i] : -extr[i]);
		
		/* extrude along normal */
		path2[i].x = path1[i].x + ((float)extr[i])*nrm[iextr + i*rad].x;
		path2[i].y = path1[i].y + ((float)extr[i])*nrm[iextr + i*rad].y;
	}
}

void snake_init_extrude_bounds(float *bminus, float *bplus, float rad, int pathn)
{
	int i;
	for(i=0;i<pathn;i++){
		bminus[i]=-rad;
		bplus[i] = rad;
	}
}

/* inputs:
 *  bplus,bminus: amount to extrude in the +normal and -normal direction
 * subroutine refines these numbers to avoid self-collisions in path
 * outpus:
 *  bminus,bplus.
 */
/* this subroutine is a *tad* complicated because I wanted it to be very
 * efficient without using extra data structures like distance trees.
 * The worse case performance is pathn*pathn intersection calculations.
 */
void snake_refine_extrude_bounds(point2d32f *path, float *bminus, float *bplus, int pathn, int closed)
{
	int i;
	int i0,i2;
	point2d32f norml;
	point2d32f minus,plus;
	point2d32f pi;
	point2d32f normli;
	float slope;
	
	float ti;
	int j;
	int intersects;
	
	for(i=0;i<pathn;i++){
		
		CALC_PREV_NEXT
		
		/* get normal vector */
		norml = normal2d(path[i0],path[i2]);
		
		/* build intersection-free interval by repeatedly 'shrinking'
		 * interval until no more intersections remain */
		j=0;
		while(1){
			/* translate extrusion bounds to 2d points */
			minus.x = path[i].x + bminus[i]*norml.x;
			minus.y = path[i].y + bminus[i]*norml.y;
			plus.x  = path[i].x + bplus [i]*norml.x;
			plus.y  = path[i].y + bplus [i]*norml.y;

#ifdef DEBUG_PRINT2
			printf("i%d minus, path[i]: %f,%f %f,%f\n",
			       i, minus.x, minus.y, path[i].x, path[i].y);
#endif

			/* check if normal line segment intersects any other line
			 * segments in path */
			intersects=0;
			for(;j<pathn;j++){	/* no need to start from 0 every time */
				if(find_intersect(minus,plus,path[j],path[j+1],&pi)){
					
					/* make sure intersection is not bogus */
					/* note that to get accurate results we have to do
					 * this check; we can't just not do path[i] */
					if((fabs(pi.x - path[i].x) < ALIB_ZERO)
					 &&(fabs(pi.y - path[i].y) < ALIB_ZERO)) continue;
					
					intersects=1;
					break;
				}
			}

#ifdef DEBUG_PRINT2
			printf("i: %d intersections found: %d with %d\n",
			        i, intersects, j);
#endif

			/* no more intersections? */
			if(!intersects) break;
			
			/* translate intersection point to position along normal */
			/* note that |norml|=1.0 */
			ti = (pi.x-path[i].x)*norml.x + (pi.y-path[i].y)*norml.y;
			
			/* set new bounds to a safe distance out of intersection */
			/* do it so that bound is constant distance from line */
			normli = normal2d(path[j],path[j+1]);
			slope = fabs(normli.x*norml.x + normli.y*norml.y);
			
			/* intersections at steep angles aren't problematic,
			 * their angles are so off they will not be considered */
			/* just skip them */
			if(slope < SLOPE_CUTOFF) {j++; continue;}
			slope = 1.0/slope;
			if(ti < 0) bminus[i] = ti + SELF_DIST*slope;	
			if(ti > 0) bplus [i] = ti - SELF_DIST*slope;
			
			/* situation hopeless? */
			if(bminus[i] > bplus[i]){
				bminus[i]=0.0; bplus[i]=0.0;
				break;
			}
		}
		
		if(bminus[i] >= -MIN_RAD) bminus[i]=-MIN_RAD;
		if(bplus [i] <=  MIN_RAD) bplus [i]=+MIN_RAD;
	}
}

/* Calculates sum of dot products of extruded edge segments with image.
 * Actual probability calculation is left to other subroutines.
 * inputs:
 * path,n,norml
 * outputs:
 * tdot (rad*rad*n) : transition dot products
 */
 /* y: from x: to */
void snake_trans_dot(sob_t *sobx, sob_t *soby, int w, point2d32f *path, int n, tdot_t *tdot, point2d32f *nrm, float *bminus, float *bplus, int rad, int closed)
{
	int i,j,k;
	int i2;
	int aj,ak;
	float fj,fk;
	point2d32f p1,p2;
	point2d32f *nrmo;
	tdot_t *tdoto;
	tdot_t *tdotoo;

	for(i=0;i<n;i++){
		
		i2 = i+1;
		if(i2==n){
			if(!closed) break;
			i2=0;
		}
		
		nrmo = nrm + i2*rad;
		tdoto = tdot + i*(2*rad-1)*(2*rad-1);
		
		for(j=-rad+1;j<rad;j++){
			
			tdotoo = tdoto + (j+rad-1)*(2*rad-1);
			
			aj = (j >= 0 ? j : -j);
			fj = ((float)j);
			/* simplify things for path-tracing subroutines */
			if((fj < bminus[i])||(fj > bplus[i])){
				for(k=-rad+1;k<rad;k++) tdotoo[k+rad-1]=0;
			}
			p1.x = path[i].x + fj*nrm[i*rad + aj].x;
			p1.y = path[i].y + fj*nrm[i*rad + aj].y;
			
			for(k=-rad+1;k<rad;k++){
				ak = (k >= 0 ? k : -k);
				fk = ((float)k);
				if((fk < bminus[i2])||(fk > bplus[i2])){
					tdotoo[k+rad-1]=0;
					continue;
				}
				p2.x = path[i2].x + fk*nrmo[ak].x;
				p2.y = path[i2].y + fk*nrmo[ak].y;
				
				/* start:x end:y */
				tdotoo[k+rad-1] = beam_integral((float *)sobx,(float *)soby,w,p1,p2);
			}
		}	
	}
}


/* outputs: start, end, eerr */
/* returns number of points */
/* TODO: merge with above */
int export_local_max(point2d32f *start, point2d32f *end, int nmax, float *eerr, point2d32f *path, float *tprob, point2d32f *nrm, int n, int rad, int closed)
{
	int i,j,k;
	int i2;
	int aj,ak;
	float fj;
	/*float fk */
	point2d32f p1,p2;
	point2d32f *nrmo;
	float *tprobo;
	float *tproboo;
	int c=0;
	
	for(i=0;i<n;i++){
		
		i2 = i+1;
		if(i2==n){
			if(!closed) break;
			i2=0;
		}
		
		nrmo = nrm + i2*rad;
		tprobo = tprob + i*(2*rad-1)*(2*rad-1);
		
		for(j=-rad+1;j<rad;j++){
			aj = (j >= 0 ? j : -j);
			fj = ((float)j);
			p1.x = path[i].x + fj*nrm[i*rad + aj].x;
			p1.y = path[i].y + fj*nrm[i*rad + aj].y;
			
			tproboo = tprobo + (j+rad-1)*(2*rad-1);
			
			for(k=-rad+1;k<rad;k++){
				ak = (k >= 0 ? k : -k);
				/*fk = ((float)k);*/
				p2.x = path[i2].x + ((float)k)*nrmo[ak].x;
				p2.y = path[i2].y + ((float)k)*nrmo[ak].y;
				
				if(tproboo[k+rad-1] > 0.90){
					start[c] = p1;
					end[c] = p2;
					c++;
					if(c==nmax) return c;
				}
			}
		}	
	}
	return c;
}

/* assess the 'fitness' of the snake */
/* TODO: just a stub for now */
float snake_eval(int *extr, float *tprob, int pathn, int rad, int closed)
{
	int i;
	int w;
	float r;
	
	w = 2*rad-1;
	r=0.0;
	for(i=0;i<pathn-1;i++){
		r += tprob[i*w*w + (extr[i]+rad-1)*w + (extr[i+1]+rad-1)];
	}
	return r;
}

/* output: featerr */
/* given the (integer) extrusion level, returns the amount of
 * error for each segment */
void extr_to_error(float *featerr, int *extr, int pathn, float *tprob, int rad, int closed)
{
	int i,i2;
	float *tprobo;
	int w;
	
	w = 2*rad-1;
	
	for(i=0;i<pathn;i++){
		i2=i+1;
		if(i2==pathn){ if(closed) i2=0; else continue; }
		
		tprobo = tprob + i*w*w;
		
		featerr[i] = tprobo[(extr[i]+rad-1)*w + (extr[i2]+rad-1)];
	}
}
