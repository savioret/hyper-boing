#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <climits>
#include <memory>
#include "../main.h"
#include "appdata.h"
#include "appconsole.h"
#include "logger.h"
#include "eventmanager.h"
#include "harpoonshot.h"
#include "gunshot.h"
#include <SDL.h>
#include <SDL_render.h>
#include <SDL_image.h>

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

Scene::Scene(Stage* stg, std::unique_ptr<StageClear> pstgclr)
    : currentState(pstgclr ? SceneState::Playing : SceneState::Ready),
      gameOverSubState(GameOverSubState::ContinueCountdown),
      pStageClear(std::move(pstgclr)), gameOverCountdown(10),
      stage(stg), dSecond(0), timeRemaining(0), timeLine(0),
      moveTick(0), moveLastTick(0), moveCount(0),
      drawTick(0), drawLastTick(0), drawCount(0),
      boundingBoxes(false),
      readyVisible(true)
{
    gameinf.isMenu() = false;
    if (pStageClear) pStageClear->scene = this;
}

int Scene::init()
{
    GameState::init();

    char txt[MAX_PATH];

    timeLine = 0;
    dSecond = 0;
    timeRemaining = stage->timelimit;

    stage->restart();

    // Only initialize Ready state if there's no stage clear animation in progress
    // (Stage clear animation will transition to Ready when curtain opens)
    if (!pStageClear)
    {
        setState(SceneState::Ready);
    }

    gameinf.getPlayer(AppData::PLAYER1)->setX((float)stage->xpos[AppData::PLAYER1]);
    if (gameinf.getPlayer(AppData::PLAYER2))
        gameinf.getPlayer(AppData::PLAYER2)->setX((float)stage->xpos[AppData::PLAYER2]);

    CloseMusic();
    initBitmaps();

    std::snprintf(txt, sizeof(txt), "assets/music/%s", stage->music);
    OpenMusic(txt);
    PlayMusic();

    // Clear stage-level once helper (resets all stage flags)
    stageOnceHelper.clear();

    // Register warning sound with short ID (as sound, not music)
    appAudio.registerSound("last_seconds", "assets/music/last_seconds.ogg");

    // Subscribe to gameplay events
    subscribeToEvents();

    // Fire STAGE_LOADED event (stage is prepared and displayed)
    GameEventData loadedEvent(GameEventType::STAGE_LOADED);
    loadedEvent.stageLoaded.stageId = stage->id;
    EVENT_MGR.trigger(loadedEvent);

    return 1;
}

void Scene::subscribeToEvents()
{
    // Subscribe to time warning event (uses once() for cleaner pattern)
    timeWarningHandle = EVENT_MGR.subscribe(GameEventType::TIME_SECOND_ELAPSED,
        [this](const GameEventData& data) {
            if (data.timeElapsed.newTime == 11 && once("last_seconds_warning")) {
                // Parallel fade: BGM fades out WHILE warning fades in
                appAudio.fadeOutMusic(5000);
                appAudio.playSoundWithFadeIn("last_seconds", 5000, false); // false = play once
            }
        });

    // Subscribe to ball hit event for pop sound effects
    ballHitHandle = EVENT_MGR.subscribe(GameEventType::BALL_HIT,
        [this](const GameEventData& data) {
            // Play appropriate pop sound based on ball size (only if loaded)
            // Size 0 = largest (pop1), Size 1-2 = medium (pop2), Size 3 = smallest (pop3)
            int ballSize = data.ballHit.ball->getSize();

            const char* soundId = nullptr;
            if (ballSize == 0) {
                soundId = "pop1";
            } else if (ballSize <= 2) {
                soundId = "pop2";
            } else {
                soundId = "pop3";
            }

            // Only play if sound is actually loaded (prevents error spam)
            if (soundId && appAudio.isSoundLoaded(soundId)) {
                appAudio.playSound(soundId);
            }
        });

    // Subscribe to player shoot event for weapon sound effects
    playerShootHandle = EVENT_MGR.subscribe(GameEventType::PLAYER_SHOOT,
        [this](const GameEventData& data) {
            // Play appropriate weapon sound based on weapon type (only if loaded)
            const char* soundId = nullptr;
            bool isHarpoon = false;

            switch (data.playerShoot.weapon) {
                case WeaponType::HARPOON:
                    soundId = "harpoon";
                    isHarpoon = true;
                    break;
                case WeaponType::GUN:
                    soundId = "gun";
                    break;
            }

            // Only play if sound is actually loaded (prevents error spam)
            if (soundId && appAudio.isSoundLoaded(soundId)) {
                if (isHarpoon) {
                    // Harpoon sound is looped - need to track channel to stop it when shot dies
                    int channel = appAudio.playSound(soundId);  // Loop=true

                    // Find the shot that was just created and associate the audio channel
                    // The shot was added to lsShoots just before this event fired
                    if (channel >= 0 && !lsShoots.empty()) {
                        // The most recent shot(s) belong to this player
                        for (auto it = lsShoots.rbegin(); it != lsShoots.rend(); ++it) {
                            Shot* shot = it->get();
                            if (shot->getPlayer() == data.playerShoot.player &&
                                shot->getAudioChannel() < 0) {
                                shot->setAudioChannel(channel);
                                break;  // Only set channel for one shot
                            }
                        }
                    }
                } else {
                    // Gun and other weapons: one-shot sound, no tracking needed
                    appAudio.playSound(soundId);
                }
            }
        });

    // Subscribe to player hit event for death sound effect
    playerHitHandle = EVENT_MGR.subscribe(GameEventType::PLAYER_HIT,
        [this](const GameEventData& data) {
            // Play player death sound (only if loaded)
            if (appAudio.isSoundLoaded("player_dies")) {
                appAudio.playSound("player_dies");
            }
        });

    // Subscribe to pickup collected event for pickup sound effect
    pickupCollectedHandle = EVENT_MGR.subscribe(GameEventType::PICKUP_COLLECTED,
        [this](const GameEventData& data) {
            if (appAudio.isSoundLoaded("pickup")) {
                appAudio.playSound("pickup");
            }
        });
}

void Scene::unsubscribeFromEvents()
{
    // Event handles automatically unsubscribe when destroyed,
    // but we can explicitly clear them here for clarity
    timeWarningHandle = EventManager::ListenerHandle();
    ballHitHandle = EventManager::ListenerHandle();
    playerShootHandle = EventManager::ListenerHandle();
    playerHitHandle = EventManager::ListenerHandle();
    pickupCollectedHandle = EventManager::ListenerHandle();
}

/**
 * Loads scene-specific bitmaps.
 * Shared resources (balls, UI, fonts) are referenced from AppData.
 * Only stage-specific resources (background, weapons) are loaded here.
 */
