/* COMPILE: -Wall -O2 -DTEST_MODE
 */

/* Dynamic programming algorithm to solve maximum-energy beam-path problem */
/* This is actually a version of the Bellman-Ford algorithm */
/* by Alireza Nejati */

/* TODO: correct for tdot alignment */

/* This was taken from snake.c and modified */

/* kappa : penalty coefficient for straying from extr=0 */
/* lambda: penalty coefficient for complexity */

/* (current) differences with beamlets: raising tdot to the power 2,
 * binary tree */

/* Oct 1 2011: Fixed stupid bug where function was declared as int in header */

#include "prec.h"
#include <math.h>
#include <malloc.h>
#include <string.h>
#ifdef TEST_MODE
#include <stdio.h>
#endif

/* zero-penalty(ez) is calculated without giving consideration to i */
/* because i will be done on next iteration */
/* a: single segment b: accumulated segments c: (returned) +1 */
__attribute__((hot)) static void join_segs2(tdot_t *c, tdot_t *a, tdot_t *b, int *predc, int rad)
{
	int i,j,k;
	int idx;
	tdot_t max,cur;
	tdot_t *arow;
	tdot_t *bcol;
	int w;
	int jmax;
	
	w = 2*rad - 1;
	
	for(k=0;k<w;k++){	/* for end point k */
		arow = a + k*w;
		
		for(i=0;i<w;i++){	/* and start point i (attach new segment) */
			bcol = b + i;
			idx=0;
			max=-INFINITY;
			jmax=0;	/* to stop gcc's whining */
			
			for(j=0;j<w;j++){	/* and transition point j */
				cur = bcol[idx] + arow[j];
				if(cur > max) { max=cur; jmax = j; }
				idx += w;
			}
			c    [i + k*w] =  max;
			predc[i + k*w] = jmax;
		}
	}
}

float snake_trace_bellmanford(int *extr, int pathn, tdot_t *ptdot, int rad, int closed)
{
	int i,j;
	tdot_t *tdoto1,*tdoto2;
	tdot_t *tmp;
	int w;
	tdot_t tmax,tcur;
	int i0,in;	/* endpoints */
	int ilast;	/* next endpoint (since we trace back) */
	int *pred;	/* predecessors */
	int *predo;
	tdot_t *tdot;
	
	w = 2*rad-1;
	
	tmp = (tdot_t *)malloc(w*w*sizeof(tdot_t));
	if(!tmp) return -1.0;
	pred =(int    *)malloc(w*w*sizeof(int)*pathn);
	if(!pred){free(tmp); return -2.0;}
	tdot = (tdot_t *)malloc(w*w*pathn*sizeof(tdot_t));
	if(!tdot){free(tmp); free(pred); return -3.0;}
	
	memcpy(tdot, ptdot, w*w*pathn*sizeof(tdot_t));
	
	/* calculate all transitions */
	tdoto2=NULL;	/* to stop gcc's whining */
	tdoto1=tdot;
	for(i=1;i<pathn;i++){
		tdoto2 = tdoto1 + w*w;

		join_segs2(tmp, tdoto1, tdoto2, pred + i*w*w, rad);
		memcpy (tdoto2, tmp, w*w*sizeof(tdot_t));

		tdoto1 = tdoto2;

#ifdef TEST_MODE
		int k;
		for(j=0;j<w;j++){
			for(k=0;k<w;k++){
				printf("%+d:%+f ", pred[i*w*w + j*w + k]-rad+1, tmp[j*w + k]);
			}
			printf("\n");
		}
		printf("\n");
#endif
	}

	/* Now see what transition is optimal */
	
	/* find endpoints */
	tmax=-INFINITY; i0=rad; in=rad;
	for(i=0;i<w;i++){
		for(j=0;j<w;j++){
			/* endpoints must be same if closed */
			if(closed && (i!=j)) continue;	
			tcur = tdoto2[j + i*w];
			if(tcur > tmax){tmax=tcur; in=j; i0=i;}
		}
	}
	extr[0] = i0-rad+1;
	if(!closed) extr[pathn-1]=in;

	/* trace back, finding intermediate points */
	ilast=in;
	for(j=pathn+closed-2;j>0;j--){	/* we already did 0 */
		predo = pred + j*w*w;
		ilast = predo[i0*w + ilast];
		extr[j] = ilast - rad + 1;
		//printf("%d ", extr[j]);
	}
	//exit(0);
	free(tdot);
	free(tmp);
	free(pred);
	return tmax;
}

