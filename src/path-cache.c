/* find distance from point to line  */
/* this is done by noting that the area of a parallelogram is base*height */
float dist_point_to_line(point2d32f p, point2d32f p1, point2d32f p2)
{
	float dx,dy;
	float area;
	float len;
	
	dx = p2.x - p1.x;
	dy = p2.y - p1.y;
	
	/* calculate area of parallelogram p1->p->p2->p' */
	area = (p.x-p1.x)*dy-(p.y-p1.y)*dx;
	
	/* calculate length of p1->p2 */
	len = sqrt(dx*dx + dy*dy);
	
	return area / len;
}



int remove_run(point2d32f *path, float *phase, int n, int i, int j, point2d32f pi)
{
	int k;
	int reshift;
	
	if((j-i)>(n/2)){
		/* Uh oh! The part to be removed includes the
		 * starting point. Which means we have to shift
		 * the beginning of the intersection to the starting point*/
#ifdef TEST_MODE
		printf("\touter removal.\n");
#endif
		path[i] = pi;
		for(k=i;k<=j;k++) path[k-i]=path[k];
		reshift=0;
		if(phase){
			//printf("i,j:%d,%d phases: %f %f\n", i,j, phase[i+1]/(2*PI), phase[j]/(2*PI));
			//phase[i]=0.5*(phase[j]+(2*PI+phase[i]));
			phase[i]=0.5*(phase[j]+(2*PI+phase[i+1]));
			/* if the new intersection point is not the first point,
			 * phase-wise, then remember to shift the whole thing again */
			if(phase[i]>2*PI) phase[i]-=2*PI; else reshift=1;
			for(k=i;k<=j;k++) phase[k-i]=phase[k];
		}
		n -= ((n-1)-j)+i;
		if(reshift) shiftleft(path, phase, n);
	} else {
		/* i-2 i-1 i i+1 ... j-1 j j+1 ... becomes:
		 * i-2 i-1 i pi j+1 ... */
#ifdef TEST_MODE
		printf("\tinner removal.\n");
#endif
		path[i+1] = pi;
		for(k=j+1;k<n;k++) path[k-(j-i)+1]=path[k];
		if(phase){
			phase[i+1] = 0.5*(phase[i]+phase[j+1]);		/* TODO: HACK. Fixed? */
			for(k=j+1;k<n;k++) phase[k-(j-i)+1]=phase[k];
		}
		n -= (j-i-1);
	}
	return n;
}