int Scene::initBitmaps()
{
    char txt[MAX_PATH];

    // Weapon sprites are now loaded in AppData::initStageResources()
    // This ensures they're shared across all scenes and only loaded once

    // Load stage-specific background
    std::snprintf(txt, sizeof(txt), "assets/graph/bg/%s", stage->back);
    bmp.back.init(&appGraph, txt, 16, 16);
    appGraph.setColorKey(bmp.back.getBmp(), 0x00FF00);

    // Initialize font renderers using shared resources
    StageResources& res = gameinf.getStageRes();
    int offs[10] = { 0, 22, 44, 71, 93, 120, 148, 171, 198, 221 };
    int offs1[10] = { 0, 13, 18, 31, 44, 58, 70, 82, 93, 105 };
    int offs2[10] = { 0, 49, 86, 134, 187, 233, 277, 327, 374, 421 };
    
    fontNum[0].init(&res.fontnum[0]);
    fontNum[0].setValues(offs);
    fontNum[1].init(&res.fontnum[1]);
    fontNum[1].setValues(offs1);
    fontNum[2].init(&res.fontnum[2]);
    fontNum[2].setValues(offs2);

    // Load freeze countdown font (3-2-1 during last 3s of time freeze)
    freezeEffect.init(&appGraph);

    // Configure ball-info section
    TextSection& ballSection = textOverlay.getSection("ball-info");
    ballSection.setPosition(300, 20)
               .setLineHeight(8)
               .setAlpha(200);
    
    return 1;
}

void Scene::addBall(int x, int y, int size, int top, int dirX, int dirY, int id)
{
    // Get ball diameter for collision check
    const int diameters[] = { 64, 40, 24, 16 }; // size 0-3
    int ballDiameter = (size >= 0 && size <= 3) ? diameters[size] : 40;
    
    // Validate position if either coordinate is random (INT_MAX)
    if (x == INT_MAX || y == INT_MAX)
    {
        checkValidPosition(x, y, ballDiameter);
    }
    
    lsBalls.push_back(std::make_unique<Ball>(this, x, y, size, dirX, dirY, top, id));
}

void Scene::checkValidPosition(int& x, int& y, int ballDiameter)
{
    // Track which coordinates were originally random (INT_MAX)
    bool xWasRandom = (x == INT_MAX);
    bool yWasRandom = (y == INT_MAX);

    if (!xWasRandom && !yWasRandom)
    {
        // Both coordinates are fixed, no need to check
        return;
	}
    
    // Calculate initial positions for INT_MAX coordinates
    if (xWasRandom)
    {
        x = std::rand() % 600 + 32; // Random X in range [32, 632)
    }
    
    if (yWasRandom)
    {
        y = std::rand() % 394 + 22; // Random Y in range [22, 416) - playable area
    }
    
    // Try to find valid position (up to 10 attempts)
    bool validPosition = false;
    int attempts = 0;
    const int maxAttempts = 10;
    
    while (!validPosition && attempts < maxAttempts)
    {
        validPosition = true;
        
        // Create temporary ball for collision checking using existing Ball::collision() method
        Ball tempBall(this, x, y, 0, 1, 1, 0, 0);
        
        // Check for collisions with existing floors in scene using Ball's collision method
        for (const auto& floor : lsFloor)
        {
            // Skip dead floors
            if (floor->isDead())
                continue;
            
            // Use existing Ball::collision(Floor*) method
            if (tempBall.collision(floor.get()))
            {
                // Collision detected! Generate new random position
                validPosition = false;

                LOG_DEBUG("Cannot spawn ball at (x=%d, y=%d) => (floor x=%d, y=%d, w=%d, h=%d) ## COLLISION",
                    x, y, floor->getX(), floor->getY(), floor->getWidth(), floor->getHeight());
                
                // Regenerate ONLY the coordinates that were originally random
                if (xWasRandom)
                {
                    x = std::rand() % 600 + 32;
                }
                
                if (yWasRandom)
                {
                    y = std::rand() % 394 + 22;
                }
                
                break;
            }
        }
        
        attempts++;
    }
    
    if (!validPosition)
    {
        LOG_WARNING("Failed to find valid position for ball after %d attempts (x=%d, y=%d). Spawning anyway.",
            maxAttempts, x, y);
    }
    else if (attempts > 1)
    {
        LOG_DEBUG("Found valid position for ball after %d attempts (x=%d, y=%d)", 
            attempts, x, y);
    }
}

void Scene::addPickup(int x, int y, PickupType type)
{
    lsPickups.push_back(std::make_unique<Pickup>(this, x, y, type));
}

void Scene::addFloor(int x, int y, int id)
{
    lsFloor.push_back(std::make_unique<Floor>(this, x, y, id));
}

void Scene::addLadder(int x, int y, int numTiles)
{
    lsLadders.push_back(std::make_unique<Ladder>(this, x, y, numTiles));
}

Ladder* Scene::findLadderAtPlayer(Player* player) const
{
    if (!player) return nullptr;

    CollisionBox playerBox = player->getCollisionBox();

    for (const auto& ladder : lsLadders)
    {
        if (ladder->isDead()) continue;

        CollisionBox ladderBox = ladder->getCollisionBox();

        // Check if player is within ladder's vertical extent
        int playerBottom = playerBox.y + playerBox.h;
        int ladderTop = ladderBox.y;
        int ladderBottom = ladderBox.y + ladderBox.h;

        if (playerBottom >= ladderTop && playerBox.y <= ladderBottom)
        {
            // Check horizontal overlap using 50% rule
            if (has50PercentOverlap(playerBox, ladderBox))
            {
                return ladder.get();
            }
        }
    }
    return nullptr;
}

Ladder* Scene::findLadderBelowPlayer(Player* player) const
{
    if (!player) return nullptr;

    CollisionBox playerBox = player->getCollisionBox();
    int playerBottom = playerBox.y + playerBox.h;

    for (const auto& ladder : lsLadders)
    {
        if (ladder->isDead()) continue;

        // Check if player is within the top entry zone of the ladder.
        // The ladder graphic often protrudes slightly above the floor surface, so the
        // ladder top (getTopY) can be a few pixels ABOVE the player's feet when standing
        // on the platform. We accept anything within one tile height of the ladder top.
        int ladderTop = ladder->getTopY();
        int ladderTopSection = ladderTop + ladder->getTileHeight();
        if (playerBottom >= ladderTop - LADDER_ENTRY_TOLERANCE &&
            playerBottom <= ladderTopSection)
        {
            CollisionBox ladderBox = ladder->getCollisionBox();

            // Check horizontal overlap using 50% rule
            if (has50PercentOverlap(playerBox, ladderBox))
            {
                return ladder.get();
            }
        }
    }
    return nullptr;
}

