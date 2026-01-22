#pragma once

#include <cstdint>

namespace World
{
    // Entity is just an ID (index into component arrays)
    using Entity = uint32_t;
    constexpr Entity NULL_ENTITY = UINT32_MAX;

    // Component type flags (bitfield for fast queries)
    enum ComponentType : uint32_t
    {
        COMPONENT_NONE = 0,
        COMPONENT_TRANSFORM = 1 << 0, // Position, rotation, scale
        COMPONENT_RENDER = 1 << 1,    // Visual representation
        COMPONENT_METADATA = 1 << 2,  // Name, tags, etc.
    };

    // Entity metadata
    struct EntityInfo
    {
        uint32_t componentMask = COMPONENT_NONE;
        bool active = true;
    };
}
