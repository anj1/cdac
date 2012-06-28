/* COMPILE: -Wall -ggdb -lm -DTEST_MODE
 */

/* primitives for working with paths */
/* by Alireza Nejati, July 7 2011 */
/* July 11: find_intersect and remove_intersect added */
/* July 13: fixed bug in repair_path for overflowing error runs */
/* July 20: Added path_extrude */
/* July 22: added path_subdivide2,path_simplify2 for implicit closed curves*/
/* July 28: added normal2d */
/* Oct. 18: Completed remove_intersect and fixed a LOT of bugs */
/* Feb.  9: Made some modifications in path_simplify_smart and remove_intersect to handle phases */

/* TODO: convert all routines to implicit path closures */

#define DEBUG_PRINT

#define ALIB_ZERO 0.001

#define PERR_THRESH 0.06
//#define PERR_THRESH 0.10
#define PERR_RUN 10

#ifndef PI
#define PI 3.141592653589793
#endif

#include <math.h>	/* sqrt(), fabs() */
#include <float.h>	/* FLT_MAX */
#ifdef DEBUG_PRINT
#include <stdio.h>
#endif
#include "point2d.h"

#ifdef TEST_MODE
#include <stdlib.h>
#include <stdio.h>
static FILE *svg=NULL;
#endif

inline float dist2d(point2d32f p1, point2d32f p2)
{
	float lenx,leny;
	
	lenx = p2.x - p1.x;
	leny = p2.y - p1.y;
	
	return sqrt(lenx*lenx + leny*leny);
}

/* returns (unit) normal vector to specified line segment */
/* assumes counter-clockwise arrangement, protrusion from center */
inline point2d32f normal2d(point2d32f p0, point2d32f p2)
{
	float tx,ty,tlen;
	point2d32f n;
	
	tx = p2.x - p0.x;
	ty = p2.y - p0.y;
		
	tlen = 1/sqrt(tx*tx+ty*ty);
		
	n.x =  ty*tlen;
	n.y = -tx*tlen;
	
	return n;
}

/* scale path by <factor> */
void path_scale(point2d32f *path2, point2d32f *path1, int n, float factor)
{
	int i;
	for(i=0;i<n;i++){
		path2[i].x = factor*path1[i].x;
		path2[i].y = factor*path1[i].y;
	}
}

#define MAX_Q 2560

#define Q_PUSH(a,q,qlen,qstart,qend,qmax) {q[qstart]=a;qstart++;qlen++;if(qlen>=qmax)return -1;if(qstart>=qmax)qstart-=qmax;}
#define  Q_POP(a,q,qlen,qstart,qend,qmax) {a=q[qend];qend++;qlen--;if(qlen<0)return -2; if(qend>=qmax)qend-=qmax;}

/* if any segments longer than len exist, halve them */
/* returns length of new path, negative on error */
static int halve_seg(point2d32f *path, float len, int n, int nmax)
{
	point2d32f q[MAX_Q];
	int i,qend,qstart,qlen;
	point2d32f new;
	int newn;
	
	newn=n; qend=0; qstart=0; qlen=0;
	for(i=0;i<n;i++){
		Q_PUSH(path[i],q,qlen,qstart,qend,MAX_Q);
		
		if(i<(n-1)){
			if(dist2d(path[i+1],path[i]) > len){
				new.x = 0.5*(path[i].x + path[i+1].x);
				new.y = 0.5*(path[i].y + path[i+1].y); 
				Q_PUSH(new,q,qlen,qstart,qend,MAX_Q);
				newn++;
				if(newn>nmax) return -3;
			}
		}
	
		Q_POP(path[i],q,qlen,qstart,qend,MAX_Q);
	}
	for(i=n;i<newn;i++) { Q_POP(path[i],q,qlen,qstart,qend,MAX_Q); }
	
	return newn;
}

#define Q_PUSH2(qlen,qstart,qend,qmax) {qstart++;qlen++;if(qlen>=qmax)return -1;if(qstart>=qmax)qstart-=qmax;}
#define  Q_POP2(qlen,qstart,qend,qmax) {qend++;  qlen--;if(qlen<0)    return -2;if(qend  >=qmax)qend  -=qmax;}

/* like above, but works with implicitly-closed curves,
 * and also returns correct phase */
/* this subroutine was simple until I realized that it would go from large phase
 * to zero on the last segment; now it goes to 2*PI */
