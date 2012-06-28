/* OK, time to clean up my svg loading code, while adding support for
 * muliple paths.
 * The old code (svg.c) was horrible. */

/* main modifications:
 *  parsing is now done inside find_paths,
 *  we don't care what the id is,
 *  the paths is not explicitly closed,
 *  and multiple paths are returned */

#define DEBUG_PRINT

#include <libxml/tree.h>		/* libxml2 for parsing .svg files */
#include <libxml/parser.h>
#include <string.h>				/* strtok() */
#include "point2d.h"		/* point2d32f */
#ifdef DEBUG_PRINT
#include <stdio.h>
#endif

/* parses path contained in the svg xml attribute ("d") */
/* this is a veeery simple parser and probably won't work on most svg
 * files. I designed it to work with the output from Inkscape which it
 * does nicely */
/* if p is NULL, just returns n */
int parse_outline2(xmlAttr *data, point2d32f *p, int *n, int nmax)
{
	char *str;
	char *tok;
	float x,y;
	float initx,inity;	/* beginning of sub-path */
	float curx,cury;	/* cursor */
	int i;
	int absolute;		/* absolute or relative coordinates */
	int mode;			/* 0:moveto 1:lineto */
	
	mode=-1;
	
	/* first, copy string */
	str = (char *)xmlStrdup(data->children->content);
	
	/* now, tokenize it */
	/* to avoid a lot of headaches, we make the assumption that the
	 * input will be "m blablablabala z" */
	
	absolute=0;
	/* initialize strtok */
	tok = strtok(str," ");

	curx = 0;
	cury = 0;
	
	/* just a guess (also stops gcc's whining) */
	initx = curx;
	inity = cury;
	
	i=0;
	while(tok!=NULL){
		if(strlen(tok)==1){	/* command */
			
			absolute = tok[0] > 'Z' ? 0 : 1;
			
			switch(tok[0]){
				case 'm':
				case 'M':
					mode=0;
					break;
					
				case 'l':
				case 'L':
					mode=1;
					break;
					
				case 'z':
				case 'Z':
					curx = initx;
					cury = inity;
					*n = i;
					return 0;
				
				default:
					printf("Error: command '%c' not supported.\n", tok[0]);
					printf("Cell outlines must be polygons (not curves)\n");
					return -3;
			}
		} else {	/* coordinates */
		
			sscanf(tok,"%f,%f",&x,&y);
		
			if(mode==-1){
				printf("Error: first token in path data must be command.\n");
				return -1;
			}
			
			if(mode==0){
				if(i>0){
					printf("Error: Cell outlines must be contiguous.\n");
					return -2;
				}
			}
			
			if(!absolute){x += curx; y += cury;}
			
			if(i==0){
				initx = x;	/* needed for closepath, */
				inity = y;	/* as it must move cursor to beginning */
			}
			
			if(p){
				p[i].x = x;
				p[i].y = y;
			}
			i++;
			
			curx = x;
			cury = y;
			
			if(mode==0) /* moveto can only be for first pair */
				mode=1;
		}
		
		tok = strtok(NULL," ");
	}

	printf("Error: Cell path must be closed. Continuing anyway...\n");
	*n = i;
	return 0;
}

/* find paths in svg file.
 * Returns number of paths found, or negative if error */
/* Has two modes. If passed NULL, doesn't copy anything and just returns
 * the number of paths. Otherwise, parses and copies path data */
int find_paths(xmlNode *a, char **id, point2d32f **p, int **n, int nmax, int c)
{
	xmlNode *cur_node = NULL;
	xmlAttr *cur_attr = NULL;
	xmlAttr *attr;
	int got_data,got_id;
	size_t len;
	int err;
	
	for(cur_node = a; cur_node; cur_node = cur_node->next){

		/* depth-first search */
		/* paths that recursion finds are accumulated (hence c) */
		c = find_paths(cur_node->children, id, p, n, nmax, c);
		if(c < 0) return c;
		
		/* Make sure is XML element */
		if(cur_node->type != XML_ELEMENT_NODE) continue;

		/* make sure we're on a 'path' */
		if(!xmlStrEqual((xmlChar *)"path",cur_node->name)) continue;
		
		/* we got the path, now process it */
		attr = cur_node->properties;
		got_data=0; got_id=0;
		for(cur_attr=attr;cur_attr;cur_attr=cur_attr->next){
			/* Make sure is XML attribute */
			if(cur_attr->type != XML_ATTRIBUTE_NODE) continue;
			
			if(xmlStrEqual((xmlChar *)"d",cur_attr->name)){
				got_data=1;
				/* have we been given memory? */
				if(p && n){
					err = parse_outline2(cur_attr, p[c], n[c], nmax);
					if(err) return -err;
				}
			}
			
			/* store 'id' field */
			if(xmlStrEqual((xmlChar *)"id",cur_attr->name)){
				got_id=1;
				if(id){
					/* we can't use xmlStrlen because we want the number
					 * of bytes, not characters */
					len = strlen((const char *)cur_attr->children->content);
					id[c] = (char *)malloc(len+1);
					if(!id[c]) return c;	/* can't return error because that would cause memory leaks */
					memcpy(id[c], cur_attr->children->content, len);
					id[c][len]=0;
				}
			}
		}

		if(!(got_data && got_id)){
#ifdef DEBUG_PRINT
			fprintf(stderr,"Error: find_paths: missing path id or data)\n");
#endif
			return -1;
		} else c++;
	}
	return c;
}

int get_paths_from_svg(char *filename, char **id, point2d32f **p, int **n, int nmax)
{
	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;
	int ret;
	
	/* test lib */
	LIBXML_TEST_VERSION
	
	doc = xmlReadFile(filename, NULL, 0);
	
	if(doc == NULL) {
#ifdef DEBUG_PRINT
		printf("error: could not parse file %s\n", filename);
#endif
		return 1;
	}
	
	/* track through the tree and stop when we find a path with
	 * id=cell */
	root_element = xmlDocGetRootElement(doc);
	
	ret = find_paths(root_element, id, p, n, nmax, 0);

	/* finish up and clean off the santorum */
	xmlFreeDoc(doc);
	xmlCleanupParser();

	return ret;
}

