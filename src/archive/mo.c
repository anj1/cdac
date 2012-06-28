/* COMPILE: beamdynamic.c beam.c line_intrsct.c pclin.c fit_path2.c pgm_util.c matrix.c snake.c filters.c path.c svg.c vis_motion.c -Wall -I/usr/include/libxml2 -lm -lics -lavl -lxml2 -O3
 */
 
/* follow nuclei around */

/* August 10: fixed stupid bug where I was calling precalc_normals with
 * p->pathn, not pathn */
 
/* TODO: getpathfromsvg repeats initial point! */

/* Oct 17: added pgm capability to mo-ics and renamed it mo, so mo-pgm
 * is obsolete. It and mo-ics are bundled together in mo-ics-pgm and
 * archived */
 
#define SEG_LENGTH 8.0		/* 10 for series 7-90 */
#define NORMAL_DRIFT 1.0

/* TODO */
#define PRE_BLUR 1.0

#define INPUT_MAX 65536

#define pix_type uint16_t

#include <stdint.h>		/* pix_type, int32_t */
#include <stdlib.h>		/* strtol(), rand() */
#include <time.h>		/* time() (for seed) */
#include <string.h>		/* memcpy() */
#include <ctype.h>		/* tolower() */
#include <malloc.h>		/* malloc() */
#include <math.h>		/* sqrt() */
#include <stdio.h>
//#include <argp.h>			/* argp */
#include <libics.h>		/* Lib ICS */
#include <avl.h>		/* lib AVL */

#include "point2d.h"	/* point2d32f */
#include "histnorm.h"	/* edr(), normalize() */
#include "vis_motion.h" /* draw_motion() */
#include "svg.h"		/* get_path_from_svg */
#include "path.h"		/* all */
#include "prec.h"
#include "snake.h"
#include "fit_path.h"	/* fit_path() */
#include "pgm_util.h"
#include "pclin.h"		/* pclin_export_crosscorr_int() */

#ifndef PI
#define PI 3.141592654
#endif

#define ICS_CHECK(c) \
	if(c!=IcsErr_Ok){printf("Ics_Error=%d\n",c);return c;}

#define DEBUG_PRINT

enum filetype_e{
	IMG_UNKNOWN,
	IMG_ICS,
	IMG_PGM
};

void float_to_char(float *in, unsigned char *out, float coef, float offset, int size)
{
	int i;
	float tmp;
	for(i=0;i<size;i++){
		tmp=((in[i]+offset)*coef);
		if(tmp<0.0) tmp=0.0;
		if(tmp>255.0) tmp=255.0;
		out[i]=tmp;
	}
}

void conv_pix_type_to_char(pix_type *in, unsigned char *out, int size, int maxval)
{
	int i;
	int shift;
	for(shift=0;(256<<(shift+1))<=(maxval+1);shift++);
	for(i=0;i<size;i++) out[i]=(in[i]>>shift);
}

void conv_pix_type_to_float(pix_type *in, float *out, int max, int size)
{
	int i;
	float imaxf;
	imaxf = 1.0/((float)max);
	for(i=0;i<size;i++) out[i]=imaxf*((float)in[i]);
}

void dump_int_file(int *x, int n, char *filename, char *mode)
{
	int i;
	FILE *f;
	if(!(f=fopen(filename,mode))){printf("ERR:dump_int\n");return;}
	for(i=0;i<n;i++) fprintf(f,"%d ", x[i]);
	fprintf(f,"\n");
	fclose(f);
}

void dump_stats_file(char *filename, float kappa, int start, int end, float dist)
{
	FILE *f;
	if(!(f=fopen(filename,"a"))){printf("ERR:dump_stats_file\n");return;}
	fprintf(f,"%f %d %d %f\n",kappa,start,end,dist);
	fclose(f);
}

/* out: dx */
void calc_dx(float *dx, point2d32f *path, int n)
{
	int i;
	for(i=1;i<n;i++) {dx[i] = dist2d(path[i-1],path[i]); printf("dx(%d): %f\n", i, dx[i]);}
	dx[0] = dist2d(path[n-1],path[0]);
	printf("dx(0): %f\n", dx[0]);
}

/* out: dx */
void calc_phase(float *hase, point2d32f *path, int n)
{
	int i;
	float fac;
	hase[0]=0.0;
	for(i=1;i<n;i++) hase[i] = hase[i-1]+dist2d(path[i-1],path[i]);
	fac = (hase[n-1] + dist2d(path[n-1],path[0]))/(2*PI);
	for(i=1;i<n;i++) hase[i] /= fac;
}

void calc_dphase(float *dx, float *hase, int n)
{
	int i;
	for(i=1;i<n;i++) dx[i] = hase[i] - hase[i-1];
	dx[0] = hase[0]-hase[n-1]+2*PI;
}

