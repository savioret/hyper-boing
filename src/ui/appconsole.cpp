#include "appconsole.h"
#include "graph.h"
#include "sprite.h"
#include "bmfont.h"
#include "logger.h"
#include "main.h"
#include <algorithm>
#include <sstream>

// Initialize static singleton instance
std::unique_ptr<AppConsole> AppConsole::s_instance = nullptr;

AppConsole::AppConsole()
    : graph(nullptr), fontRenderer(nullptr),
      fontOwned(false), visible(false), initialized(false), overlayAlpha(0.85f),
      cursorPosition(0), cursorVisible(true), cursorBlinkTime(0), historyIndex(-1),
      maxVisibleLines(20), scrollOffset(0), fontSize(10), lineHeight(12), 
      padding(10), promptHeight(24)
{
}

AppConsole& AppConsole::instance()
{
    if (!s_instance)
    {
        s_instance = std::make_unique<AppConsole>();
    }
    return *s_instance;
}

void AppConsole::destroy()
{
    if (s_instance)
    {
        s_instance->release();
        s_instance.reset();
    }
}

bool AppConsole::init(Graph* gr)
{
    if (initialized)
        return true;
    
    graph = gr;
    
    // SIMPLIFIED: Load font with one call
    fontRenderer = std::make_unique<BMFontRenderer>();
    if (!fontRenderer->loadFont(graph, "assets/fonts/monospaced_10.fnt"))
    {
        LOG_WARNING("AppConsole: Could not load monospaced_10.fnt, trying fallback");
        
        // Try fallback font
        if (!fontRenderer->loadFont(graph, "assets/fonts/thickfont_grad_64.fnt"))
        {
            LOG_ERROR("AppConsole: Failed to load any font, using system font");
            // fontRenderer will use system font as fallback
        }
    }
    
    fontRenderer->setScale(1.0f);
    
    fontOwned = true;  // We own the fontRenderer which manages everything internally
    initialized = true;
    
    // Register built-in commands
    registerBuiltinCommands();
    
    LOG_SUCCESS("AppConsole initialized");
    
    return true;
}

bool AppConsole::initWithFont(Graph* gr, BMFontLoader* font, Sprite* texture)
{
    if (initialized)
        return true;
    
    graph = gr;
    
    // Create font renderer with external resources (legacy API)
    fontRenderer = std::make_unique<BMFontRenderer>();
    fontRenderer->init(graph, font, texture);
    fontRenderer->setScale(1.0f);
    
    fontOwned = false;  // We don't own the font resources
    initialized = true;
    
    // Register built-in commands
    registerBuiltinCommands();
    
    LOG_SUCCESS("AppConsole initialized with external font");
    
    return true;
}

void AppConsole::release()
{
    if (fontOwned && fontRenderer)
    {
        fontRenderer->release();
    }
    
    fontRenderer.reset();
    graph = nullptr;
    
    commands.clear();
    commandHistory.clear();
    inputBuffer.clear();
    
    initialized = false;
}

void AppConsole::registerBuiltinCommands()
{
    registerCommand("help", "Show available commands", 
        [this](const std::string& args) { cmdHelp(args); });
    
    registerCommand("echo", "Echo text to console", 
        [this](const std::string& args) { cmdEcho(args); });
    
    registerCommand("clear", "Clear console log", 
        [this](const std::string& args) { cmdClear(args); });
    
    registerCommand("level", "Set log level (trace/debug/info/warning/error)", 
        [this](const std::string& args) { cmdLevel(args); });
    
    registerCommand("next", "Skip to next level (only in gameplay)", 
        [this](const std::string& args) { cmdNext(args); });

    registerCommand ( "goto", "Skip to level number (only in gameplay)",
        [this]( const std::string& args ) { cmdGoto ( args ); } );
}

void AppConsole::cmdHelp(const std::string& args)
{
    LOG_INFO("=== Available Commands ===");
    for (const ConsoleCommand& cmd : commands)
    {
        LOG_INFO("  /%s - %s", cmd.name.c_str(), cmd.description.c_str());
    }
}

void AppConsole::cmdEcho(const std::string& args)
{
    if (args.empty())
    {
        LOG_WARNING("Usage: /echo <text>");
        return;
    }
    LOG_INFO("%s", args.c_str());
}

void AppConsole::cmdClear(const std::string& args)
{
    Logger::instance().clearEntries();
    LOG_INFO("Console cleared");
}

