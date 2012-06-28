#ifndef POINT2D_H
#define POINT2D_H

/* floating-point data type used */
#define float_type float

typedef struct alib_box{
	int x1,y1;
	int x2,y2;
}alib_box;

/* if we have opencv, use it's point structure for compatibility */
#if defined(__OPENCV_CORE_TYPES_H__) || defined(_CV_H_)

#define point2d32f CvPoint2D32f

#else  	/* otherwise, fall back on our own */

typedef struct point2d32f{
	float x;
	float y;
}point2d32f;

#endif

#endif /* POINT2D_H */
