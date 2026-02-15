#pragma once

#include <memory>
#include "gameobject.h"

class AnimSpriteSheet;
class Graph;

/**
 * AnimEffect - One-shot animation effect that auto-destructs when complete
 *
 * Clones an AnimSpriteSheet template, plays it once from a world position,
 * and marks itself dead when the animation finishes.
 *
 * The x, y position is the center point of the effect. The animation sprite
 * is offset by half its frame dimensions so it renders centered.
 */
class AnimEffect : public IGameObject
{
public:
    /**
	 * @param x Center X in world/screen coordinates (logical bottom-middle coordinate)
     * @param y Center Y in world/screen coordinates (logical bottom-middle coordinate)
     * @param tmpl Template AnimSpriteSheet to clone (must not be null)
     */
    AnimEffect(int x, int y, AnimSpriteSheet* tmpl);

    void update(float dt);
    void draw(Graph* graph);

    CollisionBox getCollisionBox() const override { return {0, 0, 0, 0}; }

private:
    std::unique_ptr<AnimSpriteSheet> anim;
};
