/* COMPILE: histnorm.c -lm -lics -O2
 */

/* extract2: Version 2.0 of my extract program, with customizable EVERYTHING */
/* by Alireza Nejati, July 9th 2011 */
/* August 26th: added raw output support, and fixed bugs (byte order) */

#define pix_type uint16_t

#include <stdint.h>	/* uint16_t */
#include <stdlib.h>	/* exit() */
#include <string.h>	/* memset() */
#include <stdio.h>
#include <argp.h>
#include <netinet/in.h>	/* ntohs(), htons() */
#include "libics.h"        /* Lib ICS */
#include "histnorm.h"		/* histogram normalization primitives */


#define ICS_CHECK(c) if(c != IcsErr_Ok){printf("Ics_Error = %d\n", c); return c;}

/* just the .ics file */
#define NUM_ARGS 1

const char *argp_program_version = "extract 2.0";
const char *argp_program_bug_address =
"<come@me.bro>";

/* Program documentation. */
static char doc[] =
"extract2 - extract images from .ics files.\nby Alireza Nejati (c) 2011\n";

/* A description of the arguments we accept. */
static char args_doc[] = "<filename.ics>";

/* The options we understand. */
static struct argp_option options[] = {
{"frames",  'f', "start:end", OPTION_ARG_OPTIONAL ,  "Output specified frames. It is OK to leave end blank, will just run to end of data. If neither are given, will output all frames. Example: -f 10:" },
{"raw",    'r', 0,      0,  "Do not downconvert to 8 bpp. Note that downconversion to 8 bpp is necessary for most image analysis software, hence it is the default behavior." },
{"output",   'o', "FILE", 0, "Output filenames as name_0001, name_0002, etc." },
{"png", 'p', "level", OPTION_ARG_OPTIONAL, "compress as PNG with compression [level]. Invokes pnmtopng. Default=1"},
{"jpeg", 'j', "quality", OPTION_ARG_OPTIONAL, "compress as JPEG with specified quality, as a percentage. Invokes pnmtopng and thus uses it's default behavior if not given. Default=75"},
{"normalize", 'e', "cutoff", OPTION_ARG_OPTIONAL, "histogram-normalize images (has no effect if -r is passed, or input is 8bpp already). Cutoff can be from 0 (none) to 255 (full). Default:16"},
{"gradient", 'd', "radius", OPTION_ARG_OPTIONAL, "Perform low-frequency suppression. Can enhance the effect of histogram normalization. Default radius=64"},
{"filter", 'w', 0, 0, "Perform temporal noise-reduction on image series."},
{"abrupt",'a',"level",OPTION_ARG_OPTIONAL,"Prevent histogram-normalization from changing too much between frames. The higher level is, the faster image contrast changes. Useful for algorithms that are sensitive to image brightness. With level=0, histogram from first frame is used only. Default value is 0.1 ."},
{"mb",'m',"nbins",OPTION_ARG_OPTIONAL,"Perform mode-based background extraction. Useful for removing fixed debris, scratches, or out-of-focus blurs. When used, a single frame (the background) is output along with everything else. This can then be used for subtraction. nbins represents number of bins. Default: 64"},
{"simulate",'s',0,0,"Simulate. Do not output any frames, but do everything else (useful in conjuction with -m option)."},
{ 0 }
};

/* Used by main to communicate with parse_opt. */
struct arguments
{
	char *filename[1];                /* filename.ics */
	int start,end;
	int raw;
	int compression;	/* 0: none 1: png 2: jpeg */
	int pnglevel,jpegquality;
	int histnorm;
	int histcutoff;
	int gradual;
	float abrupt;
	int filter;
	char *output_file;
	int modebg;
	int simulate;
};

