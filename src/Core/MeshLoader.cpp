#include "MeshLoader.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace VaporFrame {
namespace Core {

// Mesh methods
void Mesh::calculateBounds() {
    minBounds = glm::vec3(std::numeric_limits<float>::max());
    maxBounds = glm::vec3(std::numeric_limits<float>::lowest());
    
    for (const auto& submesh : submeshes) {
        for (const auto& vertex : submesh.vertices) {
            minBounds = glm::min(minBounds, vertex.position);
            maxBounds = glm::max(maxBounds, vertex.position);
        }
    }
}

void Mesh::calculateNormals() {
    for (auto& submesh : submeshes) {
        // Reset normals
        for (auto& vertex : submesh.vertices) {
            vertex.normal = glm::vec3(0.0f);
        }
        
        // Calculate face normals and accumulate
        for (size_t i = 0; i < submesh.indices.size(); i += 3) {
            if (i + 2 >= submesh.indices.size()) break;
            
            uint32_t i0 = submesh.indices[i];
            uint32_t i1 = submesh.indices[i + 1];
            uint32_t i2 = submesh.indices[i + 2];
            
            if (i0 >= submesh.vertices.size() || i1 >= submesh.vertices.size() || i2 >= submesh.vertices.size()) {
                continue;
            }
            
            glm::vec3 v0 = submesh.vertices[i0].position;
            glm::vec3 v1 = submesh.vertices[i1].position;
            glm::vec3 v2 = submesh.vertices[i2].position;
            
            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
            
            submesh.vertices[i0].normal += normal;
            submesh.vertices[i1].normal += normal;
            submesh.vertices[i2].normal += normal;
        }
        
        // Normalize accumulated normals
        for (auto& vertex : submesh.vertices) {
            if (glm::length(vertex.normal) > 0.0f) {
                vertex.normal = glm::normalize(vertex.normal);
            } else {
                vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f); // Default up normal
            }
        }
    }
}

void Mesh::optimize() {
    calculateBounds();
    calculateNormals();
    
    totalVertices = 0;
    totalIndices = 0;
    
    for (const auto& submesh : submeshes) {
        totalVertices += static_cast<uint32_t>(submesh.vertices.size());
        totalIndices += static_cast<uint32_t>(submesh.indices.size());
    }
}

// MeshLoader implementation
MeshLoader& MeshLoader::getInstance() {
    static MeshLoader instance;
    return instance;
}

std::shared_ptr<Mesh> MeshLoader::loadMesh(const std::string& filepath) {
    // Check cache first
    if (isCached(filepath)) {
        VF_LOG_DEBUG("Loading mesh from cache: {}", filepath);
        return meshCache[filepath];
    }
    
    clearLastError();
    
    if (!fileExists(filepath)) {
        lastError = "File does not exist: " + filepath;
        VF_LOG_ERROR("Failed to load mesh: {}", lastError);
        return nullptr;
    }
    
    std::string extension = getFileExtension(filepath);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    std::shared_ptr<Mesh> mesh;
    
    if (extension == ".obj") {
        mesh = loadOBJ(filepath);
    } else if (extension == ".ply") {
        mesh = loadPLY(filepath);
    } else {
        lastError = "Unsupported file format: " + extension;
        VF_LOG_ERROR("Failed to load mesh: {}", lastError);
        return nullptr;
    }
    
    if (mesh) {
        meshCache[filepath] = mesh;
        VF_LOG_INFO("Successfully loaded mesh: {} ({} vertices, {} indices)", 
                   filepath, mesh->totalVertices, mesh->totalIndices);
    }
    
    return mesh;
}

std::shared_ptr<Mesh> MeshLoader::loadOBJ(const std::string& filepath) {
    auto mesh = std::make_shared<Mesh>(getFilename(filepath));
    
    if (!parseOBJ(filepath, *mesh)) {
        return nullptr;
    }
    
    mesh->optimize();
    return mesh;
}

std::shared_ptr<Mesh> MeshLoader::loadPLY(const std::string& filepath) {
    auto mesh = std::make_shared<Mesh>(getFilename(filepath));
    
    if (!parsePLY(filepath, *mesh)) {
        return nullptr;
    }
    
    mesh->optimize();
    return mesh;
}

