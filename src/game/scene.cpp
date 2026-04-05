#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <climits>
#include <memory>
#include "../main.h"
#include "appdata.h"
#include "stageloader.h"
#include "appconsole.h"
#include "logger.h"
#include "eventmanager.h"
#include "harpoonshot.h"
#include "gunshot.h"
#include "clawshot.h"
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
      stage(stg), pendingQuickStage(0), secondAccum(0.0f), timeRemaining(0), timeLine(0),
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

    // Load stage content from file (background, music, and spawn sequence)
    // Skip disk read when coming from Editor with in-memory changes
    if (!stage->skipFileReload && !stage->stageFile.empty())
        StageLoader::load(*stage, stage->stageFile);
    stage->skipFileReload = false;

    timeLine = 0;
    secondAccum = 0.0f;
    timeRemaining = stage->timelimit;

    stage->restart();

    // Only initialize Ready state if there's no stage clear animation in progress
    // (Stage clear animation will transition to Ready when curtain opens)
    if (!pStageClear)
    {
        setState(SceneState::Ready);
    }

    gameinf.getPlayer(AppData::PLAYER1)->setSpawnX((float)stage->xpos[AppData::PLAYER1]);
    gameinf.getPlayer(AppData::PLAYER1)->setSpawnY((float)stage->ypos[AppData::PLAYER1]);
    gameinf.getPlayer(AppData::PLAYER1)->setPos((float)stage->xpos[AppData::PLAYER1], (float)stage->ypos[AppData::PLAYER1]);
    if (gameinf.getPlayer(AppData::PLAYER2))
    {
        gameinf.getPlayer(AppData::PLAYER2)->setSpawnX((float)stage->xpos[AppData::PLAYER2]);
        gameinf.getPlayer(AppData::PLAYER2)->setSpawnY((float)stage->ypos[AppData::PLAYER2]);
        gameinf.getPlayer(AppData::PLAYER2)->setPos((float)stage->xpos[AppData::PLAYER2], (float)stage->ypos[AppData::PLAYER2]);
    }

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
                case WeaponType::CLAW:
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

    // Subscribe to ball hit event to spawn floating score popups
    hitScoreHandle = EVENT_MGR.subscribe(GameEventType::BALL_HIT,
        [this](const GameEventData& data) {
            Ball* ball = data.ballHit.ball;
            int cx = (int)ball->getX() + ball->getDiameter() / 2;
            int cy = (int)ball->getY() - 10;
            int score = 1000 / ball->getDiameter();
            lsHitScores.push_back(
                std::make_unique<HitScore>(&hitScoreFontRenderer, cx, cy, score));
        });

    // Subscribe to hexa hit event to spawn floating score popups
    hexaHitScoreHandle = EVENT_MGR.subscribe(GameEventType::HEXA_HIT,
        [this](const GameEventData& data) {
            Hexa* hexa = data.hexaHit.hexa;
            int cx = (int)hexa->getX() + hexa->getWidth() / 2;
            int cy = (int)hexa->getY() - 10;
            int score = 1000 / hexa->getWidth();
            lsHitScores.push_back(
                std::make_unique<HitScore>(&hitScoreFontRenderer, cx, cy, score));
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
    hitScoreHandle = EventManager::ListenerHandle();
    hexaHitScoreHandle = EventManager::ListenerHandle();
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

    // Initialize hit-score popup font
    hitScoreFontRenderer.loadFont(&appGraph, "assets/fonts/maganize_18.fnt");

    // Initialize HUD timer font
    timeFont.loadFont(&appGraph, "assets/fonts/pixelgame_28.fnt");

    // Initialize HUD world-label font
    worldFont.loadFont(&appGraph, "assets/fonts/thickfont_stroke.fnt");
    worldFont.setScale(0.7f);

    // Load freeze countdown font (3-2-1 during last 3s of time freeze)
    freezeEffect.init(&appGraph);

    // Configure ball-info section
    TextSection& ballSection = textOverlay.getSection("ball-info");
    ballSection.setPosition(300, 20)
               .setLineHeight(8)
               .setAlpha(200);
    
    return 1;
}

void Scene::addBall(int x, int y, int size, int top, float dirX, int dirY, int id)
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
        y = std::rand() % 394 + 22; // Random Y in range [22, 415) - playable area
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
            
            // Use existing Ball::collision(Platform*) method
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

void Scene::addFloor(int x, int y, FloorType type, int color)
{
    lsFloor.push_back(std::make_unique<Floor>(this, x, y, type, color));
}

void Scene::addGlass(int x, int y, GlassType type, int color)
{
    lsFloor.push_back(std::make_unique<Glass>(this, x, y, type, color));
}

void Scene::addLadder(int x, int y, int numTiles)
{
    lsLadders.push_back(std::make_unique<Ladder>(this, x, y, numTiles));
}

void Scene::addHexa(int x, int y, int size, float velX, float velY, int color)
{
    lsHexas.push_back(std::make_unique<Hexa>(this, x, y, size, velX, velY, color));
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

Platform* Scene::findFloorUnderPlayer(Player* player) const
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

    Platform* bestFloor = nullptr;
    int closestY = Stage::MAX_Y + 1;

    for (const auto& floor : lsFloor)
    {
        if (floor->isDead() || floor->isInvisible()) continue;

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

    float groundY = (float)Stage::MAX_Y+1;  // Default to screen bottom
    //const char* groundSource = "MAX_Y";

    // Check floors
    Platform* floor = findFloorUnderPlayer(player);
    if (floor)
    {
        float floorY = (float)floor->getCollisionBox().y;
        if (floorY < groundY)
        {
            groundY = floorY;
            //groundSource = "floor";
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
            //groundSource = "ladder_top";
        }
    }

    //CollisionBox playerBox = player->getCollisionBox();
    //int playerFeetY = playerBox.y + playerBox.h;
    //LOG_TRACE("findGroundLevel: playerFeetY=%d, groundY=%.1f, source=%s",
    //          playerFeetY, groundY, groundSource);

    return groundY;
}

Platform* Scene::findSteppablePlatformInPath(Player* player) const
{
    if (!player || !player->isGrounded()) return nullptr;

    static constexpr int MAX_STEP_HEIGHT = 16;

    float playerFeetY = player->getY();
    // Use same narrow check as findFloorUnderPlayer so physics grounding also detects the platform
    int playerLeft  = (int)(player->getX() - SURFACE_CHECK_WIDTH / 2);
    int playerRight = (int)(player->getX() + SURFACE_CHECK_WIDTH / 2);

    for (const auto& floor : lsFloor)
    {
        if (floor->isDead() || floor->isInvisible()) continue;
        CollisionBox floorBox = floor->getCollisionBox();

        // Step height: distance from player's feet to platform top
        int floorTopY = floorBox.y;
        float stepHeight = playerFeetY - (float)floorTopY;
        if (stepHeight <= 0.0f || stepHeight > (float)MAX_STEP_HEIGHT) continue;

        // Player center must overlap the platform horizontally (same as findFloorUnderPlayer)
        int overlap = calculateHorizontalOverlap(
            playerLeft, playerRight,
            floorBox.x, floorBox.x + floorBox.w);
        if (overlap < 1) continue;

        return floor.get();
    }
    return nullptr;
}

Platform* Scene::findWallBlockingPlayer(Player* player) const
{
    if (!player) return nullptr;

    static constexpr int MAX_STEP_HEIGHT = 16;

    CollisionBox playerBox = player->getCollisionBox();
    float playerFeetY = player->getY();

    for (const auto& floor : lsFloor)
    {
        if (floor->isDead() || floor->isInvisible() || floor->isPassthrough()) continue;

        CollisionBox floorBox = floor->getCollisionBox();

        // Full AABB intersection required
        if (!intersects(playerBox, floorBox)) continue;

        // Exclude step-up candidates: when grounded and floor top is within step range below feet
        float stepHeight = playerFeetY - (float)floorBox.y;
        if (player->isGrounded() && stepHeight >= 0.0f && stepHeight <= (float)MAX_STEP_HEIGHT)
            continue;

        return floor.get();
    }
    return nullptr;
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

    case WeaponType::CLAW:
        return std::make_unique<ClawShot>(this, pl, type);

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

void Scene::queueQuickStageSwitch(int stageNumber)
{
    pendingQuickStage = stageNumber;
}

GameState* Scene::processQuickStageSwitch()
{
    if (pendingQuickStage <= 0)
        return nullptr;

    int targetStage = pendingQuickStage;
    pendingQuickStage = 0;

    // Safety guard: queueQuickStageSwitch() is public and can be called with any value,
    // so validate here even if callers (e.g. AppConsole) already checked the range.
    if (targetStage < 1 || targetStage > gameinf.getNumStages())
    {
        LOG_WARNING("Cannot switch stage: %d is out of range (1-%d)", targetStage, gameinf.getNumStages());
        return nullptr;
    }

    gameinf.getCurrentStage() = targetStage;
    return new Scene(&gameinf.getStages()[targetStage - 1]);
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
                    if (ball->deathPickupCount > 0 && !lsBalls.empty())
                        lsBalls.back()->setDeathPickups(ball->deathPickups, ball->deathPickupCount);

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
                    addFloor(obj.x, obj.y, floor->floorType, floor->floorColor);
                    if (!lsFloor.empty())
                    {
                        lsFloor.back()->setInvisible(floor->invisible);
                        lsFloor.back()->setPassthrough(floor->passthrough);
                    }

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
                addFloor(obj.x, obj.y, FloorType::HORIZ_BIG);
            }
            break;

        case StageObjectType::Glass:
            if (obj.params)
            {
                if (auto* glass = obj.getParams<GlassParams>())
                {
                    addGlass(obj.x, obj.y, glass->glassType, glass->glassColor);
                    if (!lsFloor.empty())
                    {
                        lsFloor.back()->setInvisible(glass->invisible);
                        lsFloor.back()->setPassthrough(glass->passthrough);
                        if (glass->hasDeathPickup)
                        {
                            if (auto* g = dynamic_cast<Glass*>(lsFloor.back().get()))
                                g->setDeathPickup(glass->deathPickupType);
                        }
                    }

                    // Fire STAGE_OBJECT_SPAWNED event
                    GameEventData event(GameEventType::STAGE_OBJECT_SPAWNED);
                    event.objectSpawned.id = static_cast<int>(obj.id);
                    event.objectSpawned.x = obj.x;
                    event.objectSpawned.y = obj.y;
                    EVENT_MGR.trigger(event);
                }
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

        case StageObjectType::Hexa:
            if (obj.params)
            {
                if (auto* hexa = obj.getParams<HexaParams>())
                {
                    addHexa(obj.x, obj.y, hexa->size, hexa->velX, hexa->velY, hexa->hexaColor);
                    if (hexa->deathPickupCount > 0 && !lsHexas.empty())
                        lsHexas.back()->setDeathPickups(hexa->deathPickups, hexa->deathPickupCount);

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

    // Add children to active list
    for (auto& b : pendingBalls)
        lsBalls.push_back(std::move(b));
    pendingBalls.clear();
}

void Scene::cleanupHexas()
{
    // Clean up dead hexas and create children
    for (auto it = lsHexas.begin(); it != lsHexas.end(); )
    {
        if ((*it)->isDead())
        {
            // Ask hexa to create its children
            auto children = (*it)->createChildren();

            if (children.first)
                pendingHexas.push_back(std::move(children.first));
            if (children.second)
                pendingHexas.push_back(std::move(children.second));

            it = lsHexas.erase(it);  // unique_ptr automatically deletes
        }
        else
        {
            ++it;
        }
    }

    // Add children to active list
    for (auto& h : pendingHexas)
        lsHexas.push_back(std::move(h));
    pendingHexas.clear();

    // Check win condition after all ball AND hexa removals
    if (lsBalls.empty() && pendingBalls.empty() &&
        lsHexas.empty() && pendingHexas.empty() && !stage->getItemsLeft())
    {
        // Only trigger win once - prevent calling every frame
        if (currentState != SceneState::LevelClear)
        {
            win();
        }
    }
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

    // Balls and hexas only move when NOT frozen, but flashing entities
    // still need to update their flash timer to complete the death sequence
    if (!freezeEffect.isActive())
    {
        for (const auto& ball : lsBalls)
        {
            ball->update(dt);
        }
        for (const auto& hexa : lsHexas)
        {
            hexa->update(dt);
        }
    }
    else
    {
        // During freeze: only update flashing entities (so they can die)
        for (const auto& ball : lsBalls)
        {
            if (ball->isInFlashState())
                ball->update(dt);
        }
        for (const auto& hexa : lsHexas)
        {
            if (hexa->isInFlashState())
                hexa->update(dt);
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

    for (const auto& hs : lsHitScores)
    {
        hs->update(dt);
    }
}

void Scene::spawnEffect(AnimSpriteSheet* tmpl, int x, int y, float scale)
{
    if (tmpl)
        lsEffects.push_back(std::make_unique<AnimEffect>(x, y, tmpl, scale));
}

void Scene::cleanupPhase()
{
    // === PHASE 2: Cleanup ===
    // Ball cleanup is special - creates children during cleanup
    cleanupBalls();

    // Hexa cleanup also creates children during cleanup
    cleanupHexas();

    // Standard cleanup for shots, floors, ladders, pickups, and effects (unique_ptr)
    cleanupDeadObjects(lsShoots);
    cleanupDeadObjects(lsFloor);
    cleanupDeadObjects(lsLadders);
    cleanupDeadObjects(lsPickups);
    cleanupDeadObjects(lsEffects);
    cleanupDeadObjects(lsHitScores);
}

void Scene::updateTimer(float dt)
{
    timeLine += dt;

    secondAccum += dt;
    if (secondAccum < 1.0f)
        return;

    // Run next lines only after a second is accumulated, to avoid doing time-based logic every frame
    secondAccum -= 1.0f;

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
            else if (player->isSteppingUp())
            {
                // Frozen during step-up animation (~100ms) - no input processed
                // Animation will auto-transition to WALKING via setOnStateComplete callback
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
                    float savedX = player->getX();
                    player->moveLeft();
                    Platform* steppable = findSteppablePlatformInPath(player);
                    if (steppable)
                        player->startStepUp((float)steppable->getCollisionBox().y);
                    else if (findWallBlockingPlayer(player))
                        player->setX(savedX);
                }
                else if (appInput.key(gameinf.getKeys(i).getRight()))
                {
                    float savedX = player->getX();
                    player->moveRight();
                    Platform* steppable = findSteppablePlatformInPath(player);
                    if (steppable)
                        player->startStepUp((float)steppable->getCollisionBox().y);
                    else if (findWallBlockingPlayer(player))
                        player->setX(savedX);
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
        lsHexas,
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

    if (GameState* quickSwitch = processQuickStageSwitch())
    {
        return quickSwitch;
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
    lsHexas.clear();
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

    Sprite* spr = b->getCurrentSprite();
    if (!spr) return;

    RenderProps props;
    props.x = (int)b->getX();
    props.y = (int)b->getY();

    appGraph.drawExFlash(spr, props, b->isInFlashState());
}

void Scene::draw(Hexa* h)
{
    if (!freezeEffect.areBallsVisible()) return;  // Hidden during freeze-warning blink

    Sprite* spr = h->getCurrentSprite();
    if (!spr) return;

    RenderProps props;
    props.x = (int)h->getX();
    props.y = (int)h->getY();

    appGraph.drawExFlash(spr, props, h->isInFlashState());
}

void Scene::draw(Player* pl)
{
    pl->draw(&appGraph);
}

void Scene::draw(Platform* pl)
{
    if (pl->isInvisible()) return;
    Sprite* spr = pl->getCurrentSprite();
    if (spr)
        appGraph.draw(spr, pl->getX(), pl->getY());
}

void Scene::drawScore()
{
    StageResources& res = gameinf.getStageRes();
    constexpr int MAX_LIVES_ICONS = 3;
    constexpr int LIVES_ICON_SPACING = 26;
    constexpr int LIVES_ICON_WIDTH = 16;
    constexpr int WEAPON_ICON_GAP = 8;

    if (gameinf.getPlayer(AppData::PLAYER1)->isPlaying())
    {
        Player* p1 = gameinf.getPlayer(AppData::PLAYER1);
        int lives1 = p1->getLives();
        int livesBaseX1 = 80;

        appGraph.draw(&fontNum[1], p1->getScore(), livesBaseX1, RES_Y - 55);
        appGraph.draw(&res.miniplayer[AppData::PLAYER1], 20, Stage::MAX_Y + 7);

        // Draw up to 3 lives icons
        int iconsToShow1 = (lives1 > MAX_LIVES_ICONS) ? MAX_LIVES_ICONS : lives1;
        for (int i = 0; i < iconsToShow1; i++)
        {
            appGraph.draw(&res.lives[AppData::PLAYER1], livesBaseX1 + LIVES_ICON_SPACING * i, Stage::MAX_Y + 30);
        }

        // Calculate X position after lives icons
        int afterLivesX1 = livesBaseX1 + LIVES_ICON_SPACING * iconsToShow1;

        // If more than 3 lives, show "xN" text
        if (lives1 > MAX_LIVES_ICONS)
        {
            char livesText[8];
            std::snprintf(livesText, sizeof(livesText), "x%d", lives1);
            appGraph.text(livesText, afterLivesX1, Stage::MAX_Y + 35);
            afterLivesX1 += 24;  // Space for "xN" text
        }

        // Draw weapon slot holder aligned to miniplayer bottom
        int miniBottom1 = Stage::MAX_Y + 7 + res.miniplayer[AppData::PLAYER1].getHeight();
        int holderY1 = miniBottom1 - res.itemHolder.getHeight();
        int weaponX1 = livesBaseX1 + LIVES_ICON_SPACING * MAX_LIVES_ICONS + WEAPON_ICON_GAP;
        appGraph.draw(&res.itemHolder, weaponX1 - 5, holderY1);
        Sprite* weaponIcon = nullptr;
        if (p1->getWeapon() == WeaponType::GUN)
            weaponIcon = &res.pickupSprites[0];
        else if (p1->getWeapon() == WeaponType::HARPOON && p1->getMaxShoots() == 2)
            weaponIcon = &res.pickupSprites[1];  // Double shoot
        else if (p1->getWeapon() == WeaponType::CLAW)
            weaponIcon = &res.pickupSprites[5];
        if (weaponIcon)
            appGraph.draw(weaponIcon, weaponX1, holderY1 + 5);
    }

    if (gameinf.getPlayer(AppData::PLAYER2))
        if (gameinf.getPlayer(AppData::PLAYER2)->isPlaying())
        {
            Player* p2 = gameinf.getPlayer(AppData::PLAYER2);
            int lives2 = p2->getLives();
            int livesBaseX2 = 460;

            appGraph.draw(&res.miniplayer[AppData::PLAYER2], 400, Stage::MAX_Y + 7);
            appGraph.draw(&fontNum[1], p2->getScore(), livesBaseX2, RES_Y - 55);

            // Draw up to 3 lives icons
            int iconsToShow2 = (lives2 > MAX_LIVES_ICONS) ? MAX_LIVES_ICONS : lives2;
            for (int i = 0; i < iconsToShow2; i++)
            {
                appGraph.draw(&res.lives[AppData::PLAYER2], livesBaseX2 + LIVES_ICON_SPACING * i, Stage::MAX_Y + 30);
            }

            // Calculate X position after lives icons
            int afterLivesX2 = livesBaseX2 + LIVES_ICON_SPACING * iconsToShow2;

            // If more than 3 lives, show "xN" text
            if (lives2 > MAX_LIVES_ICONS)
            {
                char livesText[8];
                std::snprintf(livesText, sizeof(livesText), "x%d", lives2);
                appGraph.text(livesText, afterLivesX2, Stage::MAX_Y + 35);
                afterLivesX2 += 24;  // Space for "xN" text
            }

            // Draw weapon slot holder aligned to miniplayer bottom
            int miniBottom2 = Stage::MAX_Y + 7 + res.miniplayer[AppData::PLAYER2].getHeight();
            int holderY2 = miniBottom2 - res.itemHolder.getHeight();
            int weaponX2 = livesBaseX2 + LIVES_ICON_SPACING * MAX_LIVES_ICONS + WEAPON_ICON_GAP;
            appGraph.draw(&res.itemHolder, weaponX2 - 5, holderY2);
            Sprite* weaponIcon2 = nullptr;
            if (p2->getWeapon() == WeaponType::GUN)
                weaponIcon2 = &res.pickupSprites[0];
            else if (p2->getWeapon() == WeaponType::HARPOON && p2->getMaxShoots() == 2)
                weaponIcon2 = &res.pickupSprites[1];  // Double shoot
            else if (p2->getWeapon() == WeaponType::CLAW)
                weaponIcon2 = &res.pickupSprites[5];
            if (weaponIcon2)
                appGraph.draw(weaponIcon2, weaponX2, holderY2 + 5);
        }
}

void Scene::drawMark(Graph& graph, StageResources& res)
{
    for (int j = 0; j < 640; j += 16)
    {
        graph.draw(&res.mark[2], j, 0);
        graph.draw(&res.mark[1], j, Stage::MAX_Y + 1);
        graph.draw(&res.mark[0], j, Stage::MAX_Y + 17);
        graph.draw(&res.mark[0], j, Stage::MAX_Y + 33);
        graph.draw(&res.mark[2], j, Stage::MAX_Y + 49);
    }

    for (int j = 0; j < 415; j += 16)
    {
        graph.draw(&res.mark[4], 0, j);
        graph.draw(&res.mark[3], Stage::MAX_X + 1, j);
    }

    graph.draw(&res.mark[0], 0, 0);
    graph.draw(&res.mark[0], Stage::MAX_X + 1, 0);
    graph.draw(&res.mark[0], 0, Stage::MAX_Y + 1);
    graph.draw(&res.mark[0], Stage::MAX_X + 1, Stage::MAX_Y + 1);
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

    // Draw ball collision circles (actual collision shape, not bounding box)
    for (const auto& b : lsBalls)
    {
        int cx, cy;
        b->getCollisionCenter(cx, cy);
        int radius = b->getCollisionRadius();
        appGraph.circle(cx, cy, radius);
        snprintf(buf, sizeof(buf), "%d,%d", cx, cy);
        appGraph.text(buf, cx - radius, cy - radius);
    }

    // Draw hexa collision circles (green to distinguish from balls)
    appGraph.setDrawColor(0, 200, 0, 255);
    for (const auto& h : lsHexas)
    {
        int cx, cy;
        h->getCollisionCenter(cx, cy);
        int radius = h->getCollisionRadius();
        appGraph.circle(cx, cy, radius);
        snprintf(buf, sizeof(buf), "%d,%d", cx, cy);
        appGraph.text(buf, cx - radius, cy - radius);
    }
    appGraph.setDrawColor(0, 0, 0, 255);  // Reset to black

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
    textOverlay.addTextF("Objects: Balls=%d Hexas=%d Shoots=%d Floors=%d Ladders=%d Pickups=%d",
            (int)lsBalls.size(),
            (int)lsHexas.size(),
            (int)lsShoots.size(),
            (int)lsFloor.size(),
            (int)lsLadders.size(),
            (int)lsPickups.size());

    textOverlay.addTextF("Stage: %d  Time=%d  Timeline=%.1f",
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

    // Draw hexas
    for (const auto& hexa : lsHexas)
    {
        draw(hexa.get());
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

    // Draw floating score popups above everything
    for (const auto& hs : lsHitScores)
    {
        hs->draw(&appGraph);
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

    Scene::drawMark(appGraph, res);
    drawScore();

    // Fixed column positions for the center HUD block
    constexpr int TIME_LABEL_X  = 235;  // X for TIME image (left edge)
    constexpr int TIME_VALUE_X  = 378;  // X for green timer number (right edge)
    constexpr int WORLD_LABEL_X = 255;  // X for WORLD text (left edge)
    constexpr int WORLD_ID_X    = 378;  // X for stage ID text (right edge)
    constexpr int ROW1_Y = Stage::MAX_Y + 10;
    constexpr int ROW2_Y = Stage::MAX_Y + 38;

    // Row 1: TIME label | timer value
    appGraph.draw(&res.time, TIME_LABEL_X, ROW1_Y);
    {
        char timeBuf[16];
        std::snprintf(timeBuf, sizeof(timeBuf), "%d", timeRemaining);
        timeFont.text(timeBuf, TIME_VALUE_X, ROW1_Y-9, TextAlign::Right);
    }

    // Row 2: WORLD label | stage display ID (honey yellow, thickfont_stroke)
    const std::string& dispId = stage->displayId.empty()
        ? std::to_string(stage->id) : stage->displayId;
    worldFont.setColor(255, 195, 30, 255);
    worldFont.text("WORLD", WORLD_LABEL_X, ROW2_Y, TextAlign::Left);
    worldFont.setColor(255, 255, 255, 255);
    worldFont.text(dispId.c_str(), WORLD_ID_X, ROW2_Y, TextAlign::Right);
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
            appGraph.draw(&fontNum[FONT_HUGE], gameOverCountdown, 320, 300, ALIGN_CENTER);
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
        int y = (415 - res.ready.getHeight()) / 2;
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