static int halve_seg2(point2d32f *path, float *phase, float len, int n, int nmax, int closed)
{
	point2d32f q1[MAX_Q];
	float q2[MAX_Q];
	int i,i2,qend,qstart,qlen;
	point2d32f new;
	float newph;
	int newn;
	float phase2;
	
	newn=n; qend=0; qstart=0; qlen=0;
	for(i=0;i<n;i++){
		q1[qstart]=path[i];
		q2[qstart]=phase[i];
		Q_PUSH2(qlen,qstart,qend,MAX_Q);
		
		if((i<(n-1))||(closed)){
			i2 = closed ? (i+1)%n : i+1;
			if(dist2d(path[i2],path[i]) > len){
				phase2 = phase[i2];
				if(i==n-1) phase2=2*PI;		/* correct phase for wrap */
				
				new.x = 0.5*(path [i].x + path [i2].x);
				new.y = 0.5*(path [i].y + path [i2].y);
				newph = 0.5*(phase[i]   + phase2);
				
				q1[qstart]=new;
				q2[qstart]=newph;
				Q_PUSH2(qlen,qstart,qend,MAX_Q);
				newn++;
				if(newn>nmax) return -3;
			}
		}	
	
		path [i]=q1[qend];
		phase[i]=q2[qend];
		Q_POP2(qlen,qstart,qend,MAX_Q);
	}
	for(i=n;i<newn;i++){
		path [i]=q1[qend];
		phase[i]=q2[qend];
		Q_POP2(qlen,qstart,qend,MAX_Q);
	}
	
	return newn;
}

/* apply the above procedure repeatedly until no more segments of length
 * more than len exist */
int path_subdivide(point2d32f *path, float len, int n, int nmax)
{
	int newn;

	newn=n;
	do{
		n = newn;
		newn = halve_seg(path,len,n,nmax);
		if(newn<0) return newn;
	}while(newn!=n);
	
	return newn;
}

/* like above, but works with implicitly-closed curves
 * and returns correct phase */
int path_subdivide2(point2d32f *path, float *phase, float len, int n, int nmax, int closed)
{
	int newn;

	newn=n;
	do{
		n = newn;
		newn = halve_seg2(path,phase,len,n,nmax,closed);
		if(newn<0) return newn;
	}while(newn!=n);
	
	return newn;
}

/* if any two successive segments are together shorter than len,
 * merges them into single segment */
/* cleverly written to only require a single function call */
int path_simplify(point2d32f *path, float len, int n, int nmax)
{
	int i,j;
	float thislen,prevlen;
	
	prevlen=len;
	j=0;
	for(i=0;i<n-1;i++){
		thislen = dist2d(path[i+1],path[i]);
		path[j] = path[i];
		if(prevlen+thislen > len) j++; else thislen=prevlen+thislen;
		prevlen=thislen;
	}
	path[j]=path[n-1];
	return j+1;
}

/* like above,
 * but also returns the correct phase parameter for each point */
/* TODO: handling of implicitly closed curves */
int path_simplify2(point2d32f *path, float *phase, float len, int n)
{
	int i,i2,j;
	float thislen,prevlen;
	
	prevlen=len;
	j=0;
	for(i=0;i<n-1;i++){
		i2=(i+1);
		thislen = dist2d(path[i2],path[i]);
		path [j] = path [i];
		phase[j] = phase[i];
		if(prevlen+thislen > len) j++; else thislen=prevlen+thislen;
		prevlen=thislen;
	}
	path[j]=path[n-1];
	return j+1;
}

/* like above,
 * but doesn't remove segments that form angles more acute than given */
/* angle=0: same behavior as path_simplify2
 * angle=PI: remove nothing */
/* TODO: handle implicitly closed curves */
int path_simplify_smart(point2d32f *path, float *phase, float len, int n, float angle)
{
	int i,i2,j;
	float thislen,prevlen;
	float thisang;
	float rangle;
	
	rangle = PI-angle;	/* convert obtuse angle into acute angle */
	
	thisang=0.0;	/* start parallel */
	prevlen=len;
	j=0;
	for(i=0;i<n-1;i++){
		i2=(i+1);
		thislen = dist2d(path[i2],path[i]);
		
		/* take dot product */
		/* todo: make this wrap around (for closed curves) */
		if(j>0) thisang = acos((
			(path[i].x-path[j-1].x)*(path[i2].x-path[i].x) + 
			(path[i].y-path[j-1].y)*(path[i2].y-path[i].y))
				/ (thislen*prevlen));
		
		/* path[j] changes at each iteration but j stays the same,
		 * until we exceed the length requirement and angle requirement */
		path [j] = path [i];
		phase[j] = phase[i];
		
		/* if the below criterion is satisfied, vertex is removed and we
		 * accumulate more length */
		if((len>prevlen+thislen)&&(thisang<rangle)) thislen=prevlen+thislen; else j++;
		prevlen=thislen;
	}
	/* We have to do this right now because of i<n-1 in the for loop */
	/* with support for implicit closed curves this should be removed */
	path [j] = path [n-1];
	phase[j] = phase[n-1];
	return j+1;
}

/* returns 1 if the line segments defined by pa1-pa2 and pb1-pb2
 * intersect. The intersection point is returned in *pi */
/* if lines are parallel, *pi is not modified */
/* this subroutine started out simple but then I realized I have to take the
 * case of vertical lines into account. It should now be bullet-proof.*/
