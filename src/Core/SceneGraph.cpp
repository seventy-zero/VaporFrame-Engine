#include "SceneGraph.h"
#include "Logger.h"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace VaporFrame {
namespace Core {

// TransformComponent Implementation
void TransformComponent::setPosition(const glm::vec3& pos) {
    position = pos;
    transformDirty = true;
    worldTransformDirty = true;
}

void TransformComponent::setRotation(const glm::vec3& rot) {
    rotation = rot;
    transformDirty = true;
    worldTransformDirty = true;
}

void TransformComponent::setScale(const glm::vec3& scl) {
    scale = scl;
    transformDirty = true;
    worldTransformDirty = true;
}

void TransformComponent::setRotationQuaternion(const glm::quat& quat) {
    rotation = glm::degrees(glm::eulerAngles(quat));
    transformDirty = true;
    worldTransformDirty = true;
}

glm::quat TransformComponent::getRotationQuaternion() const {
    return glm::quat(glm::radians(rotation));
}

glm::mat4 TransformComponent::getLocalTransform() {
    if (transformDirty) {
        updateLocalTransform();
    }
    return localTransform;
}

glm::mat4 TransformComponent::getWorldTransform() {
    if (worldTransformDirty) {
        updateWorldTransform();
    }
    return worldTransform;
}

void TransformComponent::lookAt(const glm::vec3& target, const glm::vec3& up) {
    glm::mat4 lookAtMatrix = glm::lookAt(position, target, up);
    glm::quat rotation = glm::quat_cast(lookAtMatrix);
    setRotationQuaternion(rotation);
}

void TransformComponent::translate(const glm::vec3& offset) {
    position += offset;
    transformDirty = true;
    worldTransformDirty = true;
}

void TransformComponent::rotate(const glm::vec3& axis, float angle) {
    glm::quat currentRotation = getRotationQuaternion();
    glm::quat newRotation = glm::rotate(currentRotation, glm::radians(angle), axis);
    setRotationQuaternion(newRotation);
}

void TransformComponent::updateLocalTransform() {
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 rotation = glm::mat4_cast(getRotationQuaternion());
    glm::mat4 scaling = glm::scale(glm::mat4(1.0f), scale);
    
    localTransform = translation * rotation * scaling;
    transformDirty = false;
}

void TransformComponent::updateWorldTransform() {
    if (owner && owner->getParent()) {
        TransformComponent* parentTransform = owner->getParent()->getTransform();
        if (parentTransform) {
            worldTransform = parentTransform->getWorldTransform() * getLocalTransform();
        } else {
            worldTransform = getLocalTransform();
        }
    } else {
        worldTransform = getLocalTransform();
    }
    worldTransformDirty = false;
}

// SceneNode Implementation
SceneNode::SceneNode(EntityID id, const std::string& name)
    : id(id), name(name) {
    // Automatically add a transform component
    addComponent<TransformComponent>();
}

SceneNode::~SceneNode() {
    // Remove all components
    for (auto& [type, component] : components) {
        component->onDetach(this);
    }
    components.clear();
    
    // Remove all children
    removeAllChildren();
}

void SceneNode::setParent(SceneNode* newParent) {
    if (parent == newParent) return;
    
    // Remove from current parent
    if (parent) {
        auto& parentChildren = parent->children;
        auto it = std::find_if(parentChildren.begin(), parentChildren.end(),
                              [this](const std::unique_ptr<SceneNode>& child) {
                                  return child.get() == this;
                              });
        if (it != parentChildren.end()) {
            parentChildren.erase(it);
        }
    }
    
    // Set new parent
    parent = newParent;
    
    // Add to new parent
    if (parent) {
        // Note: This assumes the SceneNode is already owned by a unique_ptr
        // In practice, you'd need to handle the ownership transfer properly
    }
    
    // Mark transform as dirty
    if (auto transform = getTransform()) {
        transform->worldTransformDirty = true;
    }
}

void SceneNode::addChild(std::unique_ptr<SceneNode> child) {
    if (child) {
        child->setParent(this);
        child->setScene(scene);
        children.push_back(std::move(child));
    }
}

std::unique_ptr<SceneNode> SceneNode::removeChild(EntityID childId) {
    auto it = std::find_if(children.begin(), children.end(),
                          [childId](const std::unique_ptr<SceneNode>& child) {
                              return child->getID() == childId;
                          });
    
    if (it != children.end()) {
        std::unique_ptr<SceneNode> child = std::move(*it);
        children.erase(it);
        child->setParent(nullptr);
        child->setScene(nullptr);
        return child;
    }
    
    return nullptr;
}

void SceneNode::removeAllChildren() {
    for (auto& child : children) {
        child->setParent(nullptr);
        child->setScene(nullptr);
    }
    children.clear();
}

TransformComponent* SceneNode::getTransform() {
    return getComponent<TransformComponent>();
}

const TransformComponent* SceneNode::getTransform() const {
    return getComponent<TransformComponent>();
}

void SceneNode::update(float deltaTime) {
    if (!active) return;
    
    // Update all components
    for (auto& [type, component] : components) {
        component->onUpdate(deltaTime);
    }
    
    // Update all children
    for (auto& child : children) {
        child->update(deltaTime);
    }
}

void SceneNode::render() {
    if (!active) return;
    
    // Render all components
    for (auto& [type, component] : components) {
        component->onRender();
    }
    
    // Render all children
    for (auto& child : children) {
        child->render();
    }
}

SceneNode* SceneNode::findChild(const std::string& name) {
    for (auto& child : children) {
        if (child->getName() == name) {
            return child.get();
        }
        // Recursive search
        if (auto found = child->findChild(name)) {
            return found;
        }
    }
    return nullptr;
}

const SceneNode* SceneNode::findChild(const std::string& name) const {
    for (const auto& child : children) {
        if (child->getName() == name) {
            return child.get();
        }
        // Recursive search
        if (auto found = child->findChild(name)) {
            return found;
        }
    }
    return nullptr;
}

SceneNode* SceneNode::findChild(EntityID id) {
    for (auto& child : children) {
        if (child->getID() == id) {
            return child.get();
        }
        // Recursive search
        if (auto found = child->findChild(id)) {
            return found;
        }
    }
    return nullptr;
}

const SceneNode* SceneNode::findChild(EntityID id) const {
    for (const auto& child : children) {
        if (child->getID() == id) {
            return child.get();
        }
        // Recursive search
        if (auto found = child->findChild(id)) {
            return found;
        }
    }
    return nullptr;
}

// Scene Implementation
Scene::Scene(const std::string& name) : name(name) {
    VF_LOG_DEBUG("Scene '{}' created", name);
}

Scene::~Scene() {
    VF_LOG_DEBUG("Scene '{}' destroyed", name);
    rootEntities.clear();
    entityMap.clear();
}

SceneNode* Scene::createEntity(const std::string& name) {
    EntityID id = generateEntityID();
    return createEntity(id, name);
}

SceneNode* Scene::createEntity(EntityID id, const std::string& name) {
    auto entity = std::make_unique<SceneNode>(id, name);
    SceneNode* ptr = entity.get();
    
    registerEntity(ptr);
    entity->setScene(this);
    rootEntities.push_back(std::move(entity));
    
    VF_LOG_DEBUG("Created entity '{}' (ID: {}) in scene '{}'", name, id, this->name);
    return ptr;
}

void Scene::destroyEntity(EntityID id) {
    auto it = entityMap.find(id);
    if (it != entityMap.end()) {
        destroyEntity(it->second);
    }
}

void Scene::destroyEntity(SceneNode* node) {
    if (!node) return;
    
    // Remove from parent if it has one
    if (node->getParent()) {
        node->getParent()->removeChild(node->getID());
    } else {
        // Remove from root entities
        auto it = std::find_if(rootEntities.begin(), rootEntities.end(),
                              [node](const std::unique_ptr<SceneNode>& entity) {
                                  return entity.get() == node;
                              });
        if (it != rootEntities.end()) {
            unregisterEntity(node);
            rootEntities.erase(it);
        }
    }
}

SceneNode* Scene::getEntity(EntityID id) {
    auto it = entityMap.find(id);
    return it != entityMap.end() ? it->second : nullptr;
}

const SceneNode* Scene::getEntity(EntityID id) const {
    auto it = entityMap.find(id);
    return it != entityMap.end() ? it->second : nullptr;
}

SceneNode* Scene::findEntity(const std::string& name) {
    for (auto& entity : rootEntities) {
        if (entity->getName() == name) {
            return entity.get();
        }
        if (auto found = entity->findChild(name)) {
            return found;
        }
    }
    return nullptr;
}

const SceneNode* Scene::findEntity(const std::string& name) const {
    for (const auto& entity : rootEntities) {
        if (entity->getName() == name) {
            return entity.get();
        }
        if (auto found = entity->findChild(name)) {
            return found;
        }
    }
    return nullptr;
}

void Scene::update(float deltaTime) {
    VF_LOG_DEBUG("Scene '{}' updating entities", name);
    for (auto& entity : rootEntities) {
        entity->update(deltaTime);
    }
    VF_LOG_DEBUG("Scene '{}' finished updating entities", name);
}

void Scene::render() {
    VF_LOG_DEBUG("Scene '{}' rendering entities", name);
    for (auto& entity : rootEntities) {
        entity->render();
    }
    VF_LOG_DEBUG("Scene '{}' finished rendering entities", name);
}

void Scene::addRootEntity(std::unique_ptr<SceneNode> entity) {
    if (entity) {
        registerEntity(entity.get());
        entity->setScene(this);
        rootEntities.push_back(std::move(entity));
    }
}

std::unique_ptr<SceneNode> Scene::removeRootEntity(EntityID id) {
    auto it = std::find_if(rootEntities.begin(), rootEntities.end(),
                          [id](const std::unique_ptr<SceneNode>& entity) {
                              return entity->getID() == id;
                          });
    
    if (it != rootEntities.end()) {
        std::unique_ptr<SceneNode> entity = std::move(*it);
        rootEntities.erase(it);
        unregisterEntity(entity.get());
        entity->setScene(nullptr);
        return entity;
    }
    
    return nullptr;
}

SceneNode* Scene::createChildEntity(SceneNode* parent, const std::string& name) {
    if (!parent) return nullptr;
    EntityID id = generateEntityID();
    auto child = std::make_unique<SceneNode>(id, name);
    SceneNode* ptr = child.get();
    registerEntity(ptr);
    child->setScene(this);
    parent->addChild(std::move(child));
    return ptr;
}

std::vector<SceneNode*> Scene::getEntitiesWithComponent(const std::type_index& componentType) {
    std::vector<SceneNode*> entities;
    
    std::function<void(SceneNode*)> searchEntity = [&](SceneNode* entity) {
        if (entity->hasComponent(componentType)) {
            entities.push_back(entity);
        }
        
        for (auto& child : entity->getChildren()) {
            searchEntity(child.get());
        }
    };
    
    for (auto& entity : rootEntities) {
        searchEntity(entity.get());
    }
    
    return entities;
}

void Scene::saveToFile(const std::string& filename) {
    // TODO: Implement scene serialization
    VF_LOG_INFO("Scene '{}' saved to '{}'", name, filename);
}

void Scene::loadFromFile(const std::string& filename) {
    // TODO: Implement scene deserialization
    VF_LOG_INFO("Scene '{}' loaded from '{}'", name, filename);
}

size_t Scene::getEntityCount() const {
    return entityMap.size();
}

size_t Scene::getComponentCount() const {
    size_t count = 0;
    for (const auto& [id, entity] : entityMap) {
        // Count components for each entity
        // This would need access to the components map
    }
    return count;
}

void Scene::registerEntity(SceneNode* entity) {
    if (entity) {
        entityMap[entity->getID()] = entity;
    }
}

void Scene::unregisterEntity(SceneNode* entity) {
    if (entity) {
        entityMap.erase(entity->getID());
    }
}

EntityID Scene::generateEntityID() {
    return nextEntityID++;
}

// Built-in Component Implementations
void MeshComponent::onAttach(SceneNode* node) {
    if (autoLoad && !meshPath.empty()) {
        loadMesh(meshPath);
    }
}

void MeshComponent::onUpdate(float deltaTime) {
    // Mesh components don't need per-frame updates
}

void MeshComponent::onRender() {
    if (visible && mesh) {
        // TODO: Implement actual mesh rendering with Vulkan
        VF_LOG_DEBUG("Rendering mesh: {} ({} vertices, {} indices)", 
                    mesh->name, mesh->totalVertices, mesh->totalIndices);
    } else if (visible && !meshPath.empty()) {
        VF_LOG_DEBUG("Rendering mesh: {} (not loaded)", meshPath);
    }
}

bool MeshComponent::loadMesh(const std::string& path) {
    meshPath = path;
    mesh = MeshLoader::getInstance().loadMesh(path);
    
    if (mesh) {
        VF_LOG_INFO("Successfully loaded mesh: {} for component", path);
        return true;
    } else {
        VF_LOG_ERROR("Failed to load mesh: {} for component", path);
        return false;
    }
}

void MeshComponent::setMesh(std::shared_ptr<Mesh> newMesh) {
    mesh = newMesh;
    if (mesh) {
        meshPath = mesh->name;
        VF_LOG_INFO("Set mesh: {} for component", mesh->name);
    }
}

void CameraComponent::onUpdate(float deltaTime) {
    // TODO: Implement camera update logic
}

glm::mat4 CameraComponent::getViewMatrix() const {
    if (auto transform = owner->getTransform()) {
        glm::vec3 position = transform->getPosition();
        glm::vec3 forward = glm::normalize(transform->getRotationQuaternion() * glm::vec3(0, 0, -1));
        glm::vec3 up = glm::normalize(transform->getRotationQuaternion() * glm::vec3(0, 1, 0));
        return glm::lookAt(position, position + forward, up);
    }
    return glm::mat4(1.0f);
}

glm::mat4 CameraComponent::getProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}

