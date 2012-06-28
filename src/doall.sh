#!/bin/sh

# This script performs the steps of extracting the images from the ICS
# file (and performing enhancements), analyzing them, and presenting the
# output in a neat format.

# I wrote this to simplify creating the images in my paper.
# Alireza Nejati
# Nov 22 2011

# examples:
# 08_0210/$ ~/micro/src/doall.sh "/home/anej001/data/Time lapse astros exp 1/Time lapse astros exp 1_8_10.ics" /tmp 0200 0220 /home/anej001/micro/run/08_0210 0.0200 201-220 5 08_0200-all <<< "n 360 150 540 380"

# ~/micro/src/doall.sh "/home/anej001/data/Time lapse astros exp 1/Time lapse astros exp 1_7_10.ics" /tmp 0090 0110 /home/anej001/micro/run/07_0090 0.0500 91-110 5 07_0090-all <<< "n 250 320 410 502"

#-----------------------------------------------------------------------
# Subroutines.
# Clean up and exit.
cleanup()
{
	# Delete temporary folder.
	# We must do it this way to prevent deleting important folders.
	cd /tmp
	rm -R $rnd
	exit $rc
}

# Check for errors, if any occured clean up and exit.
errchk()
{
	if [[ $rc != 0 ]] ; then
		cleanup
	fi
}

# ----------------------------------------------------------------------
# Step 0. Preparation.
EXPECTED_ARGS=9
E_BADARGS=65

# Check for command-line arguments.
# Check for correct number of input arguments
if [ $# -lt $EXPECTED_ARGS ]
then
	echo "<data path> <binary path> <startframe> <endframe> <svg folder> <kappa> <output frames> <ncols> <outname> [cmp]"
	echo "We expect the first svg file to be named <startframe>.svg"
	echo "if the 'cmp' parameter (without quotes) is given, does error computations as well"
	echo "./doall.sh \"/home/anej001/data/Time lapse astros exp 1/Time lapse astros exp 1_10_10.ics\" /tmp 0116 0146 /home/anej001/micro/run/10_0116 0.02 116,120,146 3 10_0116-all"
	exit $E_BADARGS
fi

DATAPATH=$1
BINPATH=$2
STARTFRAME=$3
ENDFRAME=$4
SVGPATH=$5
KAPPA=$6
OUTFRAMES=$7
NCOLS=$8
OUTNAME=$9
SVGNAME=$OUTNAME-k$KAPPA
COMPARE=$10

# Generate random folder. 
rnd=`od -vAn -N3 -tu4 < /dev/urandom | sed -r 's/^[[:space:]]*//'`
TMPDIR=/tmp/$rnd
mkdir $TMPDIR
cd $TMPDIR

# ----------------------------------------------------------------------
# Step 1. Tracking.
# Use extract2 to produce all the frames.
# We have a raw version for robie, and 8-bit versions for robie-present
#$BINPATH/extract2 -r -f$STARTFRAME:$ENDFRAME -o raw "$DATAPATH"
$BINPATH/extract2 -e -f$STARTFRAME:$ENDFRAME -o enh "$DATAPATH"
rc=$?
errchk

# Use robie to perform tracking.
#$BINPATH/robie raw_%04d.pgm $SVGPATH/$STARTFRAME.svg $STARTFRAME $ENDFRAME $KAPPA . -t
$BINPATH/robie "$DATAPATH" $SVGPATH/$STARTFRAME.svg $STARTFRAME $ENDFRAME $KAPPA . -t
rc=$?
errchk

# Use robie-present to arrange in neat format.
$BINPATH/robie-present enh_%04d.pgm sup-k$KAPPA\_%04d.txt $OUTFRAMES $NCOLS $OUTNAME robie
rc=$?
errchk

# if cmp has been specified, do comparison as well
# save results in /tmp/avg.csv
if [ $COMPARE = cmp ]; then
	echo -n "$KAPPA, " >> /tmp/avg.csv
	$BINPATH/cellcmp2 $OUTFRAMES sup-k$KAPPA\_%04d.txt none $SVGPATH/%04d.svg -r >> /tmp/avg.csv
	
	$BINPATH/cellcmp2 $OUTFRAMES sup-k$KAPPA\_%04d.txt none $SVGPATH/%04d.svg -s >> /tmp/avg.csv
	
	#$BINPATH/cellcmp2 $OUTFRAMES sup-k$KAPPA\_%04d.txt /home/anej001/micro/run/10_0116/absnake/%04dm-ABSnake-r1-z1.txt $SVGPATH/%04d.svg -s > /tmp/10_0116-dist.csv
	#$BINPATH/cellcmp2 $OUTFRAMES sup-k$KAPPA\_%04d.txt none $SVGPATH/%04d.svg -s > /tmp/10_0116-dist.csv
	#$BINPATH/cellcmp2 $OUTFRAMES sup-k$KAPPA\_%04d.txt /home/anej001/micro/run/07_0090/absnake/raw/ABSnake-r1-z%d.txt $SVGPATH/%04d.svg -s > /tmp/07_0090-dist.csv
	
	rc=$?
	errchk
fi

# Save produced output.
cp $OUTNAME.svg /tmp/$SVGNAME.svg
cp $OUTNAME.jpg /tmp

# ----------------------------------------------------------------------
# Step x. Cleanup
rc=0
cleanup
