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
		//const auto& index : shape.mesh.indices
		for (int i = 0; i < shape.mesh.indices.size();i+=3) {
			std::array<Vertex, 3> tmp_vertices;
			for (int j = i; j < i + 3; j++) {
				tmp_vertices[j - i].pos = {
					attrib.vertices[3 * shape.mesh.indices[j].vertex_index + 0],
					attrib.vertices[3 * shape.mesh.indices[j].vertex_index + 1],
					attrib.vertices[3 * shape.mesh.indices[j].vertex_index + 2]
				};
				tmp_vertices[j - i].normal = {
					attrib.normals[3 * shape.mesh.indices[j].normal_index + 0],
					attrib.normals[3 * shape.mesh.indices[j].normal_index + 1],
					attrib.normals[3 * shape.mesh.indices[j].normal_index + 2]
				};
				tmp_vertices[j - i].texCoord = {
					attrib.texcoords[2 * shape.mesh.indices[j].texcoord_index + 0],
					1.0f - attrib.texcoords[2 * shape.mesh.indices[j].texcoord_index + 1]
				};

				tmp_vertices[j - i].color = { 1.0f, 1.0f, 1.0f };
				//vertices_.push_back(tmp_vertices[j - i]);
			}

			CalculateTangents(tmp_vertices);
			for (int k = 0; k < 3; k++) {
				if (uniqueVertices.count(tmp_vertices[k]) == 0) {
					uniqueVertices[tmp_vertices[k]] = static_cast<uint32_t>(vertices_.size());
					vertices_.push_back(tmp_vertices[k]);
				}
				indices_.push_back(uniqueVertices[tmp_vertices[k]]);
			}
		}
	}


}

void VertexDataManager::CalculateTangents(std::array<Vertex, 3>& vertices) {
	for (int i = 0; i < 3; i++) {
		glm::vec2 uv_v1 = vertices[(i + 1) % 3].texCoord - vertices[i].texCoord;
		glm::vec2 uv_v2 = vertices[(i + 2) % 3].texCoord - vertices[i].texCoord;

		glm::vec3 pos_v1 = vertices[(i + 1) % 3].pos - vertices[i].pos;
		glm::vec3 pos_v2 = vertices[(i + 2) % 3].pos - vertices[i].pos;

		float f = 1.0f / (uv_v1.x * uv_v2.y - uv_v2.x * uv_v1.y);
		float u = uv_v2.y * f;
		float v = -uv_v1.y * f;

		vertices[i].tangent   = glm::normalize(u * pos_v1 + v * pos_v2);
		vertices[i].bitangent = glm::cross(vertices[i].tangent, vertices[i].normal);
	}
}