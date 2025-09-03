#pragma once
#include <vector>
#include <array>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <float.h>
#include <cmath>

// Vertex structure for triangle data
struct Vertex {
	float position[3];
	float pad0;
	float normal[3];
	float pad1;
	float texCoord[2];
	float pad2[2];

	// Constructor
	Vertex() {
		position[0] = position[1] = position[2] = 0.0f;
		normal[0] = normal[1] = normal[2] = 0.0f;
		texCoord[0] = texCoord[1] = 0.0f;
		pad0 = pad1 = 0.0f;
		pad2[0] = 0.0f;
		pad2[1] = 0.0f;
	}
};

// Triangle structure for BVH
struct Triangle {
	Vertex v0, v1, v2;
	uint32_t materialIndex;
	uint32_t padding[3];

	Triangle() : materialIndex(0) {
		padding[0] = padding[1] = padding[2] = 0;
	}

	// Calculate bounding box
	void getBounds(float minBounds[3], float maxBounds[3]) const {
		for (int i = 0; i < 3; i++) {
			minBounds[i] = std::min({ v0.position[i], v1.position[i], v2.position[i] });
			maxBounds[i] = std::max({ v0.position[i], v1.position[i], v2.position[i] });
		}
	}

	// Calculate centroid
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
	uint32_t leftChild;    // If 0, this is a leaf node
	float maxBounds[3];
	uint32_t triangleCount; // For leaf nodes: number of triangles, for internal: right child
	uint32_t triangleOffset; // For leaf nodes: offset into triangle array
	uint32_t padding[3];     // Align to 64 bytes for GPU

							 // Constructor to initialize all values
	BVHNode() {
		minBounds[0] = minBounds[1] = minBounds[2] = 0.0f;
		maxBounds[0] = maxBounds[1] = maxBounds[2] = 0.0f;
		leftChild = 0;
		triangleCount = 0;
		triangleOffset = 0;
		padding[0] = padding[1] = padding[2] = 0;
	}
};

class OBJLoader {
private:
	std::vector<Triangle> triangles;
	std::vector<Material> materials;
	std::vector<BVHNode> bvhNodes;
	std::vector<uint32_t> triangleIndices; // For BVH leaf nodes

										   // Temporary storage during OBJ parsing
	std::vector<std::array<float, 3>> vertices;
	std::vector<std::array<float, 3>> normals;
	std::vector<std::array<float, 2>> texCoords;

	// GPU buffers
	GLuint bvhBuffer = 0;
	GLuint triangleBuffer = 0;
	GLuint materialBuffer = 0;

public:
	~OBJLoader() {
		cleanup();
	}

	bool loadOBJ(const std::string& filename);
	bool loadMTL(const std::string& filename);
	void buildBVH();
	void uploadToGPU();
	void cleanup();

	// Utility functions
	void normalizeSize();
	void centerAtOrigin();
	void getBoundingBox(float minBounds[3], float maxBounds[3]) const;
	void transform(const float* matrix);

	// Getters
	size_t getTriangleCount() const { return triangles.size(); }
	size_t getBVHNodeCount() const { return bvhNodes.size(); }
	size_t getMaterialCount() const { return materials.size(); }

	GLuint getBVHBuffer() const { return bvhBuffer; }
	GLuint getTriangleBuffer() const { return triangleBuffer; }
	GLuint getMaterialBuffer() const { return materialBuffer; }

private:
	// BVH construction
	uint32_t buildBVHRecursive(std::vector<uint32_t>& triangleIndices, uint32_t start, uint32_t end, int depth = 0);
	void calculateBounds(const std::vector<uint32_t>& triangleIndices, uint32_t start, uint32_t end,
		float minBounds[3], float maxBounds[3]);
	int findBestSplit(const std::vector<uint32_t>& triangleIndices, uint32_t start, uint32_t end);

	// Utility functions
	void multiplyMatrix4(const float* a, const float* b, float* result);
	void transformVertex(const float* vertex, const float* matrix, float* result);
	std::string trim(const std::string& str);
	std::vector<std::string> split(const std::string& str, char delimiter);
};

