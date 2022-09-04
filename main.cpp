/*
* BMP reader / writer for 32bit and 24bit only!
* https://solarianprogrammer.com/2018/11/19/cpp-reading-writing-bmp-images/
*/

#include <iostream>

#include "BMP.h"

int main(int argc, const char* argv[]) {

	// Create a BMP image in memory:
	BMP bmp2(800, 400);

	// Edit pixel data
	bmp2.fill_region(50, 20, 100, 200, 0, 0, 255, 255);
	
	// Save the image:
	bmp2.write("test.bmp");


	BMP bmp3(800, 400, false);
	
	bmp3.fill_region(50, 20, 100, 200, 0, 0, 255, 255);
	
	// Save the image:
	bmp3.write("test_24.bmp");

	return 0;
}