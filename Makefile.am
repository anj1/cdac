AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

AM_CPPFLAGS = @libxml_CFLAGS@
AM_CFLAGS = -O2
LDADD = @libxml_LIBS@ -lavl -lics -lm


bin_PROGRAMS = robie robiepresent extract2 cellcmp2 textextr svgtotext
robie_SOURCES = src/robie.c src/textutil.c src/compressimg.c src/beamdynamic.c src/beam.c src/line_intrsct.c src/pclin.c src/fit_path2.c src/pgm_util.c src/matrix.c src/snake.c src/filters.c src/path.c src/svg2.c src/vis_motion.c
robiepresent_SOURCES = src/robie-present.c src/textutil.c src/compressimg.c src/pgm_util.c src/framelist.c 
extract2_SOURCES = src/extract2.c src/histnorm.c 
cellcmp2_SOURCES = src/cellcmp2.c src/framelist.c src/hausdorff.c src/svg2.c src/textutil.c 
textextr_SOURCES = src/textextr.c src/textutil.c
svgtotext_SOURCES = src/svgtotext.c src/svg2.c

#dist_noinst_SCRIPTS = autogen.sh
