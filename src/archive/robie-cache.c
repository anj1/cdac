#if 0
		//sprintf(cmdlin,"pnmtojpeg --quality=80 --progressive < movec.pgm > movec_%04d.jpg\n", i);
		//sprintf(cmdlin,"pnmtopng -compression 9 < movec.pgm > movec_%04d.png\n", i);
		sprintf(cmdlin,"mv movec.pgm movec/%04d.pgm\n", i);
		if(system(cmdlin)){
			printf("error during pnmtojpeg invoke\n");
			return 55;
		}
#endif


#if 0
		/* export out pretty pictures */
		conv_pix_type_to_char(next, buf, dims[1]*dims[2],maxval);
		int mn;
		mn = export_local_max(mstart, mend, MAX_PATHN, merr, s.path[0].p, s.tdot, s.norml, s.path[0].n, MAX_SEARCH_RAD, 1);
		draw_motion(mstart, mend, mn, buf, NULL, merr, 1.0, 1, dims[1], dims[2]);
		export_8bit_pgm(buf,dims[1],dims[2],"/tmp/merr.pgm");
		
		conv_pix_type_to_char(next, buf, dims[1]*dims[2],maxval);
		//for(k=0;k<dims[1]*dims[2];k++) buf[k]=0;
		paint_nrm(buf,dims[1],dims[2],s.path[1].p,s.path[1].n,s.norml,s.bminus,s.bplus,NULL,MAX_SEARCH_RAD);
		export_8bit_pgm(buf,dims[1],dims[2],"/tmp/norml.pgm");

#endif
