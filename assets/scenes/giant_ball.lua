-- Giant Ball Test Scene
scene = {
    name = "Giant Ball",

    camera = {
        position = {0.0, 0.0, 6.0},
        target = {0.0, 0.0, 0.0},
        fov = 45.0,
        followDistance = 8.0,
        followHeight = 2.0,
        smoothness = 0.15
    },

    -- Spawn inside the sphere
    playerStart = {0.0, 0.0, 0.0},

    objects = {
        {
            name = "GiantBall",
            position = {0.0, 0.0, 0.0},
            rotation = {0.0, 0.0, 0.0},
            scale = {1.0, 1.0, 1.0},
            modelType = "sphere_30.0",
            albedo = {180, 200, 255, 255},
            metallic = 0.0,
            roughness = 0.85
        }
    },

    npcs = {},

    lights = {
        {
            name = "InnerLight",
            type = "point",
            position = {0.0, 0.0, 0.0},
            color = {255, 255, 255, 255},
            intensity = 2.0,
            radius = 40.0
        },
        {
            name = "Sun",
            type = "directional",
            direction = {-0.3, -1.0, -0.4},
            color = {255, 244, 229, 255},
            intensity = 0.3
        }
    },

    particles = {},

    skybox = "assets/textures/skybox.png",

    gravity = 9.8
}

return scene
