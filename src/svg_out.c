#include <stdio.h>	/* FILE, fprintf() */
#include "point2d.h"

void svg_export_pathdata(FILE *f, point2d32f *path, int n)
{
	int i;
	
	if(n < 1) return;
	
	fprintf(f, "m %f,%f ", path[0].x, path[0].y);
	for(i=1;i<n;i++) fprintf(f, "L %.2f,%.2f ", path[i].x, path[i].y);
	fprintf(f, "z ");
}

void svg_export_pathanim(FILE *f, point2d32f *path, int n, float startt, float dt)
{
	fprintf(f,"<animate ");
	fprintf(f,"attributeName=\"d\" ");
	//fprintf(f,"attributeType=\"CSS\" ");
	fprintf(f,"begin=\"%fs\" ", startt);
	fprintf(f,"dur=\"%fs\" ", dt);
	fprintf(f,"to=\" ");
	svg_export_pathdata(f,path,n);
	fprintf(f,"\" ");
	fprintf(f,"fill=\"freeze\" ");
	fprintf(f,"/>\n");
}

/* doctype should be "svg" */
void xml_export_start(FILE *f, char *doctype)
{
	fprintf(f,"<?xml version=\"1.0\" standalone=\"no\"?>\n");
	
	fprintf(f,"<!DOCTYPE %s PUBLIC \"-//W3C//DTD SVG 1.1//EN\"\n", doctype);
	fprintf(f,"\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n");
}

void svg_export_start(FILE *f, int width, int height)
{
	fprintf(f,"<svg ");
	fprintf(f," width=\"%d\" ", width );
	fprintf(f,"height=\"%d\" ", height);
	fprintf(f,"version=\"1.1\" ");
	fprintf(f,"xmlns=\"http://www.w3.org/2000/svg\" ");
	fprintf(f,">\n");
}

/* style: an svg style string.
 * id: the path's id
 * path: the initial path data
 */
void svg_export_path_start(FILE *f, char *style, char *id, point2d32f *path, int n)
{
	fprintf(f,"<path ");
	fprintf(f,"style=\"%s\" ",style);
	fprintf(f,"id=\"%s\" ",id);
	fprintf(f,"d=\" ");
	svg_export_pathdata(f,path,n);
	fprintf(f,"\" ");
	fprintf(f,">\n");
}

void svg_export_path_end(FILE *f)
{
	fprintf(f,"</path>\n");
}

void svg_export_end(FILE *f)
{
	fprintf(f,"</svg>\n");
}
