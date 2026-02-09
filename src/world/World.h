/**
 * @file World.h
 * @brief Entity Component System (ECS) world manager
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
     * @brief ECS World Manager coordinating entities and components
     *
     * Manages all game entities using a data-oriented Entity Component System:
     * - Efficient Structure of Arrays (SoA) component storage
     * - Fast entity creation/destruction with ID reuse
     * - Bitfield-based component queries
     * - Name-based entity lookup
     * - Automatic index management for component arrays
     */
    class WorldManager
    {
    private:
        std::vector<EntityInfo> entities; ///< Entity metadata
        std::vector<Entity> freeList;     ///< Reusable entity IDs

        TransformComponents transforms; ///< All transform data
        RenderComponents renders;       ///< All render data
        MetadataComponents metadata;    ///< All metadata

        std::unordered_map<std::string, Entity> nameToEntity; ///< Name->Entity lookup

        std::vector<int> entityToTransformIndex; ///< Entity->Transform index mapping
        std::vector<int> entityToRenderIndex;    ///< Entity->Render index mapping
        std::vector<int> entityToMetadataIndex;  ///< Entity->Metadata index mapping

    public:
        /**
         * @brief Construct world manager and reserve initial capacity
         */
        WorldManager()
        {
            entities.reserve(256);
            freeList.reserve(64);

            entityToTransformIndex.reserve(256);
            entityToRenderIndex.reserve(256);
            entityToMetadataIndex.reserve(256);

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

                entityToTransformIndex.push_back(-1);
                entityToRenderIndex.push_back(-1);
                entityToMetadataIndex.push_back(-1);
            }

            return id;
        }

        /**
         * @brief Destroy entity and remove all components
         * @param entity Entity to destroy
         */
        void DestroyEntity(Entity entity)
        {
            if (entity >= entities.size() || !entities[entity].active)
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
         * @return True if valid
         */
        bool IsValid(Entity entity) const
        {
            return entity < entities.size() && entities[entity].active;
        }

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

            int index = static_cast<int>(transforms.Size());
            transforms.Add(pos, rot, scale);
            entityToTransformIndex[entity] = index;
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

            int index = static_cast<int>(renders.Size());
            renders.Add(model, geoID, albedo, metallic, roughness);
            entityToRenderIndex[entity] = index;
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

            int index = static_cast<int>(metadata.Size());
            metadata.Add(name, isStatic);
            entityToMetadataIndex[entity] = index;
            entities[entity].componentMask |= COMPONENT_METADATA;

            if (!name.empty())
                nameToEntity[name] = entity;
        }

        /**
         * @brief Remove transform component from entity
         * @param entity Target entity
         * @note Expensive operation - requires reindexing
         */
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

        /**
         * @brief Remove render component from entity
         * @param entity Target entity
         * @note Expensive operation - requires reindexing
         */
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

        /**
         * @brief Remove metadata component from entity
         * @param entity Target entity
         * @note Expensive operation - requires reindexing
         */
        void RemoveMetadata(Entity entity)
        {
            if (!HasMetadata(entity))
                return;
            int index = entityToMetadataIndex[entity];

            const std::string &name = metadata.names[index];
            if (!name.empty())
                nameToEntity.erase(name);

            metadata.Remove(index);
            entityToMetadataIndex[entity] = -1;
            entities[entity].componentMask &= ~COMPONENT_METADATA;
            ReindexAfterRemoval(entityToMetadataIndex, index);
        }

        /**
         * @brief Check if entity has transform component
         * @param entity Entity to check
         * @return True if has transform
         */
        bool HasTransform(Entity entity) const
        {
            return IsValid(entity) && (entities[entity].componentMask & COMPONENT_TRANSFORM);
        }

        /**
         * @brief Check if entity has render component
         * @param entity Entity to check
         * @return True if has render
         */
        bool HasRender(Entity entity) const
        {
            return IsValid(entity) && (entities[entity].componentMask & COMPONENT_RENDER);
        }

        /**
         * @brief Check if entity has metadata component
         * @param entity Entity to check
         * @return True if has metadata
         */
        bool HasMetadata(Entity entity) const
        {
            return IsValid(entity) && (entities[entity].componentMask & COMPONENT_METADATA);
        }

        /**
         * @brief Check if entity has all specified components
         * @param entity Entity to check
         * @param mask Component bitmask
         * @return True if has all components
         */
        bool HasComponents(Entity entity, uint32_t mask) const
        {
            return IsValid(entity) && ((entities[entity].componentMask & mask) == mask);
        }

        /**
         * @brief Get pointer to entity position
         * @param entity Target entity
         * @return Position pointer or nullptr
         */
        Vector3 *GetPosition(Entity entity)
        {
            if (!HasTransform(entity))
                return nullptr;
            return &transforms.positions[entityToTransformIndex[entity]];
        }

        /**
         * @brief Get pointer to entity rotation
         * @param entity Target entity
         * @return Rotation pointer or nullptr
         */
        Vector3 *GetRotation(Entity entity)
        {
            if (!HasTransform(entity))
                return nullptr;
            return &transforms.rotations[entityToTransformIndex[entity]];
        }

        /**
         * @brief Get pointer to entity scale
         * @param entity Target entity
         * @return Scale pointer or nullptr
         */
        Vector3 *GetScale(Entity entity)
        {
            if (!HasTransform(entity))
                return nullptr;
            return &transforms.scales[entityToTransformIndex[entity]];
        }

        /**
         * @brief Get const reference to all transforms
         * @return Transform components
         */
        const TransformComponents &GetTransforms() const { return transforms; }

        /**
         * @brief Get const reference to all render components
         * @return Render components
         */
        const RenderComponents &GetRenders() const { return renders; }

        /**
         * @brief Get const reference to all metadata
         * @return Metadata components
         */
        const MetadataComponents &GetMetadata() const { return metadata; }

        /**
         * @brief Get mutable reference to all transforms
         * @return Transform components
         */
        TransformComponents &GetTransforms() { return transforms; }

        /**
         * @brief Get mutable reference to all render components
         * @return Render components
         */
        RenderComponents &GetRenders() { return renders; }

        /**
         * @brief Get mutable reference to all metadata
         * @return Metadata components
         */
        MetadataComponents &GetMetadata() { return metadata; }

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
         * @brief Get all entities with specific component mask
         * @param componentMask Bitmask of required components
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
            return (entity < entityToTransformIndex.size()) ? entityToTransformIndex[entity] : -1;
        }

        /**
         * @brief Get render component index for entity
         * @param entity Entity to query
         * @return Component index or -1 if not found
         */
        int GetRenderIndex(Entity entity) const
        {
            return (entity < entityToRenderIndex.size()) ? entityToRenderIndex[entity] : -1;
        }

        /**
         * @brief Get metadata component index for entity
         * @param entity Entity to query
         * @return Component index or -1 if not found
         */
        int GetMetadataIndex(Entity entity) const
        {
            return (entity < entityToMetadataIndex.size()) ? entityToMetadataIndex[entity] : -1;
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
            entityToTransformIndex.clear();
            entityToRenderIndex.clear();
            entityToMetadataIndex.clear();
        }

    private:
        /**
         * @brief Reindex component array after removal
         * @param indexArray Index mapping array
         * @param removedIndex Index that was removed
         */
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