bool MeshLoader::parseOBJ(const std::string& filepath, Mesh& mesh) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        lastError = "Failed to open file: " + filepath;
        return false;
    }
    
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::string currentMaterial = "default";
    
    // Create default material
    Material defaultMat("default");
    mesh.materials.push_back(defaultMat);
    
    // Create default submesh
    Submesh defaultSubmesh("default");
    mesh.submeshes.push_back(defaultSubmesh);
    
    std::string line;
    uint32_t lineNumber = 0;
    
    while (std::getline(file, line)) {
        lineNumber++;
        line = trimString(line);
        
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        std::vector<std::string> tokens = splitString(line, ' ');
        if (tokens.empty()) continue;
        
        std::string command = tokens[0];
        
        if (command == "v") { // Vertex position
            if (tokens.size() >= 4) {
                glm::vec3 pos = parseVec3(tokens[1] + " " + tokens[2] + " " + tokens[3]);
                positions.push_back(pos);
            }
        } else if (command == "vn") { // Vertex normal
            if (tokens.size() >= 4) {
                glm::vec3 norm = parseVec3(tokens[1] + " " + tokens[2] + " " + tokens[3]);
                normals.push_back(norm);
            }
        } else if (command == "vt") { // Vertex texture coordinate
            if (tokens.size() >= 3) {
                glm::vec2 tex = parseVec2(tokens[1] + " " + tokens[2]);
                texCoords.push_back(tex);
            }
        } else if (command == "f") { // Face
            if (tokens.size() >= 4) {
                processOBJFace(line, positions, normals, texCoords, 
                              mesh.submeshes.back().vertices, mesh.submeshes.back().indices);
            }
        } else if (command == "usemtl") { // Use material
            if (tokens.size() >= 2) {
                currentMaterial = tokens[1];
                // Find or create material
                bool found = false;
                for (size_t i = 0; i < mesh.materials.size(); i++) {
                    if (mesh.materials[i].name == currentMaterial) {
                        mesh.submeshes.back().materialIndex = static_cast<uint32_t>(i);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    Material newMat(currentMaterial);
                    mesh.materials.push_back(newMat);
                    mesh.submeshes.back().materialIndex = static_cast<uint32_t>(mesh.materials.size() - 1);
                }
            }
        } else if (command == "mtllib") { // Material library
            if (tokens.size() >= 2) {
                std::string mtlPath = getDirectory(filepath) + "/" + tokens[1];
                parseMTL(mtlPath, mesh.materials);
            }
        }
    }
    
    file.close();
    return true;
}

bool MeshLoader::parseMTL(const std::string& filepath, std::vector<Material>& materials) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        VF_LOG_INFO("Failed to open material file: {}", filepath);
        return false;
    }
    
    Material* currentMaterial = nullptr;
    std::string line;
    
    while (std::getline(file, line)) {
        line = trimString(line);
        
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        std::vector<std::string> tokens = splitString(line, ' ');
        if (tokens.empty()) continue;
        
        std::string command = tokens[0];
        
        if (command == "newmtl") { // New material
            if (tokens.size() >= 2) {
                materials.emplace_back(tokens[1]);
                currentMaterial = &materials.back();
            }
        } else if (command == "Ka" && currentMaterial) { // Ambient
            if (tokens.size() >= 4) {
                currentMaterial->ambient = parseVec3(tokens[1] + " " + tokens[2] + " " + tokens[3]);
            }
        } else if (command == "Kd" && currentMaterial) { // Diffuse
            if (tokens.size() >= 4) {
                currentMaterial->diffuse = parseVec3(tokens[1] + " " + tokens[2] + " " + tokens[3]);
            }
        } else if (command == "Ks" && currentMaterial) { // Specular
            if (tokens.size() >= 4) {
                currentMaterial->specular = parseVec3(tokens[1] + " " + tokens[2] + " " + tokens[3]);
            }
        } else if (command == "Ns" && currentMaterial) { // Shininess
            if (tokens.size() >= 2) {
                currentMaterial->shininess = std::stof(tokens[1]);
            }
        } else if (command == "d" && currentMaterial) { // Alpha
            if (tokens.size() >= 2) {
                currentMaterial->alpha = std::stof(tokens[1]);
            }
        } else if (command == "map_Kd" && currentMaterial) { // Diffuse map
            if (tokens.size() >= 2) {
                currentMaterial->diffuseMap = tokens[1];
            }
        } else if (command == "map_Bump" && currentMaterial) { // Normal map
            if (tokens.size() >= 2) {
                currentMaterial->normalMap = tokens[1];
            }
        }
    }
    
    file.close();
    return true;
}

