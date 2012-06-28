#ifndef SNAKE_H
#define SNAKE_H

void snake_static_prob(float *sobx, float *soby, int w, point2d32f *path, float *pathprob, float *bminus, float *bplus, int pathn, int closed, int rad, float sd);
void snake_static_prob2(int *sobx, int *soby, int w, point2d32f *path, point2d32f *nrm, float *pathprob, float *bminus, float *bplus, int pathn, int closed, int rad, float sd, float *sobhist, float noisesd);

void snake_deform(point2d32f *path1, point2d32f *path2, float *extr, int pathn, int closed);
void snake_deform2(point2d32f *path1, point2d32f *path2, point2d32f *nrm, float *extr, int pathn, int rad, int closed);
void snake_deform2_int(point2d32f *path1, point2d32f *path2, point2d32f *nrm, int *extr, int pathn, int rad, int closed);

void pathprob_trace_greedy(float *extr, float *patherr, float *pathprob, int pathn, int rad);
void pathprob_trace_max   (float *extr, float *patherr, float *pathprob, int pathn, int rad, int closed, float sd);

void diffuse_pathprob(float *pathprob2, float *pathprob1, int pathn, int rad);
void pathprob_maxima_suppress(float *pathprob2, float *pathprob1, int pathn, int rad);

void snake_init_extrude_bounds(float *bminus, float *bplus, float rad, int pathn);
void snake_refine_extrude_bounds(point2d32f *path, float *bminus, float *bplus, int pathn, int closed);

void snake_trans_dot(sob_t *sobx, sob_t *soby, int w, point2d32f *path, int n, tdot_t *tdot, point2d32f *nrm, float *bminus, float *bplus, int rad, int closed);

void snake_trans_prob(tdot_t *tdot, int pathn, int rad, int closed, float sigma, float tau);

void snake_greedy       (float *extr, int pathn, float *tprob, int rad, int closed);
void snake_trace_optimal(int   *extr, int pathn, float *tprob, int rad, int closed);
void snake_trace_force  (int   *extr, int pathn, float *tprob, int rad, int closed);
void snake_trace_join(int *extr_out, int pathn, int *tdot, int rad, int closed, int nlist, int ntop);
void snake_trace_dynamic(int *extr, int pathn, int *tprob, int rad, int closed);
void snake_trace_bellman(int *extr, int pathn, int *tdot, int rad, int closed);

void snake_trace_force_multi(void **status, int *extr, int pathn, float *tprob, int rad, int closed);

int export_local_max(point2d32f *start, point2d32f *end, int nmax, float *eerr, point2d32f *path, float *tprob, point2d32f *norml, int n, int rad, int closed);

void extr_to_error(float *featerr, int *extr, int pathn, float *tprob, int rad, int closed);

float snake_eval(int *extr, float *tprob, int pathn, int rad, int closed);
#endif
