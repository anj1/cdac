/* COMPILE: svg_out.c -Wall -lm -DTEST_MODE
 */

/* find intersections of a line segment with a (integer) square grid */
/* by Alireza Nejati.
 * September 2011 */

/* I have tested this extensively and it should work for *all*
 * reasonable inputs */
 
/* Note that only crossings are calculated; if the line segment is
 * incident on the grid, no such points are returned.
 * Also, duplicate points are ok
 */
 
#include "point2d.h"
#include <math.h>
#ifdef TEST_MODE
#include <stdio.h>
#endif

/* intersection points returned in ip */
/* maxn is the maximum size of ip */
/* total distance travelled to current point is in *dist */
/* returns number of intersection points; negative if error */
/* algorithm is simple and elegant and works with pathological cases */
int line_intersect_grid(point2d32f *ip, float *dist, int maxn, float x1, float y1, float x2, float y2)
{
	/* v: vertical grid, h: horizontal grid */
	
	float t;
	float tv,th;	/* time */
	float dtv,dth;	/* differences in time */
	float x,y;		/* current position */
	float xh,yh,xv,yv;		/* current position on grids */
	float dx,dy;
	float len;		/* euclidean length of line */
	int i;
	
	dx = x2-x1;
	dy = y2-y1;
	
	len = sqrt(dx*dx + dy*dy);
	
	dth = 1.0/fabs(dy);
	dtv = 1.0/fabs(dx);
	
	/* find first intersection point using Thales */
	if(dy!=0.0){	/* needed because sometimes y1-floor(y1)=0.0 too */
		if(y2 > y1)
			th = (floor(y1)+1-y1)*dth;
		else
			th = (y1-floor(y1))*dth;
	} else {
		th = 1.0/0.0;
	}
	
	if(dx!=0){
		if(x2 > x1)
			tv = (floor(x1)+1-x1)*dtv;
		else
			tv = (x1-floor(x1))*dtv;
	} else {
		tv = 1.0/0.0;
	}
	
	//printf("th, tv: %f, %f\n", th, dtv);
	
	/* main loop */
	t = 0;
	dist[0]=0.0;
	x = x1; y = y1;

	for(i=0; t<1.0; i++){
		ip[i].x = x;
		ip[i].y = y;
		
		if(i > maxn) return -1;

		/* is next point on vertical or horizontal grid? */
		if(th < tv){
			xh = th*dx;
			yh = th*dy;
			
			dist[i+1] = th*len;
			
			t=th;
			x=x1+xh;
			y=y1+yh;
			
			th+=dth;
		} else {
			xv = tv*dx;
			yv = tv*dy;
			
			dist[i+1] = tv*len;
			
			t=tv;
			x=x1+xv;
			y=y1+yv;
			
			tv+=dtv;
		}
	}

	ip[i].x = x2;
	ip[i].y = y2;
	dist[i]=len;		/* overwrite */
	i++;
	
	return i;
}

#ifdef TEST_MODE

#include <stdlib.h>	/* rand() */
#include <time.h>
#include "svg_out.h"

const char plainstyle[] = "fill:none;stroke:#000000;stroke-width:0.1px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1";

void test_line_intersect_grid(float x1, float y1, float x2, float y2)
{
	point2d32f ip[100];
	float dist[100];
	int i,n;

	printf("line_intersect_grid on (%f,%f)-(%f,%f)\n",x1,y1,x2,y2);

	n = line_intersect_grid(ip, dist, 100, x1, y1, x2, y2);
	
	for(i=0;i<n;i++){
		printf("%f %f dist: %f\n", ip[i].x, ip[i].y, dist[i]);
	}
}

#define RAND_FLT (((float)rand())/((float)RAND_MAX))

void test_to_svg(char *filnam)
{
	FILE *f;
	char pid[32];
	int i,n;
	point2d32f p1,p2;
	point2d32f ip[200];
	float dist[200];
	
	f = fopen(filnam,"w");
	if(!f){
		printf("ERROR: couldn't export to %s\n", filnam);
		return;
	}
	
	srand(time(0));
	
	xml_export_start(f, "svg");
	svg_export_start(f, 20, 20);
	
	for(i=0;i<314;i++){
		
		/*p1.x = 20.0*RAND_FLT;
		p2.x = 20.0*RAND_FLT;
		
		p1.y = 20.0*RAND_FLT;
		p2.y = 20.0*RAND_FLT;*/
		p1.x = 10.1;
		p1.y = 10.1;
		
		p2.x = p1.x + 9.0*cos(((float)i)*0.02);
		p2.y = p1.y + 9.0*sin(((float)i)*0.02);
		
		n = line_intersect_grid(ip,dist,200,p1.x,p1.y,p2.x,p2.y);
		if(n<2) printf("ERROR: n=%d\n", n);
		
		sprintf(pid,"path%d",rand());
		svg_export_path_start(f,(char *)plainstyle,pid,ip,n);
		svg_export_path_end(f);
	}

	svg_export_end(f);
	
	fclose(f);
}

int main()
{
	printf("Repeated points are ok (as long as length diff=0)\n");
	printf("line ends on grid points\n");
	test_line_intersect_grid(0,0,1,1);
	test_line_intersect_grid(0,0,2,2);
	test_line_intersect_grid(0,0,2,3);
	
	printf("\nGeneral cases\n");
	test_line_intersect_grid(0.5,0.5,1.5,1.5);
	test_line_intersect_grid(0.5,0.5,1.0,1.5);
	test_line_intersect_grid(0.5,0.0,2.0,1.5);
	test_line_intersect_grid(1.9,1.9,5.9,5.1);

	printf("\nReverse directions\n");
	test_line_intersect_grid(1,1,0,0);
	test_line_intersect_grid(0,1,1,0);
	test_line_intersect_grid(2,3,0,0);
	
	printf("\nPathological cases\n");
	/* perfectly horizontal and vertical lines */
	test_line_intersect_grid(0.5,0.5,0.5,1.5);
	test_line_intersect_grid(0.5,0.5,1.5,0.5);
	/* inside same square */
	test_line_intersect_grid(0.5,0.0,0.0,0.5);
	test_line_intersect_grid(0.4,0.4,0.6,0.6);
	/* same point */
	test_line_intersect_grid(0.5,0.5,0.5,0.5);
	
	printf("\nProblematic cases revealed during testing\n");
	test_line_intersect_grid(5.0,9.6,5.0,2.8);
	
	test_to_svg("intersect.svg");
	
	return 0;
}

#endif