void AppConsole::cmdLevel(const std::string& args)
{
    if (args.empty())
    {
        LOG_WARNING("Usage: /level <trace|debug|info|warning|error>");
        return;
    }
    
    std::string levelStr = args;
    std::transform(levelStr.begin(), levelStr.end(), levelStr.begin(), ::tolower);
    
    if (levelStr == "trace")
        Logger::instance().setMinLevel(LogLevel::TRACE);
    else if (levelStr == "debug")
        Logger::instance().setMinLevel(LogLevel::DEBUG);
    else if (levelStr == "info")
        Logger::instance().setMinLevel(LogLevel::INFO);
    else if (levelStr == "warning" || levelStr == "warn")
        Logger::instance().setMinLevel(LogLevel::WARNING);
    else if (levelStr == "error" || levelStr == "err")
        Logger::instance().setMinLevel(LogLevel::ERR);
    else
    {
        LOG_WARNING("Unknown log level: %s", args.c_str());
        return;
    }
    
    LOG_SUCCESS("Log level set to: %s", levelStr.c_str());
}

void AppConsole::cmdNext(const std::string& args)
{
    AppData& appData = AppData::instance();
    
    // Check if we're in a Scene (gameplay screen)
    Scene* currentScene = dynamic_cast<Scene*>(appData.currentScreen.get());
    
    if (!currentScene)
    {
        LOG_WARNING("/next can only be used during gameplay (Scene)");
        return;
    }
    
    // Check if there's a next stage
    if (appData.currentStage >= appData.numStages)
    {
        LOG_INFO("Already on the last stage");
        return;
    }
    
    // Trigger level clear to advance to next stage
    LOG_SUCCESS("Skipping to next level...");
    currentScene->win();
}

void AppConsole::cmdGoto ( const std::string& args )
{
    AppData& appData = AppData::instance ();

    // Check if we're in a Scene (gameplay screen)
    Scene* currentScene = dynamic_cast<Scene*>( appData.currentScreen.get () );

    if ( !currentScene )
    {
        LOG_WARNING ( "/goto can only be used during gameplay (Scene)" );
        return;
    }

    // Parse level number from args
    if ( args.empty() )
    {
        LOG_WARNING ( "Usage: /goto <level_number> (1-%d)", appData.numStages );
        return;
    }

    int targetLevel = 0;
    try
    {
        targetLevel = std::stoi( args );
    }
    catch ( const std::exception& )
    {
        LOG_WARNING ( "Invalid level number: %s (must be 1-%d)", args.c_str(), appData.numStages );
        return;
    }

    // Validate level range [1..numStages]
    if ( targetLevel < 1 || targetLevel > appData.numStages )
    {
        LOG_WARNING ( "Level %d is out of range (valid: 1-%d)", targetLevel, appData.numStages );
        return;
    }

    // Check if already on target level
    if ( targetLevel == appData.currentStage )
    {
        LOG_INFO ( "Already on level %d", targetLevel );
        return;
    }

    // Use Scene's skipToStage method for proper state management
    LOG_SUCCESS ( "Skipping to level %d...", targetLevel );
    currentScene->skipToStage( targetLevel );
}


void AppConsole::registerCommand(const std::string& name, const std::string& desc, CommandHandler handler)
{
    // Check if command already exists
    for (ConsoleCommand& cmd : commands)
    {
        if (cmd.name == name)
        {
            cmd.handler = handler;
            cmd.description = desc;
            return;
        }
    }
    
    ConsoleCommand cmd;
    cmd.name = name;
    cmd.description = desc;
    cmd.handler = handler;
    commands.push_back(cmd);
}

void AppConsole::unregisterCommand(const std::string& name)
{
    commands.erase(
        std::remove_if(commands.begin(), commands.end(),
            [&name](const ConsoleCommand& cmd) { return cmd.name == name; }),
        commands.end()
    );
}

bool AppConsole::handleEvent(const SDL_Event& event)
{
    // Toggle console with F9 (like Roblox) or backtick (`)
    if (event.type == SDL_KEYDOWN)
    {
        if (event.key.keysym.sym == SDLK_F9 || 
            event.key.keysym.sym == SDLK_BACKQUOTE)
        {
            toggle();
            LOG_DEBUG("AppConsole toggled: %s", visible ? "visible" : "hidden");
            return true;
        }
        
        if (visible)
        {
            handleKeyDown(event.key.keysym.sym, (SDL_Keymod)event.key.keysym.mod);
            return true;
        }
    }
    
    // Handle mouse wheel for scrolling (when console is visible)
    if (event.type == SDL_MOUSEWHEEL && visible)
    {
        if (event.wheel.y > 0)
        {
            // Scroll up
            scrollUp(3);
        }
        else if (event.wheel.y < 0)
        {
            // Scroll down
            scrollDown(3);
        }
        return true;
    }
    
    if (event.type == SDL_TEXTINPUT && visible)
    {
        handleTextInput(event.text.text);
        return true;
    }
    
    return false;
}

