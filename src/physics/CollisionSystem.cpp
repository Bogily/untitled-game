#include "CollisionSystem.h"
#include "rlgl.h"
#include <cmath>
#include <algorithm>

CollisionSystem::CollisionSystem()
{
    objects.reserve(32); // Reserve space for typical number of collision objects
}

void CollisionSystem::AddBox(Vector3 position, Vector3 size, const std::string &name, Color debugColor, Vector3 rotation)
{
    CollisionObject obj(COLLISION_BOX, position, size, debugColor, name);
    obj.rotation = rotation;
    objects.push_back(std::move(obj));
}

void CollisionSystem::AddSphere(Vector3 position, float radius, const std::string &name, Color debugColor)
{
    CollisionObject obj(COLLISION_SPHERE, position, {radius * 2, radius * 2, radius * 2}, debugColor, name);
    obj.radius = radius;
    objects.push_back(std::move(obj));
}

void CollisionSystem::AddCapsule(Vector3 position, float radius, float height, const std::string &name, Color debugColor)
{
    CollisionObject obj(COLLISION_CAPSULE, position, {radius * 2, height, radius * 2}, debugColor, name);
    obj.radius = radius;
    obj.height = height;
    objects.push_back(std::move(obj));
}

void CollisionSystem::AddCylinder(Vector3 position, float radius, float height, const std::string &name, Color debugColor)
{
    CollisionObject obj(COLLISION_CYLINDER, position, {radius * 2, height, radius * 2}, debugColor, name);
    obj.radius = radius;
    obj.height = height;
    objects.push_back(std::move(obj));
}

bool CollisionSystem::CheckPlayerCollision(Vector3 playerPosition, float playerRadius, float playerHeight)
{
    for (const auto &obj : objects)
    {
        if (!obj.isActive)
            continue;

        bool collided = false;
        switch (obj.shape)
        {
        case COLLISION_BOX:
            collided = CheckBoxCollision(playerPosition, playerRadius, playerHeight, obj);
            break;
        case COLLISION_SPHERE:
            collided = CheckSphereCollision(playerPosition, playerRadius, playerHeight, obj);
            break;
        case COLLISION_CAPSULE:
            collided = CheckCapsuleCollision(playerPosition, playerRadius, playerHeight, obj);
            break;
        case COLLISION_CYLINDER:
            collided = CheckCylinderCollision(playerPosition, playerRadius, playerHeight, obj);
            break;
        }

        if (collided)
            return true;
    }
    return false;
}

Vector3 CollisionSystem::ResolvePlayerCollision(Vector3 playerPosition, Vector3 attemptedPosition, float playerRadius, float playerHeight)
{
    // Check if the attempted position causes collision
    if (!CheckPlayerCollision(attemptedPosition, playerRadius, playerHeight))
    {
        return attemptedPosition; // No collision, use attempted position
    }

    // Start with current position (safe position)
    Vector3 resolvedPosition = playerPosition;

    // Try sliding along X axis
    Vector3 slideX = playerPosition;
    slideX.x = attemptedPosition.x;
    if (!CheckPlayerCollision(slideX, playerRadius, playerHeight))
    {
        resolvedPosition.x = slideX.x;
    }

    // Try sliding along Z axis
    Vector3 slideZ = playerPosition;
    slideZ.z = attemptedPosition.z;
    if (!CheckPlayerCollision(slideZ, playerRadius, playerHeight))
    {
        resolvedPosition.z = slideZ.z;
    }

    // Try sliding up/down (important for slopes)
    Vector3 slideY = resolvedPosition;
    slideY.y = attemptedPosition.y;
    if (!CheckPlayerCollision(slideY, playerRadius, playerHeight))
    {
        resolvedPosition.y = slideY.y;
    }

    // For curved objects and rotated boxes, try pushing away
    if (resolvedPosition.x == playerPosition.x && resolvedPosition.z == playerPosition.z)
    {
        // Both axes failed, try to find collision normal and slide along it
        for (const auto &obj : objects)
        {
            if (!obj.isActive)
                continue;

            Vector3 pushDirection = {0, 0, 0};

            // Calculate push direction based on object type
            if (obj.shape == COLLISION_SPHERE || obj.shape == COLLISION_CYLINDER || obj.shape == COLLISION_CAPSULE)
            {
                // For round objects, push away from center (XZ plane)
                Vector2 playerPos2D = {attemptedPosition.x, attemptedPosition.z};
                Vector2 objPos2D = {obj.position.x, obj.position.z};

                float dx = playerPos2D.x - objPos2D.x;
                float dz = playerPos2D.y - objPos2D.y;
                float dist = sqrtf(dx * dx + dz * dz);

                if (dist > 0.001f)
                {
                    // Normalize and create push direction
                    pushDirection.x = dx / dist;
                    pushDirection.z = dz / dist;

                    // Try moving along the surface
                    Vector3 slideAround = playerPosition;
                    Vector3 moveDir = Vector3Subtract(attemptedPosition, playerPosition);

                    // Project movement onto the tangent (perpendicular to push direction)
                    float dotProduct = moveDir.x * pushDirection.x + moveDir.z * pushDirection.z;
                    Vector3 tangent;
                    tangent.x = moveDir.x - dotProduct * pushDirection.x;
                    tangent.z = moveDir.z - dotProduct * pushDirection.z;
                    tangent.y = 0;

                    slideAround = Vector3Add(playerPosition, tangent);

                    if (!CheckPlayerCollision(slideAround, playerRadius, playerHeight))
                    {
                        resolvedPosition = slideAround;
                        break;
                    }
                }
            }
        }
    }

    return resolvedPosition;
}

