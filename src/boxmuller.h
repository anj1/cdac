#ifndef BOXMULLER_H
#define BOXMULLER_H

/* for floating-point type, default to double */
#ifndef float_type
#define float_type double
#endif

float_type box_muller();
void box_muller2(float_type *x, float_type *y);

#endif	/* BOXMULLER_H */
