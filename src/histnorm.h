#ifndef HISTNORM_H
#define HISTNORM_H

void edrc(pix_type *in, int w, int h, int *pemin, int *pemax, unsigned char cop);
void edr(pix_type *in, int w, int h, int *pemin, int *pemax);
void normalize(pix_type *in, unsigned char *out, int w, int h, int emin, int emax);

#ifdef POINT2D_H
void edrc_region(pix_type *in, int w, alib_box bounds, int *pemin, int *pemax, int cop);
#endif

#endif /* HISTNORM_H */
