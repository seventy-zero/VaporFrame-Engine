#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <typeindex>
#include <any>
#include <functional>
#include "MeshLoader.h"

namespace VaporFrame {
namespace Core {

// Forward declarations
class SceneNode;
class Component;
class Scene;

// Entity ID type
using EntityID = uint32_t;

// Component base class
class Component {
public:
    virtual ~Component() = default;
    
    // Component lifecycle
    virtual void onAttach(SceneNode* node) {}
    virtual void onDetach(SceneNode* node) {}
    virtual void onUpdate(float deltaTime) {}
    virtual void onRender() {}
    
    // Component identification
    virtual std::string getTypeName() const = 0;
    
protected:
    SceneNode* owner = nullptr;
    friend class SceneNode;
};

// Transform component (built-in)
class TransformComponent : public Component {
public:
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f); // Euler angles in degrees
    glm::vec3 scale = glm::vec3(1.0f);
    
    // Local and world transforms
    glm::mat4 localTransform = glm::mat4(1.0f);
    glm::mat4 worldTransform = glm::mat4(1.0f);
    
    // Transform flags
    bool transformDirty = true;
    bool worldTransformDirty = true;
    
    // Component interface
    std::string getTypeName() const override { return "Transform"; }
    
    // Transform methods
    void setPosition(const glm::vec3& pos);
    void setRotation(const glm::vec3& rot);
    void setScale(const glm::vec3& scl);
    void setRotationQuaternion(const glm::quat& quat);
    
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getRotation() const { return rotation; }
    glm::vec3 getScale() const { return scale; }
    glm::quat getRotationQuaternion() const;
    
    // Matrix getters
    glm::mat4 getLocalTransform();
    glm::mat4 getWorldTransform();
    
    // Transform utilities
    void lookAt(const glm::vec3& target, const glm::vec3& up = glm::vec3(0, 1, 0));
    void translate(const glm::vec3& offset);
    void rotate(const glm::vec3& axis, float angle);
    
private:
    void updateLocalTransform();
    void updateWorldTransform();
};

// Scene Node (Entity)
class SceneNode {
public:
    SceneNode(EntityID id, const std::string& name = "Entity");
    ~SceneNode();
    
    // Entity identification
    EntityID getID() const { return id; }
    const std::string& getName() const { return name; }
    void setName(const std::string& newName) { name = newName; }
    
    // Hierarchy management
    void setParent(SceneNode* parent);
    SceneNode* getParent() const { return parent; }
    const std::vector<std::unique_ptr<SceneNode>>& getChildren() const { return children; }
    
    void addChild(std::unique_ptr<SceneNode> child);
    std::unique_ptr<SceneNode> removeChild(EntityID childId);
    void removeAllChildren();
    
    // Component management
    template<typename T, typename... Args>
    T* addComponent(Args&&... args) {
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = component.get();
        component->owner = this;
        component->onAttach(this);
        components[std::type_index(typeid(T))] = std::move(component);
        return ptr;
    }
    
    template<typename T>
    T* getComponent() {
        auto it = components.find(std::type_index(typeid(T)));
        if (it != components.end()) {
            return static_cast<T*>(it->second.get());
        }
        return nullptr;
    }
    
    template<typename T>
    const T* getComponent() const {
        auto it = components.find(std::type_index(typeid(T)));
        if (it != components.end()) {
            return static_cast<const T*>(it->second.get());
        }
        return nullptr;
    }
    
    template<typename T>
    bool hasComponent() const {
        return components.find(std::type_index(typeid(T))) != components.end();
    }
    // [FIX] Add non-templated version for ECS queries
    bool hasComponent(const std::type_index& type) const {
        return components.find(type) != components.end();
    }
    
    template<typename T>
    void removeComponent() {
        auto it = components.find(std::type_index(typeid(T)));
        if (it != components.end()) {
            it->second->onDetach(this);
            components.erase(it);
        }
    }
    
    // Transform shortcuts (delegates to TransformComponent)
    TransformComponent* getTransform();
    const TransformComponent* getTransform() const;
    
    // Scene management
    void setScene(Scene* scene) { this->scene = scene; }
    Scene* getScene() const { return scene; }
    
    // Update and render
    void update(float deltaTime);
    void render();
    
    // Utility methods
    bool isActive() const { return active; }
    void setActive(bool active) { this->active = active; }
    
    // Find child by name
    SceneNode* findChild(const std::string& name);
    const SceneNode* findChild(const std::string& name) const;
    
