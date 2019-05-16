#pragma once
#include <vector>
#include <string>
#include <fstream>
class ShaderManager
{
public:
	ShaderManager();
	~ShaderManager();
	static std::vector<char> readFile(const std::string& filename);
};

