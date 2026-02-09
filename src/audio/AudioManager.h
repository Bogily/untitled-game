/**
 * @file AudioManager.h
 * @brief Audio system manager for sound effects and music
 */

#pragma once

/**
 * @brief Audio manager for game sound effects
 *
 * Handles initialization and playback of audio resources.
 */
class AudioManager
{
public:
    /**
     * @brief Initialize audio system
     */
    void Init();

    /**
     * @brief Play sound effect by name
     * @param soundName Sound identifier
     */
    void PlaySoundEffect(const char *soundName);
};
