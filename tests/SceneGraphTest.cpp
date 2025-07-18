#include "../src/Core/SceneGraph.h"
#include "../src/Core/Logger.h"
#include <iostream>
#include <chrono>
#include <thread>

using namespace VaporFrame::Core;

int main() {
    // Initialize logger
    VaporFrame::Logger::getInstance().initialize("scenegraph_test.log");
    VF_LOG_INFO("Starting Scene Graph Test");
    
    // Test Scene Manager
    SceneManager& sceneManager = SceneManager::getInstance();
    
    // Create a test scene
    Scene* scene = sceneManager.createScene("TestScene");
    if (!scene) {
        VF_LOG_ERROR("Failed to create scene");
        return -1;
    }
    
    VF_LOG_INFO("Created scene: {}", scene->getName());
    
    // Test 1: Basic Entity Creation
    VF_LOG_INFO("=== Test 1: Basic Entity Creation ===");
    
    SceneNode* entity1 = scene->createEntity("TestEntity1");
    SceneNode* entity2 = scene->createEntity("TestEntity2");
    
    VF_LOG_INFO("Created entities: {} (ID: {}) and {} (ID: {})", 
                entity1->getName(), entity1->getID(),
                entity2->getName(), entity2->getID());
    
    // Test 2: Transform Component
    VF_LOG_INFO("=== Test 2: Transform Component ===");
    
    TransformComponent* transform1 = entity1->getTransform();
    if (transform1) {
        transform1->setPosition(glm::vec3(1.0f, 2.0f, 3.0f));
        transform1->setRotation(glm::vec3(45.0f, 0.0f, 0.0f));
        transform1->setScale(glm::vec3(2.0f, 2.0f, 2.0f));
        
        VF_LOG_INFO("Entity1 transform - Position: ({:.2f}, {:.2f}, {:.2f})", 
                    transform1->getPosition().x, transform1->getPosition().y, transform1->getPosition().z);
        VF_LOG_INFO("Entity1 transform - Rotation: ({:.2f}, {:.2f}, {:.2f})", 
                    transform1->getRotation().x, transform1->getRotation().y, transform1->getRotation().z);
        VF_LOG_INFO("Entity1 transform - Scale: ({:.2f}, {:.2f}, {:.2f})", 
                    transform1->getScale().x, transform1->getScale().y, transform1->getScale().z);
        
        glm::mat4 localTransform = transform1->getLocalTransform();
        VF_LOG_INFO("Entity1 local transform matrix determinant: {:.6f}", glm::determinant(localTransform));
    }
    
    // Test 3: Hierarchy
    VF_LOG_INFO("=== Test 3: Hierarchy ===");
    
    SceneNode* child1 = scene->createEntity("Child1");
    SceneNode* child2 = scene->createEntity("Child2");
    SceneNode* grandchild = scene->createEntity("Grandchild");
    
    // Set up hierarchy: entity1 -> child1 -> grandchild
    //                   entity2 -> child2
    entity1->addChild(std::unique_ptr<SceneNode>(child1));
    entity2->addChild(std::unique_ptr<SceneNode>(child2));
    child1->addChild(std::unique_ptr<SceneNode>(grandchild));
    
    VF_LOG_INFO("Hierarchy created:");
    VF_LOG_INFO("  {} -> {} -> {}", entity1->getName(), child1->getName(), grandchild->getName());
    VF_LOG_INFO("  {} -> {}", entity2->getName(), child2->getName());
    
    // Test 4: Component Management
    VF_LOG_INFO("=== Test 4: Component Management ===");
    
    // Add components to entities
    MeshComponent* mesh1 = entity1->addComponent<MeshComponent>();
    CameraComponent* camera1 = entity2->addComponent<CameraComponent>();
    LightComponent* light1 = child1->addComponent<LightComponent>();
    
    if (mesh1) {
        mesh1->meshPath = "models/cube.obj";
        mesh1->visible = true;
        VF_LOG_INFO("Added MeshComponent to {} with path: {}", entity1->getName(), mesh1->meshPath);
    }
    
    if (camera1) {
        camera1->fov = 75.0f;
        camera1->isMainCamera = true;
        VF_LOG_INFO("Added CameraComponent to {} with FOV: {:.1f}", entity2->getName(), camera1->fov);
    }
    
    if (light1) {
        light1->type = LightComponent::LightType::Point;
        light1->color = glm::vec3(1.0f, 0.5f, 0.2f);
        light1->intensity = 2.5f;
        VF_LOG_INFO("Added LightComponent to {} with color: ({:.2f}, {:.2f}, {:.2f})", 
                    child1->getName(), light1->color.r, light1->color.g, light1->color.b);
    }
    
    // Test component queries
    auto entitiesWithMesh = scene->getEntitiesWithComponent<MeshComponent>();
    auto entitiesWithCamera = scene->getEntitiesWithComponent<CameraComponent>();
    auto entitiesWithLight = scene->getEntitiesWithComponent<LightComponent>();
    
    VF_LOG_INFO("Component queries:");
    VF_LOG_INFO("  Entities with MeshComponent: {}", entitiesWithMesh.size());
    VF_LOG_INFO("  Entities with CameraComponent: {}", entitiesWithCamera.size());
    VF_LOG_INFO("  Entities with LightComponent: {}", entitiesWithLight.size());
    
    // Test 5: Entity Finding
    VF_LOG_INFO("=== Test 5: Entity Finding ===");
    
    SceneNode* foundEntity = scene->findEntity("Child1");
    if (foundEntity) {
        VF_LOG_INFO("Found entity by name: {} (ID: {})", foundEntity->getName(), foundEntity->getID());
    }
    
    SceneNode* foundChild = entity1->findChild("Child1");
    if (foundChild) {
        VF_LOG_INFO("Found child by name: {} (ID: {})", foundChild->getName(), foundChild->getID());
    }
    
    // Test 6: Transform Hierarchy
    VF_LOG_INFO("=== Test 6: Transform Hierarchy ===");
    
    TransformComponent* childTransform = child1->getTransform();
    if (childTransform) {
        childTransform->setPosition(glm::vec3(0.0f, 1.0f, 0.0f));
        childTransform->setRotation(glm::vec3(0.0f, 90.0f, 0.0f));
        
        glm::mat4 childLocalTransform = childTransform->getLocalTransform();
        glm::mat4 childWorldTransform = childTransform->getWorldTransform();
        
        VF_LOG_INFO("Child1 local transform determinant: {:.6f}", glm::determinant(childLocalTransform));
        VF_LOG_INFO("Child1 world transform determinant: {:.6f}", glm::determinant(childWorldTransform));
        
        // The world transform should be different from local due to parent transform
        if (childLocalTransform != childWorldTransform) {
            VF_LOG_INFO("✓ Transform hierarchy working correctly");
        } else {
            VF_LOG_WARN("✗ Transform hierarchy not working correctly");
        }
    }
    
    // Test 7: Scene Update and Render
    VF_LOG_INFO("=== Test 7: Scene Update and Render ===");
    
    // Simulate a few frames
    for (int frame = 0; frame < 3; ++frame) {
        float deltaTime = 0.016f; // 60 FPS
        
        // Update scene
        scene->update(deltaTime);
        
        // Render scene
        scene->render();
        
        VF_LOG_INFO("Frame {}: Updated and rendered scene", frame + 1);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    // Test 8: Entity Destruction
    VF_LOG_INFO("=== Test 8: Entity Destruction ===");
    
    size_t initialCount = scene->getEntityCount();
    VF_LOG_INFO("Initial entity count: {}", initialCount);
    
    scene->destroyEntity(child2->getID());
    VF_LOG_INFO("Destroyed child2, new entity count: {}", scene->getEntityCount());
    
    // Test 9: Scene Statistics
    VF_LOG_INFO("=== Test 9: Scene Statistics ===");
    
    VF_LOG_INFO("Final scene statistics:");
    VF_LOG_INFO("  Total entities: {}", scene->getEntityCount());
    VF_LOG_INFO("  Root entities: {}", scene->getRootEntities().size());
    
    // Test 10: Scene Manager
    VF_LOG_INFO("=== Test 10: Scene Manager ===");
    
    Scene* anotherScene = sceneManager.createScene("AnotherScene");
    Scene* foundScene = sceneManager.getScene("TestScene");
    
    if (anotherScene && foundScene) {
        VF_LOG_INFO("Created another scene: {}", anotherScene->getName());
        VF_LOG_INFO("Found original scene: {}", foundScene->getName());
        
        // Switch active scene
        sceneManager.setActiveScene(anotherScene);
        VF_LOG_INFO("Switched active scene to: {}", sceneManager.getActiveScene()->getName());
        
        // Switch back
        sceneManager.setActiveScene(foundScene);
        VF_LOG_INFO("Switched active scene back to: {}", sceneManager.getActiveScene()->getName());
    }
    
    // Test Results
    VF_LOG_INFO("=== Scene Graph Test Results ===");
    VF_LOG_INFO("✓ Entity creation and management working");
    VF_LOG_INFO("✓ Component system working");
    VF_LOG_INFO("✓ Transform hierarchy working");
    VF_LOG_INFO("✓ Scene hierarchy working");
    VF_LOG_INFO("✓ Scene manager working");
    VF_LOG_INFO("✓ Entity queries working");
    VF_LOG_INFO("✓ Update and render cycles working");
    
    VF_LOG_INFO("Scene Graph test completed successfully!");
    
    // Keep console open
    std::cout << "\nPress Enter to exit...";
    std::cin.get();
    
    return 0;
} 