/* 5 stars */
int find_intersect(point2d32f pa1, point2d32f pa2, point2d32f pb1, point2d32f pb2, point2d32f *pi)
{
	float sa,oa;	/* slope, offset */
	float sb,ob;
	float dxa,dxb;
	int dxaz,dxbz;	/* is dx zero ? */
	
	/* precalculations and conditionals */
	dxa = pa2.x - pa1.x;
	dxb = pb2.x - pb1.x;
	dxaz = fabs(dxa) < ALIB_ZERO ? 1 : 0;
	dxbz = fabs(dxb) < ALIB_ZERO ? 1 : 0;
	
	/* remove unworkable case first (both vertical) */
	if(dxaz && dxbz) return 0;
	
	sa=FLT_MAX; sb=FLT_MAX; oa=0; ob=0;	/* to stop gcc from whining */
	if(!dxaz){
		sa = (pa2.y - pa1.y)/dxa;
		oa = pa1.y - sa*pa1.x;
	}
	if(!dxbz){
		sb = (pb2.y - pb1.y)/dxb;
		ob = pb1.y - sb*pb1.x;
	}
	
	/* lines parallel? (would result in divide by zero) */
	if(fabs(sa-sb) < ALIB_ZERO) return 0;
	
	/* if not, calc intersection point */
	
	if((!dxaz)&&(!dxbz)){
		pi->x = (ob-oa)/(sa-sb);
		pi->y = sa*pi->x + oa;
	} else {
		if(dxaz){
			pi->x = pa2.x;
			pi->y = sb*pi->x + ob;
			if(((pi->y-pa1.y)*(pi->y-pa2.y)) > 0) return 0;
		} else {	/* dxb < ALIB_ZERO */
			pi->x = pb2.x;
			pi->y = sa*pi->x + oa;
			if(((pi->y-pb1.y)*(pi->y-pb2.y)) > 0) return 0;
		}
	}

	/* intersection point on line segments? */
	if(((pi->x - pa1.x)*(pi->x - pa2.x) <= 0)&&
	   ((pi->x - pb1.x)*(pi->x - pb2.x) <= 0)){
		return 1;
	}
	return 0;
}

