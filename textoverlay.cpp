#include "textoverlay.h"
#include "graph.h"
#include "bmfont.h"
#include <algorithm>

TextOverlay::TextOverlay()
    : graph(nullptr), enabled(true)
{
}

void TextOverlay::init(Graph* graphContext)
{
    graph = graphContext;
    
    // Create default section at top-left corner
    // Line height adjusted to match font5x7 character height (7 pixels + 1 pixel spacing = 8)
    addSection("default", 10, 10, 0, 0, 10, 8);
}

void TextOverlay::addSection(const std::string& name, int x, int y, 
                            int width, int height, 
                            int padding, int lineHeight, 
                            Uint8 bgAlpha)
{
    TextSection section(name, x, y, width, height, padding, lineHeight, bgAlpha);
    sections[name] = section;
}

TextSection& TextOverlay::getSection(const std::string& name)
{
    // If section doesn't exist, create it with default settings
    if (sections.find(name) == sections.end())
    {
        if (name == "default")
        {
            addSection("default", 10, 10);
        }
        else
        {
            // Create section with default position
            addSection(name, 10, 10);
        }
    }
    
    return sections[name];
}

void TextOverlay::addText(const std::string& text, const std::string& sectionName)
{
    // If section doesn't exist, create default section
    if (sections.find(sectionName) == sections.end())
    {
        if (sectionName == "default")
        {
            addSection("default", 10, 10);
        }
        else
        {
            // Create section with auto-positioned coordinates
            // Stack sections vertically if not found
            addSection(sectionName, 10, 10);
        }
    }
    
    sections[sectionName].addLine(text);
}

void TextOverlay::clear(const std::string& sectionName)
{
    if (sectionName.empty())
    {
        // Clear all sections
        for (auto& pair : sections)
        {
            pair.second.clear();
        }
    }
    else
    {
        auto it = sections.find(sectionName);
        if (it != sections.end())
        {
            it->second.clear();
        }
    }
}

void TextOverlay::clearAll()
{
    sections.clear();
}

void TextOverlay::removeSection(const std::string& name)
{
    sections.erase(name);
}

bool TextOverlay::hasSection(const std::string& name) const
{
    return sections.find(name) != sections.end();
}

void TextOverlay::render()
{
    if (!enabled || !graph)
        return;
    
    for (const auto& pair : sections)
    {
        const TextSection& section = pair.second;
        if (!section.lines.empty())
        {
            renderSection(section);
        }
    }
}

void TextOverlay::renderSection(const TextSection& section)
{
    if (!graph || section.lines.empty())
        return;
    
    // Calculate section dimensions if auto-sizing
    int sectionWidth = section.width;
    int sectionHeight = section.height;
    
    if (sectionWidth == 0)
    {
        // Auto-calculate width based on longest text
        int maxWidth = 0;
        for (const std::string& line : section.lines)
        {
            int textWidth = calculateTextWidth(line, section.customFont);
            maxWidth = std::max(maxWidth, textWidth);
        }
        sectionWidth = maxWidth + (section.padding * 2);
    }
    
    if (sectionHeight == 0)
    {
        // Auto-calculate height based on number of lines
        sectionHeight = (int)section.lines.size() * section.lineHeight + (section.padding * 2);
    }
    
    // Draw semi-transparent background
    SDL_SetRenderDrawBlendMode(graph->getRenderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(graph->getRenderer(), 0, 0, 0, section.bgAlpha);
    SDL_Rect bgRect = { section.x, section.y, sectionWidth, sectionHeight };
    SDL_RenderFillRect(graph->getRenderer(), &bgRect);
    SDL_SetRenderDrawBlendMode(graph->getRenderer(), SDL_BLENDMODE_NONE);
    
    // Draw text lines
    int textX = section.x + section.padding;
    int textY = section.y + section.padding;
    
    for (const std::string& line : section.lines)
    {
        if (section.customFont)
        {
            // Use custom font for this section
            section.customFont->text(line.c_str(), textX, textY);
        }
        else
        {
            // Use system font via graph->text()
            graph->text(line.c_str(), textX, textY);
        }
        textY += section.lineHeight;
    }
}

int TextOverlay::calculateTextWidth(const std::string& text, BMFontRenderer* font)
{
    if (font)
    {
        // Use custom font width calculation
        return font->getTextWidth(text.c_str());
    }
    
    // Font 5x7 with 1 pixel spacing = 6 pixels per character at scale 1x
    // This matches the actual rendering in graph->text()
    return (int)text.length() * 6;
}
