/* COMPILE: textutil.c compressimg.c beamdynamic.c beam.c line_intrsct.c pclin.c fit_path2.c pgm_util.c matrix.c snake.c filters.c path.c svg2.c vis_motion.c -Wall -I/usr/include/libxml2 -lm -lics -lavl -lxml2 -O3 -DDEBUG_MODE
 */

/* Robust Object Inference Environment */
/* Track cell outlines and nuclei */
/* created by forking mo (my old system) */

/* August 10: fixed stupid bug where I was calling precalc_normals with
 * p->pathn, not pathn */
 
/* TODO: getpathfromsvg repeats initial point! */

/* Oct 17: added pgm capability to mo-ics and renamed it mo, so mo-pgm
 * is obsolete. It and mo-ics are bundled together in mo-ics-pgm and
 * archived */

#define SEG_LENGTH 8.0		/* 10 for series 7-90, 6 for old-style behavior */
#define NORMAL_DRIFT 1.0

/* TODO */
#define PRE_BLUR 1.0

#define INPUT_MAX 65536

#define pix_type uint16_t

#define TEXT_OUT_FMT "sup-k%5.4f_%04d.txt"
#define  SVG_OUT_FMT "sup-k%5.4f_%04d"

#include <stdint.h>		/* pix_type, int32_t */
#include <stdlib.h>		/* strtol(), rand() */
#include <time.h>		/* time() (for seed) */
#include <string.h>		/* memcpy() */
#include <ctype.h>		/* tolower() */
#include <malloc.h>		/* malloc() */
#include <math.h>		/* sqrt() */
#include <stdio.h>
//#include <sys/types.h>
//#include <argp.h>			/* argp */
#include <libics.h>		/* Lib ICS */
#include <avl.h>		/* lib AVL */

#include "point2d.h"	/* point2d32f */
#include "histnorm.h"	/* edr(), normalize() */
//#include "vis_motion.h" /* draw_motion() */
#include "svg2.h"		/* get_path_from_svg */
#include "path.h"		/* all */
#include "prec.h"
#include "snake.h"
#include "fit_path.h"	/* fit_path() */
#include "pgm_util.h"
#include "pclin.h"		/* pclin_export_crosscorr_int() */
#include "compressimg.h"
#include "textutil.h"	/* for loading phases */

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

/* imagename is the name of the embedded image. If it is NULL, an image
 * is not embedded */
