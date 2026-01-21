#pragma once

#include <SDL.h>
#include <string>
#include <vector>
#include <unordered_map>

class Graph;
class BMFontRenderer;

/**
 * TextSection struct
 * Represents a section of text overlay with position, size, and text lines.
 */
struct TextSection
{
    std::string name;
    int x;
    int y;
    int width;   // 0 means auto-calculate from content
    int height;  // 0 means auto-calculate from content
    int padding;
    int lineHeight;
    Uint8 bgAlpha; // Background transparency (0-255)
    
    // Non-owning pointer to custom font (lifetime managed by owner, typically GameState)
    // nullptr means use system font
    BMFontRenderer* customFont;
    
    std::vector<std::string> lines;
    
    // Default constructor for std::unordered_map
    TextSection()
        : name(""), x(0), y(0), width(0), height(0), 
          padding(10), lineHeight(8), bgAlpha(180), customFont(nullptr)
    {
    }
    
    TextSection(const std::string& sectionName, int posX, int posY, 
                int w = 0, int h = 0, int pad = 10, int lineH = 8, Uint8 alpha = 180)
        : name(sectionName), x(posX), y(posY), width(w), height(h), 
          padding(pad), lineHeight(lineH), bgAlpha(alpha), customFont(nullptr)
    {
    }
    
    void clear()
    {
        lines.clear();
    }
    
    void addLine(const std::string& text)
    {
        lines.push_back(text);
    }
    
    // Modifier methods that return reference for chaining
    TextSection& setPosition(int posX, int posY)
    {
        x = posX;
        y = posY;
        return *this;
    }
    
    TextSection& setSize(int w, int h)
    {
        width = w;
        height = h;
        return *this;
    }
    
    TextSection& setPadding(int pad)
    {
        padding = pad;
        return *this;
    }
    
    TextSection& setLineHeight(int lineH)
    {
        lineHeight = lineH;
        return *this;
    }
    
    TextSection& setAlpha(Uint8 alpha)
    {
        bgAlpha = alpha;
        return *this;
    }
    
    /**
     * Set custom font for this section.
     * @param font Non-owning pointer to BMFontRenderer. 
     *             Lifetime must be managed by caller (typically GameState).
     *             Pass nullptr to use system font.
     */
    TextSection& setFont(BMFontRenderer* font)
    {
        customFont = font;
        return *this;
    }
};

/**
 * TextOverlay class
 * 
 * Manages multiple text sections that can be rendered as overlays on the screen.
 * Each section has its own position, background, and text lines.
 * Useful for debug information, HUD elements, or system messages.
 * 
 * MEMORY MANAGEMENT:
 * - TextOverlay does NOT own BMFontRenderer instances
 * - Font lifetime must be managed by the owner (typically GameState)
 * - Use raw pointers (non-owning) for font references
 * - Fonts must remain valid for the lifetime of the overlay or until changed
 * 
 * Supports custom fonts per section using BMFontRenderer.
 * If no custom font is set, uses the system font (5x7 bitmap).
 * 
 * Example usage with custom font:
 * ```cpp
 * // In GameState or similar owner class:
 * std::unique_ptr<BMFontLoader> fontLoader = std::make_unique<BMFontLoader>();
 * std::unique_ptr<BMFontRenderer> fontRenderer = std::make_unique<BMFontRenderer>();
 * std::unique_ptr<Sprite> fontTexture = std::make_unique<Sprite>();
 * 
 * fontLoader->load("assets/graph/font/myfont.fnt");
 * fontTexture->init(&appGraph, "assets/graph/font/myfont.png");
 * fontRenderer->init(&appGraph, fontLoader.get(), fontTexture.get());
 * 
 * // Pass non-owning pointer to overlay
 * textOverlay.getSection("custom-section")
 *     .setPosition(100, 100)
 *     .setFont(fontRenderer.get());  // Non-owning pointer
 * ```
 */
class TextOverlay
{
private:
    Graph* graph;  // Non-owning pointer, lifetime managed externally
    std::unordered_map<std::string, TextSection> sections;
    bool enabled;
    
    void renderSection(const TextSection& section);
    int calculateTextWidth(const std::string& text, BMFontRenderer* font);
    
public:
    TextOverlay();
    ~TextOverlay() {}
    
    /**
     * Initialize the overlay with a graph context
     * @param graphContext Non-owning pointer to Graph (must outlive TextOverlay)
     */
    void init(Graph* graphContext);
    
    /**
     * Add or update a section with position and optional size
     * @param name Section identifier
     * @param x Top-left X position
     * @param y Top-left Y position
     * @param width Section width (0 = auto-calculate)
     * @param height Section height (0 = auto-calculate)
     * @param padding Inner padding for text
     * @param lineHeight Height of each text line
     * @param bgAlpha Background transparency (0-255)
     */
    void addSection(const std::string& name, int x, int y, 
                   int width = 0, int height = 0, 
                   int padding = 10, int lineHeight = 8, 
                   Uint8 bgAlpha = 180);
    
    /**
     * Get a reference to a section for modification
     * Creates the section if it doesn't exist
     * @param name Section identifier
     * @return Reference to the section
     */
    TextSection& getSection(const std::string& name);
    
    /**
     * Add text to a section
     * @param text Text to add
     * @param sectionName Section to add to (defaults to "default")
     */
    void addText(const std::string& text, const std::string& sectionName = "default");
    
    /**
     * Clear all text from a specific section
     * @param sectionName Section to clear (empty = clear all sections)
     */
    void clear(const std::string& sectionName = "");
    
    /**
     * Clear all sections and text
     */
    void clearAll();
    
    /**
     * Remove a section completely
     */
    void removeSection(const std::string& name);
    
    /**
     * Render all sections
     */
    void render();
    
    /**
     * Enable or disable the overlay rendering
     */
    void setEnabled(bool enable) { enabled = enable; }
    bool isEnabled() const { return enabled; }
    
    /**
     * Check if a section exists
     */
    bool hasSection(const std::string& name) const;
};
