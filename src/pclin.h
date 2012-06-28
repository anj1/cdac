#ifndef PCLIN_H
#define PCLIN_H

void pclin_export_crosscorr_int(char *filename, float step, float *pdx1, int *pf1, int n1, float *pdx2, int *pf2, int n2);

#ifdef _Complex_I
void pclin_fourier_trans_real(float complex *out, int m, float *pdx, float *pf, int n);
void pclin_fourier_trans_cplx(float complex *out, int m, float *pdx, float *pr, float *pi, int n);
#endif

#endif /* PCLIN_H */
