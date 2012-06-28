/* COMPILE: line_intrsct.c -Wall -O2 -lm -DTEST_BEAM
 */

/* Known issues: sometimes gives erroneous results when the line
 * endpoints are *right* on the image boundary (because it multiplies
 * them with 0, and if they are INF, then a NaN results)
 */
 
#include <stdio.h>
#include <math.h>
#include "point2d.h"
#include "line_intrsct.h"

#define MAX_IP	256

/* 5-star */
/* sample from a interpolation of a 2d image */
/* note that the interpolation is quadratic; but it's close enough to linear
 * that we just pretend it's linear */
#define SAMP_PC_LIN(img) (\
(img[idx      ]*vx + img[idx      +1]*wx)*vy + \
(img[idx+width]*vx + img[idx+width+1]*wx)*wy)

float beam_integral(float *sobx, float *soby, int width, point2d32f p1, point2d32f p2)
{
	point2d32f ip[MAX_IP];
	float dist[MAX_IP];
	int i,n;
	
	float sobx1,sobx2;
	float soby1,soby2;
	float dist1,dist2;
	float tmpx,tmpy;
	int xi,yi;	/* integer versions of x and y */
	int idx;	/* pointer to top left corner of current square */
	float wx,wy,vx,vy;
	float sumx,sumy;
	float dlen;
	float r;
	
	/* this case must be treated specially, because the limit must be
	 * returned (x/sqrt(x) as x goes to zero) */
	if((p1.x==p2.x)&&(p1.y==p2.y)) return 0.0;
	
	/* calculate intersection points with grid */
	n = line_intersect_grid(ip, dist, MAX_IP, p1.x, p1.y, p2.x, p2.y);
	/* verified that dist[n-1]==length of line */
	if(n < 0){
		fprintf(stderr,"ERROR:beam_integral:line segment too large!\n");
		return 0.0;
	}
	
	/* now imagine image is piecewise linear and calc line integral */
	sumx=0.0; sumy=0.0; dist2=-1.0;	/* -1 to guarantee setting sobx2,soby2
	                                   on i==0 */
	soby2=0.0; sobx2=0.0;	/* to stop gcc's whining */
	for(i=0;i<n;i++){
		sobx1 = sobx2;
		soby1 = soby2;
		dist1 = dist2;
		
		dist2 = dist[i];
		if(dist2==dist1) continue;	/* repeated points */
		
		wx = modff(ip[i].x, &tmpx);	xi=tmpx;
		wy = modff(ip[i].y, &tmpy); yi=tmpy;
		vx = 1.0 - wx;
		vy = 1.0 - wy;
		idx = xi + width*yi;
		/* printf("x,y:%f,%f xi,yi: %d,%d\n", ip[i].x, ip[i].y, xi, yi); */
		sobx2 = SAMP_PC_LIN(sobx);
		soby2 = SAMP_PC_LIN(soby);
		
		if(i==0) continue;	/* start at first line segment */
		
		/* calculate line integral */
		/* we exploit linearity */
		/* TODO: this can be optimized */
		dlen = dist2 - dist1;
		sumx += (sobx2 + sobx1)*dlen;
		sumy += (soby2 + soby1)*dlen;
		/* printf("soby1:%f soby2:%f dlen: %f\n",soby1,soby2,dlen); */
	}

#ifdef TEST_BEAM
	printf("sumx:%f sumy:%f\n", 0.5*sumx, 0.5*sumy);
#endif

	/* return normalized dot product with normal (dist[n-1] is length) */
	/* first division is for compensating for length of normal,
	 * second is to obtain statistic */
	//r = 0.5*(sumx*(p2.y-p1.y)-sumy*(p2.x-p1.x))/(sqrt(dist[n-1])*dist[n-1]);
	r = 0.5*(sumx*(p2.y-p1.y)-sumy*(p2.x-p1.x))/dist[n-1];
	//r = 0.5*(sumx*(p2.x-p1.x)+sumy*(p2.y-p1.y))/dist[n-1];
	//return r*r/dist[n-1];
	//return fabs(r)/dist[n-1];
	return -r/dist[n-1];
}

#ifdef TEST_BEAM

void test_interpolation(int width, float *img)
{
	FILE *f;
	float wx,wy;
	float vx,vy;
	float tmpx,tmpy;
	int xi,yi,idx;
	float x,y;
	int i;
	
	f = fopen("intrp.pgm","w");
	fprintf(f,"P2\n91 91\n255\n");
	i=0;
	for(y=0.0;y<9.0;y+=0.1){
		for(x=0.0;x<9.0;x+=0.1){
			wx = modff(x, &tmpx); xi=tmpx;
			wy = modff(y, &tmpy); yi=tmpy;
			vx = 1.0 - wx;
			vy = 1.0 - wy;
			idx = xi + width*yi;
			fprintf(f,"%d\n", (int)(15.0*SAMP_PC_LIN(img)));
			i++;
		}
		/* printf("y:%f yi:%d wy:%f vy:%f %f %d\n",y,yi,wy,vy,1.0*SAMP_PC_LIN(img),(int)(15.0*SAMP_PC_LIN(img)));
		printf("\t%f %f %f %f\n",img[idx],img[idx+1],img[idx+width],img[idx+width+1]); */
	}
	printf("count: %d\n", i);
	
	fclose(f);			
}

int main()
{
	float sobx[100];
	float soby[100];
	point2d32f p1,p2;
	int i,j;
	
	sobx[0]=0.0; sobx[1]=0.5;
	sobx[2]=0.5; sobx[3]=1.0;
	
	soby[0]=0.0; soby[1]=0.0;
	soby[2]=0.0; soby[3]=0.0;
	
	p1.x=0.5; p1.y=0.0;
	p2.x=0.0; p2.y=0.5;
	beam_integral(sobx, soby, 2, p1, p2);
	
	sobx[0]=0.0; sobx[1]=0.5; sobx[2]=1.0;
	sobx[3]=0.5; sobx[4]=1.0; sobx[5]=1.5;
	sobx[6]=1.0; sobx[7]=1.5; sobx[8]=2.0;
	
	soby[0]=0.0; soby[1]=0.5; soby[2]=1.0;
	soby[3]=0.5; soby[4]=1.0; soby[5]=1.5;
	soby[6]=1.0; soby[7]=1.5; soby[8]=2.0;
	
	p1.x=1.9999; p1.y=1.5;
	p2.x=0.5; p2.y=0.0;
	beam_integral(sobx, soby, 3, p1, p2);
	
	for(j=0;j<10;j++){
		for(i=0;i<10;i++){
			sobx[j*10 + i] = sqrt(i*i + j*j);
			soby[j*10 + i] = sqrt(i*i + j*j);
			printf("%06.3f ", sobx[j*10 + i]);
		}
		printf("\n");
	}
	sobx[6*10 + 6] = 0.0;
	/* export (to test interpolation) */
	test_interpolation(10, sobx);
	
	/* draw a semicircle */
	for(i=0;i<10;i++){
		p1.x = 0.1;
		p1.y = 0.2;
		
		p2.x = p1.x + 8.9*cos(((float)i)*0.157079633);
		p2.y = p1.y + 8.9*sin(((float)i)*0.157079633);
		
		/*p1.x = 8.9;
		p1.y = 0;
		p2.x = 8.9;
		p2.y = 9.0;*/
		beam_integral(sobx, soby, 10, p1, p2);
	}
	
	return 0;
}

#endif