void export_path_svg_start(FILE *f, int w, int h, char *imagename)
{
	char xmlhead[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>";
	
	fprintf(f,"%s\n",xmlhead);
	fprintf(f,"<svg id=\"svg3119\" ");
	fprintf(f,"xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\" ");
	fprintf(f,"xmlns=\"http://www.w3.org/2000/svg\" ");
	fprintf(f,"height=\"%d\" width=\"%d\" ", h, w);
	fprintf(f,"version=\"1.1\" ");
	fprintf(f,"xmlns:cc=\"http://creativecommons.org/ns#\" ");
	fprintf(f,"xmlns:dc=\"http://purl.org/dc/elements/1.1/\" ");
	fprintf(f,"xmlns:xlink=\"http://www.w3.org/1999/xlink\" ");
	fprintf(f,">\n");
	
	/* link image */
	/* this has to come first otherwise it will be on top of curve and
	 * curve can't be seen */
	if(imagename){
		fprintf(f,"<image ");
		fprintf(f,"x=\"0\" y=\"0\" height=\"%d\" width=\"%d\" ", h, w);
		fprintf(f,"xlink:href=\"%s\" ", imagename);
		fprintf(f,"/>\n");
	}
}

/* write curve data for a path */
void export_path_svg(FILE *f, char *id, point2d32f *path, int n, char *pathstyle)
{
	int i;
	
	fprintf(f,"<path id=\"%s\" d=\"m %f,%f ", id, path[0].x, path[0].y);
	for(i=1;i<n;i++) fprintf(f,"%f,%f ", path[i].x - path[i-1].x, path[i].y - path[i-1].y);
	fprintf(f,"z\" ");
	
	/* tell it how to render it */
	fprintf(f,"style=\"%s\" />", pathstyle);
}

void export_path_svg_end(FILE *f)
{
	fprintf(f,"</svg>\n");
}

/* export a cell outline structure in different ways; such as
 * bitmap superimposed on input image, and vector data */
void export_spaths(spath *s, int nspaths, unsigned char *buf, int w, int h, char *outdir, int idx, int export_jpeg)
{
	int i,j;
	FILE *f;
	char basename[256];
	char jpegname[256];
	char jpegpath[256];
	char xsvgpath[256];
	
	char pathstyle[2048];
	
	/* load path styles from file */
	f = fopen("robie.conf","r");
	if(!f){
		/* if file not found, use default style */
		strcpy(pathstyle,"stroke:#FF0000;stroke-miterlimit:4;stroke-dasharray:none;stroke-width:3.0;fill:none");
	} else {
		if((i=fseek(f,0,SEEK_END))>2048){
			fprintf(stderr,"ERROR: style string too long.");
			return;
		}
		fseek(f,0,SEEK_SET);
		fread(pathstyle,1,i,f);
		pathstyle[i+1]=0;
	}
	
	for(j=0;j<s->npath;j++){
		sprintf(basename,SVG_OUT_FMT,s->path[j].opts.kappa,idx);
		if(export_jpeg) {
			sprintf(jpegname,"%s.jpg",basename);
			sprintf(jpegpath,"%s/%s.jpg",outdir,basename);
		}
		
		sprintf(xsvgpath,"%s/%s.svg",outdir,basename);

#if 0
		if(nspaths > 1){
			fprintf(stderr,"ERROR: export_spaths: support for multiple paths not added yet.\n");
			
			for(i=0;i<nspaths;i++){
				/*draw_motion((struct point2d32f *) s[i].path[j].p,
							(struct point2d32f *)(s[i].path[j].p+1),
							s[i].path[j].n-1,buf,NULL,s[i].path[j].perr,0.1,0,w,h);*/
			}
		
			return;
		}
#endif
		if(export_jpeg) export_to_compressed(jpegpath, buf, w, h);
		
		f = fopen(xsvgpath,"w");
		if(!f){
			fprintf(stderr,"ERROR: couldn't open output file %s\n", xsvgpath);
			return;
		}
		
		/* start exporting svg; export style types and background image */
		if(export_jpeg){
			export_path_svg_start(f, w, h, jpegname);
		} else {
			export_path_svg_start(f, w, h, NULL);
		}
		
		/* export individual paths */
		for(i=0;i<nspaths;i++){
			export_path_svg(f, s[i].id, s[i].path[j].p, s[i].path[j].n, pathstyle);
		}
		
		/* end */
		export_path_svg_end(f);
		fclose(f);
	}
}

/* the text format is as follows:
 * number-of-cells
 * <list of cells>
 * 
 * Where each cell is:
 * cell-name
 * number-of-outlines
 * <list of outlines>
 * 
 * And each outline is:
 * number-of-vertices
 * vertex1.x vertex1.y
 * vertex2.x vertex2.y
 * ...
 */
void export_spaths_text(spath *s, int nspaths, char *outdir, float kappa, int idx)
{
	int i,j,k;
	
	point2d32f *p;
	float *ph;
	char textname[256];
	char textpath[256];
	FILE *f;
	
	/* open file */
	sprintf(textname,TEXT_OUT_FMT,kappa,idx);
	sprintf(textpath,"%s/%s",outdir,textname);
	
	f = fopen(textpath,"w");
	if(!f){
		fprintf(stderr,"ERROR: couldn't write to output file.\n");
		return;
	}
	
	/* magic text */
	fprintf(f,"robie\n");
	
	/* list of cells */
	fprintf(f,"%d\n",nspaths);
	for(i=0;i<nspaths;i++){
		
		/* list of outlines for each cell */
		fprintf(f,"%s\n",s[i].id);
		fprintf(f,"%d\n",s[i].npath);
		for(j=0;j<s[i].npath;j++){
			
			/* individual outline */
			p  = s[i].path[j].p;
			ph = s[i].path[j].phase;
			fprintf(f,"%d\n",s[i].path[j].n);
			for(k=0;k<s[i].path[j].n;k++){
				fprintf(f,"%f %f %f\n",p[k].x,p[k].y,ph[k]);
			}
		}
	}
	
	fclose(f);
}


/* returns pointer to array of spaths */
#define MAX_SPATH 1024
spath *import_paths_from_svg(char *filename, int *pnpaths)
{
	int npaths;
	int i;
	char *id[MAX_SPATH];
	point2d32f *p[MAX_SPATH];
	int *n[MAX_SPATH];
	spath *this;
	spath *s;
	
	/* first, get number of paths we'll need */
	npaths = get_paths_from_svg(filename, NULL, NULL, NULL, 0);
	if(npaths < 0){
		printf("Error: couldn't load paths from SVG file %s (error %d)\n", filename, npaths);
		exit(11);
	}

	/* now, allocate memory and load the paths */
	s = (spath *)malloc(npaths * sizeof(spath));
	if(!s){
		printf("Error: couldn't allocate required memory for paths.\n");
		exit(12);
	}

	/* make the references correct */
	for(i=0;i<npaths;i++){
		this = s + i;
		n[i] = &(this->path[0].n);
		p[i] = this->path[0].p;
	}

	npaths = get_paths_from_svg(filename, id, p, n, MAX_PATHN);

	/* this has to be treated separately because it's malloc'd */
	for(i=0;i<npaths;i++){
		this = s + i;
		this->id = id[i];
	}

#ifdef DEBUG_MODE
	printf("SVG file %s loaded; %d path(s) found.\n",filename,npaths);
	printf("path id: %s n:%d\n", s[0].id, s[0].path[0].n);
	/*for(i=0;i<s[0].path[0].n;i++) printf("%f,%f ",s[0].path[0].p[i].x,s[0].path[0].p[0].y);
	printf("\n"); */
#endif

	*pnpaths = npaths;
	return s;
}

/* prepare path by calculating phases and subdividing */
void prepare_path(spath *ss, int nspath)
{
	int i;
	spath *ts;
	
	for(i=0;i<nspath;i++){
		ts = ss + i;
		calc_phase(ts->path[0].phase,ts->path[0].p,ts->path[0].n);
		ts->path[0].n = path_subdivide2(ts->path[0].p, ts->path[0].phase, SEG_LENGTH, ts->path[0].n, MAX_PATHN, 1);
		ts->npath = 1;
	}	
}

/* like above, but gets phases from file */
void prepare_path_file(spath *ss, int nspath, char *fil)
{
	int i,j;
	spath *ts;
	FILE *f;
	cell_t *c;
	int nc;
	
	f = fopen(fil,"r");
	if(!f){
		printf("Error: couldn't open %s",fil);
		exit(67);
	}
	
	nc=loadcells(f, &c);
	if(nc<0){
		printf("Error: couldn't load phases from file.\n");
		exit(68);
	}
	fclose(f);
	
	
	for(i=0;i<nspath;i++){
		ts = ss + i;
		
		/* find phase in file */
		/* search by id of cell */
		for(j=0;j<nc;j++){
			if(strcmp(c[j].id,ts->id)==0){
				if(c[j].npaths!=1){
					printf("Error: input cells must have only one path!\n");
					exit(69);
				}
				if(c[j].n[0]!=ts->path[0].n){
					printf("Error: number of points in svg file and phase file don't agree.\n");
					printf("\tid:%s svg:%d txt:%d\n",c[j].id,ts->path[0].n,c[j].n[0]);
					exit(70);
				}
				break;
			}
		}
		
		memcpy(ts->path[0].phase,c[j].phase[0],ts->path[0].n*sizeof(float));
		ts->path[0].n = path_subdivide2(ts->path[0].p, ts->path[0].phase, SEG_LENGTH, ts->path[0].n, MAX_PATHN, 1);
		ts->npath = 1;
	}	
	
	
}

int main(int argc, char **argv)
{

    Ics_Error icserr;
    ICS* ics;
    size_t dims[ICS_MAXDIM];
    int ndims;
    int i,j;
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
	
	spath *ss;	/* set of paths */
	spath *ts;
	int nspath;
	
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
	
	int export_jpeg;
	int export_text;
	int export_svg;
	
	int low_conf_rm;
	
    if(argc < 7){
        printf("usage: robie <filename.ics | filename<format>.pgm> <outline.svg> startframe endframe kappa <output-dir> <phase.txt> [-t] [low_conf_rm]\n");
        printf("\tIf filename is .pgm, we assume it's a series of pgm files, and the format string must be a printf-type string with a %%[]d inside it.\n");
        printf("phase.txt is a file containing phases in robie format. It can be 'none'\n");
        printf("If the -t option is given, only outputs outlines as plain text files.\n");
        printf("If the -b option is given, outputs both svg and plain text.\n");
        printf("If the -s option is given, outputs svg with no jpeg.\n");
        printf("Path style for svg is read from file robie.conf in current dir.\n");
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
	startframe = strtol(argv[3],NULL,10);
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
	endframe = strtol(argv[4],NULL,10);
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

	/* extract svg outlines from file */
	ss = import_paths_from_svg(argv[2],&nspath);
	
	/* get energy penalty */
	kappa = atof(argv[5]);
	
	/* get phases */
	if((argc>=8)&&(strcmp(argv[7],"none")!=0)){
		prepare_path_file(ss,nspath,argv[7]);
	} else {
		prepare_path(ss,nspath);
	}
	
	/* get whether we should output svg files with embedded .jpeg images
	 * or just plain text files with the outlines */
	export_jpeg = 1;
	export_svg  = 1;
	export_text = 0;
	if(argc>=9){
		argv[8][3]=0;	/* avoid buffer overruns */
		if(strcmp("-t",argv[8])==0){
			export_jpeg = 0;
			export_svg  = 0;
			export_text = 1;
		}
		if(strcmp("-b",argv[8])==0){
			export_jpeg = 1;
			export_svg  = 1;
			export_text = 1;
		}
		if(strcmp("-s",argv[8])==0){
			export_jpeg = 0;
			export_svg  = 1;
			export_text = 1;
		}
	}
	
	/* get whether low-confidence line segments should be removed */
	low_conf_rm = 0;
	if(argc==10){
		low_conf_rm = strcmp("yes",argv[8]) == 0 ? 1 : 0;
	}
	
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
		
		//if(i==trackstart) continue;	/* obtain at least 2 frames */
		if(i < startframe) continue;
		
		if(i > startframe){	/* we have accumulated at least 2 frames and are ready for tracking */
			/* we are at the desired frame sequence. */
			printf("frame %i->%i\n", i-1,i);
			
			conv_pix_type_to_float(next,tmp1,maxval,dims[1]*dims[2]);
			
			/* the meat of the problem */
			for(j=0;j<nspath;j++){
				ts=ss+j;
				err=fit_path(tmp1,dims[1],dims[2],sobx,soby,sobhist,ts,MAX_SEARCH_RAD,NORMAL_DRIFT,SEG_LENGTH,16,kappa,low_conf_rm);
				
				//err=fit_path(tmp1,dims[1],dims[2],sobx,soby,sobhist,ts,MAX_SEARCH_RAD,NORMAL_DRIFT,SEG_LENGTH,16,kappa);
				
				//err=fit_path(tmp1,dims[1],dims[2],sobx,soby,sobhist,ts,MAX_SEARCH_RAD,NORMAL_DRIFT,SEG_LENGTH,16,kappa);
							
				//err=fit_path(tmp1,dims[1],dims[2],sobx,soby,sobhist,ts,MAX_SEARCH_RAD,NORMAL_DRIFT,SEG_LENGTH,16,kappa);
				
				
				
				err=fit_path(tmp1,dims[1],dims[2],sobx,soby,sobhist,ts,MAX_SEARCH_RAD/2,NORMAL_DRIFT,SEG_LENGTH/2,16,0.5*kappa,low_conf_rm);
				
				//err=fit_path(tmp1,dims[1],dims[2],sobx,soby,sobhist,ts,MAX_SEARCH_RAD/2,NORMAL_DRIFT,SEG_LENGTH/2,16,0.5*kappa);
				
				
				
				err=fit_path(tmp1,dims[1],dims[2],sobx,soby,sobhist,ts,MAX_SEARCH_RAD/4,NORMAL_DRIFT,SEG_LENGTH/4,16,0.25*kappa,low_conf_rm);
				//err=fit_path(tmp1,dims[1],dims[2],sobx,soby,sobhist,ts,MAX_SEARCH_RAD/4,NORMAL_DRIFT,SEG_LENGTH/4,16,0.25*kappa);
				//err=fit_path(tmp1,dims[1],dims[2],sobx,soby,sobhist,ts,MAX_SEARCH_RAD/4,NORMAL_DRIFT,SEG_LENGTH/4,16,0.25*kappa);
				
				if(err) return err;
			}
		
			/* export data */
			//dump_int_file(s.path[s.npath-1].extr, s.path[s.npath-1].n, "out/extr.dat", "a");

#if 0
			if(doac){
				his = &(s.path[s.npath-1]);
				calc_dphase(dx, his->phase, his->n);
				if(extr_avg(avgextr,&s)){
					printf("number of points no agree. Aborting autocorrelation calculations.\n"); 
					doac=0;
				}
				pclin_export_crosscorr_int
				  ("out/autocorr.dat", 0.004, dx, avgextr, his->n, dx, avgextr, his->n);
			}
#endif
		}	/* i > startframe */
		
		/* even if we can't do the analysis we can still output the data
		 * in a format consistent with the rest */
		if(export_svg){
			printf(" exporting\n");
			if(export_jpeg) conv_pix_type_to_char(next, buf, dims[1]*dims[2], maxval);
			export_spaths(ss,nspath,buf,dims[1],dims[2],argv[6],i,export_jpeg);
		}
		if(export_text){
			export_spaths_text(ss,nspath,argv[6],kappa,i);
		}

		/* export centroids */
		for(j=0;j<nspath;j++){
			FILE *centrf;
			char centrfname[32];
			int k;
			float centrx,centry;
			
			sprintf(centrfname,"%s.centr.dat",ss[j].id);
			centrf = fopen(centrfname,"a");
			
			centrx=0; centry=0;
			for(k=0;k<ss[j].path[0].n;k++){
				centrx += ss[j].path[0].p[k].x;
				centry += ss[j].path[0].p[k].y;
			}
			centrx /= (float)ss[j].path[0].n;
			centry /= (float)ss[j].path[0].n;
			
			fprintf(centrf,"%d %f %f\n", i, centrx, centry);
			fclose(centrf);
		}
		
#if 0	
		/* export errors */
		for(j=0;j<nspath;j++){
			FILE *errf;
			char errfname[32];
			int k;
			
			sprintf(errfname,"%s.err.dat",ss[j].id);
			errf = fopen(errfname,"a");
			fprintf(errf,"%d: ", i);
			for(k=0;k<ss[j].path[0].n;k++){
				fprintf(errf,"%f ",ss[j].path[0].perr[k]);
			}
			fprintf(errf,"\n");
			fclose(errf);
		}
#endif
    }
	
	/* TODO: check if we have even made anything yet */

#if 0
	/* compare final outline with user-defined one */
	sprintf(cmdlin,"%04d.svg",endframe);
	if(get_path_from_svg(cmdlin, "cell", refpath, &nref, MAX_PATHN)){
		printf("error: couldn't load svg file %s\n", cmdlin);
		printf("Aborting path matching.\n");
	} else {
		dump_stats_file("out/match.dat",kappa,startframe,endframe,find_path_distance(refpath,nref,s.path[0].p,s.path[0].n,1));
	}
#endif

    /* no need to free memory; that's just silly */
	if(filetype==IMG_ICS) IcsClose(ics);
	
    return 0;
}