/* Parse a single option. */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
	/* Get the input argument from argp_parse, which we
	know is a pointer to our arguments structure. */
	struct arguments *arguments = state->input;

	switch (key)
	{
	case 'f':
		sscanf(arg,"%d:%d",&(arguments->start),&(arguments->end));
		if(arguments->start<0){
			fprintf(stderr,"Error: start frame must be nonnegative.\n");
			exit(5);
		}
		break;
		
	case 'r':
		arguments->raw=1;
		break;
		
	case 'o':
		arguments->output_file = arg;
		break;

	case 'p':
		if(arguments->compression){
			fprintf(stderr,"Error: can only specify one type of compression.\n");
			exit(1);
		}
		arguments->compression=1;
		arguments->pnglevel = arg ? atoi(arg) : 1;
		printf("png compression level: %d\n", arguments->pnglevel);
		break;
		
	case 'j':
		if(arguments->compression){
			fprintf(stderr,"Error: can only specify one type of compression.\n");
			exit(2);
		}
		arguments->compression=2;
		arguments->jpegquality = arg ? atoi(arg) : 75;
		if((arg[0]<'0')||(arg[0]>'9')){
			fprintf(stderr,"Warning: %s not valid for quality level; check that option was entered correctly.\n",arg);
			arguments->jpegquality = 75;
		}
		break;
	
	case 'e':
		arguments->histnorm=1;
		arguments->histcutoff = arg ? atoi(arg) : 16;
		if(arguments->histcutoff < 0){
			fprintf(stderr,"Error: negative cutoff invalid.\n");
			exit(3);
		}
		break;
	
	case 'a':
		arguments->gradual = 1;
		arguments->abrupt = arg ? atof(arg) : 0.1;
		printf("abrupt: %f\n", arguments->abrupt);
		if(arguments->abrupt<0){
			fprintf(stderr,"Error: 'abruptness' value must be positive.\n");
			exit(4);
		}
		break;
	
	case 'm':
		arguments->modebg = arg ? atoi(arg) : 256;
		break;
	
	case 's':
		arguments->simulate = 1;
		break;
		
	case ARGP_KEY_ARG:
		if (state->arg_num >= NUM_ARGS)
		/* Too many arguments. */
		argp_usage (state);

		arguments->filename[state->arg_num] = arg;

		break;

	case ARGP_KEY_END:
		if (state->arg_num < NUM_ARGS)
		/* Not enough arguments. */
		argp_usage (state);
		break;

	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, doc };

void export_pgm(void *buf, int width, int height, pix_type maxval, int Bpc, char *filename)
{
    FILE *f;
    if(!(f = fopen(filename, "w"))){
		printf("error: couldn't open %s for output\n", filename);
		return;
	}
    fprintf(f,"P5\n%d %d\n%d\n", width, height, maxval);
    if(fwrite(buf,Bpc,width*height,f)!=width*height){
		fprintf(stderr,"Couldn't write to file %s\n",filename);
		exit(9);
	}
    fclose(f);
}

void add_to_histogram(int *thist, pix_type *img, int w, int h, int nbins)
{
	unsigned int i;
	unsigned int c=0;
	
	for(i=0;i<w*h;i++) {thist[c + (img[i]>>4)]++; c+=nbins;}
}

void calc_mode(pix_type *mode, int *thist, int w, int h, int nbins)
{
	int i,max;
	pix_type j,jmax;
	int *thistrow;
	
	for(i=0;i<w*h;i++) {
		thistrow = thist + i*nbins;
		
		max =thistrow[0];
		jmax=0;
		
		for(j=1;j<nbins;j++){
			if(thistrow[j] > max){max=thistrow[j]; jmax=j;}
		}
		
		mode[i] = 16*jmax;

	}


}

