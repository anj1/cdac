#ifndef SVG_OUT_H
#define SVG_OUT_H

void xml_export_start(FILE *f, char *doctype);
void svg_export_pathdata(FILE *f, point2d32f *path, int n);
void svg_export_pathanim(FILE *f, point2d32f *path, int n, float startt, float dt);
void svg_export_start(FILE *f, int width, int height);
void svg_export_path_start(FILE *f, char *style, char *id, point2d32f *path, int n);
void svg_export_path_end(FILE *f);
void svg_export_end(FILE *f);

#endif /* SVG_OUT_H */
