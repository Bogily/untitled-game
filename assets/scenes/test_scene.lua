-- Test Scene Definition
scene = {
    name = "Test Scene",
    
    -- Camera settings
    camera = {
        position = {0.0, 10.0, 10.0},
        target = {0.0, 0.0, 0.0},
        fov = 45.0,
        followDistance = 10.0,
        followHeight = 6.0,
        smoothness = 0.15
    },
    
    -- Player start position
    playerStart = {0.0, 0.0, 0.0},
    
    -- World objects
    objects = {
        {
            name = "GroundPlane",
            position = {0.0, 0.0, 0.0},
            rotation = {0.0, 0.0, 0.0},
            scale = {1.0, 1.0, 1.0},
            modelType = "plane_32x32",
            albedo = {77, 77, 77, 255},
            metallic = 0.0,
            roughness = 0.8,
            collision = "none"
        },
        {
            name = "TestSphere",
            position = {0.0, 1.0, 0.0},
            rotation = {0.0, 0.0, 0.0},
            scale = {1.0, 1.0, 1.0},
            modelType = "sphere_1.0",
            albedo = {204, 51, 51, 255},
            metallic = 1.0,
            roughness = 0.3,
            collision = "none"
        },
        {
            name = "RedCube",
            position = {-4.0, 1.0, -4.0},
            rotation = {0.0, 0.0, 0.0},
            scale = {1.0, 1.0, 1.0},
            modelType = "cube_2.0",
            albedo = {204, 26, 26, 255},
            metallic = 0.2,
            roughness = 0.4,
            collision = "box",
            collisionSize = {2.0, 2.0, 2.0}
        },
        {
            name = "BlueTower",
            position = {4.0, 1.0, 4.0},
            rotation = {0.0, 0.0, 0.0},
            scale = {1.0, 1.0, 1.0},
            modelType = "cube_1x4x1",
            albedo = {26, 51, 204, 255},
            metallic = 0.0,
            roughness = 0.3,
            collision = "box",
            collisionSize = {1.0, 4.0, 1.0}
        },
        {
            name = "YellowSphere",
            position = {-6.0, 1.5, 2.0},
            rotation = {0.0, 0.0, 0.0},
            scale = {1.0, 1.0, 1.0},
            modelType = "sphere_1.5",
            albedo = {230, 230, 51, 255},
            metallic = 0.0,
            roughness = 0.6,
            collision = "sphere",
            collisionRadius = 1.5
        },
        {
            name = "OrangeSphere",
            position = {6.0, 1.0, -2.0},
            rotation = {0.0, 0.0, 0.0},
            scale = {1.0, 1.0, 1.0},
            modelType = "sphere_1.0",
            albedo = {230, 128, 26, 255},
            metallic = 0.1,
            roughness = 0.3,
            collision = "sphere",
            collisionRadius = 1.0
        },
        {
            name = "Capsule",
            position = {0.0, 2.0, 6.0},
            rotation = {0.0, 0.0, 0.0},
            scale = {1.0, 1.0, 1.0},
            modelType = "cylinder_0.5x3.0",
            albedo = {77, 179, 230, 255},
            metallic = 0.0,
            roughness = 0.5,
            collision = "capsule",
            collisionRadius = 0.5,
            collisionHeight = 3.0
        },
        {
            name = "Cylinder",
            position = {-2.0, 2.0, -6.0},
            rotation = {0.0, 0.0, 0.0},
            scale = {1.0, 1.0, 1.0},
            modelType = "cylinder_0.8x4.0",
            albedo = {128, 51, 204, 255},
            metallic = 0.0,
            roughness = 0.4,
            collision = "cylinder",
            collisionRadius = 0.8,
            collisionHeight = 4.0
        },
        {
            name = "MainRamp",
            position = {8.0, 1.5, -7.0},
            rotation = {26.565, 0.0, 0.0},
            scale = {1.0, 1.0, 1.0},
            modelType = "cube_4x0.5x6",
            albedo = {102, 64, 38, 255},
            metallic = 0.0,
            roughness = 0.7,
            collision = "box",
            collisionSize = {4.0, 0.5, 6.0}
        },
        {
            name = "SteepRamp",
            position = {-8.0, 2.5, 8.0},
            rotation = {45.0, 0.0, 0.0},
            scale = {1.0, 1.0, 1.0},
            modelType = "cube_3x0.5x4",
            albedo = {128, 26, 26, 255},
            metallic = 0.0,
            roughness = 0.6,
            collision = "box",
            collisionSize = {3.0, 0.5, 4.0}
        },
        {
            name = "RampWallLeft",
            position = {5.85, 1.5, -7.0},
            rotation = {26.565, 0.0, 0.0},
            scale = {1.0, 1.0, 1.0},
            modelType = "cube_0.3x3x6",
            albedo = {77, 51, 26, 255},
            metallic = 0.0,
            roughness = 0.8,
            collision = "box",
            collisionSize = {0.3, 3.0, 6.0}
        },
        {
            name = "RampWallRight",
            position = {10.15, 1.5, -7.0},
            rotation = {26.565, 0.0, 0.0},
            scale = {1.0, 1.0, 1.0},
            modelType = "cube_0.3x3x6",
            albedo = {77, 51, 26, 255},
            metallic = 0.0,
            roughness = 0.8,
            collision = "box",
            collisionSize = {0.3, 3.0, 6.0}
        },
        {
            name = "SteepRampWallLeft",
            position = {-9.65, 2.5, 8.0},
            rotation = {45.0, 0.0, 0.0},
            scale = {1.0, 1.0, 1.0},
            modelType = "cube_0.3x5x4",
            albedo = {89, 38, 26, 255},
            metallic = 0.0,
            roughness = 0.75,
            collision = "box",
            collisionSize = {0.3, 5.0, 4.0}
        },
        {
            name = "SteepRampWallRight",
            position = {-6.35, 2.5, 8.0},
            rotation = {45.0, 0.0, 0.0},
            scale = {1.0, 1.0, 1.0},
            modelType = "cube_0.3x5x4",
            albedo = {89, 38, 26, 255},
            metallic = 0.0,
            roughness = 0.75,
            collision = "box",
            collisionSize = {0.3, 5.0, 4.0}
        }
    },
    
    -- NPCs
    npcs = {
        {
            name = "Rat Merchant",
            position = {8.0, 0.0, 2.0},
            dialogue = {
                "Hello traveler!",
                "I sell the finest wares in all the land!",
                "Care to make a purchase?"
            },
            interactionRange = 3.0
        },
        {
            name = "Miku the Wise",
            position = {-8.0, 0.0, -2.0},
            dialogue = {
                "Greetings, young one.",
                "The path ahead is perilous.",
                "Beware the darkness that lurks beyond."
            },
            interactionRange = 3.0
        }
    },
    
    -- Lights
    lights = {
        {
            name = "Sun",
            type = "directional",
            direction = {-0.3, -1.0, -0.4},
            color = {255, 244, 229, 255},
            intensity = 1.0
        },
        {
            name = "RedLight",
            type = "point",
            position = {-4.0, 3.0, -4.0},
            color = {255, 100, 100, 255},
            intensity = 2.5,
            radius = 15.0
        },
        {
            name = "BlueLight",
            type = "point",
            position = {4.0, 5.0, 4.0},
            color = {100, 150, 255, 255},
            intensity = 2.8,
            radius = 18.0
        },
        {
            name = "YellowLight",
            type = "point",
            position = {-6.0, 3.0, 2.0},
            color = {255, 255, 150, 255},
            intensity = 2.6,
            radius = 16.0
        },
        {
            name = "WhiteLight",
            type = "point",
            position = {0.0, 8.0, 0.0},
            color = {255, 255, 255, 255},
            intensity = 1.5,
            radius = 25.0
        }
    },
    
    -- Particles
    particles = {
        {
            -- Magical Star Fountain
            position = {0.0, 2.0, 0.0},
            offset = {2.0, 0.0, 2.0},
            velocity = {0.0, 1.0, 0.0},
            velocityRandom = {0.5, 0.5, 0.5},
            acceleration = {0.0, 0.5, 0.0},
            colorStart = {255, 200, 50, 255},
            colorEnd = {255, 0, 0, 0},
            sizeStart = 0.5,
            sizeEnd = 0.0,
            sizeRandom = 0.2,
            lifeMin = 1.0,
            lifeMax = 2.0,
            emissionRate = 20.0,
            maxParticles = 100,
            blendMode = "add",
            textureName = "star"
        },
        {
            -- Dark Smoke (Multiplicative)
            position = {-4.0, 3.0, -4.0},
            offset = {0.5, 0.5, 0.5},
            velocity = {0.0, 2.0, 0.0},
            velocityRandom = {0.2, 0.2, 0.2},
            acceleration = {0.0, 0.0, 0.0},
            colorStart = {100, 100, 100, 200},
            colorEnd = {50, 50, 50, 0},
            sizeStart = 1.0,
            sizeEnd = 3.0,
            sizeRandom = 0.5,
            lifeMin = 2.0,
            lifeMax = 4.0,
            emissionRate = 30.0,
            maxParticles = 200,
            blendMode = "alpha", -- Alpha is better for smoke usually, or SUBTRACT/MUL for dark smoke
            textureName = "smoke"
        },
        {
             -- Blue Glow
             position = {4.0, 5.0, 4.0},
             offset = {0.2, 0.2, 0.2},
             velocity = {0.0, 0.0, 0.0},
             velocityRandom = {0.5, 0.5, 0.5},
             acceleration = {0.0, 0.0, 0.0},
             colorStart = {0, 100, 255, 255},
             colorEnd = {0, 0, 255, 0},
             sizeStart = 1.0,
             sizeEnd = 0.1,
             sizeRandom = 0.0,
             lifeMin = 1.0,
             lifeMax = 1.0,
             emissionRate = 10.0,
             maxParticles = 50,
             blendMode = "add",
             textureName = "soft_circle"
        }
    },
    
    -- Grass settings
    grass = {
        position = {0.0, 0.0, 0.0},
        width = 30.0,
        length = 30.0,
        bladeCount = 10000
    },
    
    -- Water settings
    water = {
        position = {0.0, 0.5, 0.0},
        width = 20.0,
        length = 20.0
    },
    
    -- Skybox
    skybox = "assets/textures/skybox.png",
    
    -- Physics
    gravity = 9.8
}

return scene
