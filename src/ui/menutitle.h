#pragma once

#include "../core/gameobject.h"
#include "../core/sprite.h"
#include "../core/action.h"
#include <memory>

class Graph;

/**
 * MenuTitle class
 *
 * Encapsulates the animated title screen elements with built-in animation.
 * Inherits from IGameObject for position and update management.
 * All inner sprites are drawn relative to (xPos, yPos).
 *
 * Animation sequence:
 * 1. "BOING" text drops from top
 * 2. "HYPER" text slides from left
 * 3. Background fades in
 * 4. Colored balls explode outward from center
 */
class MenuTitle : public IGameObject
{
private:
    // Sprites
    Sprite title_boing;
    Sprite title_hyper;
    Sprite title_bg;
    Sprite title_bg_redball;
    Sprite title_bg_greenball;
    Sprite title_bg_blueball;

    // Animation sequence
    std::unique_ptr<ActionSequence> animation;

    Graph* graph; // Reference to graph system

    // Build the 4-phase animation sequence
    void buildAnimation();

public:
    MenuTitle(Graph* graph);
    virtual ~MenuTitle();

    void init();
    void update(float dt);
    void draw();

    // Check if animation has completed
    bool isAnimationFinished() const;
};