bool MeshLoader::parsePLY(const std::string& filepath, Mesh& mesh) {
    // Basic PLY parsing - can be expanded later
    std::ifstream file(filepath);
    if (!file.is_open()) {
        lastError = "Failed to open PLY file: " + filepath;
        return false;
    }
    
    // For now, just create a placeholder
    VF_LOG_INFO("PLY parsing not fully implemented yet");
    file.close();
    return false;
}

void MeshLoader::processOBJFace(const std::string& line, std::vector<glm::vec3>& positions,
                               std::vector<glm::vec3>& normals, std::vector<glm::vec2>& texCoords,
                               std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
    std::vector<std::string> tokens = splitString(line, ' ');
    if (tokens.size() < 4) return; // Need at least 3 vertices + "f"
    
    std::vector<Vertex> faceVertices;
    
    for (size_t i = 1; i < tokens.size(); i++) {
        std::vector<std::string> vertexData = splitString(tokens[i], '/');
        
        int posIndex = -1, texIndex = -1, normIndex = -1;
        
        if (!vertexData[0].empty()) posIndex = std::stoi(vertexData[0]) - 1;
        if (vertexData.size() > 1 && !vertexData[1].empty()) texIndex = std::stoi(vertexData[1]) - 1;
        if (vertexData.size() > 2 && !vertexData[2].empty()) normIndex = std::stoi(vertexData[2]) - 1;
        
        Vertex vertex;
        
        // Position
        if (posIndex >= 0 && posIndex < static_cast<int>(positions.size())) {
            vertex.position = positions[posIndex];
        }
        
        // Normal
        if (normIndex >= 0 && normIndex < static_cast<int>(normals.size())) {
            vertex.normal = normals[normIndex];
        }
        
        // Texture coordinate
        if (texIndex >= 0 && texIndex < static_cast<int>(texCoords.size())) {
            vertex.texCoord = texCoords[texIndex];
        }
        
        faceVertices.push_back(vertex);
    }
    
    // Triangulate if necessary (simple fan triangulation)
    for (size_t i = 2; i < faceVertices.size(); i++) {
        vertices.push_back(faceVertices[0]);
        vertices.push_back(faceVertices[i - 1]);
        vertices.push_back(faceVertices[i]);
        
        indices.push_back(static_cast<uint32_t>(vertices.size() - 3));
        indices.push_back(static_cast<uint32_t>(vertices.size() - 2));
        indices.push_back(static_cast<uint32_t>(vertices.size() - 1));
    }
}

glm::vec3 MeshLoader::parseVec3(const std::string& str) {
    std::vector<std::string> tokens = splitString(str, ' ');
    if (tokens.size() >= 3) {
        return glm::vec3(std::stof(tokens[0]), std::stof(tokens[1]), std::stof(tokens[2]));
    }
    return glm::vec3(0.0f);
}

glm::vec2 MeshLoader::parseVec2(const std::string& str) {
    std::vector<std::string> tokens = splitString(str, ' ');
    if (tokens.size() >= 2) {
        return glm::vec2(std::stof(tokens[0]), std::stof(tokens[1]));
    }
    return glm::vec2(0.0f);
}

std::vector<std::string> MeshLoader::splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    
    return tokens;
}

