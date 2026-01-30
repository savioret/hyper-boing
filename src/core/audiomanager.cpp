#include "audiomanager.h"
#include "logger.h"
#include <sys/stat.h>

// Initialize static singleton instance
std::unique_ptr<AudioManager> AudioManager::s_instance = nullptr;

// Helper function to check if file exists
static bool fileExists(const char* filename)
{
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

AudioManager::AudioManager()
    : currentMusic(nullptr), currentTrack(""), isInitialized(false)
{
}

AudioManager& AudioManager::instance()
{
    if (!s_instance)
    {
        s_instance = std::make_unique<AudioManager>();
    }
    return *s_instance;
}

void AudioManager::destroy()
{
    if (s_instance)
    {
        s_instance->closeAll();
        
        if (s_instance->isInitialized)
        {
            Mix_CloseAudio();
        }
        
        s_instance.reset();
    }
}

bool AudioManager::init()
{
    if (isInitialized)
        return true;
    
    LOG_INFO("Initializing SDL_mixer...");
    
    // Initialize SDL_mixer with default frequency and format
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        LOG_ERROR("SDL_mixer initialization failed: %s", Mix_GetError());
        return false;
    }
    
    // Allocate mixing channels for sound effects
    Mix_AllocateChannels(16);
    
    isInitialized = true;
    LOG_SUCCESS("SDL_mixer initialized");
    
    // Print supported audio decoders
    LOG_DEBUG("=== SDL_mixer Audio Decoders ===");
    int numDecoders = Mix_GetNumMusicDecoders();
    LOG_DEBUG("Available music decoders: %d", numDecoders);
    for (int i = 0; i < numDecoders; i++)
    {
        LOG_TRACE("  - %s", Mix_GetMusicDecoder(i));
    }
    
    return true;
}

const char* AudioManager::resolveTrackPath(const char* idOrPath) const
{
    auto it = trackAliases.find(idOrPath);
    return (it != trackAliases.end()) ? it->second.c_str() : idOrPath;
}

const char* AudioManager::resolveSoundPath(const char* idOrPath) const
{
    auto it = soundAliases.find(idOrPath);
    return (it != soundAliases.end()) ? it->second.c_str() : idOrPath;
}

void AudioManager::registerTrack(const char* id, const char* filepath)
{
    trackAliases[id] = filepath;
    preloadMusic(filepath);
    LOG_DEBUG("Registered track ID '%s' -> '%s'", id, filepath);
}

void AudioManager::registerSound(const char* id, const char* filepath)
{
    soundAliases[id] = filepath;
    loadSound(filepath);
    LOG_DEBUG("Registered sound ID '%s' -> '%s'", id, filepath);
}

bool AudioManager::preloadMusic(const char* filename)
{
    if (!isInitialized && !init())
        return false;

    // Resolve ID to filepath if it's an alias
    filename = resolveTrackPath(filename);

    std::string filenameStr(filename);
    
    // Check if already loaded
    if (loadedMusic.find(filenameStr) != loadedMusic.end())
    {
        LOG_TRACE("Music already loaded: %s", filename);
        return true;
    }
    
    // Check if file exists
    if (!fileExists(filename))
    {
        LOG_ERROR("Music file not found: %s", filename);
        return false;
    }
    
    LOG_DEBUG("Loading music: %s", filename);
    
    Mix_Music* music = Mix_LoadMUS(filename);
    if (!music)
    {
        LOG_ERROR("Failed to load music %s: %s", filename, Mix_GetError());
        LOG_WARNING("Make sure the file is a valid OGG Vorbis or MP3 file");
        return false;
    }
    
    loadedMusic[filenameStr] = music;
    LOG_SUCCESS("Preloaded music: %s", filename);
    
    return true;
}

bool AudioManager::openMusic(const char* filename)
{
    if (!isInitialized && !init())
        return false;

    // Resolve ID to filepath if it's an alias
    filename = resolveTrackPath(filename);

    std::string filenameStr(filename);
    
    // Stop current music if playing
    if (Mix_PlayingMusic())
    {
        Mix_HaltMusic();
    }
    
    // Check if track is already loaded
    auto it = loadedMusic.find(filenameStr);
    if (it != loadedMusic.end())
    {
        currentMusic = it->second;
        currentTrack = filename;
        LOG_TRACE("Switched to preloaded music: %s", filename);
        return true;
    }
    
    // Load new track
    LOG_DEBUG("Loading music on-demand: %s", filename);
    
    Mix_Music* music = Mix_LoadMUS(filename);
    if (!music)
    {
        LOG_ERROR("Failed to load music %s: %s", filename, Mix_GetError());
        return false;
    }
    
    loadedMusic[filenameStr] = music;
    currentMusic = music;
    currentTrack = filename;
    LOG_SUCCESS("Loaded music: %s", filename);
    
    return true;
}

bool AudioManager::play()
{
    if (!isInitialized || !currentMusic)
        return false;
    
    if (Mix_PlayMusic(currentMusic, -1) < 0)
    {
        LOG_ERROR("Failed to play music: %s", Mix_GetError());
        return false;
    }
    
    return true;
}

bool AudioManager::stop()
{
    if (!isInitialized)
        return false;
    
    Mix_HaltMusic();
    return true;
}

bool AudioManager::resume()
{
    if (!isInitialized)
        return false;
    
    if (Mix_PausedMusic())
    {
        Mix_ResumeMusic();
    }
    else if (currentMusic)
    {
        return play();
    }
    
    return true;
}

