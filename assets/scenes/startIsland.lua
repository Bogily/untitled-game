-- Start Island Scene
scene = {
    name = "Start Island",

    -- Camera settings
    camera = {
        position = { 0.0, 8.0, 15.0 },
        target = { 0.0, 2.0, 0.0 },
        fov = 45.0,
        followDistance = 10.0,
        followHeight = 5.0,
        smoothness = 0.15
    },

    -- Player start position (on the island)
    playerStart = { 0.0, 2.0, 0.0 },

    -- World objects
    objects = {
        {
            name = "StartIsland",
            position = { 0.0, 0.0, 0.0 },
            rotation = { 0.0, 0.0, 0.0 },
            scale = { 1.0, 1.0, 1.0 },
            modelType = "assets/models/startIsland.glb",
            albedo = { 200, 180, 100, 255 }
        },
        {
            name = "CrookedTree",
            position = { 0.0, 2.0, 0.0 },
            rotation = { 0.0, 0.0, 0.0 },
            scale = { 0.1, 0.1, 0.1 },
            modelType = "assets/models/crooked tree/crooked tree.glb",
            albedo = { 255, 255, 255, 255 }
        },
        {
            name = "PBR-TEST",
            position = { .0, 1.0, 0.0 },
            rotation = { 0.0, 0.0, 0.0 },
            scale = { 1.0, 1.0, 1.0 },
            modelType = "assets/models/pbr_materials_test.glb",
            albedo = { 255, 255, 255, 255 }
        }
    },

    -- NPCs (optional)
    npcs = {},

    -- Lights
    lights = {
        {
            name = "MainLight",
            type = "directional",
            direction = { -0.5, -1.0, -0.5 },
            color = { 255, 255, 255, 255 },
            intensity = 1.0
        }
    },

    -- Particles (optional)
    particles = {},

    -- Water plane underneath the island (grass is disabled)
    water = {
        position = { 0.0, 0.5, 0.0 },
        width = 50.0,
        length = 50.0
    },

    -- Skybox
    skybox = "assets/textures/skybox.png",

    -- Physics
    gravity = 9.8
}
return scene
