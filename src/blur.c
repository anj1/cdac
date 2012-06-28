/* COMPILE: -Wall -O2 -DTEST_MODE
 */
 
/* blur_gaussian
   blur_cusp
   blur_diffuse
   blur_box
*/

#ifdef TEST_MODE
#include <stdio.h>
#endif

/* simple diffusion blur */
/* for speed, uses the following kernel:
     3
   3 4 3
     3
   
   Also, doesn't make any special consideration for edges.
   So, the best course of action is to simply ignore the output at them!
*/
void blur_diffuse(int *in, int *out, int w, int h)
{
	int i;
	
	int *in11,*in21,*in12,*in10,*in01;
	int *out11;
	int edge;
	
	/* init pointers */
	
	            in10=in  +1;
	in01=in+w;  in11=in+w+1;  in21=in+w+2;
	            in12=in+w*2+1;
	
	            out11=out+w+1;
	
	/* 5 increments, 4 additions, 1 subtraction, 3 shifts,
	 * 5 memory reads, 1 memory write */
	for(i=0;i<(w*h)-w*2-1;i++){
		edge = *in10 + *in21 + *in12 + *in01;
		edge = (edge << 2) - edge;		/* edge*3 */
		*out11 = (((*in11) << 2) + edge) >> 4;
		
		in11++; in21++; in12++; in10++; in01++;
		out11++;
	}
}

/* amount: 0 = only diffusion 256 = max blur */
/* warning: in is modified! */
void blur_cusp(int *in, int *tmp, int w, int h, int amount)
{
	int ramount;
	int sum;
	int i,j;
	
	ramount = 256 - amount;

	/* forward */
	sum=in[0];
	for(i=0;i<w*h;i++){
		sum = (sum*amount + in[i]*ramount) >> 8;
		tmp[i] = sum;
	}
	/* backward */
	sum=tmp[w*h-1];
	for(i=w*h-1;i>=0;i--){
		sum = (sum*amount + tmp[i]*ramount) >> 8;
		in[i] = sum;
	}
	/* intermission (gauss-izing) */
	for(i=1;i<(w*h)-1;i++){
		tmp[i] = (in[i-1] + in[i] + in[i+1]) / 3;
	}
	
	/* upward */
	sum=tmp[0];
	for(j=0;j<w;j++){
		for(i=j;i<w*h;i+=w){
			sum = (sum*amount + tmp[i]*ramount) >> 8;
			in[i] = sum;
		}
	}
	/* downward */
	sum=in[w*h-1];
	for(j=w-1;j>=0;j--){
		for(i=w*(h-1)+j;i>=0;i-=w){
			sum = (sum*amount + in[i]*ramount) >> 8;
			tmp[i] = sum;
		}
	}
	/* final (gauss-izing) */
	for(j=1;j<(w-1);j++){
		for(i=j+w;i<(w*h)-h;i+=w){
			in[i] = (tmp[i-w] + tmp[i] + tmp[i+w]) / 3;
		}
	}
}

void blur_cusp2(int *in, int *tmp, int w, int h, int amount)
{
	int ramount;
	int sum;
	int i,j;
	
	ramount = 256 - amount;
	
	/* forward */
	sum=in[0];
	for(i=0;i<w*h;i++){
		sum = (sum*amount + in[i]*ramount) >> 8;
		tmp[i] = sum;
	}
	/* backward */
	sum=tmp[w*h-1];
	for(i=w*h-1;i>=0;i--){
		sum = (sum*amount + tmp[i]*ramount) >> 8;
		in[i] = sum;
	}
	/* upward */
	sum=in[0];
	for(j=0;j<w;j++){
		for(i=j;i<w*h;i+=w){
			sum = (sum*amount + in[i]*ramount) >> 8;
			tmp[i] = sum;
		}
	}
	/* downward */
	sum=tmp[w*h-1];
	for(j=w-1;j>=0;j--){
		for(i=w*(h-1)+j;i>=0;i-=w){
			sum = (sum*amount + tmp[i]*ramount) >> 8;
			in[i] = sum;
		}
	}
}

#ifdef TEST_MODE

void printest(int *a, int n)
{
	int i,j;
	
	for(j=0;j<n;j++){
		for(i=0;i<n;i++){
			printf("% 5d ", a[j*n + i]);
		}
		printf("\n");
	}
}

#define SIMX 5

int main()
{
	int inb[SIMX*SIMX];
	int tmp[SIMX*SIMX];
	int i,c;
	
	for(c=0;c<10;c++){
		for(i=0;i<SIMX*SIMX;i++) inb[i]=0;
		inb[12]=255;
		
		blur_cusp(inb, tmp, SIMX, SIMX, 220);
		
		printest(inb, SIMX);
		printf("\n");
	}

	
	return 0;
}

#endif /* TEST_MODE */
