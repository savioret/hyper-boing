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

bool AudioManager::preloadMusic(const char* filename)
{
    if (!isInitialized && !init())
        return false;
    
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

bool AudioManager::loadSound(const char* filename)
{
    if (!isInitialized && !init())
        return false;
    
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

bool AudioManager::playSound(const char* filename)
{
    if (!isInitialized && !init())
        return false;
    
    std::string key(filename);
    auto it = loadedSounds.find(key);
    
    if (it == loadedSounds.end())
    {
        if (!loadSound(filename))
            return false;
        it = loadedSounds.find(key);
    }
    
    if (Mix_PlayChannel(-1, it->second, 0) < 0)
    {
        LOG_ERROR("Failed to play sound %s: %s", filename, Mix_GetError());
        return false;
    }
    
    return true;
}

void AudioManager::stopAllSounds()
{
    if (isInitialized)
    {
        Mix_HaltChannel(-1);
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
    std::string key(filename);
    return loadedMusic.find(key) != loadedMusic.end();
}

bool AudioManager::isSoundLoaded(const char* filename) const
{
    std::string key(filename);
    return loadedSounds.find(key) != loadedSounds.end();
}