Floor* Scene::findFloorUnderPlayer(Player* player) const
{
    if (!player) return nullptr;

    // Use player's logical position (stable across sprite changes) instead of collision box
    // Player Y is feet position (bottom-middle), X is horizontal center
    float playerFeetY = player->getY();
    float playerCenterX = player->getX();

    // Use a consistent width for floor checks (doesn't change with sprite)
    // This prevents floor detection from breaking when sprite changes (e.g., shooting)
    int playerLeft = (int)(playerCenterX - SURFACE_CHECK_WIDTH / 2);
    int playerRight = (int)(playerCenterX + SURFACE_CHECK_WIDTH / 2);

    Floor* bestFloor = nullptr;
    int closestY = Stage::MAX_Y + 1;

    for (const auto& floor : lsFloor)
    {
        if (floor->isDead()) continue;

        CollisionBox floorBox = floor->getCollisionBox();
        int floorLeft = floorBox.x;
        int floorRight = floorBox.x + floorBox.w;
        int floorTopY = floorBox.y;

        // Floor top must be at or below player's feet (within tolerance for landing)
        if (floorTopY < (int)playerFeetY - SURFACE_STEP_TOLERANCE) continue;

        // Check horizontal overlap - player BB must overlap floor BB by at least 1 pixel
        int overlapWidth = calculateHorizontalOverlap(playerLeft, playerRight, floorLeft, floorRight);
        if (overlapWidth < 1) continue;

        // Find closest floor below (or at feet level)
        if (floorTopY < closestY)
        {
            closestY = floorTopY;
            bestFloor = floor.get();
        }
    }

    return bestFloor;
}

Ladder* Scene::findLadderTopUnderPlayer(Player* player) const
{
    if (!player) return nullptr;

    // Use player's logical position (stable across sprite changes)
    float playerFeetY = player->getY();
    float playerCenterX = player->getX();

    // Use consistent width for ladder checks (matches floor check)
    int playerLeft = (int)(playerCenterX - SURFACE_CHECK_WIDTH / 2);
    int playerRight = (int)(playerCenterX + SURFACE_CHECK_WIDTH / 2);

    Ladder* bestLadder = nullptr;
    int closestY = Stage::MAX_Y + 1;

    for (const auto& ladder : lsLadders)
    {
        if (ladder->isDead()) continue;

        int ladderTopY = ladder->getTopY();
        int ladderLeft = ladder->getCenterX() - ladder->getTileWidth() / 2;
        int ladderRight = ladder->getCenterX() + ladder->getTileWidth() / 2;

        // Ladder top must be at or below player's feet (within tolerance)
        if (ladderTopY < (int)playerFeetY - SURFACE_STEP_TOLERANCE) continue;

        // Check horizontal overlap - player BB must overlap ladder BB by at least 1 pixel
        int overlapWidth = calculateHorizontalOverlap(playerLeft, playerRight, ladderLeft, ladderRight);
        if (overlapWidth < 1) continue;

        // Find closest ladder top below (or at feet level)
        if (ladderTopY < closestY)
        {
            closestY = ladderTopY;
            bestLadder = ladder.get();
            LOG_TRACE("Ladder top candidate: ladderTopY=%d, playerFeetY=%.1f, overlapW=%d",
                      ladderTopY, playerFeetY, overlapWidth);
        }
    }

    return bestLadder;
}

float Scene::findGroundLevel(Player* player) const
{
    if (!player) return (float)Stage::MAX_Y;

    float groundY = (float)Stage::MAX_Y;  // Default to screen bottom
    const char* groundSource = "MAX_Y";

    // Check floors
    Floor* floor = findFloorUnderPlayer(player);
    if (floor)
    {
        float floorY = (float)floor->getCollisionBox().y;
        if (floorY < groundY)
        {
            groundY = floorY;
            groundSource = "floor";
        }
    }

    // Check ladder tops
    Ladder* ladder = findLadderTopUnderPlayer(player);
    if (ladder)
    {
        float ladderTopY = (float)ladder->getTopY();
        if (ladderTopY < groundY)
        {
            groundY = ladderTopY;
            groundSource = "ladder_top";
        }
    }

    CollisionBox playerBox = player->getCollisionBox();
    int playerFeetY = playerBox.y + playerBox.h;
    //LOG_TRACE("findGroundLevel: playerFeetY=%d, groundY=%.1f, source=%s",
    //          playerFeetY, groundY, groundSource);

    return groundY;
}

/**
 * Factory method to create appropriate Shot subclass based on weapon type
 * @param pl Player firing the shot
 * @param type Weapon type
 * @return unique_ptr to created Shot object
 */
std::unique_ptr<Shot> Scene::createShot(Player* pl, WeaponType type)
{
    StageResources& res = gameinf.getStageRes();

    switch (type)
    {
    case WeaponType::HARPOON:
        return std::make_unique<HarpoonShot>(this, pl, type);

    case WeaponType::GUN:
        return std::make_unique<GunShot>(this, pl, res.gunBulletAnim.get());

    default:
        // Fallback to harpoon for unknown types
        return std::make_unique<HarpoonShot>(this, pl, WeaponType::HARPOON);
    }
}

void Scene::shoot(Player* pl)
{
    if (!pl->canShoot()) return;

    const WeaponConfig& config = WeaponConfig::get(pl->getWeapon());

    lsShoots.push_back(createShot(pl, pl->getWeapon()));

    pl->shoot();  // Updates player state and animation

    // Fire PLAYER_SHOOT event
    GameEventData event(GameEventType::PLAYER_SHOOT);
    event.playerShoot.player = pl;
    event.playerShoot.weapon = pl->getWeapon();
    EVENT_MGR.trigger(event);
}

/**
 * Get player by index for console commands and other external access
 * @param index Player index (0 for player 1, 1 for player 2)
 * @return Pointer to player if active and playing, nullptr otherwise
 */
Player* Scene::getPlayer(int index)
{
    if (index == 0 && gameinf.getPlayer(0)->isPlaying())
    {
        return gameinf.getPlayer(0);
    }
    else if (index == 1 && gameinf.getPlayer(1) && gameinf.getPlayer(1)->isPlaying())
    {
        return gameinf.getPlayer(1);
    }
    return nullptr;
}

/**
 * Handles the logic when a ball is hit. It creates two smaller
 * balls if possible, or removes the ball if it's already the
 * smallest size. It also grants points to the player.
 */

void Scene::win()
{
    skipToStage( AppData::instance ().currentStage + 1 );
}

