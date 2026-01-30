#pragma once

#include <SDL.h>
#include <SDL_mixer.h>
#include <string>
#include <map>
#include <memory>

/**
 * @class AudioManager
 * @brief Singleton audio manager for music and sound effects using SDL2_mixer
 *
 * Provides centralized audio playback with support for:
 * - Multiple preloaded music tracks for instant switching
 * - Sound effects with channel management
 * - Fade in/out transitions
 * - Cross-platform support (Windows, Linux, Mac, Emscripten)
 * - ID-based audio registration for easy access
 *
 * Usage:
 * @code
 * AudioManager& audio = AudioManager::instance();
 * audio.init();
 * audio.registerTrack("bgm1", "assets/music/level1.ogg");
 * audio.openMusic("bgm1");
 * audio.play();
 * @endcode
 */
class AudioManager
{
private:
    static std::unique_ptr<AudioManager> s_instance;

    std::map<std::string, Mix_Music*> loadedMusic;    ///< Cached music tracks
    std::map<std::string, Mix_Chunk*> loadedSounds;   ///< Cached sound effects
    std::map<std::string, std::string> trackAliases;  ///< ID -> filepath mapping for music
    std::map<std::string, std::string> soundAliases;  ///< ID -> filepath mapping for sounds
    Mix_Music* currentMusic;                          ///< Currently playing music track
    std::string currentTrack;                         ///< Filename of current track
    bool isInitialized;                               ///< SDL_mixer initialization state

    /**
     * @brief Resolves a track ID or filepath to actual filepath
     * @param idOrPath Track ID (registered) or direct filepath
     * @return Resolved filepath
     */
    const char* resolveTrackPath(const char* idOrPath) const;
    
    /**
     * @brief Resolves a sound ID or filepath to actual filepath
     * @param idOrPath Sound ID (registered) or direct filepath
     * @return Resolved filepath
     */
    const char* resolveSoundPath(const char* idOrPath) const;

    /**
     * @brief Private constructor for singleton pattern
     */
    AudioManager();
    
    // Delete copy constructor and assignment operator
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;
    
    // Friend declaration for make_unique
    friend std::unique_ptr<AudioManager> std::make_unique<AudioManager>();

public:
    /**
     * @brief Gets the singleton instance
     * @return Reference to AudioManager instance
     */
    static AudioManager& instance();
    
    /**
     * @brief Destroys the singleton instance and cleans up resources
     */
    static void destroy();
    
    /**
     * @brief Initializes SDL_mixer audio subsystem
     * @return true if initialization succeeded, false otherwise
     * @note Must be called before any other audio operations
     * @note Initializes with 44.1kHz, stereo, 2048 buffer size, 16 mixing channels
     */
    bool init();

    /**
     * @brief Registers a music track with an ID for easy access
     * @param id Short identifier for the track (e.g., "bgm_level1")
     * @param filepath Path to music file (OGG, MP3, etc.)
     * @note Track is preloaded immediately for instant playback
     */
    void registerTrack(const char* id, const char* filepath);
    
    /**
     * @brief Registers a sound effect with an ID for easy access
     * @param id Short identifier for the sound (e.g., "jump", "explosion")
     * @param filepath Path to sound file (OGG, WAV)
     * @note Sound is preloaded immediately into memory
     */
    void registerSound(const char* id, const char* filepath);

    /**
     * @brief Opens a music track for playback
     * @param filename Track ID (if registered) or direct filepath
     * @return true if track loaded successfully, false otherwise
     * @note Stops currently playing music
     * @note Does not start playback - call play() to start
     */
    bool openMusic(const char* filename);
    
    /**
     * @brief Preloads a music track into memory without playing
     * @param filename Track ID or filepath to preload
     * @return true if preload succeeded, false otherwise
     * @note Useful for reducing lag when switching tracks
     */
    bool preloadMusic(const char* filename);
    
