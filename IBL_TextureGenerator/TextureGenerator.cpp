#include "TextureGenerator.h"

void TextureGenerator::Init()
{
	raw_data_.resize(static_cast<uint32_t>(width_ * height_));
}

void TextureGenerator::SaveTextureInFile(std::string file_name)
{
	std::vector<char> result_buffer(3 * raw_data_.size());
	int char_cnt = 0;
	for (auto pixel : raw_data_)
	{
		
	}
	//for (int j = ny - 1; j >= 0; j--) {
	//	for (int i = 0; i < nx; i++) {
	//		Vec3 col(0, 0, 0);
	//		for (int s = 0; s < ns; s++)
	//		{
	//			float u = float(i + (double)rand() / RAND_MAX) / float(nx);
	//			float v = float(j + (double)rand() / RAND_MAX) / float(ny);
	//			Ray r = cam.GetRay(u, v);
	//			r.direction_.normalize();
	//			col += DeNAN(GetColorByRay(r, world, important_hitables, 0));
	//		}
	//		col /= float(ns);
	//		result_buffer[char_cnt++] = char(255.99 * std::min(sqrt(col[0]), 1.0f));
	//		result_buffer[char_cnt++] = char(255.99 * std::min(sqrt(col[1]), 1.0f));
	//		result_buffer[char_cnt++] = char(255.99 * std::min(sqrt(col[2]), 1.0f));
	//	}
	//}
	//std::stringstream file_name;
	//std::cout << time(0) - start_time << std::endl;
	//file_name << "result/temp/" << "test_res.png";
	stbi_write_png(file_name.c_str(), width_, height_, 3, result_buffer.data(), 0);
}
