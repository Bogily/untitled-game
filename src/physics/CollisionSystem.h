#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>

enum CollisionShape
{
    COLLISION_BOX,
    COLLISION_SPHERE,
    COLLISION_CAPSULE,
    COLLISION_CYLINDER
};

struct CollisionObject
{
    CollisionShape shape;
    Vector3 position;
    Vector3 size;     // For box: width, height, depth
    Vector3 rotation; // Rotation in degrees (yaw, pitch, roll)
    float radius;     // For sphere, cylinder, capsule
    float height;     // For cylinder, capsule
    Color debugColor;
    std::string name;
    bool isActive;

    CollisionObject()
        : shape(COLLISION_BOX),
          position{0, 0, 0},
          size{1, 1, 1},
          rotation{0, 0, 0},
          radius(0.5f),
          height(2.0f),
          debugColor(GREEN),
          name(""),
          isActive(true) {}

    CollisionObject(CollisionShape s, Vector3 pos, Vector3 sz, Color color = GREEN, const std::string &n = "")
        : shape(s),
          position(pos),
          size(sz),
          rotation{0, 0, 0},
          radius(sz.x),
          height(sz.y),
          debugColor(color),
          name(n),
          isActive(true) {}
};

class CollisionSystem
{
public:
    CollisionSystem();

    // Add collision objects
    void AddBox(Vector3 position, Vector3 size, const std::string &name = "", Color debugColor = GREEN, Vector3 rotation = {0, 0, 0});
    void AddSphere(Vector3 position, float radius, const std::string &name = "", Color debugColor = YELLOW);
    void AddCapsule(Vector3 position, float radius, float height, const std::string &name = "", Color debugColor = BLUE);
    void AddCylinder(Vector3 position, float radius, float height, const std::string &name = "", Color debugColor = PURPLE);

    // Collision detection with player (cylinder)
    bool CheckPlayerCollision(Vector3 playerPosition, float playerRadius, float playerHeight);
    Vector3 ResolvePlayerCollision(Vector3 playerPosition, Vector3 attemptedPosition, float playerRadius, float playerHeight);
    float GetGroundHeightBelow(Vector3 playerPosition, float playerRadius, float maxCheckDistance = 10.0f);
    Vector3 GetGroundNormal(Vector3 playerPosition, float playerRadius);
    float GetSlopeAngle(Vector3 playerPosition, float playerRadius);
    bool IsGroundWalkable(Vector3 playerPosition, float playerRadius, float maxWalkableAngle = 45.0f);

    // Drawing
    void DrawDebug(bool showLabels = true);

    // Management
    void Clear();
    int GetObjectCount() const { return static_cast<int>(objects.size()); }
    CollisionObject *GetObject(int index);
    void RemoveObject(int index);
    void SetObjectActive(int index, bool active);

private:
    std::vector<CollisionObject> objects;

    // Collision detection helpers
    bool CheckBoxCollision(Vector3 pos, float radius, float height, const CollisionObject &box);
    bool CheckSphereCollision(Vector3 pos, float radius, float height, const CollisionObject &sphere);
    bool CheckCapsuleCollision(Vector3 pos, float radius, float height, const CollisionObject &capsule);
    bool CheckCylinderCollision(Vector3 pos, float radius, float height, const CollisionObject &cylinder);

    // Helper functions
    float DistancePointToLineSegment(Vector3 point, Vector3 lineStart, Vector3 lineEnd);
    bool Circle2DIntersect(Vector2 center1, float radius1, Vector2 center2, float radius2);
    Vector2 GetClosestPointOnBox2D(Vector2 point, Vector2 boxCenter, Vector2 boxHalfSize);
};
