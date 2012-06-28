// Intended to be used with ImageJ (copy or link into the plugins dir)
// by Alireza Nejati

// Script to test out different ranges of parameters for ABSnake, to
// find the parameter set that works best.

// Import all classes under java.io.*
importPackage(Packages.java.io)

//imp = new Opener().openImage("/home/alireza/micro/run/pgm/frame_0117.pgm");

var imp = IJ.getImage();

var r_max = 1.00;

for(var ia_min = 0.25; ia_min <= 1.00; ia_min = ia_min + 0.25){
	for(var ia_max = 1.00; ia_max <= 8.00; ia_max = 2*ia_max){
		var r_min = 0.5;
		var r_max = 2.0;
		//for(var r_max = 1.00; r_max <= 2.00; r_max = r_max + 0.25){
			//for(var r_min = 0.5; r_min <= r_max; r_min = 2*r_min){
				
				// Set snake options by calling ABSnake Advanced Options
				IJ.run("ABSnake Advanced Options", "distance_search=100 displacement_min=0.10 displacement_max=2 threshold_dist_positive=100 threshold_dist_negative=100 inv_alpha_min=" + String(ia_min) + " inv_alpha_max=" + String(ia_max) + " reg_min=" + String(r_min) + " reg_max=" + String(r_max) + " mul_factor=0.9900");

				// Run the snake
				imp2 = IJ.run(imp, "ABSnake", "gradient_threshold=5 number_of_iterations=" + 50 + " step_result_show=1 draw_color=Green");

				// required for Close
				IJ.selectWindow("Draw");
				
				IJ.saveAs("Png", "/home/anej001/micro/run/10_0116/frame_0117-a" + ia_min + "-" + ia_max + "r" + r_min + "-" + r_max + ".png");
				
				// Close window to avoid excessive clutter
				IJ.run("Close");
			//}
		//}
	}
}
