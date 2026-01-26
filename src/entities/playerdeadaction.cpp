#include "playerdeadaction.h"
#include "../core/sprite.h"
#include "logger.h"
#include "player.h"
#include "../main.h"    // For RES_Y, MAX_X

PlayerDeadAction::PlayerDeadAction(Player* player, float velX, float velY)
    : player(player),
      velocityX(velX),
      velocityY(velY),
      // Infinite rotation: 0 to 360 degrees in 0.5 seconds, loops forever
      rotationMotion(0.0f, 360.0f, 0.5f, Easing::Linear, 0, false)
{
}

void PlayerDeadAction::start()
{
    if (player && player->getSprite())
    {
        // Set to dead frame (single sprite, not animated sequence)
        player->setFrame(ANIM_DEAD);

        // Initialize rotation to 0
        player->setRotation(0.0f);

        // Start rotation motion
        rotationMotion.reset();
    }
}

bool PlayerDeadAction::update(float dt)
{
    if (isDone || !player) return false;

    LOG_DEBUG("PlayerDeadAction update: rotation = %.2f", rotationMotion.value());

    Sprite* sprite = player->getSprite();
    if (!sprite) return false;


    // Update rotation animation (continuous 720 deg/sec = 2 rotations/sec)
    rotationMotion.update(dt);
    player->setRotation(rotationMotion.value());

    // Update position with physics
    float x = player->getX();
    float y = player->getY();

    x += velocityX;
    y += velocityY;
    velocityY += GRAVITY;

    // Bounce off right edge
    if ( player->getFacing() == FacingDirection::RIGHT )
    {
        if ( x + player->getWidth() >= MAX_X )
        {
            velocityX = -2.0f;
        }
    }
    else
    {
        if ( x <= MIN_X )
        {
            velocityX = 2.0f;
        }
    }

    player->setX(x);
    player->setY(y);

    // Check if fallen below screen
    if (y > RES_Y)
    {
        isDone = true;
        player->setRotation(0);
        return false;
    }

    return true;
}