void AppConsole::handleTextInput(const char* text)
{
    inputBuffer.insert(cursorPosition, text);
    cursorPosition += strlen(text);
}

void AppConsole::handleKeyDown(SDL_Keycode key, SDL_Keymod mod)
{
    // Check for Ctrl+Up/Down for scrolling (like Minecraft/Roblox)
    bool ctrlPressed = (mod & KMOD_CTRL) != 0;
    
    switch (key)
    {
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            if (!inputBuffer.empty())
            {
                processInput(inputBuffer);
                commandHistory.push_back(inputBuffer);
                historyIndex = -1;
                inputBuffer.clear();
                cursorPosition = 0;
                scrollToBottom(); // Auto-scroll to bottom on command execution
            }
            break;
            
        case SDLK_BACKSPACE:
            if (cursorPosition > 0)
            {
                inputBuffer.erase(cursorPosition - 1, 1);
                cursorPosition--;
            }
            break;
            
        case SDLK_DELETE:
            if (cursorPosition < inputBuffer.length())
            {
                inputBuffer.erase(cursorPosition, 1);
            }
            break;
            
        case SDLK_LEFT:
            if (cursorPosition > 0)
                cursorPosition--;
            break;
            
        case SDLK_RIGHT:
            if (cursorPosition < inputBuffer.length())
                cursorPosition++;
            break;
            
        case SDLK_HOME:
            if (ctrlPressed)
            {
                // Ctrl+Home: Scroll to top
                const std::vector<LogEntry>& entries = Logger::instance().getEntries();
                scrollOffset = std::max(0, (int)entries.size() - maxVisibleLines);
            }
            else
            {
                cursorPosition = 0;
            }
            break;
            
        case SDLK_END:
            if (ctrlPressed)
            {
                // Ctrl+End: Scroll to bottom
                scrollToBottom();
            }
            else
            {
                cursorPosition = inputBuffer.length();
            }
            break;
            
        case SDLK_UP:
            if (ctrlPressed)
            {
                // Ctrl+Up: Scroll up (like Minecraft)
                scrollUp(1);
            }
            else if (!commandHistory.empty())
            {
                // Normal Up: Command history
                if (historyIndex < 0)
                    historyIndex = (int)commandHistory.size() - 1;
                else if (historyIndex > 0)
                    historyIndex--;
                
                inputBuffer = commandHistory[historyIndex];
                cursorPosition = inputBuffer.length();
            }
            break;
            
        case SDLK_DOWN:
            if (ctrlPressed)
            {
                // Ctrl+Down: Scroll down (like Minecraft)
                scrollDown(1);
            }
            else if (!commandHistory.empty() && historyIndex >= 0)
            {
                // Normal Down: Command history
                historyIndex++;
                if (historyIndex >= (int)commandHistory.size())
                {
                    historyIndex = -1;
                    inputBuffer.clear();
                }
                else
                {
                    inputBuffer = commandHistory[historyIndex];
                }
                cursorPosition = inputBuffer.length();
            }
            break;
            
        case SDLK_PAGEUP:
            scrollUp(5);
            break;
            
        case SDLK_PAGEDOWN:
            scrollDown(5);
            break;
            
        case SDLK_ESCAPE:
            hide();
            break;
    }
}

void AppConsole::processInput(const std::string& input)
{
    if (input.empty())
        return;
    
    // Check if it's a command (starts with /)
    if (input[0] == '/')
    {
        executeCommand(input.substr(1));
    }
    else
    {
        // Treat as echo
        LOG_INFO("> %s", input.c_str());
    }
}

void AppConsole::executeCommand(const std::string& input)
{
    // Parse command and arguments
    std::istringstream iss(input);
    std::string cmdName;
    iss >> cmdName;
    
    // Get rest as arguments
    std::string args;
    std::getline(iss >> std::ws, args);
    
    // Convert command name to lowercase
    std::transform(cmdName.begin(), cmdName.end(), cmdName.begin(), ::tolower);
    
    // Find and execute command
    for (const ConsoleCommand& cmd : commands)
    {
        if (cmd.name == cmdName)
        {
            cmd.handler(args);
            return;
        }
    }
    
    LOG_WARNING("Unknown command: /%s (type /help for available commands)", cmdName.c_str());
}

void AppConsole::update()
{
    // Blink cursor
    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - cursorBlinkTime > 500)
    {
        cursorVisible = !cursorVisible;
        cursorBlinkTime = currentTime;
    }
}

