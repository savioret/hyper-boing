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

    // Only initialize Ready state if there's no stage clear animation in progress
    // (Stage clear animation will transition to Ready when curtain opens)
    if (!pStageClear)
    {
        setState(SceneState::Ready);
    }

    gameinf.getPlayer(PLAYER1)->setX((float)stage->xpos[PLAYER1]);
    if (gameinf.getPlayer(PLAYER2))
        gameinf.getPlayer(PLAYER2)->setX((float)stage->xpos[PLAYER2]);

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
                case WeaponType::HARPOON2:
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
}

void Scene::unsubscribeFromEvents()
{
    // Event handles automatically unsubscribe when destroyed,
    // but we can explicitly clear them here for clarity
    timeWarningHandle = EventManager::ListenerHandle();
    ballHitHandle = EventManager::ListenerHandle();
    playerShootHandle = EventManager::ListenerHandle();
    playerHitHandle = EventManager::ListenerHandle();
}

/**
 * Loads scene-specific bitmaps.
 * Shared resources (balls, UI, fonts) are referenced from AppData.
 * Only stage-specific resources (background, weapons) are loaded here.
 */
int Scene::initBitmaps()
{
    char txt[MAX_PATH];

    // Load weapon-specific sprites (scene-specific for now)
    // HARPOON sprites
    bmp.weapons.harpoonHead.init(&appGraph, "assets/graph/entities/weapon1.png");
    bmp.weapons.harpoonTail1.init(&appGraph, "assets/graph/entities/weapon2.png");
    bmp.weapons.harpoonTail2.init(&appGraph, "assets/graph/entities/weapon3.png");
    appGraph.setColorKey(bmp.weapons.harpoonHead.getBmp(), 0x00FF00);
    appGraph.setColorKey(bmp.weapons.harpoonTail1.getBmp(), 0x00FF00);
    appGraph.setColorKey(bmp.weapons.harpoonTail2.getBmp(), 0x00FF00);

    // GUN sprite sheet with 7 frames
    bmp.weapons.gunBullet.init(&appGraph, "assets/graph/entities/gun_bullet.png");
    // Frame data from gun_bullet.png specification
    bmp.weapons.gunBullet.addFrame(4, 1, 4, 8, -2, 0);      // Frame 0
    bmp.weapons.gunBullet.addFrame(16, 1, 8, 8, -4, 0);     // Frame 1
    bmp.weapons.gunBullet.addFrame(32, 1, 12, 8, -6, 0);    // Frame 2
    bmp.weapons.gunBullet.addFrame(52, 2, 16, 7, -8, 0);     // Frame 3
    bmp.weapons.gunBullet.addFrame(76, 0, 14, 9, -7, 0);   // Frame 4
    bmp.weapons.gunBullet.addFrame(98, 4, 10, 5, -5, -1);  // Frame 5 (impact)
    bmp.weapons.gunBullet.addFrame(116, 4, 14, 5, -7, -1); // Frame 6 (impact)
    appGraph.setColorKey(bmp.weapons.gunBullet.getTexture(), 0x00FF00);

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

void Scene::addItem(int x, int y, int id)
{
    lsItems.push_back(std::make_unique<Item>(x, y, id));
}

void Scene::addFloor(int x, int y, int id)
{
    lsFloor.push_back(std::make_unique<Floor>(this, x, y, id));
}

/**
 * Factory method to create appropriate Shot subclass based on weapon type
 * @param pl Player firing the shot
 * @param type Weapon type
 * @param xOffset Horizontal offset for multi-projectile weapons
 * @return unique_ptr to created Shot object
 */
std::unique_ptr<Shot> Scene::createShot(Player* pl, WeaponType type, int xOffset)
{
    switch (type)
    {
    case WeaponType::HARPOON:
    case WeaponType::HARPOON2:
        return std::make_unique<HarpoonShot>(this, pl, type, 0);

    case WeaponType::GUN:
        return std::make_unique<GunShot>(this, pl, &bmp.weapons.gunBullet, 0);

    default:
        // Fallback to harpoon for unknown types
        return std::make_unique<HarpoonShot>(this, pl, WeaponType::HARPOON, xOffset);
    }
}

void Scene::shoot(Player* pl)
{
    if (!pl->canShoot()) return;

    const WeaponConfig& config = WeaponConfig::get(pl->getWeapon());

    // Create multiple projectiles based on weapon config
    int projectileCount = config.projectileCount;
    int spacing = config.projectileSpacing;

    for (int i = 0; i < projectileCount; i++)
    {
        // Calculate horizontal offset for each projectile
        // Center them around the player
        int xOffset = 0;
        if (projectileCount > 1)
        {
            xOffset = (int)((i - (projectileCount - 1) / 2.0f) * spacing);
        }

        lsShoots.push_back(createShot(pl, pl->getWeapon(), xOffset));
    }

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
    CloseMusic();
    OpenMusic("assets/music/win.ogg");
    PlayMusic();
    setState(SceneState::LevelClear);

    // Fire LEVEL_CLEAR event
    GameEventData event(GameEventType::LEVEL_CLEAR);
    event.levelClear.stageId = stage->id;
    EVENT_MGR.trigger(event);

    pStageClear = std::make_unique<StageClear>(this);
}

void Scene::skipToStage(int stageNumber)
{
    AppData& appData = AppData::instance();
    
    // Validate stage number (1-indexed)
    if (stageNumber < 1 || stageNumber > appData.numStages)
    {
        return;
    }
    
    // Update current stage for display in StageClear
    appData.currentStage = stageNumber;
    
    // Trigger win sequence with target stage specified
    CloseMusic();
    OpenMusic("assets/music/win.ogg");
    PlayMusic();
    setState(SceneState::LevelClear);

    // Create StageClear with target stage number
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
        case OBJ_BALL:
            if (obj.params)
            {
                if (auto* ball = obj.getParams<BallParams>())
                {
                    addBall(obj.x, obj.y, ball->size, ball->top, ball->dirX, ball->dirY, ball->ballType);

                    // Fire STAGE_OBJECT_SPAWNED event
                    GameEventData event(GameEventType::STAGE_OBJECT_SPAWNED);
                    event.objectSpawned.id = obj.id; 
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
            
        case OBJ_FLOOR:
            if (obj.params)
            {
                if (auto* floor = obj.getParams<FloorParams>())
                {
                    addFloor(obj.x, obj.y, floor->floorType);

                    // Fire STAGE_OBJECT_SPAWNED event
                    GameEventData event(GameEventType::STAGE_OBJECT_SPAWNED);
                    event.objectSpawned.id = obj.id; 
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

        case OBJ_ACTION:
            if (obj.params)
            {
                if (auto* action = obj.getParams<ActionParams>())
                {
                    LOG_DEBUG("Executing stage action: /%s", action->command.c_str());
                    CONSOLE.executeCommand(action->command);
                }
            }
            break;
        }
    } while (obj.id != OBJ_NULL);
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
    if (lsBalls.empty() && pendingBalls.empty() && !stage->itemsleft)
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
    for (const auto& ball : lsBalls)
    {
        ball->update(dt);
    }

    for (const auto& shot : lsShoots)
    {
        shot->update(dt);
    }

    for (const auto& floor : lsFloor)
    {
        floor->update(dt);
    }
}

void Scene::cleanupPhase()
{
    // === PHASE 2: Cleanup ===
    // Ball cleanup is special - creates children during cleanup
    cleanupBalls();

    // Standard cleanup for shots and floors (unique_ptr)
    cleanupDeadObjects(lsShoots);
    cleanupDeadObjects(lsFloor);
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

            gameinf.player[PLAYER1]->setPlaying(false);
            if (gameinf.player[PLAYER2])
                gameinf.player[PLAYER2]->setPlaying(false);

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
                    gameinf.getPlayer(PLAYER1)->init();
                    if (gameinf.getPlayer(PLAYER2))
                        gameinf.getPlayer(PLAYER2)->init();
                    gameinf.initStages();
                    return new Scene(stage);
                }
                else  // GameOverSubState::Definitive
                {
                    // No continue - return to menu
                    LOG_INFO("Game Over: Player returning to menu");
                    gameinf.player[PLAYER1].reset();
                    gameinf.player[PLAYER2].reset();
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
        if (gameinf.getPlayer(i) &&
            !gameinf.getPlayer(i)->isDead() &&
            gameinf.getPlayer(i)->isPlaying())
        {
            if (appInput.key(gameinf.getKeys(i).getShoot())) { shoot(gameinf.getPlayer(i)); }
            else if (appInput.key(gameinf.getKeys(i).getLeft())) gameinf.getPlayer(i)->moveLeft();
            else if (appInput.key(gameinf.getKeys(i).getRight())) gameinf.getPlayer(i)->moveRight();
            else gameinf.getPlayer(i)->stop();
        }
    }

    // Always update players (even if dead, for death animation)
    for (i = 0; i < 2; i++)
    {
        if (gameinf.getPlayer(i))
        {
            gameinf.getPlayer(i)->update(dt);
        }
    }

    // Check game over condition
    if (gameinf.getPlayer(PLAYER1) && !gameinf.getPlayer(PLAYER2))
    {
        if (!gameinf.getPlayer(PLAYER1)->isPlaying())
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
    else if (gameinf.getPlayer(PLAYER1) && gameinf.getPlayer(PLAYER2))
    {
        if (!gameinf.getPlayer(PLAYER1)->isPlaying() && !gameinf.getPlayer(PLAYER2)->isPlaying())
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
        { gameinf.player[PLAYER1].get(), gameinf.player[PLAYER2].get() },
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
            gameinf.getPlayer(i)->update(dt);
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
                // Create new scene with ready screen disabled initially
                Scene* newScene = new Scene(&gameinf.getStages()[nextStageId - 1], std::move(pStageClear));
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

    // Release only scene-specific sprites (background, weapons)
    bmp.back.release();
    bmp.weapons.harpoonHead.release();
    bmp.weapons.harpoonTail1.release();
    bmp.weapons.harpoonTail2.release();
    // Note: SpriteSheet releases automatically in destructor

    // Shared resources (balls, floors, UI, fonts) are managed by AppData
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
    
    if (gameinf.getPlayer(PLAYER1)->isPlaying())
    {
        appGraph.draw(&fontNum[1], gameinf.getPlayer(PLAYER1)->getScore(), 80, RES_Y - 55);
        appGraph.draw(&res.miniplayer[PLAYER1], 20, MAX_Y + 7);
        for (int i = 0; i < gameinf.getPlayer(PLAYER1)->getLives(); i++)
        {
            appGraph.draw(&res.lives[PLAYER1], 80 + 26 * i, MAX_Y + 30);
        }
    }

    if (gameinf.getPlayer(PLAYER2))
        if (gameinf.getPlayer(PLAYER2)->isPlaying())
        {
            appGraph.draw(&res.miniplayer[PLAYER2], 400, MAX_Y + 7);
            appGraph.draw(&fontNum[1], gameinf.getPlayer(PLAYER2)->getScore(), 460, RES_Y - 55);
            for (int i = 0; i < gameinf.getPlayer(PLAYER2)->getLives(); i++)
            {
                appGraph.draw(&res.lives[PLAYER2], 460 + 26 * i, MAX_Y + 30);
            }
        }
}

void Scene::drawMark()
{
    StageResources& res = gameinf.getStageRes();
    
    for (int j = 0; j < 640; j += 16)
    {
        appGraph.draw(&res.mark[2], j, 0);
        appGraph.draw(&res.mark[1], j, MAX_Y + 1);
        appGraph.draw(&res.mark[0], j, MAX_Y + 17);
        appGraph.draw(&res.mark[0], j, MAX_Y + 33);
        appGraph.draw(&res.mark[2], j, MAX_Y + 49);
    }

    for (int j = 0; j < 416; j += 16)
    {
        appGraph.draw(&res.mark[4], 0, j);
        appGraph.draw(&res.mark[3], MAX_X + 1, j);
    }

    appGraph.draw(&res.mark[0], 0, 0);
    appGraph.draw(&res.mark[0], MAX_X + 1, 0);
    appGraph.draw(&res.mark[0], 0, MAX_Y + 1);
    appGraph.draw(&res.mark[0], MAX_X + 1, MAX_Y + 1);
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

    // Draw player bounding boxes using the same collision box as Ball::collision()
    for (int i = 0; i < 2; i++)
    {
        Player* pl = gameinf.getPlayer(i);
        if (pl && pl->isPlaying() && pl->isVisible())
        {
            // Use Player's getCollisionBox() to ensure debug matches actual collision
            CollisionBox box = pl->getCollisionBox();
            appGraph.rectangle(box.x, box.y, box.x + box.w, box.y + box.h);
        }
    }

    // Draw ball bounding boxes (circles represented as bounding squares)
    for (const auto& b : lsBalls)
    {
        int x = (int)b->getX();
        int y = (int)b->getY();
        int d = b->getDiameter();
        appGraph.rectangle(x, y, x + d, y + d);
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
                    int spriteX = xPos + sprite->getXOff();
                    int spriteY = yPos + sprite->getYOff();
                    int w = sprite->getWidth();
                    int h = sprite->getHeight();
                    appGraph.rectangle(spriteX, spriteY, spriteX + w, spriteY + h);
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
    if (appData.getPlayer(PLAYER1))
    {
		Player* p = appData.getPlayer(PLAYER1);
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

            textOverlay.addTextFS("ball-info", "Ball%d: x=%.0f y=%.0f sz=%d dia=%d dx=%d dy=%d",
                    ballCount,
                    ball->getX(),
                    ball->getY(),
                    ball->getSize(),
                    ball->getDiameter(),
                    ball->getDirX(),
                    ball->getDirY());
            ballCount++;
        }
    }

    // Add game state info to default section
    textOverlay.addTextF("Objects: Balls=%d Shoots=%d Floors=%d",
            (int)lsBalls.size(),
            (int)lsShoots.size(),
            (int)lsFloor.size());

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
}

void Scene::drawPlayers()
{
    if (gameinf.getPlayer(PLAYER1)->isVisible() && gameinf.getPlayer(PLAYER1)->isPlaying())
        draw(gameinf.getPlayer(PLAYER1));

    if (gameinf.getPlayer(PLAYER2))
        if (gameinf.getPlayer(PLAYER2)->isVisible() && gameinf.getPlayer(PLAYER2)->isPlaying())
            draw(gameinf.getPlayer(PLAYER2));
}

void Scene::drawHUD()
{
    StageResources& res = gameinf.getStageRes();

    drawMark();
    drawScore();
    appGraph.draw(&res.time, 320 - res.time.getWidth() / 2, MAX_Y + 3);
    appGraph.draw(&fontNum[FONT_BIG], timeRemaining, 300, MAX_Y + 25);
}

void Scene::drawStateOverlay()
{
    StageResources& res = gameinf.getStageRes();

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
