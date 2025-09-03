#pragma once

#include <vector>
#include <array>
#include <memory>
#include <string>
#include <cstdint>


#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#include <glad/glad.h>

// Forward declarations
struct cgltf_data;
struct cgltf_node;
struct cgltf_mesh;
struct cgltf_primitive;

// Vertex structure for triangle data
struct Vertex {
	float position[3];
	float normal[3];
	float texCoord[2];
};

// Triangle structure for BVH
struct Triangle {
	Vertex v0, v1, v2;
	uint32_t materialIndex;

	static inline bool finite3(const float p[3]) {
		return std::isfinite(p[0]) && std::isfinite(p[1]) && std::isfinite(p[2]);
	}

	void getBounds(float minBounds[3], float maxBounds[3]) const {
		if (!finite3(v0.position) || !finite3(v1.position) || !finite3(v2.position)) {
			for (int i = 0; i < 3; i++) {
				minBounds[i] = FLT_MAX;
				maxBounds[i] = -FLT_MAX;
			}
			return;
		}
		for (int i = 0; i < 3; i++) {
			minBounds[i] = std::min({ v0.position[i], v1.position[i], v2.position[i] });
			maxBounds[i] = std::max({ v0.position[i], v1.position[i], v2.position[i] });
		}
	}

	void getCentroid(float centroid[3]) const {
		for (int i = 0; i < 3; i++) {
			centroid[i] = (v0.position[i] + v1.position[i] + v2.position[i]) / 3.0f;
		}
	}
};

// Material structure
struct Material {
	float albedo[3] = { 0.8f, 0.8f, 0.8f };
	float emissive[3] = { 0.0f, 0.0f, 0.0f };
	float specularChance = 0.02f;
	float specularRoughness = 0.5f;
	float specularColor[3] = { 1.0f, 1.0f, 1.0f };
	float IOR = 1.0f;
	float refractionChance = 0.0f;
	float refractionRoughness = 0.0f;
	float refractionColor[3] = { 0.0f, 0.0f, 0.0f };
};

// BVH Node structure (GPU-friendly)
struct BVHNode {
	float minBounds[3];
	uint32_t leftChild;
	float maxBounds[3];
	uint32_t triangleCount;
	uint32_t triangleOffset;
	uint32_t padding[3];

	BVHNode() {
		minBounds[0] = minBounds[1] = minBounds[2] = 0.0f;
		maxBounds[0] = maxBounds[1] = maxBounds[2] = 0.0f;
		leftChild = 0;
		triangleCount = 0;
		triangleOffset = 0;
		padding[0] = padding[1] = padding[2] = 0;
	}
};

class GLTFLoader {
private:
	std::vector<Triangle> triangles;
	std::vector<Material> materials;
	std::vector<BVHNode> bvhNodes;
	std::vector<uint32_t> triangleIndices;

	GLuint bvhBuffer = 0;
	GLuint triangleBuffer = 0;
	GLuint materialBuffer = 0;

public:
	~GLTFLoader() {
		cleanup();
	}

	bool loadGLTF(const std::string& filename);
	void buildBVH();
	void uploadToGPU();
	void cleanup();

	// Getters
	size_t getTriangleCount() const { return triangles.size(); }
	size_t getBVHNodeCount() const { return bvhNodes.size(); }
	size_t getMaterialCount() const { return materials.size(); }

	GLuint getBVHBuffer() const { return bvhBuffer; }
	GLuint getTriangleBuffer() const { return triangleBuffer; }
	GLuint getMaterialBuffer() const { return materialBuffer; }

private:
	void processNode(cgltf_data* data, cgltf_node* node, const float* parentTransform = nullptr);
	void processMesh(cgltf_data* data, cgltf_mesh* mesh, const float* transform);
	void processPrimitive(cgltf_data* data, cgltf_primitive* primitive, const float* transform, uint32_t materialIndex);
	void loadMaterials(cgltf_data* data);

	// BVH construction
	uint32_t buildBVHRecursive(std::vector<uint32_t>& triangleIndices, uint32_t start, uint32_t end, int depth = 0);
	void calculateBounds(const std::vector<uint32_t>& triangleIndices, uint32_t start, uint32_t end,
		float minBounds[3], float maxBounds[3]);
	int findBestSplit(const std::vector<uint32_t>& triangleIndices, uint32_t start, uint32_t end);

