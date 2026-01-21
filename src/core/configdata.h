#pragma once

#include <string>

/**
 * ConfigData class
 *
 * Handles loading and saving game configuration to a file.
 */
class ConfigData
{
public:
    ConfigData();
    ~ConfigData();

    bool load();
    bool save();

private:
    void loadDefaults();
    std::string configPath;
    
    // Helper to get the path
    void initPath();
};
