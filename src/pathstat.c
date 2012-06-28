
/* statistical operations on paths */

#include "point2d.h"
#include "boxmuller.h"

/* perturbs a path using gaussian-distributed noise */
/* path1 and path2 can both be equal */
void perturb_path(point2d32f *path1, point2d32f *path2, int n, float sd)
{
	int i;
	float px,py;
	
	for(i=0;i<n;i++){
		box_muller2(&px,&py);
		path2[i].x = path1[i].x + px*sd;
		path2[i].y = path1[i].y + py*sd;
	}
}
