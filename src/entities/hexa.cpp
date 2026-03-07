#include "hexa.h"
#include "../main.h"
#include "animspritesheet.h"
#include "eventmanager.h"
#include "../game/collisionsystem.h"
#include <cmath>

// Static member initialization
constexpr int Hexa::SIZES[][2];

Hexa::Hexa(Scene* scn, int x, int y, int sz, float vx, float vy)
    : scene(scn), size(sz), velX(vx), velY(vy)
{
    this->xPos = (float)x;
    this->yPos = (float)y;

    // Clamp size to valid range
    if (size < 0) size = 0;
    if (size > 2) size = 2;

    width = SIZES[size][0];
    height = SIZES[size][1];
}

Hexa::Hexa(Scene* scn, Hexa* parent, int dir)
    : scene(scn)
{
    // Child is one size smaller
    size = parent->size + 1;
    if (size > 2) size = 2;

    width = SIZES[size][0];
    height = SIZES[size][1];

    // Inherit velocity magnitude but set direction based on dir parameter
    float speed = std::sqrt(parent->velX * parent->velX + parent->velY * parent->velY);

    // Child hexas spread outward
    velX = dir * std::abs(parent->velX);
    if (velX == 0.0f) velX = dir * 1.5f;  // Default horizontal velocity if parent was vertical

    // Inherit vertical velocity direction
    velY = parent->velY;

    // Position at parent's center, offset by direction
    float parentCenterX = parent->xPos + (parent->width / 2.0f);
    float parentCenterY = parent->yPos + (parent->height / 2.0f);

    this->xPos = parentCenterX - (width / 2.0f) + (dir * width * 0.5f);
    this->yPos = parentCenterY - (height / 2.0f);
}

void Hexa::update(float dt)
{
    // Handle flash state (countdown before actual death)
    if (flashing)
    {
        flashTimer -= dt;
        if (flashTimer <= 0.0f)
        {
            // Flash complete - now actually die
            IGameObject::kill();  // Call parent to set dead=true
        }
        return;  // Don't move during flash
    }

    // Rotate continuously
    rotation += rotationSpeed * dt;
    if (rotation >= 360.0f) rotation -= 360.0f;

    // Constant velocity movement
    xPos += velX;
    yPos += velY;

    // Screen edge bouncing
    if (velX > 0 && xPos + width >= Stage::MAX_X)
    {
        xPos = (float)(Stage::MAX_X - width);
        velX = -velX;
    }
    else if (velX < 0 && xPos <= Stage::MIN_X)
    {
        xPos = (float)Stage::MIN_X;
        velX = -velX;
    }

    if (velY > 0 && yPos + height >= Stage::MAX_Y)
    {
        yPos = (float)(Stage::MAX_Y - height);
        velY = -velY;
    }
    else if (velY < 0 && yPos <= Stage::MIN_Y)
    {
        yPos = (float)Stage::MIN_Y;
        velY = -velY;
    }
}

void Hexa::kill()
{
    if (!flashing && !dead)
    {
        flashing = true;
        flashTimer = FLASH_DURATION;
        onDeath();  // Spawn splash effect, fire events
    }
}

void Hexa::onDeath()
{
    // Spawn splash animation centered on this hexa with size-based scaling
    // Size 0 = 100%, Size 1 = 50%, Size 2 = 33%
    static const float sizeScales[] = { 1.0f, 0.5f, 0.33f };
    float scale = (size >= 0 && size <= 2) ? sizeScales[size] : 0.33f;

    StageResources& res = gameinf.getStageRes();
    AnimSpriteSheet* tmpl = res.hexaSplashAnim.get();
    if (tmpl)
    {
        int centerX = (int)xPos + width / 2;
        int centerY = (int)yPos + height / 2;
        scene->spawnEffect(tmpl, centerX, centerY + (int)(tmpl->getHeight() * scale / 2), scale);
    }

    // Fire HEXA_SPLIT event if hexa will split into smaller hexas
    if (size < 2)
    {
        GameEventData event(GameEventType::HEXA_SPLIT);
        event.hexaSplit.parentHexa = this;
        event.hexaSplit.parentSize = size;
        EVENT_MGR.trigger(event);
    }

    // Spawn death pickup if configured
    if (hasDeathPickup)
    {
        int cx = (int)xPos + width / 2;
        int cy = (int)yPos + height / 2;
        scene->addPickup(cx, cy, deathPickupType);
    }
}

void Hexa::handlePlatformBounce(const CollisionSide& side)
{
    // Reflect velocity based on collision side
    if (side.hasHorizontal())
    {
        velX = -velX;
    }
    if (side.hasVertical())
    {
        velY = -velY;
    }
}

std::pair<std::unique_ptr<Hexa>, std::unique_ptr<Hexa>> Hexa::createChildren()
{
    // Only create children if hexa is large enough to split
    if (size >= 2)
    {
        return { nullptr, nullptr };
    }

    auto child1 = std::make_unique<Hexa>(scene, this, -1);  // Left
    auto child2 = std::make_unique<Hexa>(scene, this, 1);   // Right

    return { std::move(child1), std::move(child2) };
}

bool Hexa::collision(Shot* sh)
{
    if (sh->isDead())
        return false;

    return intersects(getCollisionBox(), sh->getCollisionBox());
}

bool Hexa::collision(Platform* fl)
{
    return intersects(getCollisionBox(), fl->getCollisionBox());
}

bool Hexa::collision(Player* pl)
{
    return intersects(getCollisionBox(), pl->getCollisionBox());
}

Sprite* Hexa::getCurrentSprite() const
{
    StageResources& res = gameinf.getStageRes();
    if (res.hexaAnim)
    {
        return res.hexaAnim->getFrame(size);
    }
    return nullptr;
}
