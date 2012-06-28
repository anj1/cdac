#include <stdint.h>	/* uint16_t */
#include <malloc.h>        /* malloc() */
#include "point2d.h"	/* alib_box */

#define pix_type uint16_t

#define DEBUG_PRINT

/* histo: array of <range> elements */
static void histogram(pix_type *in, int w, int h, int range, int *histo)
{
	int i;
	for(i=0;i<=range;i++) histo[i]=0;
	for(i=0;i<w*h;i++) histo[in[i]]++;
}

static void histogram_region(pix_type *in, int w, alib_box bounds, int range, int *histo)
{
	int i,j;
	pix_type *ino;
	
	for(i=0;i<=range;i++) histo[i]=0;
	
	for(j=bounds.y1;j<bounds.y2;j++){
		ino = in + w*j;
		for(i=bounds.x1;i<bounds.x2;i++){
			histo[ino[i]]++;
		}
	}
}

/* out: emin, emax */
void edrc(pix_type *in, int w, int h, int *pemin, int *pemax, int cop)
{
	int *histo;
	int i;
	int rmin,rmax;	/* dynamic range of image */
	int emin,emax;	/* effective dynamic range */
	int cutoff;
	
	/* find maximum dynamic range of image */
	rmin=in[0]; rmax=in[0];
	for(i=0;i<w*h;i++){
		if(in[i] < rmin) rmin = in[i];
		if(in[i] > rmax) rmax = in[i];
	}
	
	/* rmax+1 because we want the rmax'th element to be available.
	 * without this it gives nasty memory stack corruption errors */
	histo = (int *)malloc((rmax+1) * sizeof(int));
	if(!histo){	/* in case of failure, fall back */
		(*pemin) = rmin;
		(*pemax) = rmax;
		return;
	}
	
	histogram(in, w, h, rmax, histo);
	
	/* determine effective dynamic range */
	cutoff = (((w*h)/(rmax-rmin))*cop)/256;	/* TODO: make user-selectable */
	emin=rmin; emax=rmax;
	for(i=rmin;i<rmax;i++) if(histo[i] > cutoff) {emin=i; break;}
	for(i=rmax;i>emin;i--) if(histo[i] > cutoff) {emax=i; break;}

#ifdef DEBUG_PRINT
	printf("dynamic:[%d,%d] effective dynamic:[%d,%d]\n",rmin,rmax,emin,emax);
	printf("cutoff:%d histo[emax]:%d\n",cutoff,histo[emax]);
#endif

	(*pemin) = emin;
	(*pemax) = emax;
	free(histo);
}

/* obtains effective dynamic range of a subregion of the image only */
/* useful for when we only want the contrast of a subregion to be accurate */
void edrc_region(pix_type *in, int w, alib_box bounds, int *pemin, int *pemax, int cop)
{
	int *histo;
	int i,j;
	int rmin,rmax;	/* dynamic range of image region */
	int emin,emax;	/* effective dynamic range */
	int cutoff;
	pix_type *ino;
	int reg_size;
	
	/* find maximum dynamic range of image region */
	rmin=in[0]; rmax=in[0];
	for(j=bounds.y1;j<bounds.y2;j++){
		ino = in + j*w;
		for(i=bounds.x1;i<bounds.x2;i++){
			if(ino[i] < rmin) rmin = ino[i];
			if(ino[i] > rmax) rmax = ino[i];
		}
	}

	/* rmax+1 because we want the rmax'th element to be available.
	 * without this it gives nasty memory stack corruption errors */
	histo = (int *)malloc((rmax+1) * sizeof(int));
	if(!histo){	/* in case of failure, fall back */
		(*pemin) = rmin;
		(*pemax) = rmax;
		return;
	}
	
	histogram_region(in, w, bounds, rmax, histo);
	
	/* determine effective dynamic range */
	reg_size = (bounds.y2-bounds.y1)*(bounds.x2-bounds.x1);
	cutoff = (((reg_size)/(rmax-rmin))*cop)/256;	/* TODO: make user-selectable */
	emin=rmin; emax=rmax;
	for(i=rmin;i<rmax;i++) if(histo[i] > cutoff) {emin=i; break;}
	for(i=rmax;i>emin;i--) if(histo[i] > cutoff) {emax=i; break;}

#ifdef DEBUG_PRINT
	printf("dynamic:[%d,%d] effective dynamic:[%d,%d]\n",rmin,rmax,emin,emax);
	printf("cutoff:%d histo[emax]:%d\n",cutoff,histo[emax]);
#endif

	(*pemin) = emin;
	(*pemax) = emax;
	free(histo);
}

/* for backwards compatibility */
void edr(pix_type *in, int w, int h, int *pemin, int *pemax)
{
	edrc(in,w,h,pemin,pemax,16);
}

/* normalizes and converts image to 8-bit, such that emin->0 and emax->255 */
void normalize(pix_type *in, unsigned char *out, int w, int h, int emin, int emax)
{
	int i;
	int rdiv;

	/* finally, convert */
	rdiv = (emax - emin);
#ifdef DEBUG_PRINT
	printf("contrast lost in conversion to 8 bit: %d/%d\n", emax-emin, 256);
#endif
	for(i=0;i<w*h;i++){
		if(in[i] > emax){
			out[i]=255;
			continue;
		}
		if(in[i] < emin){
			out[i]=0;
			continue;
		}
		
		/* we're here, input must be in effective range */
		out[i] = ((in[i]-emin)*255)/rdiv;
	}
}
