#include <stdint.h>
#include <stdio.h>
#include <netinet/in.h>	/* ntohs(), htons() */
#include <stdlib.h>	/* atoi() */

/* Nov 23: added support for comments in PGM file */

/* even though PGM supports 16 bit, not all imaging software does */
void export_8bit_pgm(unsigned char *buf, int width, int height, char *filename)
{
	FILE *f;
	if(!(f=fopen(filename,"wb"))){printf("ERR:8bit_pgm\n");return;}
	fprintf(f,"P5\n%d %d\n255\n",width,height);
	if(fwrite(buf,1,width*height,f)!=width*height){printf("ERR:export_float_pgm:fwrite\n");return;}
	fclose(f);
}

/* runs up to and including the newline characters */
/* should be good for LF, CR, LF+CR, and CR+LF systems */
void goto_nextline(FILE *f)
{
	char c,c2;
	
	while((c=fgetc(f))!=EOF){
		if((c=='\n')||(c=='\r')){
			/* handle windows and other systems */
			c2 = fgetc(f);
			if(((c2=='\r')||(c2=='\n'))&&(c2!=c)){
			} else {
				ungetc(c2,f);	/* whoops! push it back in */
			}
			break;
		}
		c++;
	}
}
		
/* get next non-comment token from file */
int get_next_token(FILE *f, char *buf)
{
	int i;
	int iscomment;
	
	/* keep trying until we get a usable token */
	do{
		iscomment=0;
		
		/* get next token */
		if(fscanf(f, "%s", buf)!=1) return 1;
		
		/* is there a '#' in the middle? */
		for(i=0;buf[i];i++) if(buf[i]=='#') { iscomment=1; break; }
		
		if(iscomment){
			/* truncate */
			buf[i]=0;
			
			/* run to next newline */
			goto_nextline(f);
		}
		
		/* should not end in the middle of the header */
		if(feof(f)) return 2;
	}while(buf[0]==0);
	
	return 0;
}

/* gets pgm header with support for comments. */
/* returns 0 if success, error otherwise */
#define MAX_TOK_SIZE 1024
int get_pgm_header(FILE *f, size_t *pwidth, size_t *pheight, int *pmaxval)
{
	char w;
	char buf[MAX_TOK_SIZE];
	
	/* ignore magic number for now */
	if(get_next_token(f, buf)) return 1;
	if(strcmp("P5",buf)!=0){
		printf("ERR:Image not binary 8-bit grayscale.\n");
		return 7;
	}
	
	if(get_next_token(f, buf)) return 2;
	*pwidth  = atoi(buf);
	
	if(get_next_token(f, buf)) return 3;
	*pheight = atoi(buf);
	
	if(get_next_token(f, buf)) return 4;
	*pmaxval = atoi(buf);
	
	/* get the raster delimiter (must be whitespace) */
	w=fgetc(f);
	if(feof(f)) return 5;
	if((w!=' ')&&(w!='\n')&&(w!='\r')&&(w!='\t')){	
		return 6;
	}
	
	return 0;
}

/* import both 8-bit and 16-bit pgms */
int import_pgm_16bit
(uint16_t *buf, size_t *pwidth, size_t *pheight, int *pmaxval, char *filename)
{
	FILE *f;
	int i,bpp;
	size_t size;
	int r;
	
	if(!(f=fopen(filename,"r"))){printf("ERR:16bit_pgm\n");return 1;}
	
	if((r=get_pgm_header(f,pwidth,pheight,pmaxval))){
		printf("ERR:8bit_pgm:get_pgm_header:%d\n",r);
		return 2;
	}
	
	if((*pmaxval)>65535){printf("ERR: pgm not <16 bit.\n");return 3;}
	bpp = *pmaxval < 256 ? 1 : 2;
	
	size=(*pwidth)*(*pheight);
	for(i=0;i<size;i++){
		buf[i]=0;
		if(fread(buf+i,bpp,1,f)!=1){printf("ERR: pgm data\n");return 4;}
		if(bpp>1) buf[i] = ntohs(buf[i]);	/* convert byte order */
	}
	fclose(f);
	return 0;
}

/* this function is optimized only for 8-bit pgms */
/* WARNING: not tested */
int import_pgm_8bit
(unsigned char *buf, size_t *pwidth, size_t *pheight, int *pmaxval, char *filename)
{
	FILE *f;
	size_t size;
	int r;
	
	if(!(f=fopen(filename,"r"))){printf("ERR:8bit_pgm\n");return 1;}
	
	if((r=get_pgm_header(f,pwidth,pheight,pmaxval))){
		printf("ERR:8bit_pgm:get_pgm_header:%d\n",r);
		return 2;
	}
	
	if((*pmaxval)>256){printf("ERR: pgm not <8 bit.\n");return 4;}
	size=(*pwidth)*(*pheight);
	
	if(fread(buf,1,size,f)!=size){
		fprintf(stderr,"ERROR: couldn't load pgm data\n");
		return 5;
	}
	fclose(f);
	return 0;
}

int peek_pgm(size_t *pwidth, size_t *pheight, int *pmaxval, char *filename)
{
	FILE *f;
	if(!(f=fopen(filename,"r"))){printf("ERR:peek_pgm\n");return 1;}
	if(get_pgm_header(f,pwidth,pheight,pmaxval)) return 2;
	fclose(f);
	return 0;
}