float CollisionSystem::GetGroundHeightBelow(Vector3 playerPosition, float playerRadius, float maxCheckDistance)
{
    float highestGround = 0.0f; // Default ground level

    for (const auto &obj : objects)
    {
        if (!obj.isActive)
            continue;

        // Only check boxes for ground (could extend to other shapes)
        if (obj.shape == COLLISION_BOX)
        {
            // Transform player position to box local space if rotated
            Vector3 localPlayerPos = playerPosition;

            if (obj.rotation.x != 0 || obj.rotation.y != 0 || obj.rotation.z != 0)
            {
                // Transform to box local space
                localPlayerPos = Vector3Subtract(playerPosition, obj.position);

                Matrix rotMatrix = MatrixIdentity();
                rotMatrix = MatrixMultiply(rotMatrix, MatrixRotateZ(-obj.rotation.z * DEG2RAD));
                rotMatrix = MatrixMultiply(rotMatrix, MatrixRotateY(-obj.rotation.y * DEG2RAD));
                rotMatrix = MatrixMultiply(rotMatrix, MatrixRotateX(-obj.rotation.x * DEG2RAD));

                localPlayerPos = Vector3Transform(localPlayerPos, rotMatrix);

                // Check if player is within bounds in local space
                float boxHalfX = obj.size.x / 2.0f + playerRadius;
                float boxHalfZ = obj.size.z / 2.0f + playerRadius;
                float boxTop = obj.size.y / 2.0f;

                if (localPlayerPos.x >= -boxHalfX && localPlayerPos.x <= boxHalfX &&
                    localPlayerPos.z >= -boxHalfZ && localPlayerPos.z <= boxHalfZ)
                {
                    // Transform the top surface point back to world space
                    Vector3 topSurfaceLocal = {localPlayerPos.x, boxTop, localPlayerPos.z};

                    Matrix fwdRotMatrix = MatrixIdentity();
                    fwdRotMatrix = MatrixMultiply(fwdRotMatrix, MatrixRotateX(obj.rotation.x * DEG2RAD));
                    fwdRotMatrix = MatrixMultiply(fwdRotMatrix, MatrixRotateY(obj.rotation.y * DEG2RAD));
                    fwdRotMatrix = MatrixMultiply(fwdRotMatrix, MatrixRotateZ(obj.rotation.z * DEG2RAD));

                    Vector3 topSurfaceWorld = Vector3Transform(topSurfaceLocal, fwdRotMatrix);
                    topSurfaceWorld = Vector3Add(topSurfaceWorld, obj.position);

                    // Check if this ground is below player and highest so far
                    if (topSurfaceWorld.y <= playerPosition.y &&
                        topSurfaceWorld.y > playerPosition.y - maxCheckDistance &&
                        topSurfaceWorld.y > highestGround)
                    {
                        highestGround = topSurfaceWorld.y;
                    }
                }
            }
            else
            {
                // No rotation - use simple AABB check
                float boxTop = obj.position.y + obj.size.y / 2.0f;
                float boxBottom = obj.position.y - obj.size.y / 2.0f;

                if (boxTop <= playerPosition.y && boxTop > playerPosition.y - maxCheckDistance)
                {
                    float boxLeft = obj.position.x - obj.size.x / 2.0f - playerRadius;
                    float boxRight = obj.position.x + obj.size.x / 2.0f + playerRadius;
                    float boxFront = obj.position.z - obj.size.z / 2.0f - playerRadius;
                    float boxBack = obj.position.z + obj.size.z / 2.0f + playerRadius;

                    if (playerPosition.x >= boxLeft && playerPosition.x <= boxRight &&
                        playerPosition.z >= boxFront && playerPosition.z <= boxBack)
                    {
                        if (boxTop > highestGround)
                        {
                            highestGround = boxTop;
                        }
                    }
                }
            }
        }
    }

    return highestGround;
}

