/**
 * @file World.h
 * @brief Entity Component System (ECS) world manager with component storage
 */

#pragma once

#include "Entity.h"
#include "Components.h"
#include <vector>
#include <unordered_map>
#include <string>

namespace World
{
    /**
     * @brief ECS World Manager
     */
    class WorldManager
    {
    private:
        // Entity lifecycle management
        std::vector<EntityInfo> entities; ///< Entity metadata and active flag
        std::vector<Entity> freeList;     ///< Recycled entity IDs

        // Component storage (Structure of Arrays)
        TransformComponents transforms; ///< All transform data
        RenderComponents renders;       ///< All render data
        MetadataComponents metadata;    ///< All metadata data

        // Entity-to-component index mapping (sparse-set)
        std::vector<int> entityToTransformIdx; ///< Entity => Transform index (-1 if none)
        std::vector<int> entityToRenderIdx;    ///< Entity => Render index (-1 if none)
        std::vector<int> entityToMetadataIdx;  ///< Entity => Metadata index (-1 if none)

        // Name-based entity lookup
        std::unordered_map<std::string, Entity> nameToEntity;

    public:
        /**
         * @brief Construct world manager and reserve initial capacity
         */
        WorldManager()
        {
            entities.reserve(256);
            freeList.reserve(64);
            entityToTransformIdx.reserve(256);
            entityToRenderIdx.reserve(256);
            entityToMetadataIdx.reserve(256);
            transforms.Reserve(256);
            renders.Reserve(256);
            metadata.Reserve(256);
        }

        /**
         * @brief Create new entity
         * @return Entity ID
         */
        Entity CreateEntity()
        {
            Entity id;
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
                entityToTransformIdx.push_back(-1);
                entityToRenderIdx.push_back(-1);
                entityToMetadataIdx.push_back(-1);
            }
            return id;
        }

        /**
         * @brief Destroy entity and remove all components
         * @param entity Entity to destroy
         */
        void DestroyEntity(Entity entity)
        {
            if (!IsValid(entity))
                return;

            RemoveTransform(entity);
            RemoveRender(entity);
            RemoveMetadata(entity);

            entities[entity].active = false;
            entities[entity].componentMask = COMPONENT_NONE;
            freeList.push_back(entity);
        }

        /**
         * @brief Check if entity is valid and active
         * @param entity Entity to check
         * @return True if valid and alive
         */
        bool IsValid(Entity entity) const
        {
            return entity < entities.size() && entities[entity].active;
        }

        // ===== Component management =====

        /**
         * @brief Add transform component to entity
         * @param entity Target entity
         * @param pos Position vector
         * @param rot Rotation vector (default {0,0,0})
         * @param scale Scale vector (default {1,1,1})
         */
        void AddTransform(Entity entity, Vector3 pos, Vector3 rot = {0, 0, 0}, Vector3 scale = {1, 1, 1})
        {
            if (!IsValid(entity) || HasTransform(entity))
                return;

            size_t index = transforms.Add(pos, rot, scale);
            entityToTransformIdx[entity] = static_cast<int>(index);
            entities[entity].componentMask |= COMPONENT_TRANSFORM;
        }

        /**
         * @brief Add render component to entity
         * @param entity Target entity
         * @param model Model pointer
         * @param geoID Geometry renderer model ID
         * @param albedo Base color
         * @param metallic Metallic factor
         * @param roughness Roughness factor
         */
        void AddRender(Entity entity, Model *model, int geoID, Color albedo, float metallic, float roughness)
        {
            if (!IsValid(entity) || HasRender(entity))
                return;

            size_t index = renders.Add(model, geoID, albedo, metallic, roughness);
            entityToRenderIdx[entity] = static_cast<int>(index);
            entities[entity].componentMask |= COMPONENT_RENDER;
        }

