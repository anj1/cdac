#ifndef FIT_PATH_H
#define FIT_PATH_H

#define MAX_PATHN  1024
#define MAX_PATHS 16
/* for 10_0000, use 16 */ 
#define MAX_SEARCH_RAD 10

/* options used / optimization parameters */
typedef struct pathopts{
	float kappa;
	float lambda;
} pathopts;

/* path candidate, along with error and confidence information */
typedef struct cpath{
	int n;						/* number of points */
	pathopts   opts;
	point2d32f p   [MAX_PATHN];	/* points */
	float      perr[MAX_PATHN];	/* error (or confidence) for points */
	float      phase[MAX_PATHN];/* internal coordinate */
	float      eval;			/* value of path */	
	int        extr[MAX_PATHN]; /* extrusion */
} cpath;

/* set of paths */
/* a collection of path candidates, along with temporary storage
 * for calculating normals, transition probabilities, etc */
typedef struct spath{
	int npath;
	char *id;							/* optional, useful for svgs */
	cpath path   [MAX_PATHS];
	float        bminus  [MAX_PATHN];	/* max. extrusion */
	float        bplus   [MAX_PATHN];	/* max. extrusion */
	tdot_t   tdot[MAX_PATHN*(2*MAX_SEARCH_RAD-1)*(2*MAX_SEARCH_RAD-1)];
	point2d32f   norml   [MAX_PATHN*MAX_SEARCH_RAD];	/* low-pass */
} spath;

int fit_path (float *img, int w, int h, float *sobx, float *soby, float *sobhist, spath *s, int search_rad, float normal_drift, float seg_length, int blockw, float kappa, int low_conf_rm);

int extr_avg(int *extr, spath *s);

#endif	/* FIT_PATH_H */
