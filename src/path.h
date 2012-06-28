#ifndef PATH_H
#define PATH_H

float dist2d(point2d32f p1, point2d32f p2);
point2d32f normal2d(point2d32f p0, point2d32f p2);

void path_scale(point2d32f *path2, point2d32f *path1, int n, float factor);

int path_subdivide(struct point2d32f *path, float len, int n, int nmax);
int path_simplify (struct point2d32f *path, float len, int n, int nmax);

int path_subdivide2(struct point2d32f *path, float *phase, float len, int n, int nmax, int closed);
int path_simplify2 (struct point2d32f *path, float *phase, float len, int n);
int path_simplify_smart(point2d32f *path, float *phase, float len, int n, float angle);

void repair_path(point2d32f *path1, point2d32f *path2, float *featerr, float thresh, int pathn, int closed);

void perturb_path(point2d32f *path1, point2d32f *path2, int n, float sd);

alib_box bounding_box(point2d32f *path, int n);

int find_intersect(point2d32f pa1, point2d32f pa2, point2d32f pb1, point2d32f pb2, point2d32f *pi);

int remove_intersect(point2d32f *path, float *phase, int n, int closed);
int remove_lowweight(point2d32f *path, float *perr, float *phase, int n, int closed);

void path_extrude(point2d32f *dstpath, point2d32f *srcpath, int n, float *extr, int all, int closed);
void path_smooth(point2d32f *dstpath, point2d32f *srcpath, int n, int closed, float dt);

void path_precalc_normals(point2d32f *nrm, point2d32f *path, int n, int rad, int closed);

float find_path_distance(point2d32f *pref, int nref, point2d32f *ptest, int ntest, int closed);

#endif	/* PATH_H */
