/* COMPILE: path.c svg_out.c -Wall -O2 -lm
 */

/* generate movie (animated svg) of a simple motile cell */
/* this file also serves as a test for path_extrude and path_smooth */

#include <math.h>	/* sin(),cos() */
#include <stdlib.h>	/* atoi() */
#include <string.h>	/* memcpy() */
#include <stdio.h>
#include "point2d.h"
#include "path.h"
#include "svg_out.h"

#ifndef PI
#define PI 3.141592653589793
#endif

#define MAX_POINTS 1000
#define CENTER_X 400
#define CENTER_Y 300
#define RADIUS 100
#define FRAME_RATE 5
#define EXTR_SPEED 4

#define PATH_STYLE "fill:none;stroke:#000000;stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1"

int main(int argc, char **argv)
{
	int n;
	int nframe;
	int i,j;
	float ang;
	FILE *svg;
	float seglen;	/* average length of each segment */
	int anim;
	int frameskip;
	
	char svgname[32];
	
	point2d32f path1[MAX_POINTS];
	point2d32f path2[MAX_POINTS];
	point2d32f orent[MAX_POINTS];	/* orientation of fibers */
	float       extr[MAX_POINTS];	/* extrusion amount */
	float      phase[MAX_POINTS];	/* the phase of each point; from 0 to 2pi */
	
	float grow;	/* rate that we grow to maintain size */
	
	float clen;
	point2d32f nrm;
	
	if(argc<6){
		printf("usage: motilecell <npoints> <nframes> <grow> <frameskip> <anim>\n");
		printf("anim should be either 1 (animated svg) or 0 (series of still svgs)\n");
		return 0;
	}
	
	n = atoi(argv[1]);
	nframe = atoi(argv[2]);
	grow = atof(argv[3]);
	frameskip = atoi(argv[4]);
	anim = atoi(argv[5]);
	
	if(n <= 0){
		printf("Error: number of points must be positive.\n");
		return 1;
	}
	
	if(n > MAX_POINTS){
		printf("Error: number of points exceeds maximum (%d)\n", MAX_POINTS);
		return 2;
	}
	
	/* create cell boundary (starts out as circle) */
	for(i=0;i<n;i++){
		ang = 2*PI*((float)i)/((float)n);
		
		path1[i].x = CENTER_X + RADIUS*cos(ang);
		path1[i].y = CENTER_Y + RADIUS*sin(ang);
		phase[i]   = ang;
	}
	seglen = 2*PI*RADIUS/((float)n);
	
	if(anim){
		svg = fopen("anim.svg","w");
		if(!svg){
			printf("Error: couldn't open output file.\n");
			return 3;
		}
		
		xml_export_start(svg,"svg");
		svg_export_start(svg,800,CENTER_Y*2);
		svg_export_path_start(svg,PATH_STYLE,"cell",path1,n);
	}

	for(i=0;i<nframe;i++){
	
		/* simplify, extrude, smooth, subdivide */
		
		/* NOTE: we can't simplify and subdivide if we require animated svg output */
		if(!anim) n = path_simplify2(path1,phase,seglen,n);
		
		/* create extrusion pattern */
		clen=0;
		for(j=1;j<n;j++){
			/*ang = phase[j];
			extr[j] = EXTR_SPEED*cos(ang);*/

			clen += dist2d(path1[j-1],path1[j]);
			//nrm = normal2d(path1[j],path1[j+1]);
			//printf("clen: %g\n", clen);
			extr[j] = 10*sin(clen*0.5);

#if 0
			extr[j]=0;
			if((phase[j] > 1.0) && (phase[j] < 3.0)){
				extr[j]=-15+50*((float)rand())/((float)RAND_MAX);
			}
#endif
		
			/*if(i<30) extr[j] = EXTR_SPEED*cos(ang*5);
			if(i>=30) extr[j] = -EXTR_SPEED*cos(ang*5);*/
		}
		
		path_extrude(path2,path1,n,extr,0,1);
		//memcpy(path2,path1,n*sizeof(point2d32f));
		
		path_smooth(path1,path2,n,1,1.0);
		
		//path_extrude(path2,path1,n,&grow,1,1);
		
		//memcpy(path1,path2,n*sizeof(point2d32f));
		
		if(!anim) n = path_subdivide2(path1,phase,2.0*seglen,n,MAX_POINTS,1);
		
		if(!(i%frameskip)){
			if(anim){
				svg_export_pathanim(svg,path1,n,((float)i)/FRAME_RATE,((float)frameskip)/FRAME_RATE);
			} else {
				sprintf(svgname,"frame_%04d.svg",i);
				svg = fopen(svgname,"w");
				if(!svg){
					printf("Error: couldn't open output file %s\n",svgname);
					return 4;
				}
				xml_export_start(svg,"svg");
				svg_export_start(svg,800,CENTER_Y*2);
				svg_export_path_start(svg,PATH_STYLE,"cell",path1,n);
				svg_export_path_end(svg);
				svg_export_end(svg);
				fclose(svg);
			}
		}
	}
	
	if(anim){
		svg_export_path_end(svg);
		svg_export_end(svg);
	
		fclose(svg);
	}

	return 0;
}
