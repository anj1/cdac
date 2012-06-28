/* COMPILE: -Wall -O2 -lm -DTEST_MODE
 */

/* a pixelcurve
   a curve composed of a chain of pixels,
   where each pixel has exactly one 'next' pixel
   and one 'prev' pixel
   and both of them must be one of it's 8 neighbors.
   There is no root node; all are equal in the eyes of God.
*/

/* Created and bug-checked on June 30th, 2011 */
/* July 4: added pixelcurve_simplify */
/* July 4: added and verified pixelcurve_defrag*/
/* July 4: made poly2pixelcurve self-allocating */

/* The implementation is grossly inefficient; I know,
 * but since it is only used rarely, I didn't see the need to rewrite it
 */
 
#include <math.h>	/* sqrt() */
#include <malloc.h>	/* malloc(), free() */

#ifdef TEST_MODE
#include <stdio.h>
#endif

struct pixelcurve_s{
	int x;
	int y;
	struct pixelcurve_s *next;
	struct pixelcurve_s *prev;
};

static inline int iabs(int a)
{
	if(a < 0) return -a;
	return a;
}

/* verify that a given chain is a pixelcurve */
int pixelcurve_verify(struct pixelcurve_s *start)
{
	struct pixelcurve_s *this;
	struct pixelcurve_s *next;
	
	this = start;
	do{
		next = this->next;
#ifdef TEST_MODE
		printf("pixelcurve_verify: x,y: %d,%d next:%d,%d\n", this->x, this->y, next->x, next->y);
#endif
		if(!next) return 3;					/* sanity */
		if(next->prev != this) return 1;	/* consistency */
		if(iabs(next->x - this->x) > 1) return 2;	/* neighbors */
		if(iabs(next->y - this->y) > 1) return 2;
		this = next;
	} while(this != start);
	
	return 0;
}

/* using a simple algorithm, make all the nodes in the pixelcurve
 * follow together contiguousy in memory */
/* allocates a new memory structure */
int pixelcurve_defrag(struct pixelcurve_s **start)
{
	int i,n;
	struct pixelcurve_s *this;
	struct pixelcurve_s *start2;
	
	/* first, find number of nodes in pixelcurve */
	this=(*start)->next; for(n=1;this!=(*start);n++) this=this->next;
	
	/* allocate memory */
	start2=(struct pixelcurve_s *)malloc(n*sizeof(struct pixelcurve_s));
	if(!start2) return 1;
	printf("pixelcurve_defrag: n:%d\n", n);
	/* reconstruct */
	this=(*start);
	for(i=0;i<n;i++){
		start2[i].x=this->x;
		start2[i].y=this->y;
		start2[i].next = start2+i+1;
		start2[i].prev = start2+i-1;
		this=this->next;
	}
	/* close */
	start2[0].prev   = start2 + n - 1;
	start2[n-1].next = start2;
	
	/* deallocate previous memory */
	free(*start);
	*start = start2;
	
	return 0;
}

/* simplify a pixelcurve by removing cusps, etc. */
/* during this procedure, 'dangling' nodes are created which can be
 * removed with pixelcurve_defrag */
void pixelcurve_simplify(struct pixelcurve_s *start)
{
	struct pixelcurve_s *this;
	struct pixelcurve_s *next;
	
	this=start;
	do{
		next=this->next;
		if((iabs(next->next->x - this->x)<=1)
		 &&(iabs(next->next->y - this->y)<=1)){
			/* our assumption is that the curve is verified; thus
			* any of these would imply direct touching neighbors */
			if((this->x==next->next->x)||(this->y==next->next->y)){
				/* cusp? shortcut! */
				this->next = next->next;
				this->next->prev = this;
			}
		}
	}while(this!=start);
}