    // Find child by ID
    SceneNode* findChild(EntityID id);
    const SceneNode* findChild(EntityID id) const;
    
private:
    EntityID id;
    std::string name;
    SceneNode* parent = nullptr;
    std::vector<std::unique_ptr<SceneNode>> children;
    std::unordered_map<std::type_index, std::unique_ptr<Component>> components;
    Scene* scene = nullptr;
    bool active = true;
    
    friend class Scene;
};

// Scene class
class Scene {
public:
    Scene(const std::string& name = "Scene");
    ~Scene();
    
    // Scene management
    const std::string& getName() const { return name; }
    void setName(const std::string& newName) { name = newName; }
    
    // Entity creation and management
    SceneNode* createEntity(const std::string& name = "Entity");
    SceneNode* createEntity(EntityID id, const std::string& name = "Entity");
    // [NEW] Safely create a child entity and parent it
    SceneNode* createChildEntity(SceneNode* parent, const std::string& name = "Entity");
    
    void destroyEntity(EntityID id);
    void destroyEntity(SceneNode* node);
    
    SceneNode* getEntity(EntityID id);
    const SceneNode* getEntity(EntityID id) const;
    
    SceneNode* findEntity(const std::string& name);
    const SceneNode* findEntity(const std::string& name) const;
    
    // Root entities
    const std::vector<std::unique_ptr<SceneNode>>& getRootEntities() const { return rootEntities; }
    
    // Scene update and render
    void update(float deltaTime);
    void render();
    
    // Scene hierarchy
    void addRootEntity(std::unique_ptr<SceneNode> entity);
    std::unique_ptr<SceneNode> removeRootEntity(EntityID id);
    
    // Entity queries
    std::vector<SceneNode*> getEntitiesWithComponent(const std::type_index& componentType);
    template<typename T>
    std::vector<SceneNode*> getEntitiesWithComponent() {
        return getEntitiesWithComponent(std::type_index(typeid(T)));
    }
    
    // Scene serialization
    void saveToFile(const std::string& filename);
    void loadFromFile(const std::string& filename);
    
    // Scene statistics
    size_t getEntityCount() const;
    size_t getComponentCount() const;
    
private:
    std::string name;
    std::vector<std::unique_ptr<SceneNode>> rootEntities;
    std::unordered_map<EntityID, SceneNode*> entityMap;
    EntityID nextEntityID = 1;
    
    // Helper methods
    void registerEntity(SceneNode* entity);
    void unregisterEntity(SceneNode* entity);
    EntityID generateEntityID();
};

// Built-in components

// Mesh component
class MeshComponent : public Component {
public:
    std::string meshPath;
    std::shared_ptr<Mesh> mesh;
    bool visible = true;
    bool autoLoad = true;
    
    std::string getTypeName() const override { return "Mesh"; }
    void onAttach(SceneNode* node) override;
    void onUpdate(float deltaTime) override;
    void onRender() override;
    
    bool loadMesh(const std::string& path);
    void setMesh(std::shared_ptr<Mesh> newMesh);
};

// Camera component
class CameraComponent : public Component {
public:
    float fov = 90.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    bool isMainCamera = false;
    
    std::string getTypeName() const override { return "Camera"; }
    void onUpdate(float deltaTime) override;
    
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;
};

// Light component
class LightComponent : public Component {
public:
    enum class LightType {
        Directional,
        Point,
        Spot
    };
    
    LightType type = LightType::Point;
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
    float range = 10.0f;
    float spotAngle = 45.0f;
    
    std::string getTypeName() const override { return "Light"; }
    void onRender() override;
};

// Script component (for custom behavior)
class ScriptComponent : public Component {
public:
    std::string scriptName;
    std::function<void(float)> updateFunction;
    std::function<void()> renderFunction;
    
    std::string getTypeName() const override { return "Script"; }
    void onUpdate(float deltaTime) override;
    void onRender() override;
};

// Scene Manager (singleton)
class SceneManager {
public:
    static SceneManager& getInstance();
    
    // Scene management
    Scene* createScene(const std::string& name);
    Scene* getScene(const std::string& name);
    Scene* getActiveScene() { return activeScene; }
    void setActiveScene(Scene* scene);
    void destroyScene(const std::string& name);
    
    // Scene switching
    void loadScene(const std::string& name);
    void unloadScene(const std::string& name);
    
    // Update and render
    void update(float deltaTime);
    void render();
    
private:
    SceneManager() = default;
    ~SceneManager() = default;
    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;
    
    std::unordered_map<std::string, std::unique_ptr<Scene>> scenes;
    Scene* activeScene = nullptr;
};

} // namespace Core
} // namespace VaporFrame 