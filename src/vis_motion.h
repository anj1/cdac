#ifndef VIS_MOTION_H
#define VIS_MOTION_H

/* primitives for motion visualization */
void draw_line(unsigned char *a, int w, int x, int y, char xo, char yo, unsigned char col);
void draw_motion(point2d32f *a, point2d32f *b, int n, unsigned char *out, char *status, float *error, float maxerr, int invert, int w, int h);
void export_motion(char *mx, char *my, unsigned char *mt, float *conf, int w, int h, char *filename);
void paint_nrm(unsigned char *buf, int w, int h, point2d32f *path, int pathn, point2d32f *nrm, float *bminus, float *bplus, float *pathprob, int rad);

#endif /* VIS_MOTION_H */
