#include "animspritesheet.h"
#include "asepriteloader.h"
#include "logger.h"

// Static empty string for getStateName() when not a state machine
const std::string AnimSpriteSheet::emptyString;

// ============================================================================
// Constructors / Destructor
// ============================================================================

AnimSpriteSheet::AnimSpriteSheet()
    : sharedSheet(nullptr)
    , ownsSheet(true)
    , stateMachinePtr(nullptr)
{
}

AnimSpriteSheet::AnimSpriteSheet(std::unique_ptr<SpriteSheet> sheet,
                                 std::unique_ptr<IAnimController> anim)
    : ownedSheet(std::move(sheet))
    , sharedSheet(nullptr)
    , ownsSheet(true)
    , animController(std::move(anim))
    , stateMachinePtr(nullptr)
{
    updateStateMachineCache();
}

AnimSpriteSheet::AnimSpriteSheet(SpriteSheet* shared,
                                 std::unique_ptr<IAnimController> clonedAnim)
    : sharedSheet(shared)
    , ownsSheet(false)
    , animController(std::move(clonedAnim))
    , stateMachinePtr(nullptr)
{
    updateStateMachineCache();
}

AnimSpriteSheet::~AnimSpriteSheet() = default;

// Move constructor
AnimSpriteSheet::AnimSpriteSheet(AnimSpriteSheet&& other) noexcept
    : ownedSheet(std::move(other.ownedSheet))
    , sharedSheet(other.sharedSheet)
    , ownsSheet(other.ownsSheet)
    , animController(std::move(other.animController))
    , stateMachinePtr(other.stateMachinePtr)
{
    other.sharedSheet = nullptr;
    other.stateMachinePtr = nullptr;
}

// Move assignment
AnimSpriteSheet& AnimSpriteSheet::operator=(AnimSpriteSheet&& other) noexcept
{
    if (this != &other)
    {
        ownedSheet = std::move(other.ownedSheet);
        sharedSheet = other.sharedSheet;
        ownsSheet = other.ownsSheet;
        animController = std::move(other.animController);
        stateMachinePtr = other.stateMachinePtr;

        other.sharedSheet = nullptr;
        other.stateMachinePtr = nullptr;
    }
    return *this;
}

// ============================================================================
// Factory Methods
// ============================================================================

std::unique_ptr<AnimSpriteSheet> AnimSpriteSheet::load(Graph* graph, const std::string& jsonPath)
{
    auto sheet = std::make_unique<SpriteSheet>();
    auto anim = AsepriteLoader::load(graph, jsonPath, *sheet);

    if (!anim)
    {
        LOG_ERROR("AnimSpriteSheet::load failed: %s", jsonPath.c_str());
        return nullptr;
    }

    return std::unique_ptr<AnimSpriteSheet>(
        new AnimSpriteSheet(std::move(sheet), std::move(anim)));
}

std::unique_ptr<AnimSpriteSheet> AnimSpriteSheet::loadAsStateMachine(Graph* graph, const std::string& jsonPath)
{
    auto sheet = std::make_unique<SpriteSheet>();
    auto anim = AsepriteLoader::loadAsStateMachine(graph, jsonPath, *sheet);

    if (!anim)
    {
        LOG_ERROR("AnimSpriteSheet::loadAsStateMachine failed: %s", jsonPath.c_str());
        return nullptr;
    }

    return std::unique_ptr<AnimSpriteSheet>(
        new AnimSpriteSheet(std::move(sheet), std::move(anim)));
}

// ============================================================================
// Core Animation API
// ============================================================================

void AnimSpriteSheet::update(float dtMs)
{
    if (animController)
    {
        animController->update(dtMs);
    }
}

Sprite* AnimSpriteSheet::getCurrentSprite() const
{
    if (!animController)
        return nullptr;

    SpriteSheet* sheet = ownsSheet ? ownedSheet.get() : sharedSheet;
    if (!sheet)
        return nullptr;

    return sheet->getFrame(animController->getCurrentFrame());
}

int AnimSpriteSheet::getCurrentFrame() const
{
    return animController ? animController->getCurrentFrame() : 0;
}

void AnimSpriteSheet::reset()
{
    if (animController)
    {
        animController->reset();
    }
}

bool AnimSpriteSheet::isComplete() const
{
    return animController ? animController->isComplete() : true;
}

// ============================================================================
// StateMachineAnim-Specific Features
// ============================================================================

bool AnimSpriteSheet::setState(const std::string& stateName)
{
    if (!stateMachinePtr)
        return false;

    stateMachinePtr->setState(stateName);
    return true;
}

const std::string& AnimSpriteSheet::getStateName() const
{
    if (!stateMachinePtr)
        return emptyString;

    return stateMachinePtr->getStateName();
}

bool AnimSpriteSheet::isStateComplete() const
{
    if (!stateMachinePtr)
        return true;

    return stateMachinePtr->isStateComplete();
}

void AnimSpriteSheet::setOnStateComplete(std::function<void(const std::string&)> callback)
{
    if (stateMachinePtr)
    {
        stateMachinePtr->setOnStateComplete(callback);
    }
}

// ============================================================================
// Cloning
// ============================================================================

std::unique_ptr<AnimSpriteSheet> AnimSpriteSheet::clone() const
{
    if (!animController)
        return nullptr;

    // Clone animation (independent playback state)
    auto clonedAnim = animController->clone();

    // Get pointer to sprite sheet (shared, not copied)
    SpriteSheet* sheetPtr = ownsSheet ? ownedSheet.get() : sharedSheet;

    return std::unique_ptr<AnimSpriteSheet>(
        new AnimSpriteSheet(sheetPtr, std::move(clonedAnim)));
}

// ============================================================================
// Direct Access
// ============================================================================

SpriteSheet* AnimSpriteSheet::getSpriteSheet() const
{
    return ownsSheet ? ownedSheet.get() : sharedSheet;
}

int AnimSpriteSheet::getFrameCount() const
{
    SpriteSheet* sheet = getSpriteSheet();
    return sheet ? sheet->getFrameCount() : 0;
}

Sprite* AnimSpriteSheet::getFrame(int index) const
{
    SpriteSheet* sheet = getSpriteSheet();
    return sheet ? sheet->getFrame(index) : nullptr;
}

bool AnimSpriteSheet::isValid() const
{
    return getSpriteSheet() != nullptr && animController != nullptr;
}

// ============================================================================
// Private Helpers
// ============================================================================

void AnimSpriteSheet::updateStateMachineCache()
{
    stateMachinePtr = dynamic_cast<StateMachineAnim*>(animController.get());
}
