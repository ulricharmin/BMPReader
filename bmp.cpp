#include <iostream>
#include <fstream>
#include <stdexcept>

#include "BMP.h"

BMP::BMP(int32_t width, int32_t height, bool has_alpha) {
	if (width <= 0 || height <= 0) {
		throw std::runtime_error("The image width and height mumst be positive numbers.");
	}

	bmp_info_header.width = width;
	bmp_info_header.height = height;
	if (has_alpha) {
		bmp_info_header.size = sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);
		file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);

		bmp_info_header.bit_count = 32;
		bmp_info_header.compression = 3;
		row_stride = width * 4; // width in pixels times the color information (RGBA)
		data.resize(row_stride * height);
		file_header.file_size = file_header.offset_data + static_cast<uint32_t>(data.size());
	}
	else {
		bmp_info_header.size = sizeof(BMPInfoHeader);
		file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);

		bmp_info_header.bit_count = 24;
		bmp_info_header.compression = 0;
		row_stride = width * 3; // width in pixels times RGB
		data.resize(row_stride * height);
		uint32_t new_stride = make_stride_aligned(4);
		
		// add the padding to (header+pixel data)
		file_header.file_size = file_header.offset_data + static_cast<uint32_t>(data.size()) + bmp_info_header.height * (new_stride - row_stride);
	}
}

BMP::BMP(const char* fname) {
	read(fname);
}

void BMP::read(const char* fname) {
	std::ifstream inp{ fname, std::ios_base::binary };
	if (inp) {
		inp.read((char*)&file_header, sizeof(file_header)); // read file header data in file_header
		if (file_header.file_type != 0x4D42) { // 0x4D42 == MB in hex. BM (BitMap) in small endian
			throw std::runtime_error("Error! Wrong file format.");
		}
		inp.read((char*)&bmp_info_header, sizeof(bmp_info_header)); // read info header data in bmp_info_header
		
		// The BMPColorHeader is only for transparent images (32 bit)
		if (bmp_info_header.bit_count == 32) {
			// Check if file has bitmask color infos
			if (bmp_info_header.size >= (sizeof(BMPInfoHeader) + sizeof(BMPColorHeader))) { // if info header contains color header
				inp.read((char*)&bmp_color_header, sizeof(bmp_color_header)); // read color header data into bmp_color_header
				// Check if the pixel data is stored as BGRA and if the color space type is sRGB
				check_color_header(bmp_color_header);
			}
			else {
				// 32 bit file must contain color header
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
			// color header is only needed for 32/16 bit (we only have 32bit).
			file_header.offset_data = sizeof(BMPInfoHeader) + sizeof(BMPFileHeader);
		}
		// current file size should only be the offset_data (header). Pixel data size will be added to file_size later on.
		file_header.file_size = file_header.offset_data;

		if (bmp_info_header.height < 0) {
			throw std::runtime_error("The program can only treat BMP images with the origin in the bottom left corner!");
		}

		// resize data vector to fit pixel data size (bytes).
		data.resize(bmp_info_header.width * bmp_info_header.height * bmp_info_header.bit_count / 8);

		// Here we check if we need to take row padding into account
		// if not, just add the pixel data to the headers
		if (bmp_info_header.width % 4 == 0) {
			inp.read((char*)data.data(), data.size());
			file_header.file_size += static_cast<uint32_t>(data.size());
		}
		else {
			// pixel data of one row in bytes
			row_stride = bmp_info_header.width * bmp_info_header.bit_count / 8;

			// calculating the padding needed.
			uint32_t new_stride = make_stride_aligned(4);
			std::vector<uint8_t> padding_row(new_stride - row_stride);

			// for each line of the image...
			for (int y = 0; y < bmp_info_header.height; ++y) {
				// read raw pixel data (no padding) into data vector.
				inp.read((char*)(data.data() + row_stride * y), row_stride);
				inp.read((char*)padding_row.data(), padding_row.size());
			}
			// add pixel data size plus the padding to file_size
			file_header.file_size += static_cast<uint32_t>(data.size()) + bmp_info_header.height * static_cast<uint32_t>(padding_row.size());
		}
	}
	else {
		throw std::runtime_error("Unable to open the inpuit image file.");
	}
}

void BMP::write(const char* fname) {
	std::ofstream of{ fname, std::ios_base::binary };
	if (of) {
		if (bmp_info_header.bit_count == 32) {
			write_headers_and_data(of);
		}
		else if (bmp_info_header.bit_count == 24) {
			if (bmp_info_header.width % 4 == 0) {
				write_headers_and_data(of);
			}
			else {
				uint32_t new_stride = make_stride_aligned(4);
				std::vector<uint8_t> padding_row(new_stride - row_stride);

				write_headers(of);

				for (int y = 0; y < bmp_info_header.height; ++y) {
					of.write((const char*)(data.data() + row_stride * y), row_stride);
					of.write((const char*)padding_row.data(), padding_row.size());
				}
			}
		}
		else {
			throw std::runtime_error("The program can treat only 24 or 32 bits per pixel BMP files");
		}
	}
	else {
		throw std::runtime_error("Unable to open the output image file.");
	}
}

void BMP::fill_region(uint32_t x0, uint32_t y0, uint32_t w, uint32_t h, uint8_t B, uint8_t G, uint32_t R, uint8_t A) {
	if (x0 + w > static_cast<uint32_t>(bmp_info_header.width) || y0 + h > static_cast<uint32_t>(bmp_info_header.height)) {
		throw std::runtime_error("The region does not fit in the image!");
	}

	uint32_t channels = bmp_info_header.bit_count / 8;
	for (uint32_t y = y0; y < y0 + h; ++y) {
		for (uint32_t x = x0; x < x0 + w; ++x) {
			data[channels * (y * bmp_info_header.width + x) + 0] = B;
			data[channels * (y * bmp_info_header.width + x) + 1] = G;
			data[channels * (y * bmp_info_header.width + x) + 2] = R;
			if (channels == 4) {
				data[channels * (y * bmp_info_header.width + x) + 3] = A;
			}
		}
	}
}

void BMP::check_color_header(BMPColorHeader& bmp_color_header) {
	BMPColorHeader expected_color_header;
	if (expected_color_header.red_mask != bmp_color_header.red_mask ||
		expected_color_header.blue_mask != bmp_color_header.blue_mask ||
		expected_color_header.green_mask != bmp_color_header.green_mask ||
		expected_color_header.alpha_mask != bmp_color_header.alpha_mask) {
		throw std::runtime_error("Unexpected color mask format! The program expects the pixel data to be in the BGRA format");
	}
	if (expected_color_header.color_space_type != bmp_color_header.color_space_type) {
		throw std::runtime_error("Unexpected color space type! The program expects sRGB values");
	}
}

void BMP::write_headers(std::ofstream& of) {
	of.write((const char*)&file_header, sizeof(file_header));
	of.write((const char*)&bmp_info_header, sizeof(bmp_info_header));
	if (bmp_info_header.bit_count == 32) {
		of.write((const char*)&bmp_color_header, sizeof(bmp_color_header));
	}
}

void BMP::write_headers_and_data(std::ofstream& of) {
	write_headers(of);
	of.write((const char*)data.data(), data.size());
}

uint32_t BMP::make_stride_aligned(uint32_t align_stride) {
	uint32_t new_stride = row_stride;
	while (new_stride % align_stride != 0) {
		new_stride++;
	}
	return new_stride;
}