/**each edge has it's own energy penalty and the energy penalty of the
 * total curve is simply the sum of the energy penalties of the edges
 * and vertices. So, for each edge, we subtract from it the energy
 * penalty associated to it's complexity, and to the vertex that it
 * comes from.
 * TODO: in non-closed curves; the penalty associated with the
 * last vertex should be taken into account
 */
/* kappa:  importance of penalty associated with not being zero
 * lambda: importance of penalty associated with being rapidly-changing
 */
void tdot_subtract_energy_edge(tdot_t *tdot, int rad, float kappa, float lambda)
{
	int i,j;
	float ez,ec;	/* penalty for not being zero, and for complexity */
	
	for(j=-rad+1;j<rad;j++){
		if(j>0){
			ez = j;
			ez = kappa*ez*ez;
		} else {
			ez=0.0;
		}
		
		for(i=-rad+1;i<rad;i++){
			/* TODO: stick these in a table */
			ec = (i-j);
			ec = lambda*ec*ec;
			
			*tdot -= ez + ec;
			tdot++;
		}
	}
}

/* a different energy penalty */
void tdot_subtract_energy_edge2(tdot_t *tdot, int rad, float kappa, float lambda)
{
	int i,j;

	for(j=-rad+1;j<rad;j++){
		for(i=-rad+1;i<rad;i++){
			if((i==0)&&(j==0)){
			} else {
				*tdot-=kappa;
			}
			tdot++;
		}
	}
}

/* this function simply applies the above to every single edge */
void tdot_subtract_energy(tdot_t *tdot, int pathn, int rad, int closed, float kappa, float lambda)
{
	int i,w;
	
	w = 2*rad-1;
	
	for(i=0;i<pathn;i++){
		tdot_subtract_energy_edge2(tdot + i*w*w, rad, kappa, lambda);
	}
}

#ifdef TEST_MODE

#define N_POINTS 3
#define EXTR_MAX 2
#define W_MAX (EXTR_MAX+EXTR_MAX-1)

int main()
{
	int i;
	int extr[N_POINTS];
	
	/* problem: trans_dot returns in: y-from x-to format,
	 * but this algorithm assumes y-to x-from format */
	/* not anymore! */
	
	/* from*w + to */
	
	/* first matrix is from point 0 to point 1 */
	tdot_t tdot[W_MAX*W_MAX*N_POINTS] =
	{ 0.0, 0.0, 0.0,
	  0.0, 1.0, 0.0,
	  0.0, 0.0, 0.0,
	  
	  0.0, 0.0, 0.0,
	  0.0, 1.0, 0.0,
	  0.0, 0.0, 0.0,
	  
	  0.0, 0.0, 6.0,
	  0.0, 1.0, 0.0,
	  0.0, 0.0, 0.0, };
	
	/*
	for(k=0;k<N_POINTS;k++){
		for(j=0;j<W_MAX;j++){
			for(i=0;i<W_MAX;i++){
				tdot[k*W_MAX*W_MAX + j*W_MAX + i]
			}
		}
	}*/
	
	/*
	for(i=0;i<W_MAX*W_MAX*N_POINTS;i++) tdot[i]=0;
	
	tdot[4   ] = 1.0;
	tdot[4+ 9] = 1.0;
	tdot[4+18] = 1.0;
	*/
	
	tdot_subtract_energy(tdot, N_POINTS, EXTR_MAX, 1, 1.0, 1.0);

	int j;
	for(j=0;j<9;j++){
		if(!(j%3)) printf("\n");
		for(i=0;i<3;i++){
			printf("%f ", tdot[j*3 + i]);
		}
		printf("\n");
	}

	snake_trace_bellmanford(extr, N_POINTS, tdot, EXTR_MAX, 1);
	
	for(i=0;i<N_POINTS;i++) printf("%d ", extr[i]);
	printf("\n");
	
	return 0;
}

#endif
