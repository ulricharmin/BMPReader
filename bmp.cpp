#include <iostream>
#include <fstream>

#include "BMP.h"

BMP::BMP(int32_t width, int32_t height, bool has_alpha) {

}

BMP::BMP(const char* fname) {
	read(fname);
}

void BMP::read(const char* fname) {
	std::ifstream inp{ fname, std::ios_base::binary };
	if (inp) {
		inp.read((char*)&file_header, sizeof(file_header));
		if (file_header.file_type != 0x4D42) { // 0x4D42 == BMP
			throw std::runtime_error("Error! Wrong file format.");
		}
		inp.read((char*)&bmp_info_header, sizeof(bmp_info_header));
		
		// The BMPColorHeader is only for transparent images (32 bit)
		if (bmp_info_header.bit_count == 32) {
			// Check if file has bitmask color infos
			if (bmp_info_header.size >= (sizeof(BMPInfoHeader) + sizeof(BMPColorHeader))) {
				inp.read((char*)&bmp_color_header, sizeof(bmp_color_header));
				// Check if the pixel data is stored as BGRA and if the color space type is sRGB
				check_color_header(bmp_color_header);
			}
			else {
				std::cerr << "Warning! The file \"" << fname << "\" does not seem to contain bit mask information" << std::endl;
				throw std::runtime_error("Error! Unrecognized file format.");
			}
		}

		// Jump to the pixel data location
		inp.seekg(file_header.offset_data, inp.beg); // move "get" pointer to pixel data. seekp moves "put" pointer. inp.beg says "offset relative to beginning of stream".
		
		// Adjust the header fields for output
		// Some editors will put extra info in the image file, we only save the headers and the data.
		if (bmp_info_header.bit_count == 32) {
			bmp_info_header.size = sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);
			file_header.offset_data = sizeof(BMPInfoHeader) + sizeof(BMPColorHeader) + sizeof(BMPFileHeader);
		}
		else {
			bmp_info_header.size = sizeof(BMPInfoHeader);
			file_header.offset_data = sizeof(BMPInfoHeader) + sizeof(BMPFileHeader);
		}
		file_header.file_size = file_header.offset_data;

		if (bmp_info_header.height < 0) {
			throw std::runtime_error("The program can only treat BMP images with the origin in the bottom left corner!");
		}

		data.resize(bmp_info_header.bit_count * bmp_info_header.width * bmp_info_header.height / 8);

		// Here we check if we need to take row padding into account
		// if yes, just add the pixel data to the headers
		if (bmp_info_header.width % 4 == 0) {
			inp.read((char*)data.data(), data.size());
			file_header.file_size += data.size();
		}
		//if not, 
		else {
			row_stride = bmp_info_header.width * bmp_info_header.bit_count / 8;
			uint32_t new_stride = make_stride_aligned(4);
			std::vector<uint8_t> padding_row(new_stride - row_stride);

			for (int y = 0; y < bmp_info_header.height; y++) {
				inp.read((char*)(data.data() + row_stride * y), row_stride);
				inp.read((char*)padding_row.data(), padding_row.size());
			}
			file_header.file_size += data.size() + bmp_info_header.height * padding_row.size();
		}
	}
	else {
		throw std::runtime_error("Unable to open the inpuit image file.");
	}
}

void BMP::write(const char* fname) {

}

void BMP::check_color_header(BMPColorHeader& bmp_color_header) {
	BMPColorHeader expected_color_header;
	if (expected_color_header.red_mask != bmp_color_header.red_mask ||
		expected_color_header.green_mask != bmp_color_header.green_mask ||
		expected_color_header.blue_mask != bmp_color_header.blue_mask ||
		expected_color_header.alpha_mask != bmp_color_header.alpha_mask) {
		throw std::runtime_error("Unexpected color mask format!. The program expects the pixel data to be in the BGRA format.");
	}
	if (expected_color_header.color_space_type != bmp_color_header.color_space_type) {
		throw std::runtime_error("Unexpected color space type! The program expects sRGB values.");
	}
}

uint32_t BMP::make_stride_aligned(uint32_t align_stride) {
	uint32_t new_stride = row_stride;
	while (new_stride % align_stride != 0) {
		new_stride++;
	}
	return new_stride;
}
