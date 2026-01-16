#include "audiomanager.h"
#include <cstdio>
#include <sys/stat.h>

// Initialize static singleton instance
AudioManager* AudioManager::s_instance = nullptr;

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
        s_instance = new AudioManager();
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
        
        delete s_instance;
        s_instance = nullptr;
    }
}

bool AudioManager::init()
{
    if (isInitialized)
        return true;
    
    printf("Initializing SDL_mixer...\n");
    
    // Initialize SDL_mixer with default frequency and format
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        printf("SDL_mixer initialization failed: %s\n", Mix_GetError());
        return false;
    }
    
    // Allocate mixing channels for sound effects
    Mix_AllocateChannels(16);
    
    isInitialized = true;
    printf("SDL_mixer initialized successfully\n");
    
    // Print supported audio decoders
    printf("\n=== SDL_mixer Audio Decoders ===\n");
    int numDecoders = Mix_GetNumMusicDecoders();
    printf("Available music decoders: %d\n", numDecoders);
    for (int i = 0; i < numDecoders; i++)
    {
        printf("  - %s\n", Mix_GetMusicDecoder(i));
    }
    printf("=================================\n\n");
    
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
        printf("Music already loaded: %s\n", filename);
        return true; // Already loaded
    }
    
    // Check if file exists
    if (!fileExists(filename))
    {
        printf("ERROR: Music file not found: %s\n", filename);
        printf("       Please check that the file exists in the correct location\n");
        return false;
    }
    
    printf("Attempting to load music: %s\n", filename);
    
    Mix_Music* music = Mix_LoadMUS(filename);
    if (!music)
    {
        printf("ERROR: Failed to load music %s\n", filename);
        printf("       SDL_mixer error: %s\n", Mix_GetError());
        printf("       Make sure the file is a valid OGG Vorbis or MP3 file\n");
        printf("       Try re-converting the file with FFmpeg or Audacity\n");
        return false;
    }
    
    loadedMusic[filenameStr] = music;
    printf("Successfully preloaded music: %s\n", filename);
    
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
        // Track already loaded, just switch to it
        currentMusic = it->second;
        currentTrack = filename;
        printf("Switched to preloaded music: %s\n", filename);
        return true;
    }
    
    // Load new track
    printf("Loading music on-demand: %s\n", filename);
    
    Mix_Music* music = Mix_LoadMUS(filename);
    if (!music)
    {
        printf("ERROR: Failed to load music %s\n", filename);
        printf("       SDL_mixer error: %s\n", Mix_GetError());
        printf("       Make sure the file exists and is a valid OGG/MP3 file\n");
        return false;
    }
    
    loadedMusic[filenameStr] = music;
    currentMusic = music;
    currentTrack = filename;
    printf("Successfully loaded music: %s\n", filename);
    
    return true;
}

bool AudioManager::play()
{
    if (!isInitialized || !currentMusic)
        return false;
    
    // Play music with infinite loop (-1)
    if (Mix_PlayMusic(currentMusic, -1) < 0)
    {
        printf("Failed to play music: %s\n", Mix_GetError());
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
        // If not paused, start playing
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
    // Stop all audio
    Mix_HaltMusic();
    Mix_HaltChannel(-1); // Stop all sound effect channels
    
    // Free all music
    for (auto& pair : loadedMusic)
    {
        Mix_FreeMusic(pair.second);
    }
    loadedMusic.clear();
    
    // Free all sounds
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
    
    // Check if already loaded
    if (loadedSounds.find(key) != loadedSounds.end())
    {
        return true;
    }
    
    Mix_Chunk* sound = Mix_LoadWAV(filename);
    if (!sound)
    {
        printf("Failed to load sound %s: %s\n", filename, Mix_GetError());
        return false;
    }
    
    loadedSounds[key] = sound;
    printf("Loaded sound: %s\n", filename);
    
    return true;
}

bool AudioManager::playSound(const char* filename)
{
    if (!isInitialized && !init())
        return false;
    
    std::string key(filename);
    auto it = loadedSounds.find(key);
    
    // Load if not already loaded
    if (it == loadedSounds.end())
    {
        if (!loadSound(filename))
            return false;
        it = loadedSounds.find(key);
    }
    
    // Play on any available channel, 0 loops (play once)
    if (Mix_PlayChannel(-1, it->second, 0) < 0)
    {
        printf("Failed to play sound %s: %s\n", filename, Mix_GetError());
        return false;
    }
    
    return true;
}

void AudioManager::stopAllSounds()
{
    if (isInitialized)
    {
        Mix_HaltChannel(-1); // Stop all channels
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
