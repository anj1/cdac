#include <stdio.h>	/* TODO */
#include "point2d.h"	/* point2d32f */

#define AFABS(a) (a > 0 ? a : -a)
void draw_line(unsigned char *a, int w, int x, int y, char xo, char yo, unsigned char col)
{
	float fx,fy,fxo,fyo;
	float siz;
	float i;
	
	fx=x; fxo=xo;
	fy=y; fyo=yo;
	
	if(AFABS(fxo) > AFABS(fyo)) {siz = AFABS(fxo);} else {siz = AFABS(fyo);}
	
	fxo /= siz;
	fyo /= siz;
	
	for(i=0;i<siz;i++){
		a[((int)fy)*w + ((int)fx)] = col;
		fx += fxo;
		fy += fyo;
	}
}

/* if error is NULL, doesn't use it */
void draw_motion(point2d32f *a, point2d32f *b, int n, unsigned char *out, char *status, float *error, float maxerr, int invert, int w, int h)
{
    int x,y,mx,my,i;
    float thiserr;
    
    /* create offset picture */
    i=0;
    
    for(i=0;i<n;i++){
		if(status!=NULL){
			if(!status[i]) continue;
		}
		
		x = a[i].x;
		y = a[i].y;
		mx = b[i].x - x;
		my = b[i].y - y;
		
		if((x+mx)< 0) continue;
		if((y+my)< 0) continue;
		if((x+mx)>=w) continue;
		if((y+my)>=h) continue;
		
		thiserr=0.0;
		if(error!=NULL) thiserr=255.0*error[i]/maxerr;
		
		if(thiserr>255.0) thiserr=255.0;
		if(invert) thiserr=255-thiserr;
		draw_line(out,w,x,y,mx,my,(unsigned char)thiserr);
		out[(y+my)*w + x + mx] = 128;
	}
}

void export_motion(char *mx, char *my, unsigned char *mt, float *conf, int w, int h, char *filename)
{
    FILE *f;
    int x,y,i;
   
    /* first create offset picture */
    for(i=0;i<w*h;i++) mt[i]=255;
    i=0;
    for(y=0;y<h;y++){
		for(x=0;x<w;x++){
			i = y*w + x;
			if(conf[i]>0.99){
				draw_line(mt,w,x,y,mx[i],my[i],0);
				mt[(y+my[i])*w + x + mx[i]] = 128;
			}
		}
	}
	
    f = fopen(filename, "wb");
   
    if(!f){
		printf("error: couldn't open %s for output\n", filename);
		return;
	}
	
    fprintf(f,"P5\n");
    fprintf(f,"%d %d\n", w, h);
    fprintf(f,"255\n");
   
    fwrite(mt, 1, w*h, f);
   
    fclose(f);
}

/* paints normals onto canvas */
/* pathprob is used to assign shade. If NULL, shades on distance */
void paint_nrm(unsigned char *buf, int w, int h, point2d32f *path, int pathn, point2d32f *nrm, float *bminus, float *bplus, float *pathprob, int rad)
{
	int i,j;
	int idx;
	float dx,dy;
	float out;
	
	//for(i=0;i<w*h;i++) buf[i]=255;
	
	for(i=0;i<pathn;i++){
		for(j=0;j<rad;j++){
			if((float)j <= bplus[i]){
				dx = path[i].x + ((float)j)*nrm[j + i*rad].x;
				dy = path[i].y + ((float)j)*nrm[j + i*rad].y;
				idx = ((int)dx) + w*((int)dy);
				if(pathprob){
					out = 25000.0*pathprob[(j+rad) + i*rad*2];
					if(out>255.0) out=255.0;
				} else {
					out = (j*128)/rad;
				}
				buf[idx] = out;
			}
		
			if((float)j <= -bminus[i]){
				dx = path[i].x - ((float)j)*nrm[j + i*rad].x;
				dy = path[i].y - ((float)j)*nrm[j + i*rad].y;
				idx = ((int)dx) + w*((int)dy);
				if(pathprob){
					out = 25000.0*pathprob[(rad-j) + i*rad*2];
					if(out>255.0) out=255.0;
				} else {
					out = (-j*128)/rad;
				}
				buf[idx] = out;
			}
		}
	}
}