// Implementation
bool OBJLoader::loadOBJ(const std::string& filename) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "Failed to open OBJ file: " << filename << std::endl;
		return false;
	}

	// Clear existing data
	vertices.clear();
	normals.clear();
	texCoords.clear();
	triangles.clear();
	materials.clear();

	// Add default material
	materials.emplace_back();
	uint32_t currentMaterial = 0;

	std::string line;
	while (std::getline(file, line)) {
		line = trim(line);
		if (line.empty() || line[0] == '#') continue;

		std::istringstream iss(line);
		std::string prefix;
		iss >> prefix;

		if (prefix == "v") {
			// Vertex position
			std::array<float, 3> vertex;
			iss >> vertex[0] >> vertex[1] >> vertex[2];
			vertices.push_back(vertex);

		}
		else if (prefix == "vn") {
			// Vertex normal
			std::array<float, 3> normal;
			iss >> normal[0] >> normal[1] >> normal[2];
			normals.push_back(normal);

		}
		else if (prefix == "vt") {
			// Texture coordinate
			std::array<float, 2> texCoord;
			iss >> texCoord[0] >> texCoord[1];
			texCoords.push_back(texCoord);

		}
		else if (prefix == "f") {
			// Face - convert to triangles
			std::vector<std::string> faceVertices;
			std::string vertexData;

			while (iss >> vertexData) {
				faceVertices.push_back(vertexData);
			}

			// Triangulate the face (simple fan triangulation)
			for (size_t i = 1; i < faceVertices.size() - 1; i++) {
				Triangle tri;
				tri.materialIndex = currentMaterial;

				// Parse each vertex of the triangle
				Vertex* triVertices[3] = { &tri.v0, &tri.v1, &tri.v2 };
				std::string vertexStrings[3] = { faceVertices[0], faceVertices[i], faceVertices[i + 1] };

				for (int v = 0; v < 3; v++) {
					std::vector<std::string> indices = split(vertexStrings[v], '/');

					// Vertex position (required)
					if (!indices.empty() && !indices[0].empty()) {
						int vertexIndex = std::stoi(indices[0]) - 1; // OBJ is 1-indexed
						if (vertexIndex >= 0 && vertexIndex < (int)vertices.size()) {
							triVertices[v]->position[0] = vertices[vertexIndex][0];
							triVertices[v]->position[1] = vertices[vertexIndex][1];
							triVertices[v]->position[2] = vertices[vertexIndex][2];
						}
					}

					// Texture coordinate (optional)
					if (indices.size() > 1 && !indices[1].empty()) {
						int texIndex = std::stoi(indices[1]) - 1;
						if (texIndex >= 0 && texIndex < (int)texCoords.size()) {
							triVertices[v]->texCoord[0] = texCoords[texIndex][0];
							triVertices[v]->texCoord[1] = texCoords[texIndex][1];
						}
					}

					// Normal (optional)
					if (indices.size() > 2 && !indices[2].empty()) {
						int normalIndex = std::stoi(indices[2]) - 1;
						if (normalIndex >= 0 && normalIndex < (int)normals.size()) {
							triVertices[v]->normal[0] = normals[normalIndex][0];
							triVertices[v]->normal[1] = normals[normalIndex][1];
							triVertices[v]->normal[2] = normals[normalIndex][2];
						}
					}
				}

				// If no normals provided, calculate face normal
				if (normals.empty()) {
					std::array<float, 3> edge1 = {
						tri.v1.position[0] - tri.v0.position[0],
						tri.v1.position[1] - tri.v0.position[1],
						tri.v1.position[2] - tri.v0.position[2]
					};
					std::array<float, 3> edge2 = {
						tri.v2.position[0] - tri.v0.position[0],
						tri.v2.position[1] - tri.v0.position[1],
						tri.v2.position[2] - tri.v0.position[2]
					};

					// Cross product
					std::array<float, 3> normal = {
						edge1[1] * edge2[2] - edge1[2] * edge2[1],
						edge1[2] * edge2[0] - edge1[0] * edge2[2],
						edge1[0] * edge2[1] - edge1[1] * edge2[0]
					};

					// Normalize
					float length = sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
					if (length > 0.0f) {
						normal[0] /= length;
						normal[1] /= length;
						normal[2] /= length;
					}

					// Apply to all vertices
					for (int v = 0; v < 3; v++) {
						triVertices[v]->normal[0] = normal[0];
						triVertices[v]->normal[1] = normal[1];
						triVertices[v]->normal[2] = normal[2];
					}
				}

				triangles.push_back(tri);
			}

		}
		else if (prefix == "mtllib") {
			// Material library
			std::string mtlFile;
			iss >> mtlFile;

			// Try to load MTL file from same directory as OBJ
			size_t lastSlash = filename.find_last_of("/\\");
			std::string mtlPath = (lastSlash != std::string::npos) ?
				filename.substr(0, lastSlash + 1) + mtlFile : mtlFile;

			loadMTL(mtlPath);

		}
		else if (prefix == "usemtl") {
			// Use material
			std::string materialName;
			iss >> materialName;

			// For simplicity, just increment material index
			// In a full implementation, you'd maintain a name->index map
			currentMaterial = std::min((uint32_t)(materials.size() - 1), currentMaterial + 1);
		}
	}

	file.close();

	std::cout << "Loaded OBJ: " << vertices.size() << " vertices, "
		<< triangles.size() << " triangles" << std::endl;

	return !triangles.empty();
}

