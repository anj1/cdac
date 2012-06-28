#!/bin/sh

# Script to generate realistic-looking cell images, with just svg input
# and some NetPBM magic.

# Script can also run in background. One trick is to run one instance
# for each svg image. Using that, I can make %100 optimal use of my cpu,
# converting 30 800x600 images in 4 seconds. That's 330 processes!

# November 8th: Added random seed to mask noise generator
# This required upgrading netpbm

EXPECTED_ARGS=4
E_BADARGS=65

# Check for correct number of input arguments
if [ $# -ne $EXPECTED_ARGS ]
then
  echo "Usage: `basename $0` filename.svg <edge breakup scale> <gauss filter sigma> <noise factor>"
  echo "    Example: file.svg 10 1 0.1"
  exit $E_BADARGS
fi

brkscale=$2
gaussw=$3
noisefactor=$4

# TODO
width=800
height=600

# Make our named pipes for routing information between programs
# In order to keep our program parallelizable, each instance gets its
# own folder, named randomly.
# don't set N too high; we pass this number to pgmaddnoise as well.
rnd=`od -vAn -N3 -tu4 < /dev/urandom | sed -r 's/^[[:space:]]*//'`
TMPPREFIX=/tmp/$rnd
echo $TMPPREFIX
mkdir $TMPPREFIX

brkimg=$TMPPREFIX/brkimg
gaussimg=$TMPPREFIX/gaussimg
cellimg=$TMPPREFIX/cellimg
tmpimg=$TMPPREFIX/tmpimg

# Create named pipes
mkfifo $brkimg
mkfifo $gaussimg
mkfifo $cellimg
mkfifo $tmpimg

# Create mask image for edges (when applied to edges will cause them to appear broken up)
pgmnoise $(($width/$brkscale)) $(($height/$brkscale)) -randomseed=$rnd | pamscale $brkscale -filter sinc > $brkimg &

# Render image
rsvg-convert $1 -o $cellimg &
pngtopnm -alpha < $cellimg > $tmpimg &

# Create a gaussian filter
pamgauss $((1+$gaussw*6)) $((1+$gaussw*6)) -sigma $gaussw > $gaussimg &

# Now create the actual image by multiplying with mask, blurring,
# offsetting to 128, and adding some noise.
# pgmaddnoise is not part of the NetPbm package and was written by me.
pamarith -multiply $tmpimg $brkimg | pnmconvol $gaussimg -nooffset | pamfunc -adder 128 | pgmaddnoise $noisefactor $rnd >  ${1%.svg}.pgm

#pnmconvol $gaussimg -nooffset < $tmpimg | pamfunc -adder 128 | pgmaddnoise $noisefactor $rnd >  ${1%.svg}.pgm

# Remember to remove named pipes
rm -R $TMPPREFIX
