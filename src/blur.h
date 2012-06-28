#ifndef BLUR_H
#define BLUR_H

void blur_diffuse(int *in, int *out, int w, int h);
void blur_cusp(int *inout, int *tmp, int w, int h, int amount);

#endif	/* BLUR_H */
