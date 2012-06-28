#ifndef WATERSHED_H
#define WATERSHED_H

int watershed(int *in, unsigned char *out, int *dup, int w, int h, int *markx, int *marky, int nmark);

int watershed_rep(int *in, unsigned char *out, int *dup, int w, int h, int *markx, int *marky, int nmark, unsigned char *rep, int nrep, double rrad);

int center_of_mass(unsigned char *rep, int w, int h, int iter, int *markx, int *marky);

#endif	/* WATERSHED_H */
