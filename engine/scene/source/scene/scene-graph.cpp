#include "scene-graph.hpp"
#include "components.hpp"

#include <core/error/assert.hpp>
#include <core/log/logger.hpp>

#include <algorithm>

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

        addRoot(entity);
        markTransformDirty(entity);

        return entity;
    }

    auto SceneGraph::destroyEntity(Entity e) -> void
    {
        destroyEntityRecursive(e);
    }

    auto SceneGraph::destroyEntityRecursive(Entity e) -> void
    {
        if (!m_registry.valid(e))
        {
            return;
        }

        removeRoot(e);
        removeDirtyRoot(e);

        if (!m_registry.allOf<RelationshipComponent>(e))
        {
            m_registry.destroy(e);
            return;
        }

        // First, destroy all children recursively
        auto& relationship = m_registry.get<RelationshipComponent>(e);
        auto child = relationship.firstChild;

        while (child != NullEntity)
        {
            if (!m_registry.allOf<RelationshipComponent>(child))
            {
                break;
            }

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
        if (!m_registry.valid(child))
        {
            return;
        }

        if (!m_registry.allOf<RelationshipComponent>(child))
        {
            return;
        }

        if (child == newParent)
        {
            AP_WARN("[SceneGraph] Refused to parent entity to itself.");
            return;
        }

        if (newParent != NullEntity)
        {
            if (!m_registry.valid(newParent) || !m_registry.allOf<RelationshipComponent>(newParent))
            {
                return;
            }

            if (isDescendant(newParent, child))
            {
                AP_WARN("[SceneGraph] Refused to create a parenting cycle.");
                return;
            }
        }

        auto& childRel = m_registry.get<RelationshipComponent>(child);
        auto const wasRoot = childRel.parent == NullEntity;

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

        if (wasRoot && newParent != NullEntity)
        {
            removeRoot(child);
        }
        else if (newParent == NullEntity)
        {
            addRoot(child);
        }

        markTransformDirty(child);
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
        if (parentRel.childrenCount > 0)
        {
            --parentRel.childrenCount;
        }

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

    auto SceneGraph::onUpdateRuntime(float /*dt*/) -> void
    {
        updateTransforms();
    }

    auto SceneGraph::updateTransforms() -> void
    {
        if (m_roots.empty())
        {
            m_dirtyRoots.clear();
            return;
        }

        auto const identityMatrix = float4x4{1.0f};

        if (!m_dirtyRoots.empty())
        {
            if constexpr (AP_ENABLE_ASSERTS != 0)
            {
                for (auto [entity, transform] : m_registry.view<TransformComponent>().each())
                {
                    if (!transform.isDirty)
                    {
                        continue;
                    }

                    AP_ASSERT(
                        isCoveredByDirtyRoots(entity),
                        "[SceneGraph] Dirty transform not covered by dirty roots (entity={}, gen={})",
                        entity.index,
                        entity.generation
                    );
                }
            }

            for (auto const& root : m_dirtyRoots)
            {
                if (!m_registry.valid(root))
                {
                    continue;
                }

                auto parentMatrix = identityMatrix;
                if (m_registry.allOf<RelationshipComponent>(root))
                {
                    auto const& rel = m_registry.get<RelationshipComponent>(root);
                    if (rel.parent != NullEntity && m_registry.allOf<TransformComponent>(rel.parent))
                    {
                        parentMatrix = m_registry.get<TransformComponent>(rel.parent).worldMatrix;
                    }
                }

                updateTransform(root, parentMatrix, false);
            }

            m_dirtyRoots.clear();
            return;
        }

        for (auto const& root : m_roots)
        {
            if (!m_registry.valid(root))
            {
                continue;
            }

            updateTransform(root, identityMatrix, false);
        }

        m_dirtyRoots.clear();
    }

    auto SceneGraph::markTransformDirty(Entity e) -> void
    {
        if (!m_registry.valid(e) || !m_registry.allOf<TransformComponent>(e))
        {
            return;
        }

        m_registry.get<TransformComponent>(e).isDirty = true;
        addDirtyRoot(e);
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

    auto SceneGraph::getRegistry() -> Registry&
    {
        return m_registry;
    }

    auto SceneGraph::getRegistry() const -> Registry const&
    {
        return m_registry;
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
                if (!m_registry.valid(child) || !m_registry.allOf<RelationshipComponent>(child))
                {
                    break;
                }

                updateTransform(child, transform.worldMatrix, shouldUpdate);

                auto const& childRel = m_registry.get<RelationshipComponent>(child);
                child = childRel.nextSibling;
            }
        }
    }

    auto SceneGraph::isDescendant(Entity maybeDescendant, Entity ancestor) const -> bool
    {
        if (maybeDescendant == NullEntity || ancestor == NullEntity)
        {
            return false;
        }

        auto current = maybeDescendant;
        while (current != NullEntity)
        {
            if (current == ancestor)
            {
                return true;
            }

            if (!m_registry.allOf<RelationshipComponent>(current))
            {
                break;
            }

            auto const& rel = m_registry.get<RelationshipComponent>(current);
            current = rel.parent;
        }

        return false;
    }

    auto SceneGraph::addRoot(Entity e) -> void
    {
        if (e == NullEntity)
        {
            return;
        }

        if (std::find(m_roots.begin(), m_roots.end(), e) != m_roots.end())
        {
            return;
        }

        m_roots.push_back(e);
    }

    auto SceneGraph::removeRoot(Entity e) -> void
    {
        auto it = std::find(m_roots.begin(), m_roots.end(), e);
        if (it == m_roots.end())
        {
            return;
        }

        *it = m_roots.back();
        m_roots.pop_back();
    }

    auto SceneGraph::addDirtyRoot(Entity e) -> void
    {
        if (e == NullEntity)
        {
            return;
        }

        if (std::find(m_dirtyRoots.begin(), m_dirtyRoots.end(), e) != m_dirtyRoots.end())
        {
            return;
        }

        if (isCoveredByDirtyRoots(e))
        {
            return;
        }

        m_dirtyRoots.erase(
            std::remove_if(
                m_dirtyRoots.begin(),
                m_dirtyRoots.end(),
                [&](Entity root) { return isDescendant(root, e); }
            ),
            m_dirtyRoots.end()
        );

        m_dirtyRoots.push_back(e);
    }

    auto SceneGraph::removeDirtyRoot(Entity e) -> void
    {
        auto it = std::find(m_dirtyRoots.begin(), m_dirtyRoots.end(), e);
        if (it == m_dirtyRoots.end())
        {
            return;
        }

        *it = m_dirtyRoots.back();
        m_dirtyRoots.pop_back();
    }

    auto SceneGraph::isCoveredByDirtyRoots(Entity e) const -> bool
    {
        for (auto const& root : m_dirtyRoots)
        {
            if (root == e || isDescendant(e, root))
            {
                return true;
            }
        }

        return false;
    }

} // namespace april::scene