void Scene::skipToStage(int stageNumber)
{
    AppData& appData = AppData::instance();
    
    // // Validate stage number (1-indexed)
    // if (stageNumber < 1 || stageNumber > appData.numStages)
    // {
    //     return;
    // }
    
    // DON'T update currentStage yet - StageClear needs to show the CURRENT level as "completed"
    // It will be updated when transitioning to the next scene
    
    // Trigger win sequence with target stage specified
    CloseMusic();
    OpenMusic("assets/music/win.ogg");
    PlayMusic();
    setState(SceneState::LevelClear);

    // Fire LEVEL_CLEAR event
    GameEventData event(GameEventType::LEVEL_CLEAR);
    event.levelClear.stageId = stage->id;
    EVENT_MGR.trigger(event);

    // Create StageClear with target stage number (for later transition)
    pStageClear = std::make_unique<StageClear>(this, stageNumber);
}

void Scene::checkSequence()
{
    StageObject obj;

    do
    {
        obj = stage->pop(timeLine);

        switch (obj.id)
        {
        case StageObjectType::Ball:
            if (obj.params)
            {
                if (auto* ball = obj.getParams<BallParams>())
                {
                    addBall(obj.x, obj.y, ball->size, ball->top, ball->dirX, ball->dirY, ball->ballType);

                    // Fire STAGE_OBJECT_SPAWNED event
                    GameEventData event(GameEventType::STAGE_OBJECT_SPAWNED);
                    event.objectSpawned.id = static_cast<int>(obj.id);
                    event.objectSpawned.x = obj.x;
                    event.objectSpawned.y = obj.y;
                    EVENT_MGR.trigger(event);
                }
            }
            else
            {
                // Fallback: default ball (should not happen with new API)
                addBall(obj.x, obj.y);
            }
            break;

        case StageObjectType::Floor:
            if (obj.params)
            {
                if (auto* floor = obj.getParams<FloorParams>())
                {
                    addFloor(obj.x, obj.y, floor->floorType);

                    // Fire STAGE_OBJECT_SPAWNED event
                    GameEventData event(GameEventType::STAGE_OBJECT_SPAWNED);
                    event.objectSpawned.id = static_cast<int>(obj.id);
                    event.objectSpawned.x = obj.x;
                    event.objectSpawned.y = obj.y;
                    EVENT_MGR.trigger(event);
                }
            }
            else
            {
                // Fallback: default floor (should not happen with new API)
                addFloor(obj.x, obj.y, 0);
            }
            break;

        case StageObjectType::Ladder:
            if (obj.params)
            {
                if (auto* ladder = obj.getParams<LadderParams>())
                {
                    addLadder(obj.x, obj.y, ladder->numTiles);

                    // Fire STAGE_OBJECT_SPAWNED event
                    GameEventData event(GameEventType::STAGE_OBJECT_SPAWNED);
                    event.objectSpawned.id = static_cast<int>(obj.id);
                    event.objectSpawned.x = obj.x;
                    event.objectSpawned.y = obj.y;
                    EVENT_MGR.trigger(event);
                }
            }
            else
            {
                // Fallback: default ladder (3 tiles high)
                addLadder(obj.x, obj.y, 3);
            }
            break;

        case StageObjectType::Action:
            if (obj.params)
            {
                if (auto* action = obj.getParams<ActionParams>())
                {
                    LOG_DEBUG("Executing stage action: /%s", action->command.c_str());
                    CONSOLE.executeCommand(action->command);
                }
            }
            break;

        case StageObjectType::Item:
            if (obj.params)
            {
                if (auto* pickup = obj.getParams<PickupParams>())
                {
                    addPickup(obj.x, obj.y, pickup->pickupType);

                    // Fire STAGE_OBJECT_SPAWNED event
                    GameEventData event(GameEventType::STAGE_OBJECT_SPAWNED);
                    event.objectSpawned.id = static_cast<int>(obj.id);
                    event.objectSpawned.x = obj.x;
                    event.objectSpawned.y = obj.y;
                    EVENT_MGR.trigger(event);
                }
            }
            break;
        }
    } while (obj.id != StageObjectType::Null);
}

void Scene::cleanupBalls()
{
    // Clean up dead balls and create children
    for (auto it = lsBalls.begin(); it != lsBalls.end(); )
    {
        if ((*it)->isDead())
        {
            // Ask ball to create its children
            auto children = (*it)->createChildren();
            
            if (children.first) 
                pendingBalls.push_back(std::move(children.first));
            if (children.second) 
                pendingBalls.push_back(std::move(children.second));

            it = lsBalls.erase(it);  // unique_ptr automatically deletes
        }
        else
        {
            ++it;
        }
    }

    // Check win condition after all removals
    if (lsBalls.empty() && pendingBalls.empty() && !stage->getItemsLeft())
    {
        // Only trigger win once - prevent calling every frame
        if (currentState != SceneState::LevelClear)
        {
            win();
        }
    }

    // Add children to active list
    for (auto& b : pendingBalls)
        lsBalls.push_back(std::move(b));
    pendingBalls.clear();
}

void Scene::setState(SceneState newState)
{
    currentState = newState;

    switch (newState)
    {
        case SceneState::Ready:
            readyVisible = true;
            // 13 toggles = 6.5 visible↔invisible cycles, ends invisible
            readyBlinkAction = blink(&readyVisible, 0.2f, 9);
            break;

        case SceneState::GameOver:
            gameOverCountdown = 10;
            gameOverInputDelay = std::make_unique<DelayAction>(2.0f);
            gameOverSubState = GameOverSubState::ContinueCountdown;
            break;

        case SceneState::Playing:
            // No special initialization needed
            break;

        case SceneState::LevelClear:
            // LevelClear state is managed via pStageClear
            break;
    }
}

void Scene::setTimeFrozen(bool frozen, float duration)
{
    if (frozen)
        freezeEffect.start(duration);
    else
        freezeEffect.stop();
}

void Scene::updateFPSCounters()
{
    moveTick = SDL_GetTicks();
    if (moveTick - moveLastTick > 1000)
    {
        fpsv = moveCount;
        moveCount = 0;
        moveLastTick = moveTick;
    }
    else
    {
        moveCount++;
    }
}

void Scene::updateEntities(float dt)
{
    // === PHASE 1: Movement ===
    // Update all objects without modifying lists

    // Update freeze timer and ball-blink
    freezeEffect.update(dt);

    // Balls only update when NOT frozen
    if (!freezeEffect.isActive())
    {
        for (const auto& ball : lsBalls)
        {
            ball->update(dt);
        }
    }

    for (const auto& shot : lsShoots)
    {
        shot->update(dt);
    }

    for (const auto& floor : lsFloor)
    {
        floor->update(dt);
    }

    for (const auto& ladder : lsLadders)
    {
        ladder->update(dt);
    }

    // Pickups always fall regardless of freeze state
    for (const auto& pickup : lsPickups)
    {
        pickup->update(dt);
    }

    for (const auto& effect : lsEffects)
    {
        effect->update(dt);
    }
}