/* create pixelcurve with pixels (,) */
/* creates them all in a contiguous area of memory, starting at b */
/* returns number of pixels in curve */
int line2pixelcurve(double x1, double y1, double x2, double y2, struct pixelcurve_s *b, int npc)
{
	double plen;
	double incx,incy;
	double x,y;
	double i;
	int c;
	
	plen=(x2-x1);
	if(fabs(y2-y1)>fabs(plen)) plen=(y2-y1);
	plen=fabs(plen);
	
	incx = (x2-x1)/plen;
	incy = (y2-y1)/plen;
	
	c=0;
	x=x1; y=y1;
	for(i=0;i<(plen-1.0);i++){
		/* move to next */
		x+=incx;
		y+=incy;

		/* set coordinates */
		b[c].x = (int)x;
		b[c].y = (int)y;

#ifdef TEST_MODE
		printf("line2pixelcurve: x,y: %g=%d,%g=%d \n",x,b[c].x,y,b[c].y);
#endif

		/* build curve */
		b[c].prev = b + c - 1;
		b[c].next = b + c + 1;
		c++;
		
		if(c==npc) return -1;	/* not enough space! */
	}
	return c;
}

/* takes in a (closed) polygon and turns it into a pixelcurve */
/* the polygon must be explicitly closed! */
/* allocates b */
/* returns number of pixels in pixelcurve;
 * negative if error */
int poly2pixelcurve(double *x, double *y, int n, struct pixelcurve_s **start)
{
	int i;
	double x1,y1,x2,y2;
	int npcu;
	int err;
	struct pixelcurve_s *b;
	double lenx,leny,len;
	int npc;
	
	if(x[n-1]!=x[0]) return -1;
	if(y[n-1]!=y[0]) return -1;

	/* first, get a feel for the length of the curve */
	/* being explicitly closed helps to simplify code here too */
	len=0;
	for(i=0;i<n;i++){
		lenx = x[i+1]-x[i];
		leny = y[i+1]-y[i];
		len+=sqrt(lenx*lenx + leny*leny);
	}
	
	/* now, guess the amount of memory we need (*2 to give breathing room) */
	npc = (int)len*2;
	b = (struct pixelcurve_s *)malloc(npc*sizeof(struct pixelcurve_s));
	if(!b) return -2;
	
	/* for each line segment */
	x1=x[0]; y1=y[0];
	npcu=0;
	for(i=1;i<n;i++){
		x2 = x[i];
		y2 = y[i];
		
		/* connect */
		b[npcu].x = x1;
		b[npcu].y = y1;
		b[npcu].next = b+npcu+1;
		if(npcu!=0) b[npcu].prev = b+npcu-1;
		npcu++;
		
		/* create curve */
		err = line2pixelcurve(x1,y1,x2,y2,b+npcu,npc-npcu);
		if(err<0) return err;
		
		npcu += err;
		if(npcu > npc) return -3;	/* too many nodes! */
		
		x1=x2;	/* move to next */
		y1=y2;	
	}
	
	/* close curve */
	b[0].prev = b+npcu-1;
	b[npcu-1].next = b;
	
	*start = b;
	return npcu;
}

/* fill in 'gaps' in a pixelcurve */
/* designed to take in unverified pixelcurves and return
 * verified ones */
int pixelcurve_reconstruct(struct pixelcurve_s **start)
{
	struct pixelcurve_s *this;
	struct pixelcurve_s *next;
	int badly_stretched;
	
	badly_stretched=0;
	this=(*start);
	do{
		next=this->next;
		if(iabs(next->x - this->x) > 2) badly_stretched=1;
		if(iabs(next->y - this->y) > 2) badly_stretched=1;
	}while(this!=(*start));
	
	if(badly_stretched) return -1;
	
	/* TODO */
	
	return 0;
}

#ifdef TEST_MODE

int main()
{
	
	double x[4];
	double y[4];
	struct pixelcurve_s *pc;
	int err;
	
	x[0]=0.0; y[0]=0.0;
	x[1]=5.0; y[1]=5.0;
	x[2]=0.0; y[2]=5.0;
	x[3]=0.0; y[3]=0.0;
	
	err = poly2pixelcurve(x,y,4,&pc);
	if(err < 0){
		printf("error: %d\n", -err);
		return -err;
	}
	pixelcurve_defrag(&pc);
	
	err = pixelcurve_verify(pc);
	if(err){
		printf("logic error: %d\n", err);
		return err;
	}
	
	return 0;
}

#endif

