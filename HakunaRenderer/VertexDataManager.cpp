#include "VertexDataManager.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>


VertexDataManager::VertexDataManager()
{
}


VertexDataManager::~VertexDataManager()
{
}

void VertexDataManager::LoadModelFromFile(std::string model_path) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str())) {
		throw std::runtime_error(warn + err);
	}
	std::unordered_map<Vertex, uint32_t> uniqueVertices = {};
	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex = {};
			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = { 1.0f, 1.0f, 1.0f };
			vertices_.push_back(vertex);


			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices_.size());
				vertices_.push_back(vertex);
			}
			indices_.push_back(uniqueVertices[vertex]);
		}
	}
}