void LightComponent::onRender() {
    // TODO: Implement light rendering
    VF_LOG_DEBUG("Rendering light: type={}, color=({:.2f},{:.2f},{:.2f}), intensity={:.2f}",
                 static_cast<int>(type), color.r, color.g, color.b, intensity);
}

void ScriptComponent::onUpdate(float deltaTime) {
    if (updateFunction) {
        updateFunction(deltaTime);
    }
}

void ScriptComponent::onRender() {
    if (renderFunction) {
        renderFunction();
    }
}

// SceneManager Implementation
SceneManager& SceneManager::getInstance() {
    static SceneManager instance;
    return instance;
}

Scene* SceneManager::createScene(const std::string& name) {
    auto scene = std::make_unique<Scene>(name);
    Scene* ptr = scene.get();
    scenes[name] = std::move(scene);
    
    if (!activeScene) {
        activeScene = ptr;
    }
    
    VF_LOG_INFO("Created scene '{}'", name);
    return ptr;
}

Scene* SceneManager::getScene(const std::string& name) {
    auto it = scenes.find(name);
    return it != scenes.end() ? it->second.get() : nullptr;
}

void SceneManager::setActiveScene(Scene* scene) {
    activeScene = scene;
    if (scene) {
        VF_LOG_INFO("Set active scene to '{}'", scene->getName());
    }
}

void SceneManager::destroyScene(const std::string& name) {
    auto it = scenes.find(name);
    if (it != scenes.end()) {
        if (activeScene == it->second.get()) {
            activeScene = nullptr;
        }
        scenes.erase(it);
        VF_LOG_INFO("Destroyed scene '{}'", name);
    }
}

void SceneManager::loadScene(const std::string& name) {
    Scene* scene = getScene(name);
    if (scene) {
        setActiveScene(scene);
    } else {
        VF_LOG_WARN("Scene '{}' not found", name);
    }
}

void SceneManager::unloadScene(const std::string& name) {
    if (activeScene && activeScene->getName() == name) {
        activeScene = nullptr;
    }
}

void SceneManager::update(float deltaTime) {
    if (activeScene) {
        activeScene->update(deltaTime);
    }
}

void SceneManager::render() {
    if (activeScene) {
        activeScene->render();
    }
}

} // namespace Core
} // namespace VaporFrame 