#ifndef GLOBAL_H
#define GLOBAL_H

/* even though the microscope data is unsigned, we use a signed format
 * to make calculations easier (upper 4 bits of microscope data not used
 * therefore we have no problems) */
#define pix_type int16_t

#endif /* GLOBAL_H */