void Scene::spawnEffect(AnimSpriteSheet* tmpl, int x, int y)
{
    if (tmpl)
        lsEffects.push_back(std::make_unique<AnimEffect>(x, y, tmpl));
}

void Scene::cleanupPhase()
{
    // === PHASE 2: Cleanup ===
    // Ball cleanup is special - creates children during cleanup
    cleanupBalls();

    // Standard cleanup for shots, floors, ladders, pickups, and effects (unique_ptr)
    cleanupDeadObjects(lsShoots);
    cleanupDeadObjects(lsFloor);
    cleanupDeadObjects(lsLadders);
    cleanupDeadObjects(lsPickups);
    cleanupDeadObjects(lsEffects);
}

void Scene::updateTimer(float dt)
{
    if (dSecond < 60)
        dSecond++;
    else
    {
        dSecond = 0;
        if (timeRemaining > 0)
        {
            if (!pStageClear && currentState != SceneState::GameOver)
            {
                int previousTime = timeRemaining;
                timeRemaining--;

                // Fire TIME_SECOND_ELAPSED event
                GameEventData event(GameEventType::TIME_SECOND_ELAPSED);
                event.timeElapsed.previousTime = previousTime;
                event.timeElapsed.newTime = timeRemaining;
                EVENT_MGR.trigger(event);
            }
        }
        else if (timeRemaining == 0 && currentState != SceneState::GameOver)
        {
            setState(SceneState::GameOver);

            // Fire GAME_OVER event (reason: time expired)
            GameEventData event(GameEventType::GAME_OVER);
            event.gameOver.reason = 2;
            EVENT_MGR.trigger(event);

            gameinf.player[AppData::PLAYER1]->setPlaying(false);
            if (gameinf.player[AppData::PLAYER2])
                gameinf.player[AppData::PLAYER2]->setPlaying(false);

            CloseMusic();
            OpenMusic("assets/music/gameover.ogg");
            PlayMusic();
        }

        timeLine++;
        if (currentState == SceneState::GameOver)
        {
            if (gameOverCountdown >= 0)
            {
                gameOverCountdown--;
                // Transition to Definitive when countdown expires
                if (gameOverCountdown < 0 && gameOverSubState == GameOverSubState::ContinueCountdown)
                {
                    gameOverSubState = GameOverSubState::Definitive;
                    LOG_INFO("Game Over: Countdown expired, transitioning to Definitive");
                }
            }
        }
    }
}

GameState* Scene::handleReadyState(float dt)
{
    // Process time=0 objects on first frame of ready screen
    if (once("ready_screen_shown"))
    {
        checkSequence();
        LOG_DEBUG("Ready shown, processing time=0 stage objects.");
    }

    // Update blinking action
    if (readyBlinkAction && !readyBlinkAction->isFinished())
    {
        readyBlinkAction->update(dt);
    }
    else
    {
        // Blinking complete, transition to Playing state
        setState(SceneState::Playing);

        // Fire STAGE_STARTED event (countdown complete, gameplay begins)
        GameEventData startedEvent(GameEventType::STAGE_STARTED);
        startedEvent.stageStarted.stageId = stage->id;
        EVENT_MGR.trigger(startedEvent);
    }

    // During ready screen, skip all game logic
    return nullptr;
}

GameState* Scene::handleGameOverState(float dt)
{
    // Update input delay action (time-based, not frame-based)
    if (gameOverInputDelay && !gameOverInputDelay->isFinished())
    {
        gameOverInputDelay->update(dt);
    }

    for (int i = 0; i < 2; i++)
    {
        if (gameinf.getPlayer(i))
        {
            // Only accept input after delay has expired
            if ((!gameOverInputDelay || gameOverInputDelay->isFinished()) &&
                appInput.key(gameinf.getKeys(i).getShoot()))
            {
                if (gameOverSubState == GameOverSubState::ContinueCountdown)
                {
                    // Player chose to continue - restart from beginning
                    LOG_INFO("Game Over: Player pressed continue, restarting game");
                    gameinf.getPlayer(AppData::PLAYER1)->init();
                    if (gameinf.getPlayer(AppData::PLAYER2))
                        gameinf.getPlayer(AppData::PLAYER2)->init();
                    gameinf.initStages();
                    return new Scene(stage);
                }
                else  // GameOverSubState::Definitive
                {
                    // No continue - return to menu
                    LOG_INFO("Game Over: Player returning to menu");
                    gameinf.player[AppData::PLAYER1].reset();
                    gameinf.player[AppData::PLAYER2].reset();
                    return new Menu;
                }
            }
        }
    }

    return nullptr;
}

