#pragma once
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <vector>
#include <string>
#include <gli/save.hpp>
struct Pixel
{
	double r;
	double g;
	double b;
};

class TextureGenerator
{
public:
	uint32_t width_;
	uint32_t height_;
private:
	std::vector<Pixel> raw_data_;

public:
	void Init();
	virtual void FillRawData() = 0;
	void SaveTextureInFile(std::string file_name);
};