        /**
         * @brief Add metadata component to entity
         * @param entity Target entity
         * @param name Entity name
         * @param isStatic Whether entity is static
         */
        void AddMetadata(Entity entity, const std::string &name, bool isStatic)
        {
            if (!IsValid(entity) || HasMetadata(entity))
                return;

            size_t index = metadata.Add(name, isStatic);
            entityToMetadataIdx[entity] = static_cast<int>(index);
            entities[entity].componentMask |= COMPONENT_METADATA;

            if (!name.empty())
                nameToEntity[name] = entity;
        }

        /**
         * @brief Remove transform component from entity
         */
        void RemoveTransform(Entity entity)
        {
            if (!HasTransform(entity))
                return;

            int idx = entityToTransformIdx[entity];
            int movedIdx = transforms.RemoveSwap(idx);

            // Update index of entity whose transform was moved
            if (movedIdx >= 0)
            {
                for (Entity e = 0; e < entities.size(); ++e)
                {
                    if (entityToTransformIdx[e] == (int)transforms.Size())
                    {
                        entityToTransformIdx[e] = movedIdx;
                        break;
                    }
                }
            }

            entityToTransformIdx[entity] = -1;
            entities[entity].componentMask &= ~COMPONENT_TRANSFORM;
        }

        /**
         * @brief Remove render component from entity
         */
        void RemoveRender(Entity entity)
        {
            if (!HasRender(entity))
                return;

            int idx = entityToRenderIdx[entity];
            int movedIdx = renders.RemoveSwap(idx);

            // Update index of entity whose render was moved
            if (movedIdx >= 0)
            {
                for (Entity e = 0; e < entities.size(); ++e)
                {
                    if (entityToRenderIdx[e] == (int)renders.Size())
                    {
                        entityToRenderIdx[e] = movedIdx;
                        break;
                    }
                }
            }

            entityToRenderIdx[entity] = -1;
            entities[entity].componentMask &= ~COMPONENT_RENDER;
        }

        /**
         * @brief Remove metadata component from entity
         */
        void RemoveMetadata(Entity entity)
        {
            if (!HasMetadata(entity))
                return;

            int idx = entityToMetadataIdx[entity];
            const std::string &name = metadata.names[idx];
            if (!name.empty())
                nameToEntity.erase(name);

            int movedIdx = metadata.RemoveSwap(idx);

            // Update index of entity whose metadata was moved and update name map
            if (movedIdx >= 0)
            {
                for (Entity e = 0; e < entities.size(); ++e)
                {
                    if (entityToMetadataIdx[e] == (int)metadata.Size())
                    {
                        entityToMetadataIdx[e] = movedIdx;
                        const std::string &movedName = metadata.names[movedIdx];
                        if (!movedName.empty())
                            nameToEntity[movedName] = e;
                        break;
                    }
                }
            }

            entityToMetadataIdx[entity] = -1;
            entities[entity].componentMask &= ~COMPONENT_METADATA;
        }

        // ===== Component queries =====

        /**
         * @brief Check if entity has transform component
         * @param entity Entity to check
         * @return True if has transform
         */
        bool HasTransform(Entity entity) const
        {
            return IsValid(entity) && (entities[entity].componentMask & COMPONENT_TRANSFORM) != 0;
        }

        /**
         * @brief Check if entity has render component
         * @param entity Entity to check
         * @return True if has render
         */
        bool HasRender(Entity entity) const
        {
            return IsValid(entity) && (entities[entity].componentMask & COMPONENT_RENDER) != 0;
        }

        /**
         * @brief Check if entity has metadata component
         * @param entity Entity to check
         * @return True if has metadata
         */
        bool HasMetadata(Entity entity) const
        {
            return IsValid(entity) && (entities[entity].componentMask & COMPONENT_METADATA) != 0;
        }

        /**
         * @brief Check if entity has all specified components
         * @param entity Entity to check
         * @param mask Component bitmask
         * @return True if has all components in mask
         */
        bool HasComponents(Entity entity, uint32_t mask) const
        {
            return IsValid(entity) && (entities[entity].componentMask & mask) == mask;
        }