bool OBJLoader::loadMTL(const std::string& filename) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "Could not open MTL file: " << filename << std::endl;
		return false;
	}

	Material currentMaterial;
	std::string line;

	while (std::getline(file, line)) {
		line = trim(line);
		if (line.empty() || line[0] == '#') continue;

		std::istringstream iss(line);
		std::string prefix;
		iss >> prefix;

		if (prefix == "newmtl") {
			// New material - save current and reset
			if (materials.size() > 1) { // Don't overwrite default material
				materials.push_back(currentMaterial);
			}
			currentMaterial = Material(); // Reset to defaults

		}
		else if (prefix == "Kd") {
			// Diffuse color
			iss >> currentMaterial.albedo[0] >> currentMaterial.albedo[1] >> currentMaterial.albedo[2];

		}
		else if (prefix == "Ke") {
			// Emissive color
			iss >> currentMaterial.emissive[0] >> currentMaterial.emissive[1] >> currentMaterial.emissive[2];

		}
		else if (prefix == "Ns") {
			// Specular exponent -> roughness conversion
			float specularExponent;
			iss >> specularExponent;
			currentMaterial.specularRoughness = 1.0f - (specularExponent / 1000.0f);
			currentMaterial.specularRoughness = std::max(0.01f, std::min(1.0f, currentMaterial.specularRoughness));

		}
		else if (prefix == "Ks") {
			// Specular color
			iss >> currentMaterial.specularColor[0] >> currentMaterial.specularColor[1] >> currentMaterial.specularColor[2];

			// If specular color is non-zero, increase specular chance
			float specularMagnitude = currentMaterial.specularColor[0] +
				currentMaterial.specularColor[1] +
				currentMaterial.specularColor[2];
			if (specularMagnitude > 0.1f) {
				currentMaterial.specularChance = 0.1f;
			}
		}
	}

	// Add the last material
	materials.push_back(currentMaterial);

	file.close();
	std::cout << "Loaded MTL: " << materials.size() << " materials" << std::endl;
	return true;
}

void OBJLoader::buildBVH() {
	if (triangles.empty()) return;

	bvhNodes.clear();
	triangleIndices.clear();

	// Reserve memory to prevent reallocations during recursive construction
	bvhNodes.reserve(2 * triangles.size());

	// Initialize triangle indices
	triangleIndices.resize(triangles.size());
	for (size_t i = 0; i < triangles.size(); i++) {
		triangleIndices[i] = (uint32_t)i;
	}

	// Build BVH recursively
	buildBVHRecursive(triangleIndices, 0, (uint32_t)triangles.size());

	std::cout << "Built BVH: " << bvhNodes.size() << " nodes" << std::endl;
}

uint32_t OBJLoader::buildBVHRecursive(std::vector<uint32_t>& indices, uint32_t start, uint32_t end, int depth) {
	uint32_t nodeIndex = (uint32_t)bvhNodes.size();
	bvhNodes.emplace_back(); // This calls the constructor which initializes everything

	uint32_t triangleCount = end - start;

	// Calculate bounds first and store them
	float minBounds[3], maxBounds[3];
	calculateBounds(indices, start, end, minBounds, maxBounds);

	// Set bounds in the node
	for (int i = 0; i < 3; i++) {
		bvhNodes[nodeIndex].minBounds[i] = minBounds[i];
		bvhNodes[nodeIndex].maxBounds[i] = maxBounds[i];
	}

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

	// Build children
	uint32_t leftChild = buildBVHRecursive(indices, start, mid, depth + 1);
	uint32_t rightChild = buildBVHRecursive(indices, mid, end, depth + 1);

	// Set child pointers
	bvhNodes[nodeIndex].leftChild = leftChild; // Store as 1-based index (0 means leaf)
	bvhNodes[nodeIndex].triangleCount = rightChild; // Store right child index

	return nodeIndex;
}