	// Utility functions
	void multiplyMatrix4(const float* a, const float* b, float* result);
	void transformVertex(const float* vertex, const float* matrix, float* result);
};

// Implementation
bool GLTFLoader::loadGLTF(const std::string& filename) {
	// You'll need to link against cgltf library
	// This is a simplified version - you'd need proper error handling

	cgltf_options options = {};
	cgltf_data* data = nullptr;
	cgltf_result result = cgltf_parse_file(&options, filename.c_str(), &data);

	if (result != cgltf_result_success) {
		return false;
	}

	// Clear existing data
	triangles.clear();
	materials.clear();

	// Load materials first
	loadMaterials(data);

	// Process the scene
	if (data->scenes_count > 0) {
		cgltf_scene* scene = &data->scenes[0];
		for (size_t i = 0; i < scene->nodes_count; i++) {
			processNode(data, scene->nodes[i]);
		}
	}

	cgltf_free(data);
	return true;
}

void GLTFLoader::processNode(cgltf_data* data, cgltf_node* node, const float* parentTransform) {
	// Calculate node transform
	float nodeTransform[16];
	if (node->has_matrix) {
		memcpy(nodeTransform, node->matrix, sizeof(float) * 16);
	}
	else {
		// Build transform from TRS
		// This is simplified - you'd want a proper matrix library
		// For now, assume identity if no matrix
		float identity[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
		memcpy(nodeTransform, identity, sizeof(nodeTransform));
	}

	// Combine with parent transform
	float finalTransform[16];
	if (parentTransform) {
		multiplyMatrix4(parentTransform, nodeTransform, finalTransform);
	}
	else {
		memcpy(finalTransform, nodeTransform, sizeof(finalTransform));
	}

	// Process mesh if present
	if (node->mesh) {
		processMesh(data, node->mesh, finalTransform);
	}

	// Process children
	for (size_t i = 0; i < node->children_count; i++) {
		processNode(data, node->children[i], finalTransform);
	}
}

void GLTFLoader::processMesh(cgltf_data* data, cgltf_mesh* mesh, const float* transform) {
	for (size_t i = 0; i < mesh->primitives_count; i++) {
		cgltf_primitive* primitive = &mesh->primitives[i];
		uint32_t materialIndex = primitive->material ?
			(uint32_t)(primitive->material - data->materials) : 0;
		processPrimitive(data, primitive, transform, materialIndex);
	}
}

void GLTFLoader::processPrimitive(cgltf_data* data, cgltf_primitive* primitive,
	const float* transform, uint32_t materialIndex) {
	// Only handle triangles for now
	if (primitive->type != cgltf_primitive_type_triangles) {
		return;
	}

	// Get position accessor
	cgltf_accessor* positionAccessor = nullptr;
	cgltf_accessor* normalAccessor = nullptr;
	cgltf_accessor* texCoordAccessor = nullptr;

	for (size_t i = 0; i < primitive->attributes_count; i++) {
		if (primitive->attributes[i].type == cgltf_attribute_type_position) {
			positionAccessor = primitive->attributes[i].data;
		}
		else if (primitive->attributes[i].type == cgltf_attribute_type_normal) {
			normalAccessor = primitive->attributes[i].data;
		}
		else if (primitive->attributes[i].type == cgltf_attribute_type_texcoord) {
			texCoordAccessor = primitive->attributes[i].data;
		}
	}

	if (!positionAccessor) return;

	// Get index accessor
	cgltf_accessor* indexAccessor = primitive->indices;

	if (indexAccessor) {
		// Indexed geometry
		for (size_t i = 0; i < indexAccessor->count; i += 3) {
			Triangle tri;
			tri.materialIndex = materialIndex;

			// Get indices
			uint32_t indices[3];
			// This is simplified - you'd need proper accessor reading
			// cgltf_accessor_read_uint(indexAccessor, i, indices, 3);

			// For each vertex of the triangle
			for (int v = 0; v < 3; v++) {
				Vertex* vertex = (v == 0) ? &tri.v0 : (v == 1) ? &tri.v1 : &tri.v2;

				// Read position
				if (positionAccessor) {
					float pos[3];
					cgltf_accessor_read_float(positionAccessor, indices[v], pos, 3);
					transformVertex(pos, transform, vertex->position);
				}

				// Read normal
				if (normalAccessor) {
					cgltf_accessor_read_float(normalAccessor, indices[v], vertex->normal, 3);
					// Transform normal (inverse transpose)
				}

				// Read texture coordinates
				if (texCoordAccessor) {
					cgltf_accessor_read_float(texCoordAccessor, indices[v], vertex->texCoord, 2);
				}
			}

			triangles.push_back(tri);
		}
	}
}

void GLTFLoader::loadMaterials(cgltf_data* data) {
	materials.resize(std::max((size_t)1, data->materials_count));

	for (size_t i = 0; i < data->materials_count; i++) {
		cgltf_material* gltfMat = &data->materials[i];
		Material& mat = materials[i];

		// Load PBR metallic roughness
		if (gltfMat->has_pbr_metallic_roughness) {
			auto& pbr = gltfMat->pbr_metallic_roughness;
			mat.albedo[0] = pbr.base_color_factor[0];
			mat.albedo[1] = pbr.base_color_factor[1];
			mat.albedo[2] = pbr.base_color_factor[2];

			mat.specularRoughness = pbr.roughness_factor;
			// Convert metallic to specular chance (simplified)
			mat.specularChance = pbr.metallic_factor;
		}

		// Load emissive
		if (gltfMat->has_emissive_strength) {
			mat.emissive[0] = gltfMat->emissive_factor[0];
			mat.emissive[1] = gltfMat->emissive_factor[1];
			mat.emissive[2] = gltfMat->emissive_factor[2];
		}
	}
}

void GLTFLoader::buildBVH() {
	if (triangles.empty()) return;

	bvhNodes.clear();
	triangleIndices.clear();

	// Reserve memory to prevent reallocations during recursive construction
	// Worst case: 2 * triangleCount - 1 nodes for a binary tree
	bvhNodes.reserve(2 * triangles.size());

	// Initialize triangle indices
	triangleIndices.resize(triangles.size());
	for (size_t i = 0; i < triangles.size(); i++) {
		triangleIndices[i] = (uint32_t)i;
	}

	// Build BVH recursively
	buildBVHRecursive(triangleIndices, 0, (uint32_t)triangles.size());
}

uint32_t GLTFLoader::buildBVHRecursive(std::vector<uint32_t>& indices, uint32_t start, uint32_t end, int depth) {
	uint32_t nodeIndex = (uint32_t)bvhNodes.size();
	bvhNodes.emplace_back();

	// Calculate bounds - access node after potential reallocation
	calculateBounds(indices, start, end, bvhNodes[nodeIndex].minBounds, bvhNodes[nodeIndex].maxBounds);

	uint32_t triangleCount = end - start;

	// Leaf node criteria
	if (triangleCount <= 4 || depth > 20) {
		bvhNodes[nodeIndex].leftChild = 0; // Mark as leaf
		bvhNodes[nodeIndex].triangleCount = triangleCount;
		bvhNodes[nodeIndex].triangleOffset = start;
		return nodeIndex;
	}

	// Find best split
	int bestAxis = findBestSplit(indices, start, end);

	// Sort triangles along best axis
	std::sort(indices.begin() + start, indices.begin() + end,
		[&](uint32_t a, uint32_t b) {
		float centroidA[3], centroidB[3];
		triangles[a].getCentroid(centroidA);
		triangles[b].getCentroid(centroidB);
		return centroidA[bestAxis] < centroidB[bestAxis];
	});

	uint32_t mid = start + triangleCount / 2;

	// Build children - these calls may reallocate bvhNodes vector!
	uint32_t leftChild = buildBVHRecursive(indices, start, mid, depth + 1);
	uint32_t rightChild = buildBVHRecursive(indices, mid, end, depth + 1);

	// CRITICAL: Don't use node reference after recursive calls!
	// Access by index instead since vector may have been reallocated
	bvhNodes[nodeIndex].leftChild = leftChild + 1; // Store as 1-based index (0 means leaf)
	bvhNodes[nodeIndex].triangleCount = rightChild; // Store right child index in triangleCount for internal nodes

	return nodeIndex;
}

void GLTFLoader::calculateBounds(const std::vector<uint32_t>& indices, uint32_t start, uint32_t end,
	float minBounds[3], float maxBounds[3]) {
	if (start >= end || start >= triangles.size()) {
		// Handle edge case with default bounds
		minBounds[0] = minBounds[1] = minBounds[2] = 0.0f;
		maxBounds[0] = maxBounds[1] = maxBounds[2] = 0.0f;
		return;
	}

	minBounds[0] = minBounds[1] = minBounds[2] = FLT_MAX;
	maxBounds[0] = maxBounds[1] = maxBounds[2] = -FLT_MAX;

	for (uint32_t i = start; i < end; i++) {
		if (i >= indices.size() || indices[i] >= triangles.size()) {
			printf("Warning: Invalid triangle index %u at position %u\n",
				i < indices.size() ? indices[i] : 999999, i);
			continue;
		}

		float triMin[3], triMax[3];
		triangles[indices[i]].getBounds(triMin, triMax);

		for (int axis = 0; axis < 3; axis++) {
			minBounds[axis] = std::min(minBounds[axis], triMin[axis]);
			maxBounds[axis] = std::max(maxBounds[axis], triMax[axis]);
		}
	}

	// Debug output for first few nodes
	static int debugCount = 0;
	if (debugCount < 5) {
		printf("Bounds[%d]: min(%.2f, %.2f, %.2f) max(%.2f, %.2f, %.2f) triangles=%u-%u\n",
			debugCount, minBounds[0], minBounds[1], minBounds[2],
			maxBounds[0], maxBounds[1], maxBounds[2], start, end);
		debugCount++;
	}
}

int GLTFLoader::findBestSplit(const std::vector<uint32_t>& indices, uint32_t start, uint32_t end) {
	// Simple approach: choose the axis with largest extent
	float extent[3] = { 0, 0, 0 };
	float minBounds[3], maxBounds[3];
	calculateBounds(indices, start, end, minBounds, maxBounds);

	int bestAxis = 0;
	for (int axis = 0; axis < 3; axis++) {
		extent[axis] = maxBounds[axis] - minBounds[axis];
		if (extent[axis] > extent[bestAxis]) {
			bestAxis = axis;
		}
	}

	return bestAxis;
}

void GLTFLoader::uploadToGPU() {
	// Upload BVH nodes
	if (!bvhNodes.empty()) {
		if (bvhBuffer == 0) {
			glGenBuffers(1, &bvhBuffer);
		}
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bvhBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, bvhNodes.size() * sizeof(BVHNode),
			bvhNodes.data(), GL_STATIC_DRAW);
	}

	// Upload triangles (reordered by BVH)
	if (!triangles.empty()) {
		if (triangleBuffer == 0) {
			glGenBuffers(1, &triangleBuffer);
		}

		// Reorder triangles according to BVH
		std::vector<Triangle> reorderedTriangles(triangles.size());
		for (size_t i = 0; i < triangleIndices.size(); i++) {
			reorderedTriangles[i] = triangles[triangleIndices[i]];
		}

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, triangleBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, reorderedTriangles.size() * sizeof(Triangle),
			reorderedTriangles.data(), GL_STATIC_DRAW);
	}

	// Upload materials
	if (!materials.empty()) {
		if (materialBuffer == 0) {
			glGenBuffers(1, &materialBuffer);
		}
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, materials.size() * sizeof(Material),
			materials.data(), GL_STATIC_DRAW);
	}
}

void GLTFLoader::cleanup() {
	if (bvhBuffer != 0) {
		glDeleteBuffers(1, &bvhBuffer);
		bvhBuffer = 0;
	}
	if (triangleBuffer != 0) {
		glDeleteBuffers(1, &triangleBuffer);
		triangleBuffer = 0;
	}
	if (materialBuffer != 0) {
		glDeleteBuffers(1, &materialBuffer);
		materialBuffer = 0;
	}
}

// Utility functions
void GLTFLoader::multiplyMatrix4(const float* a, const float* b, float* result) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result[i * 4 + j] = 0;
			for (int k = 0; k < 4; k++) {
				result[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
			}
		}
	}
}

void GLTFLoader::transformVertex(const float* vertex, const float* matrix, float* result) {
	float v[4] = { vertex[0], vertex[1], vertex[2], 1.0f };
	for (int i = 0; i < 3; i++) {
		result[i] = 0;
		for (int j = 0; j < 4; j++) {
			result[i] += matrix[i * 4 + j] * v[j];
		}
	}
}
