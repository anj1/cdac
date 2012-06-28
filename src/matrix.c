/* COMPILE: -Wall -DTEST_MODE
 */

#ifdef TEST_MODE
#include <stdio.h>
#endif

/* matrix multiplication */

/* simple straight multiplication, no transposing */
/* performs AxB=C
   a[m x l]
   b[n x m]
   c[n x l]
 */
void mul_mat(float *c, float *a, float *b, int n, int m, int l)
{
	int i,j,k;
	int idx;
	float sum;
	float *arow;
	float *bcol;
	
	for(k=0;k<l;k++){
		arow = a + k*m;
		for(i=0;i<n;i++){
			bcol = b + i;
			idx=0;
			sum=0.0;
			for(j=0;j<m;j++){
				sum += bcol[idx]*arow[j];
				idx += n;
			}
			c[i + k*n] = sum;
		}
	}		
}

#ifdef TEST_MODE

int main()
{
	float a[6];
	float b[6];
	float c[4];
	
	a[0]=1.0; a[1]=0.0; a[2]=3.0;
	a[3]=2.0; a[4]=5.0; a[5]=0.0;
	
	b[0]=1.0; b[1]=0.0;
	b[2]=3.0; b[3]=2.0;
	b[4]=5.0; b[5]=0.0;
	
	mul_mat(c, a, b, 2, 3, 2);
	
	printf("c:\n");
	printf("%f %f\n%f %f\n",c[0],c[1],c[2],c[3]);
	
	return 0;
	
}

#endif