    /**
     * @brief Starts playback of the currently opened music track
     * @return true if playback started, false if no track is loaded
     * @note Loops indefinitely (-1 loops)
     */
    bool play();
    
    /**
     * @brief Stops music playback immediately
     * @return true if music was stopped, false if no music playing
     */
    bool stop();
    
    /**
     * @brief Resumes paused music or restarts current track
     * @return true if music resumed, false otherwise
     */
    bool resume();
    
    /**
     * @brief Closes all loaded music and sound effects
     * @note Frees all cached audio resources
     * @note Stops any playing music and sounds
     */
    void closeAll();
    
    /**
     * @brief Closes only music tracks, preserving sound effects
     * @note Useful for scene transitions where SFX should persist
     * @note Stops music playback and frees music memory
     */
    void closeMusic();
    
    /**
     * @brief Closes a specific music track
     * @param filename Track ID or filepath to close
     * @note Stops music if this track is currently playing
     */
    void closeTrack(const char* filename);

    /**
     * @brief Fades out currently playing music
     * @param fadeMs Duration of fade in milliseconds
     * @return true if fade started, false if no music playing
     * @note Music stops automatically when fade completes
     */
    bool fadeOutMusic(int fadeMs);
    
    /**
     * @brief Opens and plays music with fade-in effect
     * @param filename Track ID or filepath
     * @param fadeMs Fade-in duration in milliseconds
     * @param loop true to loop indefinitely, false to play once
     * @return true if playback started, false on error
     */
    bool playMusicWithFadeIn(const char* filename, int fadeMs, bool loop = true);
    
    /**
     * @brief Smoothly transitions from current music to new track
     * @param newTrack Track ID or filepath of new music
     * @param fadeMs Duration of cross-fade in milliseconds
     * @param loop true to loop new track, false to play once
     * @return true if transition started, false on error
     * @note Current music fades out while new music fades in
     */
    bool crossFadeMusic(const char* newTrack, int fadeMs, bool loop = true);
    
    /**
     * @brief Loads a sound effect into memory
     * @param filename Sound ID or filepath
     * @return true if load succeeded, false otherwise
     * @note Automatically called by playSound() if sound not cached
     */
    bool loadSound(const char* filename);
    
    /**
     * @brief Plays a sound effect once
     * @param filename Sound ID or filepath
     * @return Channel number if successful, -1 on error
     * @note Loads sound automatically if not cached
     * @note Plays on first available channel
     */
    int playSound(const char* filename);
    
    /**
     * @brief Plays a sound effect with fade-in
     * @param filename Sound ID or filepath
     * @param fadeMs Fade-in duration in milliseconds
     * @param loop true to loop sound, false to play once
     * @return Channel number if successful, -1 on error
     * @note Useful for ambient sounds or looping effects
     * @note Save channel number to stop sound later with stopChannel()
     */
    int playSoundWithFadeIn(const char* filename, int fadeMs, bool loop = false);
    
    /**
     * @brief Stops all playing sound effects immediately
     * @note Does not affect music playback
     */
    void stopAllSounds();
    
    /**
     * @brief Stops a specific audio channel
     * @param channel Channel number (from playSoundWithFadeIn)
     * @note Does nothing if channel is invalid or not playing
     */
    void stopChannel(int channel);
    
    /**
     * @brief Checks if music is currently playing
     * @return true if music is playing, false otherwise
     */
    bool getIsPlaying() const;
    
    /**
     * @brief Gets the filename of currently loaded music
     * @return Current track filename or empty string if none
     */
    const std::string& getCurrentTrack() const { return currentTrack; }
    
    /**
     * @brief Checks if a music track is loaded in cache
     * @param filename Track ID or filepath to check
     * @return true if track is cached, false otherwise
     */
    bool isTrackLoaded(const char* filename) const;
    
    /**
     * @brief Checks if a sound effect is loaded in cache
     * @param filename Sound ID or filepath to check
     * @return true if sound is cached, false otherwise
     */
    bool isSoundLoaded(const char* filename) const;
};