void AppConsole::render()
{
    if (!visible || !initialized || !graph)
        return;
    
    update();
    renderBackground();
    renderLogs();
    renderPrompt();
}

void AppConsole::renderBackground()
{
    SDL_Renderer* renderer = graph->getRenderer();
    
    // Use logical render dimensions, not window size
    const int screenW = RES_X;
    const int screenH = RES_Y;
    
    // Semi-transparent background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, (Uint8)(overlayAlpha * 255));
    
    SDL_Rect bgRect = { 0, 0, screenW, screenH };
    SDL_RenderFillRect(renderer, &bgRect);
    
    // Border line above prompt
    int promptY = screenH - promptHeight - padding;
    SDL_SetRenderDrawColor(renderer, 80, 80, 100, 200);
    SDL_RenderDrawLine(renderer, 0, promptY - 2, screenW, promptY - 2);
}

void AppConsole::renderLogs()
{
    if (!fontRenderer)
        return;
    
    const std::vector<LogEntry>& entries = Logger::instance().getEntries();
    if (entries.empty())
        return;
    
    SDL_Renderer* renderer = graph->getRenderer();
    
    // Use logical render dimensions, not window size
    const int screenW = RES_X;
    const int screenH = RES_Y;
    
    // Calculate visible area
    int logAreaHeight = screenH - promptHeight - padding * 3;
    int linesVisible = logAreaHeight / lineHeight;
    
    // Determine which entries to show
    int totalEntries = (int)entries.size();
    int startEntry = std::max(0, totalEntries - linesVisible - scrollOffset);
    int endEntry = std::min(totalEntries, startEntry + linesVisible);
    
    // Render entries
    int y = padding;
    for (int i = startEntry; i < endEntry; i++)
    {
        const LogEntry& entry = entries[i];
        
        // Set color
        fontRenderer->setColor(entry.color.r, entry.color.g, entry.color.b, 255);
        
        // Format: [HH:MM:SS] message
        char line[512];
        snprintf(line, sizeof(line), "[%s] %s", entry.timestamp.c_str(), entry.message.c_str());
        
        fontRenderer->text(line, padding, y);
        y += lineHeight;
    }
    
    // Reset color
    fontRenderer->setColor(255, 255, 255, 255);
    
    // Show scroll indicator if not at bottom
    if (scrollOffset > 0)
    {
        fontRenderer->setColor(150, 150, 150, 255);
        char scrollText[32];
        snprintf(scrollText, sizeof(scrollText), "-- %d more --", scrollOffset);
        
        int textW = fontRenderer->getTextWidth(scrollText);
        fontRenderer->text(scrollText, (screenW - textW) / 2, logAreaHeight - lineHeight);
        fontRenderer->setColor(255, 255, 255, 255);
    }
}

void AppConsole::renderPrompt()
{
    if (!fontRenderer)
        return;
    
    SDL_Renderer* renderer = graph->getRenderer();
    
    // Use logical render dimensions, not window size
    const int screenW = RES_X;
    const int screenH = RES_Y;
    
    int promptY = screenH - promptHeight - padding;
    
    // Prompt background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 30, 30, 40, 220);
    SDL_Rect promptBg = { padding, promptY, screenW - padding * 2, promptHeight };
    SDL_RenderFillRect(renderer, &promptBg);
    
    // Prompt text
    fontRenderer->setColor(100, 200, 100, 255);
    fontRenderer->text("> ", padding + 4, promptY + 4);
    
    fontRenderer->setColor(255, 255, 255, 255);
    fontRenderer->text(inputBuffer.c_str(), padding + 20, promptY + 4);
    
    // Cursor
    if (cursorVisible)
    {
        std::string beforeCursor = inputBuffer.substr(0, cursorPosition);
        int cursorX = padding + 20 + fontRenderer->getTextWidth(beforeCursor.c_str());
        
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawLine(renderer, cursorX, promptY + 4, cursorX, promptY + promptHeight - 4);
    }
}

void AppConsole::scrollUp(int lines)
{
    const std::vector<LogEntry>& entries = Logger::instance().getEntries();
    int maxScroll = std::max(0, (int)entries.size() - maxVisibleLines);
    scrollOffset = std::min(scrollOffset + lines, maxScroll);
}

void AppConsole::scrollDown(int lines)
{
    scrollOffset = std::max(0, scrollOffset - lines);
}

void AppConsole::scrollToBottom()
{
    scrollOffset = 0;
}

void AppConsole::print(const std::string& message, LogColor color)
{
    // This bypasses Logger and adds directly to the display
    // For now, just use Logger
    LOG_INFO("%s", message.c_str());
}
