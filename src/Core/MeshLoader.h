#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include "Logger.h"

namespace VaporFrame {
namespace Core {

// Vertex data structure
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec3 color;  // For vertex colors
    
    Vertex() : position(0.0f), normal(0.0f, 1.0f, 0.0f), texCoord(0.0f), color(1.0f) {}
    Vertex(const glm::vec3& pos) : position(pos), normal(0.0f, 1.0f, 0.0f), texCoord(0.0f), color(1.0f) {}
    Vertex(const glm::vec3& pos, const glm::vec3& norm) : position(pos), normal(norm), texCoord(0.0f), color(1.0f) {}
    Vertex(const glm::vec3& pos, const glm::vec3& norm, const glm::vec2& tex) : position(pos), normal(norm), texCoord(tex), color(1.0f) {}
    Vertex(const glm::vec3& pos, const glm::vec3& norm, const glm::vec2& tex, const glm::vec3& col) : position(pos), normal(norm), texCoord(tex), color(col) {}
};

// Material structure
struct Material {
    std::string name;
    glm::vec3 ambient = glm::vec3(0.1f);
    glm::vec3 diffuse = glm::vec3(0.7f);
    glm::vec3 specular = glm::vec3(1.0f);
    float shininess = 32.0f;
    float alpha = 1.0f;
    
    // Texture paths
    std::string diffuseMap;
    std::string normalMap;
    std::string specularMap;
    std::string ambientMap;
    
    Material() = default;
    Material(const std::string& n) : name(n) {}
};

// Submesh structure (for models with multiple materials)
struct Submesh {
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    Material material;
    uint32_t materialIndex = 0;
    
    Submesh() = default;
    Submesh(const std::string& n) : name(n) {}
};

// Complete mesh structure
struct Mesh {
    std::string name;
    std::vector<Submesh> submeshes;
    std::vector<Material> materials;
    
    // Bounding box
    glm::vec3 minBounds = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 maxBounds = glm::vec3(std::numeric_limits<float>::lowest());
    
    // Statistics
    uint32_t totalVertices = 0;
    uint32_t totalIndices = 0;
    
    Mesh() = default;
    Mesh(const std::string& n) : name(n) {}
    
    void calculateBounds();
    void calculateNormals();
    void optimize();
};

// Mesh loader class
class MeshLoader {
public:
    static MeshLoader& getInstance();
    
    // Main loading functions
    std::shared_ptr<Mesh> loadMesh(const std::string& filepath);
    std::shared_ptr<Mesh> loadOBJ(const std::string& filepath);
    std::shared_ptr<Mesh> loadPLY(const std::string& filepath);
    
    // Utility functions
    bool fileExists(const std::string& filepath);
    std::string getFileExtension(const std::string& filepath);
    std::string getDirectory(const std::string& filepath);
    std::string getFilename(const std::string& filepath);
    
    // Cache management
    void clearCache();
    void preloadMesh(const std::string& filepath);
    bool isCached(const std::string& filepath);
    
    // Error handling
    std::string getLastError() const { return lastError; }
    void clearLastError() { lastError.clear(); }
    
private:
    MeshLoader() = default;
    ~MeshLoader() = default;
    MeshLoader(const MeshLoader&) = delete;
    MeshLoader& operator=(const MeshLoader&) = delete;
    
    // Internal loading functions
    bool parseOBJ(const std::string& filepath, Mesh& mesh);
    bool parseMTL(const std::string& filepath, std::vector<Material>& materials);
    bool parsePLY(const std::string& filepath, Mesh& mesh);
    
    // Helper functions
    void processOBJFace(const std::string& line, std::vector<glm::vec3>& positions, 
                       std::vector<glm::vec3>& normals, std::vector<glm::vec2>& texCoords,
                       std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
    glm::vec3 parseVec3(const std::string& str);
    glm::vec2 parseVec2(const std::string& str);
    std::vector<std::string> splitString(const std::string& str, char delimiter);
    std::string trimString(const std::string& str);
    
    // Cache
    std::unordered_map<std::string, std::shared_ptr<Mesh>> meshCache;
    std::string lastError;
};

// Mesh utilities
class MeshUtils {
public:
    // Geometry generation
    static std::shared_ptr<Mesh> createCube(float size = 1.0f);
    static std::shared_ptr<Mesh> createSphere(float radius = 1.0f, uint32_t segments = 32);
    static std::shared_ptr<Mesh> createPlane(float width = 1.0f, float height = 1.0f, uint32_t segments = 1);
    static std::shared_ptr<Mesh> createCylinder(float radius = 1.0f, float height = 2.0f, uint32_t segments = 32);
    static std::shared_ptr<Mesh> createCone(float radius = 1.0f, float height = 2.0f, uint32_t segments = 32);
    
    // Mesh operations
    static void centerMesh(Mesh& mesh);
    static void scaleMesh(Mesh& mesh, float scale);
    static void scaleMesh(Mesh& mesh, const glm::vec3& scale);
    static void rotateMesh(Mesh& mesh, const glm::vec3& rotation);
    static void translateMesh(Mesh& mesh, const glm::vec3& translation);
    
    // Optimization
    static void removeDuplicateVertices(Mesh& mesh);
    static void optimizeIndices(Mesh& mesh);
    static void calculateTangents(Mesh& mesh);
    
    // Validation
    static bool validateMesh(const Mesh& mesh);
    static void fixMesh(Mesh& mesh);
};

} // namespace Core
} // namespace VaporFrame 