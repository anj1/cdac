for i in 0.03 0.05 0.07
do
	for j in r1 r2 r3 r3 r4 r5
	do
		runname=$i\_$j
		for s in *.svg
		# Convert svg files to cell outlines
		do
			cellrend2.sh $s 10 1 $runname &
		done
		
		# Pictures must be generated before we can proceed.
		wait 
		
		# Track the cell outlines that have been produced
		robie %04d.pgm 0116.svg 116 146 $runname . -t
		
		# Tile them into neat format
		robie-present %04d.pgm sup-k$i\_%04d.txt 117 146 6 out/all_$runname <<< "n 340 280 560 600"
	done
done
