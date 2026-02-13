#include "animsprite.h"
#include "spritesheet.h"
#include "animcontroller.h"

// Static empty string for getStateName when not a state machine
static const std::string emptyString;

AnimSprite::~AnimSprite() = default;

AnimSprite::AnimSprite(AnimSprite&& other) noexcept
    : spriteSheet(other.spriteSheet)
    , animController(std::move(other.animController))
    , stateMachinePtr(other.stateMachinePtr)
{
    other.spriteSheet = nullptr;
    other.stateMachinePtr = nullptr;
}

AnimSprite& AnimSprite::operator=(AnimSprite&& other) noexcept
{
    if (this != &other)
    {
        spriteSheet = other.spriteSheet;
        animController = std::move(other.animController);
        stateMachinePtr = other.stateMachinePtr;

        other.spriteSheet = nullptr;
        other.stateMachinePtr = nullptr;
    }
    return *this;
}

void AnimSprite::init(SpriteSheet* sheet, std::unique_ptr<IAnimController> ctrl)
{
    spriteSheet = sheet;
    animController = std::move(ctrl);
    updateStateMachineCache();
}

void AnimSprite::update(float dtMs)
{
    if (animController)
    {
        animController->update(dtMs);
    }
}

void AnimSprite::reset()
{
    if (animController)
    {
        animController->reset();
    }
}

Sprite* AnimSprite::getActiveSprite() const
{
    if (!spriteSheet || !animController)
        return nullptr;

    return spriteSheet->getFrame(animController->getCurrentFrame());
}

int AnimSprite::getCurrentFrame() const
{
    return animController ? animController->getCurrentFrame() : 0;
}

bool AnimSprite::isComplete() const
{
    return animController ? animController->isComplete() : true;
}

bool AnimSprite::setState(const std::string& stateName)
{
    if (!stateMachinePtr)
        return false;

    stateMachinePtr->setState(stateName);
    return true;
}

const std::string& AnimSprite::getStateName() const
{
    if (!stateMachinePtr)
        return emptyString;

    return stateMachinePtr->getStateName();
}

bool AnimSprite::isStateComplete() const
{
    if (!stateMachinePtr)
        return true;

    return stateMachinePtr->isStateComplete();
}

void AnimSprite::updateStateMachineCache()
{
    stateMachinePtr = dynamic_cast<StateMachineAnim*>(animController.get());
}
