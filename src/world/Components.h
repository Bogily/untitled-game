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
     * @brief Transform component data
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
         */
        size_t Add(Vector3 pos, Vector3 rot = {0, 0, 0}, Vector3 scale = {1, 1, 1})
        {
            positions.push_back(pos);
            rotations.push_back(rot);
            scales.push_back(scale);
            return positions.size() - 1;
        }

        /**
         * @brief Remove transform at index
         */
        int RemoveSwap(size_t index)
        {
            if (index >= positions.size())
                return -1;

            size_t lastIndex = positions.size() - 1;
            if (index != lastIndex)
            {
                positions[index] = positions[lastIndex];
                rotations[index] = rotations[lastIndex];
                scales[index] = scales[lastIndex];
            }
            positions.pop_back();
            rotations.pop_back();
            scales.pop_back();

            return (index == lastIndex) ? -1 : static_cast<int>(index);
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
     * @brief Render component data
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
         */
        size_t Add(Model *model, int geoID, Color albedo, float metallic, float roughness)
        {
            models.push_back(model);
            geometryModelIDs.push_back(geoID);
            albedos.push_back(albedo);
            metallics.push_back(metallic);
            roughnesses.push_back(roughness);
            return models.size() - 1;
        }

        /**
         * @brief Remove render component at index
         */
        int RemoveSwap(size_t index)
        {
            if (index >= models.size())
                return -1;

            size_t lastIndex = models.size() - 1;
            if (index != lastIndex)
            {
                models[index] = models[lastIndex];
                geometryModelIDs[index] = geometryModelIDs[lastIndex];
                albedos[index] = albedos[lastIndex];
                metallics[index] = metallics[lastIndex];
                roughnesses[index] = roughnesses[lastIndex];
            }
            models.pop_back();
            geometryModelIDs.pop_back();
            albedos.pop_back();
            metallics.pop_back();
            roughnesses.pop_back();

            return (index == lastIndex) ? -1 : static_cast<int>(index);
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
     * @brief Metadata component
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
         */
        size_t Add(const std::string &name, bool staticObj)
        {
            names.push_back(name);
            isStatic.push_back(staticObj);
            return names.size() - 1;
        }

        /**
         * @brief Remove metadata at index
         */
        int RemoveSwap(size_t index)
        {
            if (index >= names.size())
                return -1;

            size_t lastIndex = names.size() - 1;
            if (index != lastIndex)
            {
                names[index] = names[lastIndex];
                isStatic[index] = isStatic[lastIndex];
            }
            names.pop_back();
            isStatic.pop_back();

            return (index == lastIndex) ? -1 : static_cast<int>(index);
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