Vector3 CollisionSystem::GetGroundNormal(Vector3 playerPosition, float playerRadius)
{
    Vector3 defaultNormal = {0, 1, 0}; // Default: flat ground pointing up

    for (const auto &obj : objects)
    {
        if (!obj.isActive)
            continue;

        if (obj.shape == COLLISION_BOX)
        {
            // Check if player is standing on this box
            Vector3 localPlayerPos = Vector3Subtract(playerPosition, obj.position);

            if (obj.rotation.x != 0 || obj.rotation.y != 0 || obj.rotation.z != 0)
            {
                // Transform to local space
                Matrix rotMatrix = MatrixIdentity();
                rotMatrix = MatrixMultiply(rotMatrix, MatrixRotateZ(-obj.rotation.z * DEG2RAD));
                rotMatrix = MatrixMultiply(rotMatrix, MatrixRotateY(-obj.rotation.y * DEG2RAD));
                rotMatrix = MatrixMultiply(rotMatrix, MatrixRotateX(-obj.rotation.x * DEG2RAD));

                localPlayerPos = Vector3Transform(localPlayerPos, rotMatrix);

                // Check if within XZ bounds
                float boxHalfX = obj.size.x / 2.0f + playerRadius;
                float boxHalfZ = obj.size.z / 2.0f + playerRadius;

                if (localPlayerPos.x >= -boxHalfX && localPlayerPos.x <= boxHalfX &&
                    localPlayerPos.z >= -boxHalfZ && localPlayerPos.z <= boxHalfZ)
                {
                    // Player is on this rotated surface
                    // Surface normal in local space is (0, 1, 0)
                    // Transform it to world space
                    Vector3 localNormal = {0, 1, 0};

                    Matrix fwdRotMatrix = MatrixIdentity();
                    fwdRotMatrix = MatrixMultiply(fwdRotMatrix, MatrixRotateX(obj.rotation.x * DEG2RAD));
                    fwdRotMatrix = MatrixMultiply(fwdRotMatrix, MatrixRotateY(obj.rotation.y * DEG2RAD));
                    fwdRotMatrix = MatrixMultiply(fwdRotMatrix, MatrixRotateZ(obj.rotation.z * DEG2RAD));

                    Vector3 worldNormal = Vector3Transform(localNormal, fwdRotMatrix);
                    return Vector3Normalize(worldNormal);
                }
            }
        }
    }

    return defaultNormal;
}

float CollisionSystem::GetSlopeAngle(Vector3 playerPosition, float playerRadius)
{
    Vector3 normal = GetGroundNormal(playerPosition, playerRadius);
    Vector3 upVector = {0, 1, 0};

    // Calculate angle between surface normal and up vector
    float dotProduct = Vector3DotProduct(normal, upVector);
    dotProduct = fminf(1.0f, fmaxf(-1.0f, dotProduct)); // Clamp to [-1, 1]
    float angleRad = acosf(dotProduct);
    float angleDeg = angleRad * RAD2DEG;

    return angleDeg;
}

bool CollisionSystem::IsGroundWalkable(Vector3 playerPosition, float playerRadius, float maxWalkableAngle)
{
    float slopeAngle = GetSlopeAngle(playerPosition, playerRadius);
    return slopeAngle <= maxWalkableAngle;
}