int main (int argc, char **argv)
{
	pix_type *img;
	unsigned char *buf;
	float emin,emax;
	int curemin,curemax;
	struct arguments arguments;
	size_t dims[ICS_MAXDIM];
    int ndims;
    Ics_Error icserr;
    ICS* ics;
    Ics_DataType dt;
    char curname[1024];
    char cmdline[1024];
    int i,j,size;
    pix_type maxval;
    int *thist;		/* temporal histogram for mode-based bg */
    
	/* Default values. */
	memset(&arguments,0,sizeof(struct arguments));
	arguments.output_file = "frame";
	arguments.end = -1;
	
	/* Parse our arguments. */
	argp_parse(&argp, argc, argv, 0, 0, &arguments);
	
	/* give some warnings about conflicting arguments */
	if((!arguments.histnorm)&&(arguments.gradual)) fprintf(stderr,"Warning: abruptness value has no effect with histogram normalization turned off.\n");
	
	/* Open file */
    icserr = IcsOpen(&ics,arguments.filename[0],"r");
    ICS_CHECK(icserr);

	/* Sniff out file; make sure it is compatible with our prog */
    icserr = IcsGetLayout(ics, &dt, &ndims, dims);
    ICS_CHECK(icserr);
    if(dt != Ics_uint16){
        printf("Sorry, image modes other than 16-bit (unsigned) ");
        printf("not supported at the moment.\n");
        return 0;
    }
    
    /* '-1' means go to end */
    if(arguments.end==-1) arguments.end=dims[3];
    
    /* allocate memories */
    img = malloc(dims[1]*dims[2]*sizeof(pix_type));
    buf = malloc(dims[1]*dims[2]*sizeof(unsigned char));
    if((!img)||(!buf)){
		fprintf(stderr,"Error: couldn't allocate enough memory.\n");
		return 6;
	}
	if(arguments.modebg){
		thist = calloc(dims[1]*dims[2]*arguments.modebg,sizeof(int));
		if(!thist){
			fprintf(stderr,"Error: not enough memory for bins.\n");
			return 13;
		}
	}
	
    printf("tracking...\n");
    
    for(i=0;i<=arguments.end;i++){
		icserr = IcsGetDataBlock(ics, img, dims[1]*dims[2]*sizeof(pix_type));
		if(icserr==13) break;	/* End-Of-File */
        ICS_CHECK(icserr);
		
		/* for each pixel, create a temporal histogram of the values
		 * it takes. Put these values into bins. After all frames
		 * have been processed, we take the mode. */
		if(arguments.modebg){
			add_to_histogram(thist,img,dims[1],dims[2],arguments.modebg);
		}
			
        if(i>=arguments.start){
			printf("frame %d\n", i);
			
			/* Raw output. No processing. */
			if(arguments.raw){
				sprintf(curname,"%s_%04d.pgm", arguments.output_file,i);
				size = dims[1]*dims[2];
				maxval=img[0];	/* Obtain maxval for pgm */
				for(j=0;j<size;j++){
					if(img[j] > maxval) maxval = img[j];
					img[j] = htons(img[j]);	/* change byte order */
				}
				if(!arguments.simulate){
					export_pgm(img,dims[1],dims[2],maxval,sizeof(pix_type),curname);
				}
				continue;
			}
			
			/* TODO: temporal noise reduction */
			
			/* TODO: low-stop filtering */
			
			/* do normalization, if specified at command line */
			if(arguments.histnorm){
				edrc(img, dims[1], dims[2], &curemin, &curemax, arguments.histcutoff);
				if(arguments.gradual){
					emin = (emin+((float)(curemin))*arguments.abrupt)/(arguments.abrupt+1.0);
					emax = (emax+((float)(curemax))*arguments.abrupt)/(arguments.abrupt+1.0);
				}
				if((i==arguments.start)||(!arguments.gradual)){
					emin=(float)curemin;
					emax=(float)curemax;
				}
				normalize(img, buf, dims[1], dims[2], (int)emin, (int)emax);
			} else {
				for(j=0;j<dims[1]*dims[2];j++) buf[j]=(img[j] >> 4);
			}
			
			/* output */
			if(arguments.simulate) continue;
			
			if(!arguments.compression){
				sprintf(curname, "%s_%04d.pgm", arguments.output_file, i);
			} else {
				strcpy(curname,"tmp.pgm");
			}
			export_pgm(buf,dims[1],dims[2],255,1,curname);
			
			if(arguments.compression==1){
				sprintf(cmdline, "pnmtopng -compression %d < tmp.pgm > %s_%04d.png\n",
					arguments.pnglevel,
					arguments.output_file, i);
				if(system(cmdline)) return 10;
			}
			if(arguments.compression==2){
				sprintf(cmdline, "pnmtojpeg --progressive --quality=%d < tmp.pgm > %s_%04d.jpg\n",
					arguments.jpegquality,
					arguments.output_file, i);
				if(system(cmdline)) return 11;
			}
			if(arguments.compression){
				if(system("rm tmp.pgm")) return 12;
			}
		}
	}
	
	/* calculate per-pixel mode and spit it out */
	if(arguments.modebg){
		calc_mode(img,thist,dims[1],dims[2],arguments.modebg);
		for(j=0;j<dims[1]*dims[2];j++){
			img[j] = htons(img[j]);	/* change byte order */
		}
		export_pgm(img,dims[1],dims[2],4096,sizeof(pix_type),"background.pgm");
	}
	
	return 0;
}
