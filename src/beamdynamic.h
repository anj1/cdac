#ifndef BEAMDYNAMIC_H
#define BEAMDYNAMIC_H

float snake_trace_bellmanford(int *extr, int pathn, tdot_t *tdot, int rad, int closed);

void tdot_subtract_energy(tdot_t *tdot, int pathn, int rad, int closed, float kappa, float lambda);

#endif	/* BEAMDYNAMIC_H */
