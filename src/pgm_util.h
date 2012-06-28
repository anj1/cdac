#ifndef PGM_UTIL_H
#define PGM_UTIL_H

void export_8bit_pgm(unsigned char *buf, int width, int height, char *filename);

/* import both 8-bit and 16-bit pgms */
int import_pgm_16bit
(pix_type *buf, size_t *pwidth, size_t *pheight, int *pmaxval, char *filename);

/* this function is optimized only for 8-bit pgms */
int import_pgm_8bit
(unsigned char *buf, size_t *pwidth, size_t *pheight, int *pmaxval, char *filename);

int peek_pgm(size_t *pwidth, size_t *pheight, int *pmaxval, char *filename);

#endif	/* PGM_UTIL_H */
