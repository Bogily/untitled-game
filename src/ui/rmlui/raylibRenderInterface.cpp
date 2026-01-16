#include "raylibRenderInterface.h"
#include "raylibFileInterface.h"
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include <vector>

struct CompiledGeometry
{
    std::vector<Rml::Vertex> vertices;
    std::vector<int> indices;
};

static Texture2D defaultTexture = {0};
static Rml::Matrix4f *activeTransform = nullptr;

static void RenderTriangle(const Rml::Vertex &vertex, const Rml::Vector2f &translation)
{
    rlColor4ub(vertex.colour.red, vertex.colour.green, vertex.colour.blue, vertex.colour.alpha);
    rlTexCoord2f(vertex.tex_coord.x, vertex.tex_coord.y);

    if (activeTransform)
    {
        Vector3 vec{vertex.position.x + translation.x, vertex.position.y + translation.y, 0.0f};
        Vector3 dest = Vector3Transform(vec, MatrixTranspose(*(Matrix *)activeTransform->data()));
        rlVertex2f(dest.x, dest.y);
    }
    else
    {
        rlVertex2f(vertex.position.x + translation.x, vertex.position.y + translation.y);
    }
}

Rml::CompiledGeometryHandle RaylibRenderInterface::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)
{
    auto *geo = new CompiledGeometry();
    geo->vertices.assign(vertices.begin(), vertices.end());
    geo->indices.assign(indices.begin(), indices.end());
    return reinterpret_cast<Rml::CompiledGeometryHandle>(geo);
}

void RaylibRenderInterface::RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture)
{
    auto *geo = reinterpret_cast<CompiledGeometry *>(geometry);
    if (!geo)
        return;

    Texture2D target = texture == 0 ? defaultTexture : *reinterpret_cast<Texture2D *>(texture);
    if (target.id == 0 || geo->indices.size() < 3)
        return;

    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();

    rlBegin(RL_TRIANGLES);
    rlSetTexture(target.id);

    for (size_t i = 0; i + 2 < geo->indices.size(); i += 3)
    {
        if (rlCheckRenderBatchLimit(3))
        {
            rlBegin(RL_TRIANGLES);
            rlSetTexture(target.id);
        }

        RenderTriangle(geo->vertices[geo->indices[i]], translation);
        RenderTriangle(geo->vertices[geo->indices[i + 1]], translation);
        RenderTriangle(geo->vertices[geo->indices[i + 2]], translation);
    }

    rlEnd();
    rlSetTexture(0);
    rlEnableBackfaceCulling();
}

void RaylibRenderInterface::ReleaseGeometry(Rml::CompiledGeometryHandle geometry)
{
    delete reinterpret_cast<CompiledGeometry *>(geometry);
}

void RaylibRenderInterface::EnableScissorRegion(bool enable)
{
    if (!enable)
        EndScissorMode();
}

void RaylibRenderInterface::SetScissorRegion(Rml::Rectanglei region)
{
    BeginScissorMode(region.Left(), region.Top(), region.Width(), region.Height());
}

Rml::TextureHandle RaylibRenderInterface::LoadTexture(Rml::Vector2i &texture_dimensions, const Rml::String &source)
{
    const std::string path = RaylibFileInterface::ParsePath(source);
    if (!::FileExists(path.c_str()))
        return 0;

    Texture2D texture = ::LoadTexture(path.c_str());
    if (texture.id == 0)
        return 0;

    auto *allocation = static_cast<Texture2D *>(RL_MALLOC(sizeof(Texture2D)));
    allocation[0] = texture;
    texture_dimensions.x = texture.width;
    texture_dimensions.y = texture.height;
    return reinterpret_cast<Rml::TextureHandle>(allocation);
}

Rml::TextureHandle RaylibRenderInterface::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions)
{
    if (defaultTexture.id == 0)
    {
        Image img = GenImageColor(2, 2, WHITE);
        defaultTexture = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    Image img = GenImageColor(source_dimensions.x, source_dimensions.y, BLANK);
    img.data = const_cast<void *>(reinterpret_cast<const void *>(source.data()));
    Texture2D texture = LoadTextureFromImage(img);
    if (texture.id == 0)
        return 0;

    auto *allocation = static_cast<Texture2D *>(RL_MALLOC(sizeof(Texture2D)));
    allocation[0] = texture;
    return reinterpret_cast<Rml::TextureHandle>(allocation);
}

void RaylibRenderInterface::ReleaseTexture(Rml::TextureHandle texture)
{
    if (texture == 0)
        return;
    auto *tex = reinterpret_cast<Texture2D *>(texture);
    ::UnloadTexture(*tex);
    RL_FREE(tex);
}

RaylibRenderInterface::~RaylibRenderInterface()
{
    if (defaultTexture.id != 0)
        UnloadTexture(defaultTexture);
    defaultTexture = {0};
}

void RaylibRenderInterface::SetTransform(const Rml::Matrix4f *newTransform)
{
    activeTransform = const_cast<Rml::Matrix4f *>(newTransform);
}

void RaylibRenderInterface::BeginFrame()
{
    SetTransform(nullptr);
}

void RaylibRenderInterface::EndFrame()
{
}
