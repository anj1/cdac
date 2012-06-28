#include <stdio.h>

/* parses frame list and returns number of frames in it */
/* input:  framelst,
 * output: frames
 */
const char ferr[]="ERROR: frame list can only contain chars 0-9,-\n";
#define BUILDRANGE \
if(range){ \
	if(frames[c-1]>=tmp){ \
		fprintf(stderr,"ERROR: Range upper bound smaller than lower bound.\n");\
		return -4; \
	} \
	for(j=frames[c-1]+1;j<=tmp;j++,c++){ \
		frames[c] = j; \
	} \
	range=0; \
} else c++;
int parse_framelst(char *framelst, int *frames)
{
	int i,j,c=0;
	int range=0;
	int tmp=0;	/* to stop gcc's whining */
	char cur;
	
	for(i=0;framelst[i];i++){
		cur = framelst[i];
		if(cur >= '0' && cur <= '9'){
			tmp = tmp*10 + cur-'0';
			continue;
		}
		if(i==0){
			fprintf(stderr,"ERROR: <frames> must start with number\n");
			return -1;
		}
		frames[c] = tmp;
		
		switch(cur){
			case ',':
				BUILDRANGE
				break;
				
			case '-':
				range=1;
				frames[c] = tmp;
				c++;
				break;
				
			default:
				fprintf(stderr,ferr);
				return -2;
		}
		tmp=0;
	}
	frames[c] = tmp;
	BUILDRANGE
	return c;
}
