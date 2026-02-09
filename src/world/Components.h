/**
 * @file Components.h
 * @brief ECS component storage using Structure of Arrays (SoA)
 */

#pragma once

#include "raylib.h"
#include "raymath.h"
#include <string>
#include <vector>

namespace World
{
/**
 * @def COMPONENT_ALIGN
 * @brief SIMD alignment for performance
 */
#define COMPONENT_ALIGN alignas(16)

    /**
     * @brief Transform component data (Structure of Arrays)
     *
     * Stores position, rotation, and scale for all entities.
     * SoA layout improves cache performance when iterating transforms.
     */
    struct TransformComponents
    {
        std::vector<Vector3> positions; ///< Entity positions
        std::vector<Vector3> rotations; ///< Entity rotations (Euler angles in degrees)
        std::vector<Vector3> scales;    ///< Entity scales

        /**
         * @brief Reserve memory for components
         * @param count Number of components to reserve
         */
        void Reserve(size_t count)
        {
            positions.reserve(count);
            rotations.reserve(count);
            scales.reserve(count);
        }

        /**
         * @brief Add new transform component
         * @param pos Position vector
         * @param rot Rotation vector (default {0,0,0})
         * @param scale Scale vector (default {1,1,1})
         */
        void Add(Vector3 pos, Vector3 rot = {0, 0, 0}, Vector3 scale = {1, 1, 1})
        {
            positions.push_back(pos);
            rotations.push_back(rot);
            scales.push_back(scale);
        }

        /**
         * @brief Remove transform at index
         * @param index Component index to remove
         */
        void Remove(size_t index)
        {
            if (index >= positions.size())
                return;
            positions.erase(positions.begin() + index);
            rotations.erase(rotations.begin() + index);
            scales.erase(scales.begin() + index);
        }

        /**
         * @brief Get number of transform components
         * @return Component count
         */
        size_t Size() const { return positions.size(); }

        /**
         * @brief Clear all transform data
         */
        void Clear()
        {
            positions.clear();
            rotations.clear();
            scales.clear();
        }
    };

    /**
     * @brief Render component data for visual representation
     *
     * Stores model references and PBR material properties.
     */
    struct RenderComponents
    {
        std::vector<Model *> models;       ///< Model pointers
        std::vector<int> geometryModelIDs; ///< Geometry renderer model IDs
        std::vector<Color> albedos;        ///< Base colors
        std::vector<float> metallics;      ///< Metallic factors
        std::vector<float> roughnesses;    ///< Roughness factors

        /**
         * @brief Reserve memory for components
         * @param count Number of components to reserve
         */
        void Reserve(size_t count)
        {
            models.reserve(count);
            geometryModelIDs.reserve(count);
            albedos.reserve(count);
            metallics.reserve(count);
            roughnesses.reserve(count);
        }

        /**
         * @brief Add new render component
         * @param model Model pointer
         * @param geoID Geometry renderer model ID
         * @param albedo Base color
         * @param metallic Metallic factor
         * @param roughness Roughness factor
         */
        void Add(Model *model, int geoID, Color albedo, float metallic, float roughness)
        {
            models.push_back(model);
            geometryModelIDs.push_back(geoID);
            albedos.push_back(albedo);
            metallics.push_back(metallic);
            roughnesses.push_back(roughness);
        }

        /**
         * @brief Remove render component at index
         * @param index Component index to remove
         */
        void Remove(size_t index)
        {
            if (index >= models.size())
                return;
            models.erase(models.begin() + index);
            geometryModelIDs.erase(geometryModelIDs.begin() + index);
            albedos.erase(albedos.begin() + index);
            metallics.erase(metallics.begin() + index);
            roughnesses.erase(roughnesses.begin() + index);
        }

        /**
         * @brief Get number of render components
         * @return Component count
         */
        size_t Size() const { return models.size(); }

        /**
         * @brief Clear all render data
         */
        void Clear()
        {
            models.clear();
            geometryModelIDs.clear();
            albedos.clear();
            metallics.clear();
            roughnesses.clear();
        }
    };

    /**
     * @brief Metadata component for entity identification
     *
     * Stores entity names and static/dynamic hints.
     */
    struct MetadataComponents
    {
        std::vector<std::string> names; ///< Entity names
        std::vector<bool> isStatic;     ///< Static optimization hint (doesn't move)

        /**
         * @brief Reserve memory for components
         * @param count Number of components to reserve
         */
        void Reserve(size_t count)
        {
            names.reserve(count);
            isStatic.reserve(count);
        }

        /**
         * @brief Add new metadata component
         * @param name Entity name
         * @param staticObj Whether entity is static
         */
        void Add(const std::string &name, bool staticObj)
        {
            names.push_back(name);
            isStatic.push_back(staticObj);
        }

        /**
         * @brief Remove metadata at index
         * @param index Component index to remove
         */
        void Remove(size_t index)
        {
            if (index >= names.size())
                return;
            names.erase(names.begin() + index);
            isStatic.erase(isStatic.begin() + index);
        }

        /**
         * @brief Get number of metadata components
         * @return Component count
         */
        size_t Size() const { return names.size(); }

        /**
         * @brief Clear all metadata
         */
        void Clear()
        {
            names.clear();
            isStatic.clear();
        }
    };
}
