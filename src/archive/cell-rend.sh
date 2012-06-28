# arguments: multiplier adder mix
# multiplier determines brightness of outline
# mix determines amount of noise
# the higher adder is, the brighter the background is and less contrast
# typical values: 0.2 96 0.1

# Directory for temporary files
p=/dev/shm

# Create gaussian filter in temporary folder
# This might seem like a really bad filter but due to the way pnmconvol
# handles it, it's actually quite accurate.
g=$p/gauss5x5.pgm
echo P2 > $g
echo 5 5 >> $g
echo 546 >> $g
echo 274 277 280 277 274 >> $g
echo 277 289 299 289 277 >> $g
echo 280 299 314 299 280 >> $g
echo 277 289 299 289 277 >> $g
echo 274 277 280 277 274 >> $g

# Run renderer
for i in *.svg
do
	rsvg-convert $i -o $p/tmp.png
	rm $p/noise.pgm
	pgmnoise 800 600 > $p/noise.pgm
	pngtopnm -alpha < $p/tmp.png | pamfunc -multiplier=$1 | pamfunc -adder=$2 > $p/tmp.pgm
	ppmmix $3 $p/tmp.pgm $p/noise.pgm | pamchannel 1 > $p/tmp2.pgm
	pnmconvol $g $p/tmp2.pgm > `echo $i | sed -e 's/svg$/pgm/'`
done

# clean up
rm $g