        // ===== Data accessors =====

        /**
         * @brief Get pointer to entity position
         * @param entity Target entity
         * @return Position pointer or nullptr if no transform
         */
        Vector3 *GetPosition(Entity entity)
        {
            if (!HasTransform(entity))
                return nullptr;
            return &transforms.positions[entityToTransformIdx[entity]];
        }

        /**
         * @brief Get pointer to entity rotation
         * @param entity Target entity
         * @return Rotation pointer or nullptr if no transform
         */
        Vector3 *GetRotation(Entity entity)
        {
            if (!HasTransform(entity))
                return nullptr;
            return &transforms.rotations[entityToTransformIdx[entity]];
        }

        /**
         * @brief Get pointer to entity scale
         * @param entity Target entity
         * @return Scale pointer or nullptr if no transform
         */
        Vector3 *GetScale(Entity entity)
        {
            if (!HasTransform(entity))
                return nullptr;
            return &transforms.scales[entityToTransformIdx[entity]];
        }

        // ===== Component array access =====

        /**
         * @brief Get const reference to all transforms (for iteration)
         * @return Transform components
         */
        const TransformComponents &GetTransforms() const { return transforms; }

        /**
         * @brief Get const reference to all render components (for iteration)
         * @return Render components
         */
        const RenderComponents &GetRenders() const { return renders; }

        /**
         * @brief Get const reference to all metadata (for iteration)
         * @return Metadata components
         */
        const MetadataComponents &GetMetadata() const { return metadata; }

        /**
         * @brief Get mutable reference to all transforms (use with caution)
         * @return Transform components
         */
        TransformComponents &GetTransforms() { return transforms; }

        /**
         * @brief Get mutable reference to all render components (use with caution)
         * @return Render components
         */
        RenderComponents &GetRenders() { return renders; }

        /**
         * @brief Get mutable reference to all metadata (use with caution)
         * @return Metadata components
         */
        MetadataComponents &GetMetadata() { return metadata; }

        // ===== Utilities =====

        /**
         * @brief Find entity by name
         * @param name Entity name to search
         * @return Entity ID or NULL_ENTITY if not found
         */
        Entity GetEntityByName(const std::string &name) const
        {
            auto it = nameToEntity.find(name);
            return (it != nameToEntity.end()) ? it->second : NULL_ENTITY;
        }

        /**
         * @brief Get all active entities matching component query
         * @param componentMask Required component bitmask
         * @return Vector of matching entity IDs
         */
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

        /**
         * @brief Get transform component index for entity
         * @param entity Entity to query
         * @return Component index or -1 if not found
         */
        int GetTransformIndex(Entity entity) const
        {
            return (entity < entityToTransformIdx.size()) ? entityToTransformIdx[entity] : -1;
        }

        /**
         * @brief Get render component index for entity
         * @param entity Entity to query
         * @return Component index or -1 if not found
         */
        int GetRenderIndex(Entity entity) const
        {
            return (entity < entityToRenderIdx.size()) ? entityToRenderIdx[entity] : -1;
        }

        /**
         * @brief Get metadata component index for entity
         * @param entity Entity to query
         * @return Component index or -1 if not found
         */
        int GetMetadataIndex(Entity entity) const
        {
            return (entity < entityToMetadataIdx.size()) ? entityToMetadataIdx[entity] : -1;
        }

        /**
         * @brief Get count of active entities
         * @return Number of active entities
         */
        size_t GetEntityCount() const
        {
            size_t count = 0;
            for (const auto &info : entities)
                if (info.active)
                    count++;
            return count;
        }

        /**
         * @brief Clear all entities and components
         */
        void Clear()
        {
            entities.clear();
            freeList.clear();
            transforms.Clear();
            renders.Clear();
            metadata.Clear();
            nameToEntity.clear();
            entityToTransformIdx.clear();
            entityToRenderIdx.clear();
            entityToMetadataIdx.clear();
        }
    };
}
