/**
 * @file PostProcessingRenderer.h
 * @brief Post-processing effects pipeline
 */

#pragma once

#include "raylib.h"
#include "raymath.h"

/**
 * @brief MSAA render texture with resolve support
 *
 * Contains multisampled color and depth textures for rendering,
 * plus resolved textures for post-processing.
 */
struct MSAARenderTexture
{
    Texture colorMS;          ///< Multisampled color texture (GL_TEXTURE_2D_MULTISAMPLE)
    Texture colorResolved;    ///< Resolved color texture for post-processing
    Texture depthMS;          ///< Multisampled depth texture
    Texture depthResolved;    ///< Resolved depth texture
    unsigned int fboMS;       ///< Framebuffer object for MSAA rendering
    unsigned int fboResolve;  ///< Framebuffer object for resolve pass
    unsigned int sampleCount; ///< Number of samples (4, 8, or 16)
    int width;                ///< Texture width
    int height;               ///< Texture height
};

/**
 * @brief Post-processing renderer applying fullscreen effects
 *
 * Captures scene to texture and applies effects like grayscale
 * and depth buffer visualization. Supports multiple effect passes.
 */
class PostProcessingRenderer
{
public:
    /**
     * @brief Construct post-processing renderer
     */
    PostProcessingRenderer();

    /**
     * @brief Destroy renderer and cleanup resources
     */
    ~PostProcessingRenderer();

    /**
     * @brief Initialize render textures and shaders
     * @param screenWidth Screen width in pixels
     * @param screenHeight Screen height in pixels
     */
    void Init(int screenWidth, int screenHeight);

    /**
     * @brief Shutdown and cleanup resources
     */
    void Shutdown();

    /**
     * @brief Begin rendering to offscreen texture
     *
     * Call before rendering scene geometry.
     */
    void BeginSceneCapture();

    /**
     * @brief End scene capture
     *
     * Call after all scene geometry is rendered.
     */
    void EndSceneCapture();

    /**
     * @brief Apply configured post-processing effects
     *
     * Renders processed scene to screen.
     */
    void ApplyEffects();

    /**
     * @brief Enable or disable grayscale effect
     * @param enable True to enable grayscale
     */
    void SetGrayscaleEnabled(bool enable) { enableGrayscale = enable; }

    /**
     * @brief Check if grayscale is enabled
     * @return True if enabled
     */
    bool GetGrayscaleEnabled() const { return enableGrayscale; }

    /**
     * @brief Enable or disable depth buffer visualization
     * @param enable True to show depth buffer
     */
    void SetDepthDebugEnabled(bool enable) { enableDepthDebug = enable; }

    /**
     * @brief Check if depth debug is enabled
     * @return True if enabled
     */
    bool GetDepthDebugEnabled() const { return enableDepthDebug; }

    /**
     * @brief Get scene render texture
     * @return Scene texture with depth attachment
     */
    RenderTexture2D GetSceneTexture() const { return sceneTexture; }

    /**
     * @brief Enable MSAA rendering
     * @param sampleCount Number of samples (4, 8, or 16)
     */
    void EnableMSAA(unsigned int sampleCount);

    /**
     * @brief Disable MSAA rendering
     */
    void DisableMSAA();

    /**
     * @brief Check if MSAA is enabled
     * @return True if MSAA is active
     */
    bool IsMSAAEnabled() const { return msaaTexture.sampleCount > 0; }

    /**
     * @brief Query maximum MSAA level supported by GPU
     * @return Maximum sample count (4, 8, 16, or 0 if unsupported)
     */
    static unsigned int QueryMaxMSAASamples();

    /**
     * @brief Create MSAA render texture
     * @param width Texture width in pixels
     * @param height Texture height in pixels
     * @param sampleCount Number of samples (4, 8, or 16)
     * @return MSAA render texture structure
     */
    static MSAARenderTexture LoadMSAARenderTexture(int width, int height, unsigned int sampleCount);

    /**
     * @brief Resolve MSAA texture to regular texture
     * @param msaaTexture MSAA render texture to resolve
     */
    static void ResolveMSAA(const MSAARenderTexture &msaaTexture);

    /**
     * @brief Unload MSAA render texture resources
     * @param msaaTexture MSAA render texture to unload
     */
    static void UnloadMSAARenderTexture(MSAARenderTexture &msaaTexture);

private:
    RenderTexture2D sceneTexture;  ///< Main scene render target with depth attachment
    MSAARenderTexture msaaTexture; ///< MSAA render texture (if MSAA enabled)

    Shader grayscaleShader; ///< Grayscale effect shader
    Shader depthShader;     ///< Depth visualization shader

    int width;  ///< Screen width
    int height; ///< Screen height

    bool enableGrayscale;  ///< Grayscale effect toggle
    bool enableDepthDebug; ///< Depth debug toggle

    /**
     * @brief Render fullscreen quad with shader
     * @param shader Shader to apply
     * @param source Source texture
     */
    void RenderFullscreenQuad(Shader shader, RenderTexture2D source);

    /**
     * @brief Create render texture with depth attachment
     * @param screenWidth Width in pixels
     * @param screenHeight Height in pixels
     * @return Render texture with depth buffer
     */
    RenderTexture2D LoadRenderTextureWithDepth(int screenWidth, int screenHeight);

    /**
     * @brief Unload render texture with depth buffer
     * @param target Render texture to unload
     */
    void UnloadRenderTextureWithDepth(RenderTexture2D target);
};