GameState* Scene::handlePlayingState(float dt)
{
    int i;

    // Player input is only processed during Playing state
    for (i = 0; i < 2; i++)
    {
        Player* player = gameinf.getPlayer(i);

        // Track shoot key edge: fire only on press, not while held (Super Pang behaviour)
        bool shootKeyNow = appInput.key(gameinf.getKeys(i).getShoot());
        bool shootJustPressed = shootKeyNow && !shootKeyWasDown[i];
        shootKeyWasDown[i] = shootKeyNow;

        if (player && !player->isDead() && player->isPlaying())
        {
            // Get UP/DOWN keys from configuration
            SDL_Scancode upKey = gameinf.getKeys(i).getUp();
            SDL_Scancode downKey = gameinf.getKeys(i).getDown();

            // Handle shooting (can shoot while climbing)
            if (shootJustPressed)
            {
                shoot(player);
            }

            // Handle climbing
            if (player->isClimbing())
            {
                // On ladder - UP/DOWN to climb, LEFT/RIGHT to detach and walk
                if (appInput.key(gameinf.getKeys(i).getLeft()))
                {
                    // Detach from ladder and walk left
                    player->stopClimbing();  // Clears ladder, sets IDLE
                    player->moveLeft();      // Sets WALKING, updates facing, moves left
                }
                else if (appInput.key(gameinf.getKeys(i).getRight()))
                {
                    // Detach from ladder and walk right
                    player->stopClimbing();  // Clears ladder, sets IDLE
                    player->moveRight();     // Sets WALKING, updates facing, moves right
                }
                else if (appInput.key(upKey))
                {
                    player->climbUp();
                }
                else if (appInput.key(downKey))
                {
                    player->climbDown();
                }
            }
            else if (player->getState() == PlayerState::WAKING_UP)
            {
                // Frozen during waking up animation (300ms) - no input processed
                // Animation will auto-transition to IDLE via callback
            }
            else
            {
                // Not climbing - normal movement + ladder entry
                if (appInput.key(upKey))
                {
                    // Try to enter ladder (works both grounded and while falling)
                    Ladder* ladder = findLadderAtPlayer(player);
                    if (ladder && player->canEnterLadder(ladder))
                    {
                        // Don't allow entering if player is already at or above the ladder top
                        // (prevents toggling when holding UP at ladder top)
                        float playerFeetY = player->getY();
                        float ladderTopY = (float)ladder->getTopY();

                        if (playerFeetY > ladderTopY + 2.0f)  // 2px tolerance
                        {
                            player->startClimbing(ladder);
                        }
                    }
                }
                else if (appInput.key(downKey))
                {
                    // Try to climb down from platform/ladder top
                    Ladder* ladder = findLadderBelowPlayer(player);
                    if (ladder && player->canEnterLadder(ladder))
                    {
                        player->startClimbing(ladder);
                        // Start moving down slightly to enter the ladder
                        player->setY(player->getY() + player->getClimbSpeed());
                    }
                }
                else if (player->getState() == PlayerState::SHOOTING)
                {
                    // Frozen during shooting animation - call stop() to handle
                    // SHOOTING → IDLE transition when shotCounter expires
                    player->stop();
                }
                else if (appInput.key(gameinf.getKeys(i).getLeft()))
                {
                    player->moveLeft();
                }
                else if (appInput.key(gameinf.getKeys(i).getRight()))
                {
                    player->moveRight();
                }
                else if ( player->getState() != PlayerState::IDLE )
                {
                    player->stop();
                }
            }
        }
    }

    // Always update players (even if dead, for death animation)
    for (i = 0; i < 2; i++)
    {
        if (gameinf.getPlayer(i))
        {
            gameinf.getPlayer(i)->update(dt, this);
        }
    }

    // Check game over condition
    if (gameinf.getPlayer(AppData::PLAYER1) && !gameinf.getPlayer(AppData::PLAYER2))
    {
        if (!gameinf.getPlayer(AppData::PLAYER1)->isPlaying())
        {
            setState(SceneState::GameOver);

            // Fire GAME_OVER event (reason: player 1 dead)
            GameEventData event(GameEventType::GAME_OVER);
            event.gameOver.reason = 0;  // Player 1 dead
            EVENT_MGR.trigger(event);

            CloseMusic();
            OpenMusic("assets/music/gameover.ogg");
            PlayMusic();
        }
    }
    else if (gameinf.getPlayer(AppData::PLAYER1) && gameinf.getPlayer(AppData::PLAYER2))
    {
        if (!gameinf.getPlayer(AppData::PLAYER1)->isPlaying() && !gameinf.getPlayer(AppData::PLAYER2)->isPlaying())
        {
            setState(SceneState::GameOver);

            // Fire GAME_OVER event (reason: both players dead)
            GameEventData event(GameEventType::GAME_OVER);
            event.gameOver.reason = 1;  // Both players dead
            EVENT_MGR.trigger(event);

            CloseMusic();
            OpenMusic("assets/music/gameover.ogg");
            PlayMusic();
        }
    }

    // === COLLISION PIPELINE ===
    // Phase 1: Detect collisions and resolve physics
    CollisionSystem::Context ctx = {
        lsBalls,
        lsShoots,
        lsFloor,
        lsPickups,
        { gameinf.player[AppData::PLAYER1].get(), gameinf.player[AppData::PLAYER2].get() },
        true  // checkPlayerCollisions = true during Playing state
    };
    ContactList contacts = collisionSystem.detectAndResolve(ctx);

    // Phase 2: Apply game rules to contacts
    gameRules.processContacts(contacts, this);

    return nullptr;
}

GameState* Scene::handleLevelClearState(float dt)
{
    // During level clear, only update players (no input processing)
    for (int i = 0; i < 2; i++)
    {
        if (gameinf.getPlayer(i) && gameinf.getPlayer(i)->isPlaying())
        {
            gameinf.getPlayer(i)->update(dt, this);
        }
    }

    return nullptr;
}

GameState* Scene::updateStageProgression(float dt)
{
    int res;

    if (pStageClear)
    {
        res = pStageClear->moveAll();
        if (res == -1)
        {
            // Check if this is a console-triggered skip to a specific stage
            int nextStageId;
            if (pStageClear->getTargetStage() > 0)
            {
                // Console command specified a target stage
                nextStageId = pStageClear->getTargetStage();
            }
            else
            {
                // Normal progression: advance to next stage
                nextStageId = stage->id + 1;
            }

            if (nextStageId <= gameinf.getNumStages())
            {
                gameinf.getCurrentStage() = nextStageId;
                // Create new scene WITHOUT passing StageClear - let new scene start fresh with Ready countdown
                Scene* newScene = new Scene(&gameinf.getStages()[nextStageId - 1]);  // No pStageClear argument!
                return newScene;
            }
            else
                return new Menu();
        }
        if (res == 0)
        {
            // Stage clear opening animation complete, enable ready screen
            pStageClear.reset();
            setState(SceneState::Ready);
        }
    }
    else if (currentState == SceneState::Playing) // Only check sequence during playing state
    {
        checkSequence();
    }

    return nullptr;
}

GameState* Scene::moveAll(float dt)
{
    updateFPSCounters();

    if (goback)
    {
        goback = false;
        gameinf.isMenu() = true;
        return new Menu;
    }

    // Dispatch to state handler
    GameState* result = nullptr;
    switch (currentState)
    {
        case SceneState::Ready:
            result = handleReadyState(dt);
            break;
        case SceneState::Playing:
            result = handlePlayingState(dt);
            break;
        case SceneState::GameOver:
            result = handleGameOverState(dt);
            break;
        case SceneState::LevelClear:
            result = handleLevelClearState(dt);
            break;
    }

    if (result) return result;

    // During Ready state, skip entity updates, cleanup, and timer to freeze the scene
    if (currentState != SceneState::Ready)
    {
        // Common: update entities, cleanup, time management
        updateEntities(dt);
        cleanupPhase();
        updateTimer(dt);
    }

    return updateStageProgression(dt);
}

int Scene::release()
{
    // Unsubscribe from all events
    unsubscribeFromEvents();

    // Clean up entities - unique_ptr automatically deletes
    lsBalls.clear();
    lsShoots.clear();
    lsFloor.clear();
    lsLadders.clear();
    lsPickups.clear();

    // Release only scene-specific sprites (background)
    bmp.back.release();

    // Shared resources (balls, floors, UI, fonts, weapons) are managed by AppData
    // and persist across scene transitions

    CloseMusic();

    return 1;
}

void Scene::drawBackground()
{
// Draw background
    appGraph.draw(&bmp.back, 0, 0);
}

