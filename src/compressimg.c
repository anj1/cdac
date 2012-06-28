#include <stdio.h>
#include <stdlib.h>		/* exit() */
#include <unistd.h>		/* exec(), write() */
#include <sys/wait.h>	/* waitpid() */

/* export image to jpeg by invoking pnmtojpeg */
/* since we embed this image into svg, it's OK and will not affect the
 * quality of our output drawings */

/* Nov 22: Added waitpid() to wait to finish encoding (without it,
 * the jpeg output might be truncated if the shell script calling the
 * program terminates */
 
char cmprs_prg[] = "pnmtojpeg";
char *cmprs_args[] = {"pnmtojpeg", "-quality=70", "-optimise", "-progressive", NULL};
//char *cmprs_args[] = {"pnmtojpeg", "-optimise", "-progressive", NULL};
//char *cmprs_args[] = {"pnmtojpeg", "-quality=70", "-optimise", NULL};

/*
char cmprs_ext[] = "png";
char cmprs_prg[] = "pnmtopng";
char *cmprs_args[] = {cmprs_prg, NULL}; */
void export_to_compressed(char *filnam, unsigned char *buf, size_t w, size_t h)
{
	pid_t pid;
	int pipe_pgm[2];
	size_t n;
	char pgmhead[256];
	int nh;
	
	if(pipe(pipe_pgm)){
		fprintf(stderr,"Error: couldn't create pipe to export to jpeg.\n");
		return;
	}
	
	pid = fork();
	if(pid < (pid_t) 0){
		fprintf(stderr,"Error: export_to_compressed: fork failed.\n");
		return;
	}
	
	if(pid == (pid_t) 0){
		/* child process. */
	
		/* set output to be redirected to specified file */
		/* equivalent of '>' in shell */
		if(!freopen(filnam, "w", stdout)){
			fprintf(stderr,"Error: couldn't redirect output to file.\n");
			exit(2);	/* DON'T return! This is another process! */
		}
		
		/* set input to be redirected from pipe */
		/* equivalent to '|' */
		dup2(pipe_pgm[0], 0);
		
		/* now run the conversion program */
		if(execvp(cmprs_prg,cmprs_args)){
			fprintf(stderr,"Couldn't execute pnmtojpeg; is NetPBM installed?\n");
			exit(3);
		}
		
	} else {

		/* parent process */
		nh = sprintf(pgmhead,"P5\n%ld %ld\n255\n",w,h);
		if(write(pipe_pgm[1],pgmhead,nh)<nh){
			fprintf(stderr,"Couldn't write pgm image header to pipe.\n");
			return;
		}
		
		n = write(pipe_pgm[1],buf,w*h);
		if(n < w*h){
			fprintf(stderr,"Couldn't write pgm image to pipe.\n");
			return;
		}
		
		/* wait for encoding to finish */
		waitpid(pid, NULL, 0);
	}
	
}