void CollisionSystem::DrawDebug(bool showLabels)
{
    for (const auto &obj : objects)
    {
        if (!obj.isActive)
            continue;

        Color wireColor = Fade(obj.debugColor, 0.8f);

        switch (obj.shape)
        {
        case COLLISION_BOX:
        {
            // If box has rotation, use rlPushMatrix for proper transformation
            if (obj.rotation.x != 0 || obj.rotation.y != 0 || obj.rotation.z != 0)
            {
                rlPushMatrix();
                rlTranslatef(obj.position.x, obj.position.y, obj.position.z);
                rlRotatef(obj.rotation.x, 1, 0, 0); // pitch (X axis)
                rlRotatef(obj.rotation.y, 0, 1, 0); // yaw (Y axis)
                rlRotatef(obj.rotation.z, 0, 0, 1); // roll (Z axis)
                DrawCubeWiresV({0, 0, 0}, obj.size, wireColor);
                rlPopMatrix();
            }
            else
            {
                DrawCubeWiresV(obj.position, obj.size, wireColor);
            }
            break;
        }

        case COLLISION_SPHERE:
            DrawSphereWires(obj.position, obj.radius, 8, 8, wireColor);
            break;

        case COLLISION_CAPSULE:
        {
            // Draw capsule as cylinder + two hemispheres
            float halfHeight = (obj.height - obj.radius * 2) / 2.0f;
            Vector3 topCenter = {obj.position.x, obj.position.y + halfHeight, obj.position.z};
            Vector3 bottomCenter = {obj.position.x, obj.position.y - halfHeight, obj.position.z};

            // Draw top hemisphere
            DrawSphereWires(topCenter, obj.radius, 8, 8, wireColor);
            // Draw bottom hemisphere
            DrawSphereWires(bottomCenter, obj.radius, 8, 8, wireColor);
            // Draw cylinder body
            DrawCylinderWires(bottomCenter, obj.radius, obj.radius, obj.height - obj.radius * 2, 8, wireColor);
            break;
        }

        case COLLISION_CYLINDER:
        {
            Vector3 basePos = {obj.position.x, obj.position.y - obj.height / 2, obj.position.z};
            DrawCylinderWires(basePos, obj.radius, obj.radius, obj.height, 16, wireColor);
            break;
        }
        }

        // Note: Labels are disabled for now - would require BillboardText class
    }
}

void CollisionSystem::Clear()
{
    objects.clear();
}

CollisionObject *CollisionSystem::GetObject(int index)
{
    if (index >= 0 && index < static_cast<int>(objects.size()))
        return &objects[index];
    return nullptr;
}

void CollisionSystem::RemoveObject(int index)
{
    if (index >= 0 && index < static_cast<int>(objects.size()))
        objects.erase(objects.begin() + index);
}

void CollisionSystem::SetObjectActive(int index, bool active)
{
    if (index >= 0 && index < static_cast<int>(objects.size()))
        objects[index].isActive = active;
}

// ===== Collision Detection Helpers =====

bool CollisionSystem::CheckBoxCollision(Vector3 pos, float radius, float height, const CollisionObject &box)
{
    // If box has rotation, transform player position to box's local space
    Vector3 localPos = pos;

    if (box.rotation.x != 0 || box.rotation.y != 0 || box.rotation.z != 0)
    {
        // Translate to box origin
        localPos = Vector3Subtract(pos, box.position);

        // Create inverse rotation matrix (rotate in opposite direction)
        Matrix rotMatrix = MatrixIdentity();
        rotMatrix = MatrixMultiply(rotMatrix, MatrixRotateZ(-box.rotation.z * DEG2RAD)); // roll
        rotMatrix = MatrixMultiply(rotMatrix, MatrixRotateY(-box.rotation.y * DEG2RAD)); // yaw
        rotMatrix = MatrixMultiply(rotMatrix, MatrixRotateX(-box.rotation.x * DEG2RAD)); // pitch

        // Transform position
        localPos = Vector3Transform(localPos, rotMatrix);

        // Now localPos is in box's local space, box is at origin with no rotation
        // Check collision as axis-aligned box
    }
    else
    {
        // No rotation, translate to box-relative coordinates
        localPos = Vector3Subtract(pos, box.position);
    }

    // Check XZ plane first (2D circle-rectangle collision in local space)
    Vector2 playerPos2D = {localPos.x, localPos.z};
    Vector2 boxCenter2D = {0, 0}; // Box is at origin in local space
    Vector2 boxHalfSize = {box.size.x / 2, box.size.z / 2};

    Vector2 closest = GetClosestPointOnBox2D(playerPos2D, boxCenter2D, boxHalfSize);

    float distX = playerPos2D.x - closest.x;
    float distZ = playerPos2D.y - closest.y;
    float distSquared = distX * distX + distZ * distZ;

    if (distSquared >= radius * radius)
        return false;

    // Check Y axis overlap (in local space)
    float playerBottom = localPos.y;
    float playerTop = localPos.y + height;
    float boxBottom = -box.size.y / 2;
    float boxTop = box.size.y / 2;

    return (playerBottom < boxTop && playerTop > boxBottom);
}

