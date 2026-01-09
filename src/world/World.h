#pragma once

#include "Entity.h"
#include "Components.h"
#include <vector>
#include <unordered_map>
#include <string>

namespace World
{
    // World manages all entities and components
    class WorldManager
    {
    private:
        // Entity management
        std::vector<EntityInfo> entities;
        std::vector<Entity> freeList; // Reuse deleted entity IDs

        // Component storage (Structure of Arrays)
        TransformComponents transforms;
        RenderComponents renders;
        CollisionComponents collisions;
        MetadataComponents metadata;

        // Fast lookups
        std::unordered_map<std::string, Entity> nameToEntity;

        // Component index mapping (entity ID -> component array index)
        std::vector<int> entityToTransformIndex;
        std::vector<int> entityToRenderIndex;
        std::vector<int> entityToCollisionIndex;
        std::vector<int> entityToMetadataIndex;

    public:
        WorldManager()
        {
            entities.reserve(256);
            freeList.reserve(64);

            entityToTransformIndex.reserve(256);
            entityToRenderIndex.reserve(256);
            entityToCollisionIndex.reserve(256);
            entityToMetadataIndex.reserve(256);

            transforms.Reserve(256);
            renders.Reserve(256);
            collisions.Reserve(128);
            metadata.Reserve(256);
        }

        // Entity management

        Entity CreateEntity()
        {
            Entity id;

            // Reuse freed entity IDs
            if (!freeList.empty())
            {
                id = freeList.back();
                freeList.pop_back();
                entities[id] = EntityInfo{};
            }
            else
            {
                id = static_cast<Entity>(entities.size());
                entities.push_back(EntityInfo{});

                // Expand index arrays
                entityToTransformIndex.push_back(-1);
                entityToRenderIndex.push_back(-1);
                entityToCollisionIndex.push_back(-1);
                entityToMetadataIndex.push_back(-1);
            }

            return id;
        }

        void DestroyEntity(Entity entity)
        {
            if (entity >= entities.size() || !entities[entity].active)
                return;

            // Remove all components
            RemoveTransform(entity);
            RemoveRender(entity);
            RemoveCollision(entity);
            RemoveMetadata(entity);

            entities[entity].active = false;
            entities[entity].componentMask = COMPONENT_NONE;
            freeList.push_back(entity);
        }

        bool IsValid(Entity entity) const
        {
            return entity < entities.size() && entities[entity].active;
        }

        // Component management

        void AddTransform(Entity entity, Vector3 pos, Vector3 rot = {0, 0, 0}, Vector3 scale = {1, 1, 1})
        {
            if (!IsValid(entity) || HasTransform(entity))
                return;

            int index = static_cast<int>(transforms.Size());
            transforms.Add(pos, rot, scale);
            entityToTransformIndex[entity] = index;
            entities[entity].componentMask |= COMPONENT_TRANSFORM;
        }

        void AddRender(Entity entity, Model *model, int geoID, Color albedo, float metallic, float roughness)
        {
            if (!IsValid(entity) || HasRender(entity))
                return;

            int index = static_cast<int>(renders.Size());
            renders.Add(model, geoID, albedo, metallic, roughness);
            entityToRenderIndex[entity] = index;
            entities[entity].componentMask |= COMPONENT_RENDER;
        }

        void AddCollision(Entity entity, CollisionShape shape, Vector3 size, float radius, float height,
                          Vector3 rotation, Color debugColor)
        {
            if (!IsValid(entity) || HasCollision(entity))
                return;

            int index = static_cast<int>(collisions.Size());
            collisions.Add(shape, size, radius, height, rotation, debugColor);
            entityToCollisionIndex[entity] = index;
            entities[entity].componentMask |= COMPONENT_COLLISION;
        }

        void AddMetadata(Entity entity, const std::string &name, bool isStatic)
        {
            if (!IsValid(entity) || HasMetadata(entity))
                return;

            int index = static_cast<int>(metadata.Size());
            metadata.Add(name, isStatic);
            entityToMetadataIndex[entity] = index;
            entities[entity].componentMask |= COMPONENT_METADATA;

            if (!name.empty())
                nameToEntity[name] = entity;
        }

        // Component removal (expensive - requires reindexing)
        void RemoveTransform(Entity entity)
        {
            if (!HasTransform(entity))
                return;
            int index = entityToTransformIndex[entity];
            transforms.Remove(index);
            entityToTransformIndex[entity] = -1;
            entities[entity].componentMask &= ~COMPONENT_TRANSFORM;
            ReindexAfterRemoval(entityToTransformIndex, index);
        }

        void RemoveRender(Entity entity)
        {
            if (!HasRender(entity))
                return;
            int index = entityToRenderIndex[entity];
            renders.Remove(index);
            entityToRenderIndex[entity] = -1;
            entities[entity].componentMask &= ~COMPONENT_RENDER;
            ReindexAfterRemoval(entityToRenderIndex, index);
        }