void Scene::draw(Ball* b)
{
    if (!freezeEffect.areBallsVisible()) return;  // Hidden during freeze-warning blink
    StageResources& res = gameinf.getStageRes();
    appGraph.draw(&res.redball[b->getSize()], (int)b->getX(), (int)b->getY());
}

void Scene::draw(Player* pl)
{
    pl->draw(&appGraph);
}

void Scene::draw(Floor* fl)
{
    StageResources& res = gameinf.getStageRes();
    appGraph.draw(&res.floor[fl->getId()], fl->getX(), fl->getY());
}

void Scene::drawScore()
{
    StageResources& res = gameinf.getStageRes();
    
    if (gameinf.getPlayer(AppData::PLAYER1)->isPlaying())
    {
        appGraph.draw(&fontNum[1], gameinf.getPlayer(AppData::PLAYER1)->getScore(), 80, RES_Y - 55);
        appGraph.draw(&res.miniplayer[AppData::PLAYER1], 20, Stage::MAX_Y + 7);
        for (int i = 0; i < gameinf.getPlayer(AppData::PLAYER1)->getLives(); i++)
        {
            appGraph.draw(&res.lives[AppData::PLAYER1], 80 + 26 * i, Stage::MAX_Y + 30);
        }
    }

    if (gameinf.getPlayer(AppData::PLAYER2))
        if (gameinf.getPlayer(AppData::PLAYER2)->isPlaying())
        {
            appGraph.draw(&res.miniplayer[AppData::PLAYER2], 400, Stage::MAX_Y + 7);
            appGraph.draw(&fontNum[1], gameinf.getPlayer(AppData::PLAYER2)->getScore(), 460, RES_Y - 55);
            for (int i = 0; i < gameinf.getPlayer(AppData::PLAYER2)->getLives(); i++)
            {
                appGraph.draw(&res.lives[AppData::PLAYER2], 460 + 26 * i, Stage::MAX_Y + 30);
            }
        }
}

void Scene::drawMark()
{
    StageResources& res = gameinf.getStageRes();
    
    for (int j = 0; j < 640; j += 16)
    {
        appGraph.draw(&res.mark[2], j, 0);
        appGraph.draw(&res.mark[1], j, Stage::MAX_Y + 1);
        appGraph.draw(&res.mark[0], j, Stage::MAX_Y + 17);
        appGraph.draw(&res.mark[0], j, Stage::MAX_Y + 33);
        appGraph.draw(&res.mark[2], j, Stage::MAX_Y + 49);
    }

    for (int j = 0; j < 416; j += 16)
    {
        appGraph.draw(&res.mark[4], 0, j);
        appGraph.draw(&res.mark[3], Stage::MAX_X + 1, j);
    }

    appGraph.draw(&res.mark[0], 0, 0);
    appGraph.draw(&res.mark[0], Stage::MAX_X + 1, 0);
    appGraph.draw(&res.mark[0], 0, Stage::MAX_Y + 1);
    appGraph.draw(&res.mark[0], Stage::MAX_X + 1, Stage::MAX_Y + 1);
}

/**
 * Draw bounding boxes for all collidable objects
 *
 * Debug visualization showing collision boundaries matching actual collision detection logic.
 * - Players: Uses sprite offset + 5-pixel horizontal margins + 3-pixel vertical offset
 * - Balls: Full diameter box
 * - Shots: Point collision (drawn as small cross for visibility)
 * - Floors: Full rectangular bounds
 */
void Scene::drawBoundingBoxes()
{
    // Set draw color to black for bounding boxes
    appGraph.setDrawColor(0, 0, 0, 255);

    char buf[32];

    // Draw player bounding boxes using the same collision box as Ball::collision()
    for (int i = 0; i < 2; i++)
    {
        Player* pl = gameinf.getPlayer(i);
        if (pl && pl->isPlaying() && pl->isVisible())
        {
            // Use Player's getCollisionBox() to ensure debug matches actual collision
            CollisionBox box = pl->getCollisionBox();
            appGraph.rectangle(box.x, box.y, box.x + box.w, box.y + box.h);
            snprintf(buf, sizeof(buf), "%d,%d", box.x, box.y);
            appGraph.text(buf, box.x, box.y);
            // snprintf(buf, sizeof(buf), "%d,%d", box.w, box.h);
            // appGraph.text(buf, box.x, box.y + 8);
        }
    }

    // Draw ball bounding boxes (circles represented as bounding squares)
    for (const auto& b : lsBalls)
    {
        CollisionBox box = b->getCollisionBox();
        appGraph.rectangle(box.x, box.y, box.x + box.w, box.y + box.h);
        snprintf(buf, sizeof(buf), "%d,%d", box.x, box.y);
        appGraph.text(buf, box.x, box.y);
        //snprintf(buf, sizeof(buf), "%d,%d", d, d);
        //appGraph.text(buf, x, y + 8);
    }

    // Draw shot collision areas
    // Different visualization for Harpoon vs Gun
    for (const auto& s : lsShoots)
    {
        if (!s->isDead())
        {
            int xPos = (int)s->getX();
            int yPos = (int)s->getY();
            int yInit = (int)s->getYInit();

            // Check if this is a GunShot
            GunShot* gunShot = dynamic_cast<GunShot*>(s.get());

            if (gunShot)
            {
                // Gun: Show sprite bounding box (no vertical line)
                Sprite* sprite = gunShot->getCurrentSprite();
                if (sprite)
                {
                    CollisionBox box = s->getCollisionBox();
                    appGraph.rectangle(box.x, box.y, box.x + box.w, box.y + box.h);
                    snprintf(buf, sizeof(buf), "%d,%d", box.x, box.y);
                    appGraph.text(buf, box.x, box.y);
                    // snprintf(buf, sizeof(buf), "%d,%d", w, h);
                    // appGraph.text(buf, spriteX, spriteY + 8);
                }
            }
            else
            {
                // Harpoon: Show vertical chain line + collision point
                // The chain collision is centered at xPos + 8
                int chainX = xPos + 8;
                appGraph.rectangle(chainX, yPos, chainX + 1, yInit);

                // Mark the collision point
                appGraph.rectangle(chainX - 2, yPos - 2, chainX + 2, yPos + 2);
                snprintf(buf, sizeof(buf), "%d,%d", chainX, yPos);
                appGraph.text(buf, chainX + 3, yPos);
            }
        }
    }

    // Draw floor bounding boxes
    for (const auto& f : lsFloor)
    {
        int x = f->getX();
        int y = f->getY();
        int w = f->getWidth();
        int h = f->getHeight();
        appGraph.rectangle(x, y, x + w, y + h);
        snprintf(buf, sizeof(buf), "%d,%d", x, y);
        appGraph.text(buf, x, y);
        // snprintf(buf, sizeof(buf), "%d,%d", w, h);
        // appGraph.text(buf, x, y + 8);
    }

    // Draw ladder bounding boxes (green color to distinguish)
    appGraph.setDrawColor(0, 255, 0, 255);
    for (const auto& ladder : lsLadders)
    {
        CollisionBox box = ladder->getCollisionBox();
        appGraph.rectangle(box.x, box.y, box.x + box.w, box.y + box.h);
        snprintf(buf, sizeof(buf), "%d,%d", box.x, box.y);
        appGraph.text(buf, box.x, box.y);
        // snprintf(buf, sizeof(buf), "%d,%d", box.w, box.h);
        appGraph.text(buf, box.x, box.y + 8);

        // Also draw the "standing" top area (thin line at top)
        CollisionBox topBox = ladder->getTopStandingBox();
        appGraph.rectangle(topBox.x, topBox.y, topBox.x + topBox.w, topBox.y + topBox.h);
    }
}

