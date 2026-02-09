/**
 * @file Entity.h
 * @brief Entity type definitions and component flags
 */

#pragma once

#include <cstdint>

namespace World
{
    /**
     * @brief Entity identifier type
     *
     * Entity is simply an index into component arrays in the ECS.
     */
    using Entity = uint32_t;

    /**
     * @brief Null/invalid entity constant
     */
    constexpr Entity NULL_ENTITY = UINT32_MAX;

    /**
     * @brief Component type bitflags for fast component queries
     */
    enum ComponentType : uint32_t
    {
        COMPONENT_NONE = 0,           ///< No components
        COMPONENT_TRANSFORM = 1 << 0, ///< Position, rotation, scale
        COMPONENT_RENDER = 1 << 1,    ///< Visual representation
        COMPONENT_METADATA = 1 << 2,  ///< Name, tags, etc.
    };

    /**
     * @brief Entity metadata and state
     */
    struct EntityInfo
    {
        uint32_t componentMask = COMPONENT_NONE; ///< Bitmask of attached components
        bool active = true;                      ///< Whether entity is active
    };
}
