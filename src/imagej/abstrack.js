// I wrote this to automate tracking video using absnake
// Then I realized that ABSnake can operate on stacks
// So this code is obsolete before even being written.

// Import all classes under java.io.*
importPackage(Packages.java.io)

var startframe = 90;

var 

for(var i = 0; i < 20; i = i + 1)
{
	// Load image.
	IJ.open("/home/alireza/micro/run/10_0116/pgm/frame_0116.pgm");
	
	// Do median filtering (and save for presentation)
	IJ.run("Median...", "radius=2");
	IJ.saveAs("PGM", "/home/anej001/test.pgm");
	
	// Load region of interest.
	IJ.roiManager("Open", "/home/alireza/micro/run/10_0116/0116.roi");
	IJ.roiManager("Select", 0);
	
	// Do the snake (will save ROI)
	IJ.run("ABSnake", "gradient_threshold=5 number_of_iterations=50 step_result_show=1 draw_color=Green save_coords");
	
	// Remove old ROI
	IJ.roiManager("Delete");
	
	// Tidy up for next iteration
	IJ.selectWindow("Draw");
	IJ.run("Close");
	IJ.selectWindow("frame_0116.pgm");
	IJ.run("Close");
}