/* converts a string to lowercase */
/* assumes is valid string or part of string (doesn't add 0) */
void strntolower(char *str, int n)
{
	int i;
	for(i=0;i<n;i++) str[i] = tolower(str[i]);
}

/* returns the lower-case version of the file extension */
/* doesn't allocate space; just points to beginning of extension in fil,
 * so if fil is modified, ext will be modified! */
/* Also, if there is trailing whitespace, will be included in ext */
/* could fail on non-ascii encodings */
/* returns 1 if non-ascii encoding found; 2 if no extension found,
 * Otherwise, 0 */
int get_file_extension(char **ext, char *fil)
{
	int i,len;
	char c;
	
	len = strlen(fil);
	for(i=len-1;i>=0;i--){
		c = fil[i];
		if(c < 0) return 1;
		if(c=='.'){
			/* Copy extension; don't copy the dot */
			i++;
			*ext = fil+i;
			strntolower(*ext,len-i);
			return 0;
		}
	}
	return 2;
}

void export_path_svg_test(char *fname, point2d32f *path, int n)
{
	FILE *f;
	int i;
	
	char xmlhead[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>";
	char svghead[] = "<svg id=\"svg3119\" xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\" xmlns=\"http://www.w3.org/2000/svg\" height=\"600\" width=\"800\" version=\"1.1\" xmlns:cc=\"http://creativecommons.org/ns#\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\">";
	
	f = fopen(fname,"w");
	if(!f) return;
	
	fprintf(f,"%s\n%s\n",xmlhead,svghead);
	
	/* write modified curve */
	fprintf(f,"<path id=\"cell\" d=\"M %f,%f ",path[0].x,path[0].y);
	for(i=1;i<n;i++) fprintf(f,"L %f,%f ", path[i].x, path[i].y);
	fprintf(f,"z\" ");
	
	/* tell it how to render it */
	fprintf(f,"stroke=\"#000\" stroke-miterlimit=\"4\" stroke-dasharray=\"none\" stroke-width=\"0.5\" fill=\"none\"/>\n");
	
	fprintf(f,"</svg>\n");
	fclose(f);
}

/* export a cell outline structure in different ways; such as
 * bitmap superimposed on input image, and vector data */
void export_spath(spath *s, pix_type *img, unsigned char *buf, int maxval, int w, int h, char *outdir, int i)
{
	int j;
	char cmdlin[1024];
	
	printf("for this frame, s.npath: %d\n", s->npath);
	for(j=0;j<s->npath;j++){
		conv_pix_type_to_char(img, buf, w*h,maxval);
		
		draw_motion((struct point2d32f *) s->path[j].p,
		            (struct point2d32f *)(s->path[j].p+1),
		             s->path[j].n-1,buf,NULL,s->path[j].perr,0.1,0,w,h);

		sprintf(cmdlin,"%s/outl-k%e_%04d.svg",outdir,s->path[j].opts.kappa,i);
		export_path_svg_test(cmdlin,s->path[j].p,s->path[j].n);
		
		sprintf(cmdlin,"%s/sup-k%e_%04d.pgm",outdir,s->path[j].opts.kappa,i);
		export_8bit_pgm(buf,w,h,cmdlin);
	}
}

int main(int argc, char **argv)
{

    Ics_Error icserr;
    ICS* ics;
    size_t dims[ICS_MAXDIM];
    int ndims;
    int i;
    Ics_DataType dt;
    /* TODO */
    pix_type *prev,*next;    /* previous frame */
	unsigned char *buf; /* 8-bit buffer */
	int startframe,endframe;
	float *tmp1;//,*tmp2,*var,*sob;
	float *sobx,*soby;
	int err;
	char curname[1024];
	char cmdlin[128];
	
	point2d32f mstart[MAX_PATHN];
	point2d32f mend[MAX_PATHN];
	float merr[MAX_PATHN];
	
	float dx[MAX_PATHN];	/* for aocorrelaion calclaion */
	
	spath s;	/* set of paths */
	
	cpath *his;
	int avgextr[MAX_PATHN];
	int doac=0;
	
	float sobhist[INPUT_MAX*4];
	
	point2d32f refpath[MAX_PATHN];
	int nref;
	
	float kappa;
	
	enum filetype_e filetype;
	char *ext;
	size_t w,h;
	int maxval;
	int trackstart;
	
    if(argc < 7){
        printf("usage: mo <filename.ics | filename<format>.pgm> <outline.svg> startframe endframe kappa <output-dir>\n");
        printf("\tIf filename is .pgm, we assume it's a series of pgm files, and the format string must be a printf-type string with a %%[]d inside it.\n");
        printf("examples:\n");
        printf("\tmo livecell.ics outline.svg 0 1000\n");
        printf("\tmo frame_%%04d.pgm 10.svg 10 20\n");
        return 0;
    }
	
	/* randomize */
	srand(time(0));
	
	/* -------------------------------------- */
	/* What type of file are we dealing with? */
	filetype = IMG_UNKNOWN;
	get_file_extension(&ext,argv[1]);
	if(strncmp(ext,"ics",3)==0) filetype = IMG_ICS;
	if(strncmp(ext,"pgm",3)==0) filetype = IMG_PGM;
	if(filetype == IMG_UNKNOWN){
		printf("Error: Unknown file type '%s'\n", ext);
		return 5;
	}
	
	/* ------------------------------------------------------- */
	/* Get file dimensions etc. (needed for allocating arrays) */
	startframe = strtol(argv[3],NULL,0);
	if(filetype == IMG_ICS){
		/* Open file */
		icserr = IcsOpen(&ics, argv[1], "r");
		ICS_CHECK(icserr);

#ifdef DEBUG_PRINT
		printf("ics file loaded.\n");
#endif

		/* Sniff out file; make sure it is compatible with our prog */
		icserr = IcsGetLayout(ics, &dt, &ndims, dims);
		ICS_CHECK(icserr);
		if(dt != Ics_uint16){
			printf("Sorry, image modes other than 16-bit (unsigned) ");
			printf("not supported at the moment in ICS backend.\n");
			return 0;
		}
	}
	if(filetype == IMG_PGM){
		sprintf(curname, argv[1], startframe);
		if(peek_pgm(&(dims[1]),&(dims[2]),&maxval,curname)){
			printf("Error: couldn't open file %s\n", curname);
			return 6;
		}
		dims[0]=1;
		ndims=3;
	}
	
	
	/* number of frames specified */
	/* we already got startframe above */
	if(filetype == IMG_ICS){
		if(startframe > dims[3]) startframe = dims[3];
	}
	if(startframe < 0){
		printf("Start frame number must be non-negative.\n");
		return 1;
	}
	endframe = strtol(argv[4],NULL,0);
	if(endframe < startframe){
		printf("End frame number must be greater than start.\n");
		return 2;
	}
	if(filetype == IMG_ICS){
		if(endframe > dims[3]) endframe = dims[3];
	}
	
#ifdef DEBUG_PRINT
    printf("dimensions: ");
    for(i=0;i<ndims;i++)
        printf("%ld ", dims[i]);
    if(filetype == IMG_ICS) printf("data type: %d", dt);
    printf("\n");
#endif

    /* TODO: warning if dimensions look non-standard */
   
	/* open svg file and extract outline, and prepare it */
	if(get_path_from_svg(argv[2], "cell", s.path[0].p, &(s.path[0].n), MAX_PATHN)){
		printf("error: couldn't load svg file %s\n", argv[2]);
		return 21;
	}
	/* HACK: remove initial repeated point */
	s.path[0].n--;
	
	calc_phase(s.path[0].phase,s.path[0].p,s.path[0].n);
	s.path[0].n = path_subdivide2(s.path[0].p, s.path[0].phase, SEG_LENGTH, s.path[0].n, MAX_PATHN, 1);
	s.npath = 1;
	
	/* get energy penalty */
	kappa = atof(argv[5]);
	
    /* allocate space before reading */
    /* Only allocate for one frame; allows images with many frames
     * to be loaded without much memory, unlike some programs
     * (I'm looking at you, ics_opener/ImageJ) ! */
    prev = (pix_type *)malloc(dims[1]*dims[2]*sizeof(pix_type));
    next = (pix_type *)malloc(dims[1]*dims[2]*sizeof(pix_type));
    sobx = (float   *)   malloc(dims[1]*dims[2]*sizeof(float));
    soby = (float   *)   malloc(dims[1]*dims[2]*sizeof(float));
    
    tmp1 = (float *)   malloc(dims[1]*dims[2]*sizeof(float));
    
    buf  = (unsigned char *)malloc(dims[1]*dims[2]*sizeof(unsigned char));
    
    if((!next)||(!prev)||(!buf)||(!sobx)||(!soby)||(!tmp1))
    {
        printf("Couldn't allocate enough memory.\n");
        return 1;
    }
	
	printf("tracking...\n");
	
    /* read sequentially from file. */
    /* Each call to IcsGetDataBlock proceeds from the end of the last */
    /* For ICS files, we have to start sequentially from beginning.
     * For directory of PGM files, we can just start directly from
     * the specified image. */
    trackstart=0;
    if(filetype==IMG_PGM) trackstart = startframe;
    for(i=trackstart;i<=endframe;i++){
		pix_type *swap;
		swap=next; next=prev; prev=swap;

		if(filetype == IMG_ICS){
			icserr = IcsGetDataBlock(ics, next, dims[1]*dims[2]*2);
			ICS_CHECK(icserr);
			maxval=4096;
		}
		if(filetype == IMG_PGM){
			sprintf(curname, argv[1], i);
			if(import_pgm_16bit(next, &w, &h, &maxval, curname)){
				printf("Error: during loading of %s.\n",curname);
				return 9;
			}
			if((w!=dims[1])||(h!=dims[2])){
				printf("Error: file %s ", curname);
				printf("has dimensions %zux%zu, ", w, h);
				printf("which doesn't match dimensions of ");
				printf("previous images (%zux%zu)\n",dims[1],dims[2]);
				return 8;
			}
		}
		
		if(i==trackstart) continue;	/* obtain at least 2 frames */
		if(i <= startframe) continue;
		
		/* we are at the desired frame sequence. */
		printf("frame %i->%i\n", i-1,i);

		conv_pix_type_to_float(next,tmp1,maxval,dims[1]*dims[2]);
		
		/* the meat of the problem */
		//export_path_svg_test("/dev/shm/svg/intrmd0.svg",s.path[0].p,s.path[0].n);
		err=fit_path(tmp1,dims[1],dims[2],sobx,soby,sobhist,&s,MAX_SEARCH_RAD,NORMAL_DRIFT,SEG_LENGTH,16,kappa);
		//export_path_svg_test("/dev/shm/svg/intrmd1.svg",s.path[0].p,s.path[0].n);
		err=fit_path(tmp1,dims[1],dims[2],sobx,soby,sobhist,&s,MAX_SEARCH_RAD/2,NORMAL_DRIFT,SEG_LENGTH/2,16,0.5*kappa);
		//export_path_svg_test("/dev/shm/svg/intrmd2.svg",s.path[0].p,s.path[0].n);
		err=fit_path(tmp1,dims[1],dims[2],sobx,soby,sobhist,&s,MAX_SEARCH_RAD/4,NORMAL_DRIFT,SEG_LENGTH/4,16,0.25*kappa);
		
		if(err) return err;
		
		/* export data */
		dump_int_file(s.path[s.npath-1].extr, s.path[s.npath-1].n, "out/extr.dat", "a");
		
		if(doac){
			his = &(s.path[s.npath-1]);
			calc_dphase(dx, his->phase, his->n);
			if(extr_avg(avgextr,&s)){ printf("number of points no agree. Aborting autocorrelation calculations.\n"); doac=0; }
			pclin_export_crosscorr_int("out/autocorr.dat", 0.004, dx, avgextr, his->n, dx, avgextr, his->n);
		}
	
		/* export out pretty pictures */
		conv_pix_type_to_char(next, buf, dims[1]*dims[2],maxval);
		int mn;
		mn = export_local_max(mstart, mend, MAX_PATHN, merr, s.path[0].p, s.tdot, s.norml, s.path[0].n, MAX_SEARCH_RAD, 1);
		draw_motion(mstart, mend, mn, buf, NULL, merr, 1.0, 1, dims[1], dims[2]);
		export_8bit_pgm(buf,dims[1],dims[2],"/tmp/merr.pgm");
		
		conv_pix_type_to_char(next, buf, dims[1]*dims[2],maxval);
		//for(k=0;k<dims[1]*dims[2];k++) buf[k]=0;
		paint_nrm(buf,dims[1],dims[2],s.path[1].p,s.path[1].n,s.norml,s.bminus,s.bplus,NULL,MAX_SEARCH_RAD);
		export_8bit_pgm(buf,dims[1],dims[2],"/tmp/norml.pgm");

		printf(" exporting\n");
		export_spath(&s,next,buf,maxval,dims[1],dims[2],argv[6],i);
		
#if 0
		//sprintf(cmdlin,"pnmtojpeg --quality=80 --progressive < movec.pgm > movec_%04d.jpg\n", i);
		//sprintf(cmdlin,"pnmtopng -compression 9 < movec.pgm > movec_%04d.png\n", i);
		sprintf(cmdlin,"mv movec.pgm movec/%04d.pgm\n", i);
		if(system(cmdlin)){
			printf("error during pnmtojpeg invoke\n");
			return 55;
		}
#endif
    }
	
	/* TODO: check if we have even made anything yet */
	
	/* compare final outline with user-defined one */
	sprintf(cmdlin,"%04d.svg",endframe);
	if(get_path_from_svg(cmdlin, "cell", refpath, &nref, MAX_PATHN)){
		printf("error: couldn't load svg file %s\n", cmdlin);
		printf("Aborting path matching.\n");
	} else {
		dump_stats_file("out/match.dat",kappa,startframe,endframe,find_path_distance(refpath,nref,s.path[0].p,s.path[0].n,1));
	}
	
    /* no need to free memory; that's just silly */
	if(filetype==IMG_ICS) IcsClose(ics);
	
    return 0;
}
