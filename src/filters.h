#ifndef FILTERS_H
#define FILTERS_H

void shift_cmp(pix_type *prev, pix_type *next, int *out,
				int width, int height,
				int xo, int yo, int shift);

int gaussian_blur(int *in, int *out, int *tmp, int width, int height,double sd);

void motion_detect(pix_type *prev, pix_type *next,
                   int w, int h,
                   double sd, int shift,
                   char maxx, char maxy,
                   char *mx, char *my, float *conf);
                   
void smooth_mv(char *in, char *out, float *conf, int w, int h);

int apply_filter(pix_type *in, int w, int h, int x, int y, int *gabor, int ngab);

int variance(pix_type *in, int *out, int *tmp1, int *tmp2, int w, int h, double sd1, double sd2, int shift);
void fast_variance(pix_type *in, int *out, int *tmp1, int w, int h);

void sobel2cf(float *in, float *outx, float *outy, int w, int h);

void hardedge(unsigned char *in, unsigned char *out, int w, int h);

void distort_vec_expl(unsigned char *in, unsigned char *out, float *vecx, float *vecy, int *acc, int width, int height);

void downsamplex2(int *out, int *in, int w, int h, int outw);

int noise_variance(int *in, int w, int h, int block);

#endif /* FILTERS_H */