bool CollisionSystem::CheckSphereCollision(Vector3 pos, float radius, float height, const CollisionObject &sphere)
{
    // Add a small margin to smooth out sliding
    float collisionMargin = 0.1f;

    // Treat player as a capsule and sphere as a sphere
    // Find closest point on player's cylinder axis to sphere center
    float playerBottom = pos.y;
    float playerTop = pos.y + height;

    float closestY = std::max(playerBottom, std::min(sphere.position.y, playerTop));
    Vector3 closestPoint = {pos.x, closestY, pos.z};

    float distance = Vector3Distance(closestPoint, sphere.position);
    return distance < (radius + sphere.radius + collisionMargin);
}

bool CollisionSystem::CheckCapsuleCollision(Vector3 pos, float radius, float height, const CollisionObject &capsule)
{
    // Add a small margin to smooth out sliding
    float collisionMargin = 0.1f;

    // Capsule-capsule collision
    // Simplified: treat both as cylinders with sphere caps
    float halfHeight1 = height / 2;
    float halfHeight2 = (capsule.height - capsule.radius * 2) / 2;

    Vector3 p1Bottom = {pos.x, pos.y, pos.z};
    Vector3 p1Top = {pos.x, pos.y + height, pos.z};

    Vector3 p2Bottom = {capsule.position.x, capsule.position.y - halfHeight2, capsule.position.z};
    Vector3 p2Top = {capsule.position.x, capsule.position.y + halfHeight2, capsule.position.z};

    // Get minimum distance between the two line segments
    float dist = DistancePointToLineSegment(p1Bottom, p2Bottom, p2Top);
    dist = std::min(dist, DistancePointToLineSegment(p1Top, p2Bottom, p2Top));
    dist = std::min(dist, DistancePointToLineSegment(p2Bottom, p1Bottom, p1Top));
    dist = std::min(dist, DistancePointToLineSegment(p2Top, p1Bottom, p1Top));

    return dist < (radius + capsule.radius + collisionMargin);
}

bool CollisionSystem::CheckCylinderCollision(Vector3 pos, float radius, float height, const CollisionObject &cylinder)
{
    // Add a small margin to smooth out sliding around the cylinder
    float collisionMargin = 0.1f;
    float effectiveRadius = cylinder.radius + collisionMargin;

    // Check 2D circle collision on XZ plane
    Vector2 playerPos2D = {pos.x, pos.z};
    Vector2 cylinderPos2D = {cylinder.position.x, cylinder.position.z};

    if (!Circle2DIntersect(playerPos2D, radius, cylinderPos2D, effectiveRadius))
        return false;

    // Check Y axis overlap
    float playerBottom = pos.y;
    float playerTop = pos.y + height;
    float cylinderBottom = cylinder.position.y - cylinder.height / 2;
    float cylinderTop = cylinder.position.y + cylinder.height / 2;

    return (playerBottom < cylinderTop && playerTop > cylinderBottom);
}

// ===== Helper Functions =====

float CollisionSystem::DistancePointToLineSegment(Vector3 point, Vector3 lineStart, Vector3 lineEnd)
{
    Vector3 lineVec = Vector3Subtract(lineEnd, lineStart);
    Vector3 pointVec = Vector3Subtract(point, lineStart);

    float lineLength = Vector3Length(lineVec);
    if (lineLength < 0.0001f)
        return Vector3Distance(point, lineStart);

    float t = Vector3DotProduct(pointVec, lineVec) / (lineLength * lineLength);
    t = std::max(0.0f, std::min(1.0f, t));

    Vector3 closestPoint = Vector3Add(lineStart, Vector3Scale(lineVec, t));
    return Vector3Distance(point, closestPoint);
}

bool CollisionSystem::Circle2DIntersect(Vector2 center1, float radius1, Vector2 center2, float radius2)
{
    float dx = center1.x - center2.x;
    float dy = center1.y - center2.y;
    float distSquared = dx * dx + dy * dy;
    float radiusSum = radius1 + radius2;
    return distSquared < (radiusSum * radiusSum);
}

Vector2 CollisionSystem::GetClosestPointOnBox2D(Vector2 point, Vector2 boxCenter, Vector2 boxHalfSize)
{
    Vector2 closest;
    closest.x = std::max(boxCenter.x - boxHalfSize.x, std::min(point.x, boxCenter.x + boxHalfSize.x));
    closest.y = std::max(boxCenter.y - boxHalfSize.y, std::min(point.y, boxCenter.y + boxHalfSize.y));
    return closest;
}
