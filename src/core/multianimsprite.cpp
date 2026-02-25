#include "multianimsprite.h"
#include "animspritesheet.h"
#include "sprite.h"

void MultiAnimSprite::registerAnimation(const std::string& name, AnimSpriteSheet* anim)
{
    if (anim)
    {
        animations[name] = anim;
    }
}

void MultiAnimSprite::unregisterAnimation(const std::string& name)
{
    animations.erase(name);
    if (activeAnimKey == name)
    {
        activeAnimKey.clear();
        useFallback = true;
    }
}

void MultiAnimSprite::clearAnimations()
{
    animations.clear();
    activeAnimKey.clear();
    useFallback = true;
}

void MultiAnimSprite::addFallbackSprite(Sprite* spr)
{
    if (spr)
    {
        fallbackSprites.push_back(spr);
    }
}

void MultiAnimSprite::setFallbackSprites(const std::vector<Sprite*>& sprites)
{
    fallbackSprites = sprites;
    // Clamp fallbackFrame to valid range
    if (!fallbackSprites.empty() && fallbackFrame >= static_cast<int>(fallbackSprites.size()))
    {
        fallbackFrame = static_cast<int>(fallbackSprites.size()) - 1;
    }
}

void MultiAnimSprite::clearFallbackSprites()
{
    fallbackSprites.clear();
    fallbackFrame = 0;
}

void MultiAnimSprite::setFallbackFrame(int frame)
{
    if (fallbackSprites.empty())
    {
        fallbackFrame = 0;
        return;
    }

    // Clamp to valid range
    if (frame < 0)
        fallbackFrame = 0;
    else if (frame >= static_cast<int>(fallbackSprites.size()))
        fallbackFrame = static_cast<int>(fallbackSprites.size()) - 1;
    else
        fallbackFrame = frame;
}

bool MultiAnimSprite::setActiveAnimation(const std::string& name)
{
    auto it = animations.find(name);
    if (it == animations.end())
    {
        return false;
    }

    activeAnimKey = name;
    useFallback = false;
    return true;
}

void MultiAnimSprite::useAsFallback()
{
    useFallback = true;
    activeAnimKey.clear();
}

void MultiAnimSprite::update(float dtMs)
{
    if (useFallback)
        return;

    AnimSpriteSheet* anim = getActiveAnimation();
    if (anim)
    {
        anim->update(dtMs);
    }
}

Sprite* MultiAnimSprite::getActiveSprite() const
{
    if (useFallback)
    {
        // Return fallback sprite at current frame
        if (fallbackSprites.empty())
            return nullptr;

        if (fallbackFrame >= 0 && fallbackFrame < static_cast<int>(fallbackSprites.size()))
            return fallbackSprites[fallbackFrame];

        return fallbackSprites[0];
    }

    // Return current frame from active animation
    AnimSpriteSheet* anim = getActiveAnimation();
    if (anim)
    {
        return anim->getCurrentSprite();
    }

    return nullptr;
}

AnimSpriteSheet* MultiAnimSprite::getActiveAnimation() const
{
    if (useFallback || activeAnimKey.empty())
        return nullptr;

    auto it = animations.find(activeAnimKey);
    if (it != animations.end())
    {
        return it->second;
    }

    // Animation key is set but animation not found - inconsistent state
    // This can happen if animation was unregistered after being set active
    // Gracefully handle by returning nullptr
    return nullptr;
}

AnimSpriteSheet* MultiAnimSprite::getAnimation(const std::string& name) const
{
    auto it = animations.find(name);
    if (it != animations.end())
    {
        return it->second;
    }
    return nullptr;
}

int MultiAnimSprite::getWidth() const
{
    if (useFallback)
    {
        // For fallback sprites, use sprite dimensions
        Sprite* spr = getActiveSprite();
        return spr ? spr->getWidth() : 0;
    }

    // For animations, use animation bounding box dimensions
    AnimSpriteSheet* anim = getActiveAnimation();
    if (anim)
    {
        return anim->getWidth();
    }

    return 0;
}

int MultiAnimSprite::getHeight() const
{
    if (useFallback)
    {
        // For fallback sprites, use sprite dimensions
        Sprite* spr = getActiveSprite();
        return spr ? spr->getHeight() : 0;
    }

    // For animations, use animation bounding box dimensions
    AnimSpriteSheet* anim = getActiveAnimation();
    if (anim)
    {
        return anim->getHeight();
    }

    return 0;
}
