#pragma once

#include "raylib.h"
#include "raymath.h"
#include <string>
#include <vector>

namespace World
{
// SIMD alignment
#define COMPONENT_ALIGN alignas(16)

    // Transform component (Structure of Arrays)
    struct TransformComponents
    {
        std::vector<Vector3> positions;
        std::vector<Vector3> rotations; // Euler angles in degrees
        std::vector<Vector3> scales;

        void Reserve(size_t count)
        {
            positions.reserve(count);
            rotations.reserve(count);
            scales.reserve(count);
        }

        void Add(Vector3 pos, Vector3 rot = {0, 0, 0}, Vector3 scale = {1, 1, 1})
        {
            positions.push_back(pos);
            rotations.push_back(rot);
            scales.push_back(scale);
        }

        void Remove(size_t index)
        {
            if (index >= positions.size())
                return;
            positions.erase(positions.begin() + index);
            rotations.erase(rotations.begin() + index);
            scales.erase(scales.begin() + index);
        }

        size_t Size() const { return positions.size(); }
        void Clear()
        {
            positions.clear();
            rotations.clear();
            scales.clear();
        }
    };

    // Render component
    struct RenderComponents
    {
        std::vector<Model *> models;
        std::vector<int> geometryModelIDs; // Pre-registered IDs in GeometryRenderer
        std::vector<Color> albedos;
        std::vector<float> metallics;
        std::vector<float> roughnesses;

        void Reserve(size_t count)
        {
            models.reserve(count);
            geometryModelIDs.reserve(count);
            albedos.reserve(count);
            metallics.reserve(count);
            roughnesses.reserve(count);
        }

        void Add(Model *model, int geoID, Color albedo, float metallic, float roughness)
        {
            models.push_back(model);
            geometryModelIDs.push_back(geoID);
            albedos.push_back(albedo);
            metallics.push_back(metallic);
            roughnesses.push_back(roughness);
        }

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

        size_t Size() const { return models.size(); }
        void Clear()
        {
            models.clear();
            geometryModelIDs.clear();
            albedos.clear();
            metallics.clear();
            roughnesses.clear();
        }
    };

    // Metadata component
    struct MetadataComponents
    {
        std::vector<std::string> names;
        std::vector<bool> isStatic; // Optimization hint: static objects don't move

        void Reserve(size_t count)
        {
            names.reserve(count);
            isStatic.reserve(count);
        }

        void Add(const std::string &name, bool staticObj)
        {
            names.push_back(name);
            isStatic.push_back(staticObj);
        }

        void Remove(size_t index)
        {
            if (index >= names.size())
                return;
            names.erase(names.begin() + index);
            isStatic.erase(isStatic.begin() + index);
        }

        size_t Size() const { return names.size(); }
        void Clear()
        {
            names.clear();
            isStatic.clear();
        }
    };
}
