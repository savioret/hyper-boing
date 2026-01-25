#pragma once

#include <SDL.h>
#include <string>
#include <vector>
#include <functional>
#include "logger.h"

// Forward declarations
class Graph;
class BMFontLoader;
class Sprite;
class BMFontRenderer;

/**
 * Command handler function type
 */
using CommandHandler = std::function<void(const std::string& args)>;

/**
 * Registered command structure
 */
struct ConsoleCommand
{
    std::string name;
    std::string description;
    CommandHandler handler;
};

/**
 * AppConsole - In-game developer console
 * 
 * Renders an overlay with recent log entries using bitmap fonts.
 * Provides a command prompt for slash commands.
 * 
 * Features:
 *   - Transparent overlay rendering
 *   - Colored log entries (from Logger)
 *   - Command input with "/" prefix
 *   - Scrollable log history
 *   - Toggleable visibility (F9 or backtick)
 * 
 * Usage:
 *   AppConsole console;
 *   console.init(&graph);
 *   console.registerCommand("echo", "Echo text", [](const std::string& args) {
 *       LOG_INFO("%s", args.c_str());
 *   });
 *   
 *   // In game loop:
 *   console.handleEvent(event);
 *   console.render();
 */
class AppConsole
{
private:
    static std::unique_ptr<AppConsole> s_instance;
    
    // Rendering
    Graph* graph;
    std::unique_ptr<BMFontRenderer> fontRenderer;  // Manages font loading internally
    bool fontOwned;  // Whether we own the font resources
    
    // State
    bool visible;
    bool initialized;
    float overlayAlpha;
    
    // Input
    std::string inputBuffer;
    size_t cursorPosition;
    bool cursorVisible;
    Uint32 cursorBlinkTime;
    std::vector<std::string> commandHistory;
    int historyIndex;
    
    // Display settings
    int maxVisibleLines;
    int scrollOffset;
    int fontSize;
    int lineHeight;
    int padding;
    int promptHeight;
    
    // Commands
    std::vector<ConsoleCommand> commands;
    
    // Private constructor for singleton
    AppConsole();
    AppConsole(const AppConsole&) = delete;
    AppConsole& operator=(const AppConsole&) = delete;
    
    // Friend declaration for make_unique
    friend std::unique_ptr<AppConsole> std::make_unique<AppConsole>();
    
    // Internal methods
    void renderBackground();
    void renderLogs();
    void renderPrompt();
    void processInput(const std::string& input);
    
    // Built-in commands
    void registerBuiltinCommands();
    void cmdHelp(const std::string& args);
    void cmdEcho(const std::string& args);
    void cmdClear(const std::string& args);
    void cmdLevel(const std::string& args);
    void cmdNext(const std::string& args);
    void cmdGoto(const std::string& args);
    void cmdWeapon(const std::string& args);
    void cmdBoxes(const std::string& args);
    void cmdEvents(const std::string& args);

public:
    // Singleton accessor
    static AppConsole& instance();
    static void destroy();
    
    // Initialization
    bool init(Graph* gr);
    bool initWithFont(Graph* gr, BMFontLoader* font, Sprite* texture);
    void release();
    
    // Event handling
    bool handleEvent(const SDL_Event& event);
    void handleTextInput(const char* text);
    void handleKeyDown(SDL_Keycode key, SDL_Keymod mod);
    
    // Rendering
    void render();
    void update();
    
    // Visibility
    void show() { visible = true; }
    void hide() { visible = false; }
    void toggle() { visible = !visible; }
    bool isVisible() const { return visible; }
    
    // Commands
    void registerCommand(const std::string& name, const std::string& desc, CommandHandler handler);
    void unregisterCommand(const std::string& name);

    // Execute a command programmatically (for stage actions)
    void executeCommand(const std::string& input);
    
    // Configuration
    void setOverlayAlpha(float alpha) { overlayAlpha = alpha; }
    void setMaxVisibleLines(int lines) { maxVisibleLines = lines; }
    void setFontSize(int size) { fontSize = size; }
    
    // Scrolling
    void scrollUp(int lines = 1);
    void scrollDown(int lines = 1);
    void scrollToBottom();
    
    // Direct log (bypasses Logger, for console-only messages)
    void print(const std::string& message, LogColor color = LogColor::Info());
};

// Global accessor macro
#define CONSOLE AppConsole::instance()