        void RemoveCollision(Entity entity)
        {
            if (!HasCollision(entity))
                return;
            int index = entityToCollisionIndex[entity];
            collisions.Remove(index);
            entityToCollisionIndex[entity] = -1;
            entities[entity].componentMask &= ~COMPONENT_COLLISION;
            ReindexAfterRemoval(entityToCollisionIndex, index);
        }

        void RemoveMetadata(Entity entity)
        {
            if (!HasMetadata(entity))
                return;
            int index = entityToMetadataIndex[entity];

            // Remove from name lookup
            const std::string &name = metadata.names[index];
            if (!name.empty())
                nameToEntity.erase(name);

            metadata.Remove(index);
            entityToMetadataIndex[entity] = -1;
            entities[entity].componentMask &= ~COMPONENT_METADATA;
            ReindexAfterRemoval(entityToMetadataIndex, index);
        }

        // Component queries

        bool HasTransform(Entity entity) const
        {
            return IsValid(entity) && (entities[entity].componentMask & COMPONENT_TRANSFORM);
        }

        bool HasRender(Entity entity) const
        {
            return IsValid(entity) && (entities[entity].componentMask & COMPONENT_RENDER);
        }

        bool HasCollision(Entity entity) const
        {
            return IsValid(entity) && (entities[entity].componentMask & COMPONENT_COLLISION);
        }

        bool HasMetadata(Entity entity) const
        {
            return IsValid(entity) && (entities[entity].componentMask & COMPONENT_METADATA);
        }

        bool HasComponents(Entity entity, uint32_t mask) const
        {
            return IsValid(entity) && ((entities[entity].componentMask & mask) == mask);
        }

        // Component access

        Vector3 *GetPosition(Entity entity)
        {
            if (!HasTransform(entity))
                return nullptr;
            return &transforms.positions[entityToTransformIndex[entity]];
        }

        Vector3 *GetRotation(Entity entity)
        {
            if (!HasTransform(entity))
                return nullptr;
            return &transforms.rotations[entityToTransformIndex[entity]];
        }

        Vector3 *GetScale(Entity entity)
        {
            if (!HasTransform(entity))
                return nullptr;
            return &transforms.scales[entityToTransformIndex[entity]];
        }

        // Batch access for systems

        const TransformComponents &GetTransforms() const { return transforms; }
        const RenderComponents &GetRenders() const { return renders; }
        const CollisionComponents &GetCollisions() const { return collisions; }
        const MetadataComponents &GetMetadata() const { return metadata; }

        TransformComponents &GetTransforms() { return transforms; }
        RenderComponents &GetRenders() { return renders; }
        CollisionComponents &GetCollisions() { return collisions; }
        MetadataComponents &GetMetadata() { return metadata; }

        // Get entity by name
        Entity GetEntityByName(const std::string &name) const
        {
            auto it = nameToEntity.find(name);
            return (it != nameToEntity.end()) ? it->second : NULL_ENTITY;
        }

        // Get all entities with specific components
        std::vector<Entity> GetEntitiesWithComponents(uint32_t componentMask) const
        {
            std::vector<Entity> result;
            result.reserve(entities.size() / 4);

            for (Entity e = 0; e < entities.size(); ++e)
            {
                if (entities[e].active && (entities[e].componentMask & componentMask) == componentMask)
                {
                    result.push_back(e);
                }
            }

            return result;
        }

        // Component index getters (for systems that need entity-to-component mapping)
        int GetTransformIndex(Entity entity) const
        {
            return (entity < entityToTransformIndex.size()) ? entityToTransformIndex[entity] : -1;
        }

        int GetRenderIndex(Entity entity) const
        {
            return (entity < entityToRenderIndex.size()) ? entityToRenderIndex[entity] : -1;
        }

        int GetCollisionIndex(Entity entity) const
        {
            return (entity < entityToCollisionIndex.size()) ? entityToCollisionIndex[entity] : -1;
        }

        int GetMetadataIndex(Entity entity) const
        {
            return (entity < entityToMetadataIndex.size()) ? entityToMetadataIndex[entity] : -1;
        }

        // Stats
        size_t GetEntityCount() const
        {
            size_t count = 0;
            for (const auto &info : entities)
                if (info.active)
                    count++;
            return count;
        }

        void Clear()
        {
            entities.clear();
            freeList.clear();
            transforms.Clear();
            renders.Clear();
            collisions.Clear();
            metadata.Clear();
            nameToEntity.clear();
            entityToTransformIndex.clear();
            entityToRenderIndex.clear();
            entityToCollisionIndex.clear();
            entityToMetadataIndex.clear();
        }

    private:
        // After removing a component, all indices after it shift down by 1
        void ReindexAfterRemoval(std::vector<int> &indexArray, int removedIndex)
        {
            for (auto &idx : indexArray)
            {
                if (idx > removedIndex)
                    idx--;
            }
        }
    };
}
