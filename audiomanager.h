#pragma once

#include <SDL.h>
#include <SDL_mixer.h>
#include <string>
#include <map>
#include <memory>

/**
 * AudioManager class (Singleton)
 *
 * Modern audio playback using SDL2_mixer for music and sound effects.
 * Supports preloading multiple tracks for instant switching.
 * Cross-platform (Windows, Linux, Mac, Emscripten).
 */
class AudioManager
{
private:
    static std::unique_ptr<AudioManager> s_instance;
    
    std::map<std::string, Mix_Music*> loadedMusic;
    std::map<std::string, Mix_Chunk*> loadedSounds;
    Mix_Music* currentMusic;
    std::string currentTrack;
    bool isInitialized;
    
    // Private constructor for singleton
    AudioManager();
    
    // Delete copy constructor and assignment operator
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;
    
    // Friend declaration for make_unique
    friend std::unique_ptr<AudioManager> std::make_unique<AudioManager>();

public:
    // Singleton accessor
    static AudioManager& instance();
    static void destroy();
    
    // Initialization
    bool init();
    
    // Music control methods
    bool openMusic(const char* filename);
    bool preloadMusic(const char* filename);
    bool play();
    bool stop();
    bool resume();
    void closeAll();
    void closeTrack(const char* filename);
    
    // Sound effects
    bool loadSound(const char* filename);
    bool playSound(const char* filename);
    void stopAllSounds();
    
    // State queries
    bool getIsPlaying() const;
    const std::string& getCurrentTrack() const { return currentTrack; }
    bool isTrackLoaded(const char* filename) const;
    bool isSoundLoaded(const char* filename) const;
};
