#pragma once

#include <vector>

#pragma pack(push, 1) // remove padding between members in memory. usually struct adds padding to align the variables properly in ram.
struct BMPFileHeader {
	uint16_t file_type{ 0x4D42 }; // Always BitMap which is 0x4D42
	uint32_t file_size{ 0 }; // Size of file in bytes
	uint16_t reserved1{ 0 }; //always 0
	uint16_t reserved2{ 0 }; //always 0
	uint32_t offset_data{ 0 }; // start/position of pixel data (bytes from beginning of the file)	
}; // with padding the struct needs 16 bytes. without only 14 bytes.

struct BMPInfoHeader {
	uint32_t size{ 0 }; // Size of this header in bytes
	int32_t width{ 0 }; // width of bitmap in pixels
	int32_t height{ 0 }; // height of bitmap in pixels
	
	uint16_t planes{ 1 }; // number of color planes for the target device. always 1
	uint16_t bit_count{ 0 }; // number of bits per pixel
	uint32_t compression{ 0 }; // compression method. 0 (uncompressed. mainly for 24 bit bitmaps) or 3 (bitfield encoding was used. if bitmap is 16 or 32 bit only 3 works) - THIS PROGRAM CONSIDERS ONLY UNCOMPRESSED BMP images
	int32_t x_pixels_per_meter{ 0 };
	int32_t y_pixels_per_meter{ 0 };
	uint32_t colors_used{ 0 }; // number of color indexes in color table. 0 for max num of colors allowed by bit_count
	uint32_t colors_important{ 0 }; // number of colors used for displaying the bitmap. if 0 all colors are required (equally important/frequent)
}; // 36 byte without padding

struct BMPColorHeader {
	// only for images with transparency (here: 32 bit)
	uint32_t red_mask{ 0x00ff0000 }; // bitmask for red channel
	uint32_t green_mask{ 0x0000ff00 }; // bitmask for green channel
	uint32_t blue_mask{ 0x000000ff }; // bitmask for blue channel
	uint32_t alpha_mask{ 0xff000000 }; // bitmask for alpha channel
	uint32_t color_space_type{ 0x73524742 }; // default sRGB (0x73524742)
	uint32_t unused[16]{ 0 }; // unused data for sRGB color space
}; // 24 byte without padding
#pragma pack(pop)

struct BMP {
	BMPFileHeader file_header;
	BMPInfoHeader bmp_info_header;
	BMPColorHeader bmp_color_header;
	std::vector<uint8_t> data;

	BMP(int32_t width, int32_t height, bool has_alpha = true);

	BMP(const char* fname);

	void read(const char* fname);

	void write(const char* fname);

private:
	uint32_t row_stride{ 0 };

	void check_color_header(BMPColorHeader &bmp_color_header);

	uint32_t make_stride_aligned(uint32_t align_stride);
};