std::string MeshLoader::trimString(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

bool MeshLoader::fileExists(const std::string& filepath) {
    return std::filesystem::exists(filepath);
}

std::string MeshLoader::getFileExtension(const std::string& filepath) {
    return std::filesystem::path(filepath).extension().string();
}

std::string MeshLoader::getDirectory(const std::string& filepath) {
    return std::filesystem::path(filepath).parent_path().string();
}

std::string MeshLoader::getFilename(const std::string& filepath) {
    return std::filesystem::path(filepath).filename().string();
}

void MeshLoader::clearCache() {
    meshCache.clear();
    VF_LOG_INFO("Mesh cache cleared");
}

void MeshLoader::preloadMesh(const std::string& filepath) {
    if (!isCached(filepath)) {
        loadMesh(filepath);
    }
}

bool MeshLoader::isCached(const std::string& filepath) {
    return meshCache.find(filepath) != meshCache.end();
}

// MeshUtils implementation
std::shared_ptr<Mesh> MeshUtils::createCube(float size) {
    auto mesh = std::make_shared<Mesh>("Cube");
    
    float halfSize = size * 0.5f;
    
    // Define vertices for a cube
    std::vector<Vertex> vertices = {
        // Front face
        Vertex(glm::vec3(-halfSize, -halfSize,  halfSize), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec2(0.0f, 0.0f)),
        Vertex(glm::vec3( halfSize, -halfSize,  halfSize), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec2(1.0f, 0.0f)),
        Vertex(glm::vec3( halfSize,  halfSize,  halfSize), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec2(1.0f, 1.0f)),
        Vertex(glm::vec3(-halfSize,  halfSize,  halfSize), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec2(0.0f, 1.0f)),
        
        // Back face
        Vertex(glm::vec3(-halfSize, -halfSize, -halfSize), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec2(1.0f, 0.0f)),
        Vertex(glm::vec3(-halfSize,  halfSize, -halfSize), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec2(1.0f, 1.0f)),
        Vertex(glm::vec3( halfSize,  halfSize, -halfSize), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec2(0.0f, 1.0f)),
        Vertex(glm::vec3( halfSize, -halfSize, -halfSize), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec2(0.0f, 0.0f)),
        
        // Top face
        Vertex(glm::vec3(-halfSize,  halfSize, -halfSize), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec2(0.0f, 1.0f)),
        Vertex(glm::vec3(-halfSize,  halfSize,  halfSize), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec2(0.0f, 0.0f)),
        Vertex(glm::vec3( halfSize,  halfSize,  halfSize), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec2(1.0f, 0.0f)),
        Vertex(glm::vec3( halfSize,  halfSize, -halfSize), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec2(1.0f, 1.0f)),
        
        // Bottom face
        Vertex(glm::vec3(-halfSize, -halfSize, -halfSize), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec2(1.0f, 1.0f)),
        Vertex(glm::vec3( halfSize, -halfSize, -halfSize), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec2(0.0f, 1.0f)),
        Vertex(glm::vec3( halfSize, -halfSize,  halfSize), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec2(0.0f, 0.0f)),
        Vertex(glm::vec3(-halfSize, -halfSize,  halfSize), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec2(1.0f, 0.0f)),
        
        // Right face
        Vertex(glm::vec3( halfSize, -halfSize, -halfSize), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec2(1.0f, 0.0f)),
        Vertex(glm::vec3( halfSize,  halfSize, -halfSize), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec2(1.0f, 1.0f)),
        Vertex(glm::vec3( halfSize,  halfSize,  halfSize), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec2(0.0f, 1.0f)),
        Vertex(glm::vec3( halfSize, -halfSize,  halfSize), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec2(0.0f, 0.0f)),
        
        // Left face
        Vertex(glm::vec3(-halfSize, -halfSize, -halfSize), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec2(0.0f, 0.0f)),
        Vertex(glm::vec3(-halfSize, -halfSize,  halfSize), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec2(1.0f, 0.0f)),
        Vertex(glm::vec3(-halfSize,  halfSize,  halfSize), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec2(1.0f, 1.0f)),
        Vertex(glm::vec3(-halfSize,  halfSize, -halfSize), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec2(0.0f, 1.0f))
    };
    
    // Define indices for a cube (6 faces, 2 triangles each, 3 vertices each)
    std::vector<uint32_t> indices = {
        0,  1,  2,    0,  2,  3,   // Front
        4,  5,  6,    4,  6,  7,   // Back
        8,  9,  10,   8,  10, 11,  // Top
        12, 13, 14,   12, 14, 15,  // Bottom
        16, 17, 18,   16, 18, 19,  // Right
        20, 21, 22,   20, 22, 23   // Left
    };
    
    Submesh submesh("Cube");
    submesh.vertices = vertices;
    submesh.indices = indices;
    
    Material material("CubeMaterial");
    material.diffuse = glm::vec3(0.7f, 0.7f, 0.7f);
    material.ambient = glm::vec3(0.1f, 0.1f, 0.1f);
    material.specular = glm::vec3(1.0f, 1.0f, 1.0f);
    material.shininess = 32.0f;
    
    mesh->materials.push_back(material);
    submesh.materialIndex = 0;
    mesh->submeshes.push_back(submesh);
    
    mesh->optimize();
    return mesh;
}

std::shared_ptr<Mesh> MeshUtils::createSphere(float radius, uint32_t segments) {
    auto mesh = std::make_shared<Mesh>("Sphere");
    
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    
    // Generate sphere vertices
    for (uint32_t lat = 0; lat <= segments; lat++) {
        float theta = lat * glm::pi<float>() / segments;
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);
        
        for (uint32_t lon = 0; lon <= segments; lon++) {
            float phi = lon * 2.0f * glm::pi<float>() / segments;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);
            
            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;
            
            glm::vec3 position = glm::vec3(x, y, z) * radius;
            glm::vec3 normal = glm::normalize(glm::vec3(x, y, z));
            glm::vec2 texCoord = glm::vec2(static_cast<float>(lon) / segments, static_cast<float>(lat) / segments);
            
            vertices.emplace_back(position, normal, texCoord);
        }
    }
    
    // Generate indices
    for (uint32_t lat = 0; lat < segments; lat++) {
        for (uint32_t lon = 0; lon < segments; lon++) {
            uint32_t current = lat * (segments + 1) + lon;
            uint32_t next = current + segments + 1;
            
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);
            
            indices.push_back(next);
            indices.push_back(next + 1);
            indices.push_back(current + 1);
        }
    }
    
    Submesh submesh("Sphere");
    submesh.vertices = vertices;
    submesh.indices = indices;
    
    Material material("SphereMaterial");
    material.diffuse = glm::vec3(0.7f, 0.7f, 0.7f);
    material.ambient = glm::vec3(0.1f, 0.1f, 0.1f);
    material.specular = glm::vec3(1.0f, 1.0f, 1.0f);
    material.shininess = 32.0f;
    
    mesh->materials.push_back(material);
    submesh.materialIndex = 0;
    mesh->submeshes.push_back(submesh);
    
    mesh->optimize();
    return mesh;
}

