#include "scene-graph.hpp"
#include <core/log/logger.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace april::scene
{
    auto SceneGraph::createEntity(std::string const& name) -> Entity
    {
        auto const entity = m_registry.create();

        // Add core components
        m_registry.emplace<IDComponent>(entity);
        m_registry.emplace<TagComponent>(entity, name);
        m_registry.emplace<TransformComponent>(entity);
        m_registry.emplace<RelationshipComponent>(entity);

        return entity;
    }

    auto SceneGraph::destroyEntity(Entity e) -> void
    {
        destroyEntityRecursive(e);
    }

    auto SceneGraph::destroyEntityRecursive(Entity e) -> void
    {
        if (!m_registry.allOf<RelationshipComponent>(e))
        {
            return;
        }

        // First, destroy all children recursively
        auto& relationship = m_registry.get<RelationshipComponent>(e);
        auto child = relationship.firstChild;

        while (child != NullEntity)
        {
            auto& childRelationship = m_registry.get<RelationshipComponent>(child);
            auto const nextChild = childRelationship.nextSibling;

            // Recursively destroy this child
            destroyEntityRecursive(child);

            child = nextChild;
        }

        // Unlink from parent if this entity has one
        if (relationship.parent != NullEntity)
        {
            unlinkFromParent(e);
        }

        // Finally, destroy this entity
        m_registry.destroy(e);
    }

    auto SceneGraph::setParent(Entity child, Entity newParent) -> void
    {
        // TODO: Circular Dependency Check.

        if (!m_registry.allOf<RelationshipComponent>(child))
        {
            return;
        }

        auto& childRel = m_registry.get<RelationshipComponent>(child);

        // If already has this parent, do nothing
        if (childRel.parent == newParent)
        {
            return;
        }

        // Unlink from old parent
        if (childRel.parent != NullEntity)
        {
            unlinkFromParent(child);
        }

        // Link to new parent
        if (newParent != NullEntity)
        {
            linkToParent(child, newParent);
        }

        // Mark transform as dirty
        if (m_registry.allOf<TransformComponent>(child))
        {
            m_registry.get<TransformComponent>(child).isDirty = true;
        }
    }

    auto SceneGraph::unlinkFromParent(Entity child) -> void
    {
        auto& childRel = m_registry.get<RelationshipComponent>(child);

        if (childRel.parent == NullEntity)
        {
            return; // No parent to unlink from
        }

        auto& parentRel = m_registry.get<RelationshipComponent>(childRel.parent);

        // Fix sibling links
        if (childRel.prevSibling != NullEntity)
        {
            // Not the first child - link prev to next
            auto& prevRel = m_registry.get<RelationshipComponent>(childRel.prevSibling);
            prevRel.nextSibling = childRel.nextSibling;
        }
        else
        {
            // This is the first child - update parent's firstChild
            parentRel.firstChild = childRel.nextSibling;
        }

        if (childRel.nextSibling != NullEntity)
        {
            // Link next to prev
            auto& nextRel = m_registry.get<RelationshipComponent>(childRel.nextSibling);
            nextRel.prevSibling = childRel.prevSibling;
        }

        // Update parent's child count
        --parentRel.childrenCount;

        // Clear child's relationship
        childRel.parent = NullEntity;
        childRel.prevSibling = NullEntity;
        childRel.nextSibling = NullEntity;
    }

    auto SceneGraph::linkToParent(Entity child, Entity parent) -> void
    {
        if (!m_registry.allOf<RelationshipComponent>(parent))
        {
            return;
        }

        auto& childRel = m_registry.get<RelationshipComponent>(child);
        auto& parentRel = m_registry.get<RelationshipComponent>(parent);

        // Set child's parent
        childRel.parent = parent;

        // Add child to parent's linked list (at the beginning)
        childRel.nextSibling = parentRel.firstChild;
        childRel.prevSibling = NullEntity;

        if (parentRel.firstChild != NullEntity)
        {
            auto& firstChildRel = m_registry.get<RelationshipComponent>(parentRel.firstChild);
            firstChildRel.prevSibling = child;
        }

        parentRel.firstChild = child;
        ++parentRel.childrenCount;
    }

    auto SceneGraph::onUpdateRuntime(float dt) -> void
    {
        // Iterate all entities and update root entities (those with no parent)
        auto transformView = m_registry.view<TransformComponent>();

        for (auto& transform : transformView)
        {
            // Find the entity that has this transform
            // Note: In a real implementation, we'd want a more efficient way to do this
            // For now, we'll iterate through all entities
            // This is a limitation of our simple view system
        }

        // Alternative approach: iterate all entities with both Transform and Relationship
        // We'll need to track entities somehow. For now, let's use a simple approach:
        // We'll iterate through potential entity IDs and check if they exist

        // Since we don't have a way to iterate entities directly, we'll need to track them
        // This is a limitation of the current design. For now, we'll document this
        // and potentially add entity tracking later.

        // TODO: Add entity tracking or multi-component view support
        // For now, this is a placeholder that would need to be called manually
        // with root entity IDs, or we need to maintain a list of root entities
    }

    auto SceneGraph::updateTransforms() -> void
    {
        // Find all root entities (those without parents) and update their transforms
        auto* relationshipPool = m_registry.getPool<RelationshipComponent>();
        if (!relationshipPool)
        {
            return;
        }

        auto const identityMatrix = float4x4{1.0f};

        // Iterate all entities with relationship components to find roots
        for (size_t i = 0; i < relationshipPool->data().size(); ++i)
        {
            auto const entity = relationshipPool->getEntity(i);
            auto const& relationship = relationshipPool->data()[i];

            // If this entity has no parent, it's a root
            if (relationship.parent == NullEntity)
            {
                updateTransform(entity, identityMatrix, false);
            }
        }
    }

    auto SceneGraph::updateCameras(uint32_t viewportWidth, uint32_t viewportHeight) -> void
    {
        if (viewportWidth == 0 || viewportHeight == 0)
        {
            return;
        }

        auto* cameraPool = m_registry.getPool<CameraComponent>();
        if (!cameraPool)
        {
            AP_WARN("[SceneGraph] No camera pool found");
            return;
        }

        // Iterate all camera components
        for (size_t i = 0; i < cameraPool->data().size(); ++i)
        {
            auto const entity = cameraPool->getEntity(i);
            auto& camera = cameraPool->data()[i];

            // Update viewport size if changed
            if (camera.viewportWidth != viewportWidth || camera.viewportHeight != viewportHeight)
            {
                camera.viewportWidth = viewportWidth;
                camera.viewportHeight = viewportHeight;
                camera.isDirty = true;
            }

            // Always update view from current world transform
            {
                // Get world transform
                if (m_registry.allOf<TransformComponent>(entity))
                {
                    auto const& transform = m_registry.get<TransformComponent>(entity);

                    // View matrix is inverse of world matrix
                    camera.viewMatrix = glm::inverse(transform.worldMatrix);
                }
                else
                {
                    camera.viewMatrix = float4x4{1.0f};
                }
            }

            // Compute projection matrix if dirty or resized
            if (camera.isDirty)
            {
                auto const aspect = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
                if (camera.isPerspective)
                {
                    camera.projectionMatrix = glm::perspective(
                        camera.fov,
                        aspect,
                        camera.nearClip,
                        camera.farClip
                    );
                }
                else
                {
                    auto const halfWidth = camera.orthoSize * aspect * 0.5f;
                    auto const halfHeight = camera.orthoSize * 0.5f;
                    camera.projectionMatrix = glm::ortho(
                        -halfWidth, halfWidth,
                        -halfHeight, halfHeight,
                        camera.nearClip,
                        camera.farClip
                    );
                }
                camera.isDirty = false;
            }

            // Compute combined matrix
            camera.viewProjectionMatrix = camera.projectionMatrix * camera.viewMatrix;
        }
    }

    auto SceneGraph::getActiveCamera() const -> Entity
    {
        auto const* cameraPool = m_registry.getPool<CameraComponent>();
        if (!cameraPool)
        {
            return NullEntity;
        }

        auto const* tagPool = m_registry.getPool<TagComponent>();
        if (!tagPool)
        {
            // No tags, return first camera
            if (cameraPool->data().size() > 0)
            {
                return cameraPool->getEntity(0);
            }
            return NullEntity;
        }

        // Look for entity with "MainCamera" tag and camera component
        for (size_t i = 0; i < tagPool->data().size(); ++i)
        {
            auto const entity = tagPool->getEntity(i);
            auto const& tag = tagPool->data()[i];

            if (tag.tag == "MainCamera" && m_registry.allOf<CameraComponent>(entity))
            {
                return entity;
            }
        }

        // Fallback: return first camera found
        if (cameraPool->data().size() > 0)
        {
            return cameraPool->getEntity(0);
        }

        return NullEntity;
    }

    auto SceneGraph::updateTransform(Entity e, float4x4 const& parentMatrix, bool parentDirty) -> void
    {
        if (!m_registry.allOf<TransformComponent>(e))
        {
            return;
        }

        auto& transform = m_registry.get<TransformComponent>(e);

        bool shouldUpdate = transform.isDirty || parentDirty;

        if (shouldUpdate)
        {
            auto const localMatrix = transform.getLocalMatrix();
            transform.worldMatrix = parentMatrix * localMatrix;
            transform.isDirty = false;
        }

        // Recursively update children
        if (m_registry.allOf<RelationshipComponent>(e))
        {
            auto const& relationship = m_registry.get<RelationshipComponent>(e);
            auto child = relationship.firstChild;

            while (child != NullEntity)
            {
                updateTransform(child, transform.worldMatrix, shouldUpdate);

                auto const& childRel = m_registry.get<RelationshipComponent>(child);
                child = childRel.nextSibling;
            }
        }
    }

} // namespace april::scene
