/* calculate Hausdorff distance between two sets of points */
/* naive algorithm */
/* note that this can also be used for paths, provided that they are
 * very finely divided (as is the case for paths in robie) */

/* p1 is the test set, p2 is the reference set */

#include <math.h>
#include "point2d.h"

#ifndef INFINITY
#define INFINITY 1e10
#endif

static inline float dist2d(point2d32f p1, point2d32f p2)
{
	float dx,dy;
	dx = p2.x - p1.x;
	dy = p2.y - p1.y;
	return sqrt(dx*dx + dy*dy);
}

/* calculate minimum distance from point p to path */
float min_dist(point2d32f *path, int n, point2d32f p)
{
	int i;
	float dis,min;
	
	min = +INFINITY;	/* if n==0 this is returned */
	
	for(i=0;i<n;i++){
		dis = dist2d(path[i],p);
		if(dis < min) min = dis;
	}
	
	return min;
}

float path_dist_hausdorff(point2d32f *p1, int n1, point2d32f *p2, int n2)
{
	int i;
	float dis,max;
	
	max = -INFINITY;
	
	for(i=0;i<n1;i++){
		dis = min_dist(p2,n2,p1[i]);
		if(dis > max) max = dis;
	}
	
	return max;
}

float path_dist_avg(point2d32f *p1, int n1, point2d32f *p2, int n2)
{
	int i;
	float sum=0;
	
	for(i=0;i<n1;i++){
		sum += min_dist(p2,n2,p1[i]);
	}
	
	return sum/((float)n1);
}