std::shared_ptr<Mesh> MeshUtils::createPlane(float width, float height, uint32_t segments) {
    auto mesh = std::make_shared<Mesh>("Plane");
    
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    
    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;
    
    // Generate vertices
    for (uint32_t z = 0; z <= segments; z++) {
        for (uint32_t x = 0; x <= segments; x++) {
            float xPos = (static_cast<float>(x) / segments - 0.5f) * width;
            float zPos = (static_cast<float>(z) / segments - 0.5f) * height;
            
            glm::vec3 position(xPos, 0.0f, zPos);
            glm::vec3 normal(0.0f, 1.0f, 0.0f);
            glm::vec2 texCoord(static_cast<float>(x) / segments, static_cast<float>(z) / segments);
            
            vertices.emplace_back(position, normal, texCoord);
        }
    }
    
    // Generate indices
    for (uint32_t z = 0; z < segments; z++) {
        for (uint32_t x = 0; x < segments; x++) {
            uint32_t topLeft = z * (segments + 1) + x;
            uint32_t topRight = topLeft + 1;
            uint32_t bottomLeft = (z + 1) * (segments + 1) + x;
            uint32_t bottomRight = bottomLeft + 1;
            
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
    
    Submesh submesh("Plane");
    submesh.vertices = vertices;
    submesh.indices = indices;
    
    Material material("PlaneMaterial");
    material.diffuse = glm::vec3(0.7f, 0.7f, 0.7f);
    material.ambient = glm::vec3(0.1f, 0.1f, 0.1f);
    material.specular = glm::vec3(1.0f, 1.0f, 1.0f);
    material.shininess = 32.0f;
    
    mesh->materials.push_back(material);
    submesh.materialIndex = 0;
    mesh->submeshes.push_back(submesh);
    
    mesh->optimize();
    return mesh;
}

// Placeholder implementations for other geometry
std::shared_ptr<Mesh> MeshUtils::createCylinder(float radius, float height, uint32_t segments) {
    // TODO: Implement cylinder generation
    VF_LOG_INFO("Cylinder generation not implemented yet");
    return createCube(1.0f); // Fallback to cube
}

std::shared_ptr<Mesh> MeshUtils::createCone(float radius, float height, uint32_t segments) {
    // TODO: Implement cone generation
    VF_LOG_INFO("Cone generation not implemented yet");
    return createCube(1.0f); // Fallback to cube
}

// Mesh operations
void MeshUtils::centerMesh(Mesh& mesh) {
    mesh.calculateBounds();
    glm::vec3 center = (mesh.minBounds + mesh.maxBounds) * 0.5f;
    translateMesh(mesh, -center);
}

void MeshUtils::scaleMesh(Mesh& mesh, float scale) {
    scaleMesh(mesh, glm::vec3(scale));
}

void MeshUtils::scaleMesh(Mesh& mesh, const glm::vec3& scale) {
    for (auto& submesh : mesh.submeshes) {
        for (auto& vertex : submesh.vertices) {
            vertex.position *= scale;
        }
    }
    mesh.calculateBounds();
}

void MeshUtils::rotateMesh(Mesh& mesh, const glm::vec3& rotation) {
    glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1,0,0)) *
                       glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0,1,0)) *
                       glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0,0,1));
    
    for (auto& submesh : mesh.submeshes) {
        for (auto& vertex : submesh.vertices) {
            vertex.position = glm::vec3(rotMat * glm::vec4(vertex.position, 1.0f));
            vertex.normal = glm::vec3(rotMat * glm::vec4(vertex.normal, 0.0f));
        }
    }
    mesh.calculateBounds();
}

