/*
* BMP reader / writer for 32bit and 24bit only!
* https://solarianprogrammer.com/2018/11/19/cpp-reading-writing-bmp-images/
*/

#include <iostream>

#include "BMP.h"

int main(int argc, const char* argv[]) {

	// Read an image from disk, modify it and write it back:
	BMP bmp("Shapes.bmp");
	bmp.write("Shapes_copy.bmp");

	// Create a BMP image in memory:
	BMP bmp2(800, 600);

	// Modify the pixel data:
	// ....

	// Save the image:
	bmp2.write("test.bmp");

	// Create a 24 bit per pixel image (BGR colors) and save it
	BMP bmp3(209, 203, false);
	// ...
	bmp3.write("test_24bits.bmp");

	return 0;
}