void Scene::drawDebugOverlay()
{
    AppData& appData = AppData::instance();
    
    if (!appData.debugMode)
    {
        // Clear overlay when debug mode is disabled
        textOverlay.clear();
        return;
    }
    
    // Call base class to populate default section with basic info
    GameState::drawDebugOverlay();

    // Add player info to default section
    if (appData.getPlayer(AppData::PLAYER1))
    {
		Player* p = appData.getPlayer(AppData::PLAYER1);
        textOverlay.addTextF("P1: Score=%d Lives=%d Shoots=%d Facing=%d Frame=%d x=%.1f y=%.1f",
            p->getScore(),
            p->getLives(),
            p->getNumShoots(),
            p->getFacing(),
            p->getFrame(),
			p->getX(),
			p->getY()
        );
    }

    // Show up to 10 balls with all info in one line per ball
    if (!lsBalls.empty())
    {
        int ballCount = 0;
        for (const auto& ball : lsBalls)
        {
            if (ballCount >= 15) break; // Limit to 15 balls

            textOverlay.addTextFS("ball-info", "Ball%d: x=%.0f y=%.0f sz=%d dia=%d dx=%d dy=%d t=%.2f",
                    ballCount,
                    ball->getX(),
                    ball->getY(),
                    ball->getSize(),
                    ball->getDiameter(),
                    ball->getDirX(),
                    ball->getDirY(),
                    ball->getTime());
            ballCount++;
        }
    }

    // Add game state info to default section
    textOverlay.addTextF("Objects: Balls=%d Shoots=%d Floors=%d Ladders=%d Pickups=%d",
            (int)lsBalls.size(),
            (int)lsShoots.size(),
            (int)lsFloor.size(),
            (int)lsLadders.size(),
            (int)lsPickups.size());

    textOverlay.addTextF("Stage: %d  Time=%d  Timeline=%d",
            stage->id, timeRemaining, timeLine);

    textOverlay.addTextF("State=%s",
            currentState == SceneState::Ready ? "Ready" :
            currentState == SceneState::Playing ? "Playing" :
            currentState == SceneState::GameOver ? "GameOver" :
            currentState == SceneState::LevelClear ? "LevelClear" : "Unknown");
}

void Scene::drawEntities()
{
    // Draw floors
    for (const auto& floor : lsFloor)
    {
        draw(floor.get());
    }

    // Draw ladders (over floors, below shots/balls/players)
    for (const auto& ladder : lsLadders)
    {
        ladder->draw(appGraph);
    }

    // Draw shots using polymorphic draw() method
    for (const auto& shot : lsShoots)
    {
        shot->draw(&appGraph);  // Polymorphic call
    }

    // Draw balls
    for (const auto& ball : lsBalls)
    {
        draw(ball.get());
    }

    // Draw pickups (above balls, below effects)
    for (const auto& pickup : lsPickups)
    {
        pickup->draw(&appGraph);
    }

    // Draw one-shot animation effects (pop sparks, muzzle flashes) above balls
    for (const auto& effect : lsEffects)
    {
        effect->draw(&appGraph);
    }
}

void Scene::drawPlayers()
{
    if (gameinf.getPlayer(AppData::PLAYER1)->isVisible() && gameinf.getPlayer(AppData::PLAYER1)->isPlaying())
        draw(gameinf.getPlayer(AppData::PLAYER1));

    if (gameinf.getPlayer(AppData::PLAYER2))
        if (gameinf.getPlayer(AppData::PLAYER2)->isVisible() && gameinf.getPlayer(AppData::PLAYER2)->isPlaying())
            draw(gameinf.getPlayer(AppData::PLAYER2));
}

void Scene::drawHUD()
{
    StageResources& res = gameinf.getStageRes();

    drawMark();
    drawScore();
    appGraph.draw(&res.time, 320 - res.time.getWidth() / 2, Stage::MAX_Y + 3);
    appGraph.draw(&fontNum[FONT_BIG], timeRemaining, 300, Stage::MAX_Y + 25);
}

void Scene::drawStateOverlay()
{
    StageResources& res = gameinf.getStageRes();

    // Freeze countdown: show 3-2-1 during the last 3 seconds of a time freeze
    freezeEffect.drawCountdown();

    // Game over overlay
    if (currentState == SceneState::GameOver)
    {
        if (gameOverSubState == GameOverSubState::ContinueCountdown)
        {
            // Show continue prompt and countdown (no "GAME OVER" text)
            appGraph.draw(&res.continu, 130, 200);
            appGraph.draw(&fontNum[FONT_HUGE], gameOverCountdown, 315, 300);
        }
        else  // GameOverSubState::Definitive
        {
            // Show only "GAME OVER" text, no continue
            appGraph.draw(&res.gameover, 100, 125);
        }
    }

    // Stage clear overlay
    if (pStageClear) pStageClear->drawAll();

    // Ready screen overlay (if in Ready state and visible - blinking)
    if (currentState == SceneState::Ready && readyVisible)
    {
        // Center the ready sprite on screen
        int x = (640 - res.ready.getWidth()) / 2;
        int y = (416 - res.ready.getHeight()) / 2;
        appGraph.draw(&res.ready, x, y);
    }
}

void Scene::updateDrawFPSCounters()
{
    drawTick = SDL_GetTicks();
    if (drawTick - drawLastTick > 1000)
    {
        fps = drawCount;
        drawCount = 0;
        drawLastTick = drawTick;
    }
    else
    {
        drawCount++;
    }
}

int Scene::drawAll()
{
    drawBackground();
    drawEntities();
    drawHUD();
    drawPlayers();
    drawStateOverlay();

    // Draw bounding boxes if debug mode is enabled
    if (boundingBoxes)
    {
        drawBoundingBoxes();
    }

    // Finalize render (debug overlay, console, flip)
    finalizeRender();

    updateDrawFPSCounters();

    return 1;
}