void OBJLoader::calculateBounds(const std::vector<uint32_t>& indices, uint32_t start, uint32_t end,
	float minBounds[3], float maxBounds[3]) {
	minBounds[0] = minBounds[1] = minBounds[2] = FLT_MAX;
	maxBounds[0] = maxBounds[1] = maxBounds[2] = -FLT_MAX;

	for (uint32_t i = start; i < end; i++) {
		if (i >= indices.size() || indices[i] >= triangles.size()) continue;

		float triMin[3], triMax[3];
		triangles[indices[i]].getBounds(triMin, triMax);

		for (int axis = 0; axis < 3; axis++) {
			minBounds[axis] = std::min(minBounds[axis], triMin[axis]);
			maxBounds[axis] = std::max(maxBounds[axis], triMax[axis]);
		}
	}
}

int OBJLoader::findBestSplit(const std::vector<uint32_t>& indices, uint32_t start, uint32_t end) {
	float minBounds[3], maxBounds[3];
	calculateBounds(indices, start, end, minBounds, maxBounds);

	int bestAxis = 0;
	float maxExtent = maxBounds[0] - minBounds[0];

	for (int axis = 1; axis < 3; axis++) {
		float extent = maxBounds[axis] - minBounds[axis];
		if (extent > maxExtent) {
			maxExtent = extent;
			bestAxis = axis;
		}
	}

	return bestAxis;
}

void OBJLoader::uploadToGPU() {
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

	std::cout << "Uploaded to GPU: " << triangles.size() << " triangles, "
		<< bvhNodes.size() << " BVH nodes, " << materials.size() << " materials" << std::endl;
}

void OBJLoader::cleanup() {
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
void OBJLoader::getBoundingBox(float minBounds[3], float maxBounds[3]) const {
	if (triangles.empty()) {
		minBounds[0] = minBounds[1] = minBounds[2] = 0.0f;
		maxBounds[0] = maxBounds[1] = maxBounds[2] = 0.0f;
		return;
	}

	minBounds[0] = minBounds[1] = minBounds[2] = FLT_MAX;
	maxBounds[0] = maxBounds[1] = maxBounds[2] = -FLT_MAX;

	for (const auto& tri : triangles) {
		float triMin[3], triMax[3];
		tri.getBounds(triMin, triMax);

		for (int i = 0; i < 3; i++) {
			minBounds[i] = std::min(minBounds[i], triMin[i]);
			maxBounds[i] = std::max(maxBounds[i], triMax[i]);
		}
	}
}

void OBJLoader::centerAtOrigin() {
	float minBounds[3], maxBounds[3];
	getBoundingBox(minBounds, maxBounds);

	float center[3] = {
		(minBounds[0] + maxBounds[0]) * 0.5f,
		(minBounds[1] + maxBounds[1]) * 0.5f,
		(minBounds[2] + maxBounds[2]) * 0.5f
	};

	for (auto& tri : triangles) {
		for (int v = 0; v < 3; v++) {
			Vertex* vertex = (v == 0) ? &tri.v0 : (v == 1) ? &tri.v1 : &tri.v2;
			vertex->position[0] -= center[0];
			vertex->position[1] -= center[1] + 10.0f;
			vertex->position[2] -= center[2];
		}
	}
}

void OBJLoader::normalizeSize() {
	float minBounds[3], maxBounds[3];
	getBoundingBox(minBounds, maxBounds);

	float maxExtent = 0.0f;
	for (int i = 0; i < 3; i++) {
		maxExtent = std::max(maxExtent, maxBounds[i] - minBounds[i]);
	}

	if (maxExtent > 0.0f) {
		float scale = 2.0f / maxExtent; // Scale to fit in 2 unit cube

		for (auto& tri : triangles) {
			for (int v = 0; v < 3; v++) {
				Vertex* vertex = (v == 0) ? &tri.v0 : (v == 1) ? &tri.v1 : &tri.v2;
				vertex->position[0] *= scale;
				vertex->position[1] *= scale;
				vertex->position[2] *= scale;
			}
		}
	}
}

// String utility functions
std::string OBJLoader::trim(const std::string& str) {
	size_t first = str.find_first_not_of(' ');
	if (first == std::string::npos) return "";
	size_t last = str.find_last_not_of(' ');
	return str.substr(first, (last - first + 1));
}

std::vector<std::string> OBJLoader::split(const std::string& str, char delimiter) {
	std::vector<std::string> tokens;
	std::stringstream ss(str);
	std::string token;

	while (std::getline(ss, token, delimiter)) {
		tokens.push_back(token);
	}

	return tokens;
}