#if 0
/* find the intersection of a line segment with a path */
/* only searches the path between <start> and <n> (this is useful for coroutine-like behavior) */
/* line segment defined by p1->p2 */
/* returns 1 if intersection, 0 otherwise */
/* intersection point returned in pi */
int find_path_intersect(point2d32f *pi, point2d32f p1, point2d32f p2, point2d32f *path, int *start, int n)
{
	for(;(*start)<n;(*start)++){
		if(find_intersect(p1,p2,path[(*start)],path[(*start)+1],pi) return 1;
	}
	return 0;
}
#endif

void shiftleft(point2d32f *path, float *phase, int n)
{
	int i;
	float tmpx,tmpy,tmpp;
	
	tmpx=path[0].x;
	tmpy=path[0].y;
	tmpp=phase[0];
	
	for(i=1;i<n;i++){
		path [i-1].x=path[i].x;
		path [i-1].y=path[i].y;
		phase[i-1]=phase[i];
	}
	
	path[n-1].x=tmpx;
	path[n-1].y=tmpy;
	phase[n-1]=tmpp;
}

/* locate self-intersections in the path.
 * returns 1 if any intersections were found */
int find_intersecting_run(point2d32f *path, int n, int closed, point2d32f *pi, int *i_intersect, int *j_intersect)
{
	int i,j,j2;
	int intersection;
	int istart;
	
	intersection=0;
	for(j=0;j<n;j++){
		
		/* wrap around if closed */
		j2=j+1;
		istart=0;
		if(j==(n-1)){
			if(!closed) break;
			j2=0;
			istart=1; /* don't want to do consecutive segments */
		}
		
		for(i=istart;i<j-1;i++){	/* j-1; don't want to do consecutive segments (they always 'intersect') */
			if(find_intersect(path[i],path[i+1],path[j],path[j2], pi)){
				/* two segments intersect; find the shortest sub-path*/
				/* make i the beginning of the segment to be cut out */
				intersection=1;
				*i_intersect = i;
				*j_intersect = j;
			
#ifdef TEST_MODE
				printf("\tintersection\n");
				if(svg){	/* svg is global, and only declared in testing*/
					fprintf(svg,"<circle cx=\"%f\" cy=\"%f\" r=\"3\" fill=\"red\" stroke=\"black\" stroke-width=\"0\"></circle>\n",pi->x,pi->y);
				}
#endif
			}
										   
			if(intersection){
				/* wait and see; we don't want to chop off too much! */
				if((j-i)<=(n/2)) break;
			}
		}
		if(intersection) break;
	}
	
	return intersection;
}

/* the rule, as with the above, is that i_intersect < j_intersect */
int find_lowweight_run(point2d32f *path, float *perr, int n, int closed, point2d32f *pi, int *i_intersect, int *j_intersect)
{
	int j,j2;
	int ii,jj;
	int inrun;
	
	inrun=0;
	for(j=0;j<2*n;j++){
		
		/* wrap around if closed */
		j2=j;
		
		if(j>=n){
			if(!closed) break;
			j2=j-n;
		}
		
		/* the second clause is to prevent remove_run, which will be
		 * called after, from deleting the wrong half of the path. */
		if(perr[j2] > PERR_THRESH){
			/* if we ended low-weight run and it was long mark it for removal */
			if(inrun > PERR_RUN){
				jj = j % n;
				ii = (j-inrun+1) % n;
				
				printf("ii:%d jj:%d\n", ii, jj);
				pi->x = 0.5*(path[ii].x + path[jj].x);
				pi->y = 0.5*(path[ii].y + path[jj].y);
				
				//printf("ii, jj: %d %d - %f\n", ii, jj);
				*i_intersect = ii;
				*j_intersect = jj;
				
				return 1;
			}
			inrun=0;
		} else {
			inrun++;
		}
	}
	return 0;
}


/* remove the intersection */
/* inputs:
 *  i: start of intersecting run
 *  j: end   of intersecting run
 *  pi: precise intersection coordinates
 * Return value: new n
 */
/* phase and perr can be null; if so they are ignored */
/* the way the cut is done is we remove the points i+1,i+2,...,j */
int remove_run(point2d32f *path, float *phase, float *perr, int n, int i, int j, point2d32f pi)
{
	int k;
	int reshift;
	
	if(i > j){
		/* Uh oh! The part to be removed includes the
		 * starting point. Which means we have to shift
		 * the beginning of the intersection to the starting point*/
		/* 0 1 ... j-1 j j+1 ... i-1 i i+1 ... n becomes:
		 * pi j+1 ... i-1 i
		 * or:
		 * j+1 ... i-1 i pi
		 * depending on reshift */
#ifdef TEST_MODE
		printf("\touter removal.\n");
#endif
		path[j] = pi;
		for(k=j;k<=i;k++) path[k-j]=path[k];
		if(perr) for(k=j;k<=i;k++) perr[k-j]=perr[k];
		reshift=0;
		if(phase){
			phase[j]=0.5*(phase[i]+(2*PI+phase[j+1]));
			/* if the new intersection point is not the first point,
			 * phase-wise, then remember to shift the whole thing again */
			if(phase[j]>2*PI) phase[j]-=2*PI; else reshift=1;
			for(k=j;k<=i;k++) phase[k-j]=phase[k];
		}
		n -= ((n-1)-i)+j;
		if(reshift) shiftleft(path, phase, n);
	} else {
		/* i-2 i-1 i i+1 ... j-1 j j+1 ... becomes:
		 * i-2 i-1 i pi j+1 ... */
#ifdef TEST_MODE
		printf("\tinner removal.\n");
#endif
		path[i+1] = pi;
		for(k=j+1;k<n;k++) path[k-(j-i)+1]=path[k];
		if(perr) for(k=j+1;k<n;k++) perr[k-(j-i)+1]=perr[k];
		if(phase){
			phase[i+1] = 0.5*(phase[i]+phase[j+1]);		/* TODO: HACK. Fixed? */
			for(k=j+1;k<n;k++) phase[k-(j-i)+1]=phase[k];
		}
		n -= (j-i-1);
	}
	return n;
}

/* remove loops in path */
/* uses naive algorithm as our path is never too long */
/* returns length of new path */
/* if closed=1, doesn't consider last-to-first one */
/* if phase=NULL, doesn't use it */
/* should work for implicit path closures */
/* 4 stars (pathological cases not tested) */
int remove_intersect(point2d32f *path, float *phase, int n, int closed)
{
	point2d32f pi;
	int j,i,tmp;
	
	pi.x=0.0; pi.y=0.0;	/* to stop gcc's whining */
	
	while(1){	/* remove intersections one by one */
		/* find intersections; return if none found */
		if(!find_intersecting_run(path,n,closed,&pi,&i,&j)) return n;
		
		/* find the correct half to chop off */
		/* which, since we have no other information,
		 * is the shortest one in this case */
		if((j-i)>(n/2)){
			tmp = i;
			i = j;
			j = tmp;
		}
		
		/* remove the intersection */
		n = remove_run(path,phase,NULL,n,i,j,pi);
	}
}

int remove_lowweight(point2d32f *path, float *perr, float *phase, int n, int closed)
{
	point2d32f pi;
	int j,i;
	
	pi.x=0.0; pi.y=0.0;	/* to stop gcc's whining */
	
	while(1){	/* remove intersections one by one */
		/* find intersections; return if none found */
		if(!find_lowweight_run(path,perr,n,closed,&pi,&i,&j)) return n;
		
		/* remove the intersection */
		n = remove_run(path,phase,perr,n,i,j,pi);
		//return n;
	}
}

/* if one path is deformed to another, uses this information (along with
 * confidence levels) to correct movements that stray from a smooth
 * deformation */
/* this subroutine used to be simple til I realized I have to consider
 * when the error-laden run overflows beyond pathn */
/* closed: number of closed (repeated) points. Usually 0 (open) or 1 (explicit closed) */
void repair_path(point2d32f *path1, point2d32f *path2, float *featerr, float thresh, int pathn, int closed)
{
		int j,k;
		int inrun,corrected;
		int j0,j1;
		
		corrected=1;
		for(k=0;corrected==1;k++){
			corrected=0;
			inrun=0;
			for(j=1;(j<pathn-1)||(inrun);j++){
				if((j==(pathn-1))&&(!closed)) return;	/* error run overflow and not closed */
				if(featerr[j%pathn] > thresh){
					inrun++;
					if(featerr[(j+1)%pathn] > thresh) continue;
					/* at end of error run */
					j = j-(inrun/2);
					j0=j-((inrun/2)+1);
					j1=j+((inrun/2)+1);
					if(j0<0) j0=pathn+j0-closed;
					if(j1>=pathn) j1=j1-pathn+closed;
#ifdef DEBUG_PRINT
					printf("k: %d run: %d j j-1 j+1: %d %d %d\n", k, inrun, j, j0, j1);
#endif
					/*path2[j].x = 0.5*(path2[(j-1)%pathn].x + path2[(j+1)%pathn].x);
					path2[j].y = 0.5*(path2[(j-1)%pathn].y + path2[(j+1)%pathn].y);*/
					path2[j%pathn].x = path1[j%pathn].x + 0.5*((path2[j1].x-path1[j1].x)+(path2[j0].x-path1[j0].x));
					path2[j%pathn].y = path1[j%pathn].y + 0.5*((path2[j1].y-path1[j1].y)+(path2[j0].y-path1[j0].y));
					featerr[j%pathn] = 0; //0.5*(featerr[j0] + featerr[j1]);
					corrected=1;
				}
				inrun=0;
#if 0
				if((fabs(path2[j].x-path1[j].x)>30.0)||(fabs(path2[j].y-path1[j].y)>30.0)){
					printf("adjusting\n");
					/*path2[j].x = 0.5*(path2[(j-1)%pathn].x + path2[(j+1)%pathn].x);
					path2[j].y = 0.5*(path2[(j-1)%pathn].y + path2[(j+1)%pathn].y);*/
					path2[j] = path1[j];
				}
#endif
			}
		}	
}

/* obtain (integral) bounding box of path */
alib_box bounding_box(point2d32f *path, int n)
{
	alib_box bb;
	float x1,y1,x2,y2;
	int i;
	
	x1=path[0].x; y1=path[0].y;
	x2=x1;        y2=y1;
	for(i=0;i<n;i++){
		if(path[i].x < x1) x1 = path[i].x;
		if(path[i].y < y1) y1 = path[i].y;
		if(path[i].x > x2) x2 = path[i].x;
		if(path[i].y > y2) y2 = path[i].y;
	}
	
	bb.x1=x1; bb.y1=y1; bb.x2=x2; bb.y2=y2;
	return bb;
}

/* extrude each point srcpath[i] on the path by factor extr[i] */
/* IMPORTANT: curve assumed to be counter-clockwise. For clockwise
 * curves, negate extr */
/* if all=1, extr[0] is applied to all points */
void path_extrude(point2d32f *dstpath, point2d32f *srcpath, int n, float *extr, int all, int closed)
{
	int i;
	int extri;
	int i0,i2;
	point2d32f norml;
	
	for(i=0;i<n;i++){
		if(!closed){
			if((i==0)||(i==(n-1))){
				/* skip endpoints */
				dstpath[i] = srcpath[i];
				continue;
			}
		}
		
		/* find neighbors */
		i0 = i-1 < 0 ? i-1+n : i-1;
		i2=((i+1)%n);
		
		norml = normal2d(srcpath[i0],srcpath[i2]);
		
		extri=i;
		if(all) extri=0;
		
		dstpath[i].x = srcpath[i].x + norml.x*extr[extri];
		dstpath[i].y = srcpath[i].y + norml.y*extr[extri];
	}
}

/* Does polarity result from a positive feedback loop (with actin polarization?) */

/* smooths a path by applying a 1-2-1 filter to it */
void path_smooth(point2d32f *dstpath, point2d32f *srcpath, int n, int closed, float dt)
{
    int i;
    int i0,i2;
    float ndt;
   
    ndt = 2.0-dt;
   
    for(i=0;i<n;i++){
		if(!closed){
			if((i==0)||(i==(n-1))){
				/* skip endpoints */
				dstpath[i] = srcpath[i];
				continue;
			}
		}
		
		/* find neighbors */
		i0 = i-1 < 0 ? i-1+n : i-1;
		i2=((i+1)%n);
		
        dstpath[i].x = 0.25*(dt*srcpath[i0].x + 2.0*ndt*srcpath[i].x + dt*srcpath[i2].x);
        dstpath[i].y = 0.25*(dt*srcpath[i0].y + 2.0*ndt*srcpath[i].y + dt*srcpath[i2].y);
    }
}

/* calculates normals up to a certain radius, what else? */
/* nrm[n x rad] is the output */
/* tangent is first calculated by convolution of neighborhood with
 * derivative of gaussian */
/* TODO: make it physically accurate */
void path_precalc_normals(point2d32f *nrm, point2d32f *path, int n, int rad, int closed)
{
	int i,j,k;
	int j2;
	float csd;
	float icsd2;
	float weight;
	float curweight;
	float dist;
	point2d32f curtan;
	
	for(k=0;k<rad;k++){
		csd = (float)(k+1);
		icsd2 = 1.0/(2*csd*csd);
		for(i=0;i<n;i++){
			
			/* do convolution to obtain tangent */
			weight=0.0; curtan.x=0.0; curtan.y=0.0;
			/* confine convolution to those within 3*sigma */
			for(j=-3*rad;j<3*rad;j++){
				j2=j+i;
				
				/* first one works for when rad > n */
				if(j2<0) { if(closed) j2 = n - ((-j2) % n);	else continue; }
				if(j2>n) { if(closed) j2 = j2 % n;          else continue; }
				
				dist = dist2d(path[i],path[j2]);
				
				curweight = exp(-dist*dist*icsd2);
				
				if(j>=0){
					curtan.x += curweight*path[j2].x;
					curtan.y += curweight*path[j2].y;
					weight += curweight;
				} else {
					curtan.x -= curweight*path[j2].x;
					curtan.y -= curweight*path[j2].y;
					weight -= curweight;
				}
			}
			
			/* we saved this to after (just a small optimization) */
			curtan.x -= weight*path[i].x;
			curtan.y -= weight*path[i].y;
			dist = sqrt(curtan.x*curtan.x + curtan.y*curtan.y);
			
			if(dist > ALIB_ZERO){
				dist = 1.0/dist;
				nrm[k + i*rad].x =  dist*curtan.y;
				nrm[k + i*rad].y = -dist*curtan.x;
			} else {
				nrm[k + i*rad].x = 0.0;
				nrm[k + i*rad].y = 0.0;
			}
		}
	}
}

/* -------------------------------------------------------------------------- */

/* Taken from Philip Nicoletti's code at http://www.codeguru.com/forum/printthread.php?t=194400 */
/* From what I can guess, it calculates the distance from the point to the line,
 * then the distance across the line that this occurs. If that distance is
 * inside the line segment, that is returned. Otherwise, the minimum of distance
 * to endpoints */
/* 5 star */
static float DistanceFromLine(float cx, float cy, float ax, float ay, float bx, float by)
{
	float r_numerator, r_denomenator, r, s, distanceLine, distanceSegment, dist1, dist2;
	/* float xx, yy;
	   float px, py; */
	
	r_numerator = (cx-ax)*(bx-ax) + (cy-ay)*(by-ay);
	r_denomenator = (bx-ax)*(bx-ax) + (by-ay)*(by-ay);
	r = r_numerator / r_denomenator;
    /*px = ax + r*(bx-ax);
    py = ay + r*(by-ay);*/
    s =  ((ay-cy)*(bx-ax)-(ax-cx)*(by-ay) ) / r_denomenator;

    distanceLine = fabs(s)*sqrt(r_denomenator);

	/*
	xx = px;
	yy = py; */

	if ((r>=0)&&(r<=1)){
		distanceSegment = distanceLine;
	} else {
		dist1 = (cx-ax)*(cx-ax) + (cy-ay)*(cy-ay);
		dist2 = (cx-bx)*(cx-bx) + (cy-by)*(cy-by);
		if(dist1 < dist2){
			/*
			xx = ax;
			yy = ay;*/
			distanceSegment = sqrt(dist1);
		} else {
			/*
			xx = bx;
			yy = by;*/
			distanceSegment = sqrt(dist2);
		}
	}

	return distanceSegment;
}

/* find closest distance from point to path */
/* naive algorithm */
/* 5 star, checked for both closed and open paths */
static float find_closest(point2d32f p, point2d32f *path, int n, int closed)
{
	int i,i2;
	float thisdist,mindist=0.0;	/* to stop gcc's whining */
	
	for(i=0;i<n;i++){
		i2=i+1;
		if(i2==n){
			if(!closed) break;
			i2=0;
		}
		
		thisdist = DistanceFromLine(p.x,p.y,path[i].x,path[i].y,path[i2].x,path[i2].y);
		
		if(i==0){ mindist=thisdist; continue; }
		if(thisdist<mindist) mindist=thisdist;
	}
	
	return mindist;;
}

/* compare test path against reference path */
/* finds average distance of each vertex of test path from reference path */
float find_path_distance(point2d32f *pref, int nref, point2d32f *ptest, int ntest, int closed)
{
	int i;
	float sum;
	
	sum=0.0;
	for(i=0;i<ntest;i++)
		sum += find_closest(ptest[i],pref,nref,closed);
		
	return sum/((float)ntest);
}

#ifdef TEST_MODE

void test1()
{
	int i;
	point2d32f p[20];
	float     ph[20];
	int newn;
	
	p[0].x=0; p[0].y=0; ph[0]=0;
	p[1].x=1; p[1].y=1; ph[1]=1;
	p[2].x=9; p[2].y=2; ph[2]=2;
	p[3].x=9; p[3].y=9; ph[3]=3;
	
	newn = path_subdivide2(p, ph, 3.0, 4, 20,0);
	printf("number of new segs: %d\n", newn);
	
	for(i=0;i<newn;i++){
		printf("phase: %f x,y: %f,%f\n", ph[i], p[i].x, p[i].y);
	}
	
	
	newn = path_simplify2(p, ph, 5.0, newn);
	//newn = path_simplify(p,5.0,newn,20);
	printf("number of new segs: %d\n", newn);
	
	for(i=0;i<newn;i++){
		printf("phase: %f x,y: %f,%f len:%f\n", ph[i], p[i].x, p[i].y, dist2d(p[(i+1)%newn],p[i]));
	}
	/*
	p[0].x=0; p[0].y=0;
	p[1].x=1; p[1].y=1;
	p[2].x=0; p[2].y=1;
	p[3].x=1; p[3].y=0;
	*/
	
	p[0].x=1; p[0].y=0;
	p[1].x=2; p[1].y=1;
	p[2].x=1; p[2].y=1;
	p[3].x=0; p[3].y=1;
	
	printf("intersect: %f %f y:%d\n", p[4].x, p[4].y, find_intersect(p[0],p[1],p[2],p[3],p+4));
	
	p[0].x=0; p[0].y=0; ph[0]=0.0*PI/3.0;
	p[1].x=1; p[1].y=0; ph[1]=1.0*PI/3.0;
	p[2].x=2; p[2].y=1; ph[2]=2.0*PI/3.0;
	p[3].x=2; p[3].y=0; ph[3]=3.0*PI/3.0;
	p[4].x=1; p[4].y=1; ph[4]=4.0*PI/3.0;
	p[5].x=0; p[5].y=1; ph[5]=5.0*PI/3.0;
	
	newn=6;
	newn = remove_intersect(p, ph, newn, 0);
	printf("after intersection removal: %d\n", newn);
	for(i=0;i<newn;i++){
		printf("x,y;t: %f, %f; %f\n", p[i].x, p[i].y, ph[i]/(2*PI));
	}
	printf("\n");
	
	p[0].x=0; p[0].y=0; ph[0]=0.0*PI/4.0;
	p[1].x=1; p[1].y=1; ph[1]=1.0*PI/4.0;
	p[2].x=2; p[2].y=1; ph[2]=2.0*PI/4.0;
	p[3].x=3; p[3].y=1; ph[3]=3.0*PI/4.0;
	p[4].x=3; p[4].y=0; ph[4]=4.0*PI/4.0;
	p[5].x=2; p[5].y=0; ph[5]=5.0*PI/4.0;
	p[6].x=1; p[6].y=0; ph[6]=6.0*PI/4.0;
	p[7].x=0; p[7].y=1; ph[7]=7.0*PI/4.0;
	
	newn=8;
	newn = remove_intersect(p, ph, newn, 0);
	printf("after intersection removal: %d\n", newn);
	for(i=0;i<newn;i++){
		printf("x,y;t: %f, %f; %f\n", p[i].x, p[i].y, ph[i]/(2*PI));
	}
	
}

void test2()
{
	printf("%f \n", DistanceFromLine(0.0,0.0,1.0,0.0,2.0,0.0));
	printf("%f \n", DistanceFromLine(0.0,1.0,1.0,0.0,2.0,0.0));
	printf("%f \n", DistanceFromLine(1.0,0.0,1.0,0.0,2.0,0.0));
	printf("%f \n", DistanceFromLine(1.5,0.0,1.0,0.0,2.0,0.0));
	printf("%f \n", DistanceFromLine(2.0,0.0,1.0,0.0,2.0,1.0));
}

void test3()
{
	point2d32f p;
	point2d32f path[3];
	
	p.x = 0.5;
	p.y = 0.3333*0.86;
	
	path[0].x=0.0;
	path[0].y=0.0;
	path[1].x=1.0;
	path[1].y=0.0;
	path[2].x=0.5;
	path[2].y=0.86;
	
	printf("%f\n", find_closest(p, path, 3, 1));
}

void test4()
{
	FILE *f;
	unsigned char b;
	float d;
	float i;
	int c;
	
	point2d32f p;
	point2d32f path[10];

	/*
	path[0].x=100.0;
	path[0].y=100.0;
	path[1].x=700.0;
	path[1].y=100.0;
	path[2].x=400.0;
	path[2].y=100+600.0*0.86;
	*/

	c=0;
	for(i=0;i<6.28;i+=0.5){
		path[c].x = 400.0 + 300.0*cos(i) + 100.0*((float)rand())/RAND_MAX;
		path[c].y = 300.0 + 300.0*sin(i) + 100.0*((float)rand())/RAND_MAX;
		c++;
	}
	
	f = fopen("test.pgm","w");
	if(!f){
		printf("couldn't open output file.\n");
		return;
	}
	
	fprintf(f,"P5\n800 600\n255\n");
	
	for(p.y = 0.0; p.y < 600.0; p.y++){
		for(p.x = 0.0; p.x < 800.0; p.x++){
			d = find_closest(p, path, 10, 1);
			if(d > 255.0) d=255.0;
			b = 255-d;
			fwrite(&b,1,1,f);
		}
	}
	
	fclose(f);
}

#define TEST_PATHLEN 128

char xmlhead[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>";
char svghead[] = "<svg id=\"svg3119\" xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\" xmlns=\"http://www.w3.org/2000/svg\" height=\"600\" width=\"800\" version=\"1.1\" xmlns:cc=\"http://creativecommons.org/ns#\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\">";

/* create a bunch of random paths and remove segments from them */
/* this is to test remove_intersect, which I suspect a lot of my bugs
 * are coming from */
void test5()
{
	int n;
	float i;
	int j,k;
	FILE *f;
	char fname[32];
	
	point2d32f path[TEST_PATHLEN+1];
	float phase[TEST_PATHLEN+1];
	
	for(j=0;j<10;j++){
		for(i=0,n=0;i<2*PI;i+=(2*PI/TEST_PATHLEN)){
			path[n].x = 400.0 + 200.0*cos(i) + 100.0*((float)rand())/RAND_MAX;
			path[n].y = 300.0 + 200.0*sin(i) + 100.0*((float)rand())/RAND_MAX;
			phase[n] = i;
			n++;
		}
		
		sprintf(fname,"%03d.svg",j);
		f = fopen(fname,"w");
		if(!f){
			printf("Couldn't open %s for output.\n", fname);
			return;
		}
		printf("%s\n",fname);
		
		/* prepare svg */
		fprintf(f,"%s\n%s\n",xmlhead,svghead);
		
		/* write original curve */
		fprintf(f,"<path id=\"orig\" d=\"M %f,%f ",path[0].x,path[0].y);
		for(k=1;k<n;k++) fprintf(f,"L %f,%f ", path[k].x, path[k].y);
		fprintf(f,"z\" ");
		
		/* tell it how to render it */
		fprintf(f,"stroke=\"#000\" stroke-miterlimit=\"4\" stroke-dasharray=\"none\" stroke-width=\"0.5\" fill=\"none\"/>\n");
		
		svg=f;
		n = remove_intersect(path, phase, n, 1);
		
		/* write modified curve */
		fprintf(f,"<path id=\"rem\" d=\"M %f,%f ",path[0].x,path[0].y);
		for(k=1;k<n;k++) fprintf(f,"L %f,%f ", path[k].x, path[k].y);
		fprintf(f,"z\" ");
		
		/* tell it how to render it */
		fprintf(f,"stroke=\"#000\" stroke-miterlimit=\"4\" stroke-dasharray=\"none\" stroke-width=\"2.0\" fill=\"none\"/>\n");
		
		/* end svg */
		fprintf(f,"</svg>\n");
		
		fclose(f);
	}
}

/* test path_simplify_smart */
void test6()
{
	int i;
	point2d32f p[20];
	float     ph[20];
	int newn;
	
	p[0].x=0;  p[0].y=0; ph[0]=0;
	p[1].x=0;  p[1].y=1; ph[1]=1;
	p[2].x=16; p[2].y=2; ph[2]=2;
	p[3].x=0;  p[3].y=3; ph[3]=3;
	
	newn = path_subdivide2(p, ph, 3.0, 4, 20,0);
	printf("number of new segs: %d\n", newn);
	
	for(i=0;i<newn;i++){
		printf("phase: %f x,y: %f,%f\n", ph[i], p[i].x, p[i].y);
	}
	
	
	newn = path_simplify_smart(p, ph, 5.0, newn, 0.0399*PI); //0.5*PI);
	//newn = path_simplify(p,5.0,newn,20);
	printf("number of new segs: %d\n", newn);
	
	for(i=0;i<newn;i++){
		printf("phase: %f x,y: %f,%f len:%f\n", ph[i], p[i].x, p[i].y, dist2d(p[(i+1)%newn],p[i]));
	}
}

int main()
{
	test5();
	return 0;
}

#endif
