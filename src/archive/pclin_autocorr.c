/* calculation of autocorrelation */
/* calculation proceeds through a series of stops */
/* at each stop, we accumulate x and find f */
/* dx is refreshed at each stop for one of the data sets */
/* we always pick the dataset with minimum dx */
/* when dx goes negative, pick next data point */
/* for initiation, simply set dx to -offset */

/* autocorrelation of periodic functions with periods that are not
   multiples of each other */

-|--|----|-|---
--|-|-|---|----

thisdx = dx1 < dx2 ? dx1 : dx2;
dx1 -= thisdx;
dx2 -= thisdx;

while(dx1 <= 0.0) { dx1 += pdx[i1+1]; i1++; }
while(dx2 <= 0.0) { dx2 += pdx[i2+1]; i2++; }

if(dx1 < dx2){
	dx2 -= dx1;
	f2 = slope2*dx1;
	i1 = (i1+1)%n1;
	dx1 = pdx1[i1];
	slope1 = (pf1[i1]-f1)/pdx1[(i1+1)%n1];
	f1 =   pf1[i1];
	i1++;
} else {
	dx1 -= dx2;
	dx2 = pdx2[(i2+1)%n2];
	i2++;
}