void MeshUtils::translateMesh(Mesh& mesh, const glm::vec3& translation) {
    for (auto& submesh : mesh.submeshes) {
        for (auto& vertex : submesh.vertices) {
            vertex.position += translation;
        }
    }
    mesh.calculateBounds();
}

// Optimization functions (placeholder implementations)
void MeshUtils::removeDuplicateVertices(Mesh& mesh) {
    // TODO: Implement duplicate vertex removal
    VF_LOG_INFO("Duplicate vertex removal not implemented yet");
}

void MeshUtils::optimizeIndices(Mesh& mesh) {
    // TODO: Implement index optimization
    VF_LOG_INFO("Index optimization not implemented yet");
}

void MeshUtils::calculateTangents(Mesh& mesh) {
    // TODO: Implement tangent calculation
    VF_LOG_INFO("Tangent calculation not implemented yet");
}

bool MeshUtils::validateMesh(const Mesh& mesh) {
    // Basic validation
    if (mesh.submeshes.empty()) {
        VF_LOG_ERROR("Mesh has no submeshes");
        return false;
    }
    
    for (const auto& submesh : mesh.submeshes) {
        if (submesh.vertices.empty()) {
            VF_LOG_ERROR("Submesh '{}' has no vertices", submesh.name);
            return false;
        }
        
        if (submesh.indices.empty()) {
            VF_LOG_ERROR("Submesh '{}' has no indices", submesh.name);
            return false;
        }
        
        // Check for invalid indices
        for (uint32_t index : submesh.indices) {
            if (index >= submesh.vertices.size()) {
                VF_LOG_ERROR("Invalid index {} in submesh '{}'", index, submesh.name);
                return false;
            }
        }
    }
    
    return true;
}

void MeshUtils::fixMesh(Mesh& mesh) {
    // Basic mesh fixing
    mesh.calculateNormals();
    mesh.calculateBounds();
    mesh.optimize();
    
    VF_LOG_INFO("Mesh '{}' fixed and optimized", mesh.name);
}

} // namespace Core
} // namespace VaporFrame 