void AudioManager::closeTrack(const char* filename)
{
    // Resolve ID to filepath if it's an alias
    filename = resolveTrackPath(filename);

    std::string key(filename);
    auto it = loadedMusic.find(key);
    
    if (it != loadedMusic.end())
    {
        if (currentMusic == it->second)
        {
            Mix_HaltMusic();
            currentMusic = nullptr;
            currentTrack = "";
        }
        
        Mix_FreeMusic(it->second);
        loadedMusic.erase(it);
    }
}

void AudioManager::closeAll()
{
    Mix_HaltMusic();
    Mix_HaltChannel(-1);
    
    for (auto& pair : loadedMusic)
    {
        Mix_FreeMusic(pair.second);
    }
    loadedMusic.clear();
    
    for (auto& pair : loadedSounds)
    {
        Mix_FreeChunk(pair.second);
    }
    loadedSounds.clear();
    
    currentMusic = nullptr;
    currentTrack = "";
}

void AudioManager::closeMusic()
{
    Mix_HaltMusic();
    
    for (auto& pair : loadedMusic)
    {
        Mix_FreeMusic(pair.second);
    }
    loadedMusic.clear();
    
    currentMusic = nullptr;
    currentTrack = "";
}

bool AudioManager::loadSound(const char* filename)
{
    if (!isInitialized && !init())
        return false;

    // Resolve ID to filepath if it's an alias
    filename = resolveSoundPath(filename);

    std::string key(filename);
    
    if (loadedSounds.find(key) != loadedSounds.end())
    {
        return true;
    }
    
    Mix_Chunk* sound = Mix_LoadWAV(filename);
    if (!sound)
    {
        LOG_ERROR("Failed to load sound %s: %s", filename, Mix_GetError());
        return false;
    }
    
    loadedSounds[key] = sound;
    LOG_DEBUG("Loaded sound: %s", filename);
    
    return true;
}

int AudioManager::playSound(const char* filename)
{
    if (!isInitialized && !init())
        return -1;

    // Resolve ID to filepath if it's an alias
    filename = resolveSoundPath(filename);

    std::string key(filename);
    auto it = loadedSounds.find(key);
    
    if (it == loadedSounds.end())
    {
        if (!loadSound(filename))
            return -1;
        it = loadedSounds.find(key);
    }

    int channel = Mix_PlayChannel(-1, it->second, 0);
    if ( channel < 0 )
    {
        LOG_ERROR("Failed to play sound %s: %s", filename, Mix_GetError());
    }
    
    return channel;
}

int AudioManager::playSoundWithFadeIn(const char* filename, int fadeMs, bool loop)
{
    if (!isInitialized && !init())
        return -1;

    // Resolve ID to filepath if it's an alias
    filename = resolveSoundPath(filename);

    std::string key(filename);
    auto it = loadedSounds.find(key);

    // Load if not already loaded
    if (it == loadedSounds.end())
    {
        if (!loadSound(filename))
            return -1;
        it = loadedSounds.find(key);
    }

    // Play with fade on first available channel
    int loops = loop ? -1 : 0;
    int channel = Mix_FadeInChannel(-1, it->second, loops, fadeMs);

    if (channel < 0)
    {
        LOG_ERROR("Failed to fade in sound %s: %s", filename, Mix_GetError());
        return -1;
    }

    LOG_DEBUG("Fading in sound over %d ms on channel %d: %s", fadeMs, channel, filename);
    return channel;
}

void AudioManager::stopAllSounds()
{
    if (isInitialized)
    {
        Mix_HaltChannel(-1);
    }
}

void AudioManager::stopChannel(int channel)
{
    if (isInitialized && channel >= 0)
    {
        Mix_HaltChannel(channel);
    }
}

bool AudioManager::getIsPlaying() const
{
    if (!isInitialized)
        return false;
    
    return Mix_PlayingMusic() != 0;
}

bool AudioManager::isTrackLoaded(const char* filename) const
{
    // Resolve ID to filepath if it's an alias
    filename = resolveTrackPath(filename);

    std::string key(filename);
    return loadedMusic.find(key) != loadedMusic.end();
}

bool AudioManager::isSoundLoaded(const char* filename) const
{
    // Resolve ID to filepath if it's an alias
    filename = resolveSoundPath(filename);

    std::string key(filename);
    return loadedSounds.find(key) != loadedSounds.end();
}

bool AudioManager::fadeOutMusic(int fadeMs)
{
    if (!isInitialized)
        return false;

    if (!Mix_PlayingMusic())
        return false;

    Mix_FadeOutMusic(fadeMs);
    return true;
}

bool AudioManager::playMusicWithFadeIn(const char* filename, int fadeMs, bool loop)
{
    if (!openMusic(filename))
        return false;

    if (!currentMusic)
        return false;

    int loops = loop ? -1 : 0;
    if (Mix_FadeInMusic(currentMusic, loops, fadeMs) < 0)
    {
        LOG_ERROR("Failed to fade in music: %s", Mix_GetError());
        return false;
    }

    LOG_DEBUG("Fading in music over %d ms: %s", fadeMs, filename);
    return true;
}

bool AudioManager::crossFadeMusic(const char* newTrack, int fadeMs, bool loop)
{
    if (!isInitialized && !init())
        return false;

    // Fade out current music if playing
    if (Mix_PlayingMusic())
    {
        fadeOutMusic(fadeMs);
    }

    // Load and fade in new track
    return playMusicWithFadeIn(newTrack, fadeMs, loop);
}
