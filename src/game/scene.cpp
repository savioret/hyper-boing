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
      levelClear(false), pStageClear(std::move(pstgclr)), gameOver(false), gameOverCount(-2),
      stage(stg), change(0), dSecond(0), timeRemaining(0), timeLine(0),
      moveTick(0), moveLastTick(0), moveCount(0),
      drawTick(0), drawLastTick(0), drawCount(0),
      boundingBoxes(false),
      readyBlinkCount(0), readyBlinkTimer(0), readyVisible(true)
{
    gameinf.isMenu() = false;
    if (pStageClear) pStageClear->scene = this;
}

int Scene::init()
{
    GameState::init();

    char txt[MAX_PATH];

    change = 0;
    levelClear = false;
    gameOver = false;
    gameOverCount = -2;
    timeLine = 0;
    dSecond = 0;
    timeRemaining = stage->timelimit;
    currentState = SceneState::Ready;
    readyBlinkCount = 0;
    readyBlinkTimer = 0;
    readyVisible = true;

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

    // Subscribe to time warning event (uses once() for cleaner pattern)
    timeWarningHandle = EVENT_MGR.subscribe(GameEventType::TIME_SECOND_ELAPSED,
        [this](const GameEventData& data) {
            if (data.timeElapsed.newTime == 11 && once("last_seconds_warning")) {
                appAudio.playSound("assets/music/last_seconds.ogg");
            }
        });

    // Fire STAGE_LOADED event (stage is prepared and displayed)
    GameEventData loadedEvent(GameEventType::STAGE_LOADED);
    loadedEvent.stageLoaded.stageId = stage->id;
    EVENT_MGR.trigger(loadedEvent);

    return 1;
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
    
    Ball* ball = new Ball(this, x, y, size, dirX, dirY, top, id);
    lsBalls.push_back(ball);
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
        for (Floor* floor : lsFloor)
        {
            // Skip dead floors
            if (floor->isDead())
                continue;
            
            // Use existing Ball::collision(Floor*) method
            SDL_Point col = tempBall.collision(floor);
            
            // If any collision detected (x or y), position is invalid
            if (col.x != 0 || col.y != 0)
            {
                // Collision detected! Generate new random position
                validPosition = false;

                LOG_DEBUG("Cannot spawn ball at (x=%d, y=%d) => (floor x=%d, y=%d, w=%d, h=%d) ## COL(%d, %d)", 
                    x, y, floor->getX(), floor->getY(), floor->getWidth(), floor->getHeight(), col.x, col.y);
                
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
    Item* item = new Item(x, y, id);
    lsItems.push_back(item);
}

void Scene::addFloor(int x, int y, int id)
{
    Floor* floor = new Floor(this, x, y, id);
    lsFloor.push_back(floor);
}

void Scene::addShoot(Player* pl)
{
    Shot* shotInstance = createShot(pl, pl->getWeapon(), 0);
    lsShoots.push_back(shotInstance);
}

/**
 * Factory method to create appropriate Shot subclass based on weapon type
 * @param pl Player firing the shot
 * @param type Weapon type
 * @param xOffset Horizontal offset for multi-projectile weapons
 * @return Pointer to created Shot object
 */
Shot* Scene::createShot(Player* pl, WeaponType type, int xOffset)
{
    switch (type)
    {
    case WeaponType::HARPOON:
    case WeaponType::HARPOON2:
        return new HarpoonShot(this, pl, type, 0);

    case WeaponType::GUN:
        return new GunShot(this, pl, &bmp.weapons.gunBullet, 0);

    default:
        // Fallback to harpoon for unknown types
        return new HarpoonShot(this, pl, WeaponType::HARPOON, xOffset);
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

        Shot* shotInstance = createShot(pl, pl->getWeapon(), xOffset);
        lsShoots.push_back(shotInstance);
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
int Scene::divideBall(Ball* ball)
{
    Ball* ball1;
    Ball* ball2;
    int res = 1;

    if (ball->size < 3)
    {
        ball1 = new Ball(this, ball);
        ball2 = new Ball(this, ball);
        ball1->setDirX(-1);
        ball2->setDirX(1);

        lsBalls.push_back(ball1);
        lsBalls.push_back(ball2);

        res = 0;
    }
    else
    {
        if (lsBalls.size() == 1 && !stage->itemsleft)
        {
            win();
        }
    }

    lsBalls.remove(ball);
    delete ball;

    return res;
}

int Scene::objectScore(int id)
{
    return 1000 / id;
}

void Scene::win()
{
    CloseMusic();
    OpenMusic("assets/music/win.ogg");
    PlayMusic();
    levelClear = true;
    currentState = SceneState::LevelClear;  // Prevent player collisions during victory

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
    levelClear = true;
    
    // Create StageClear with target stage number
    pStageClear = std::make_unique<StageClear>(this, stageNumber);
}

void Scene::checkColisions()
{
    Ball* b;
    Shot* sh;
    Floor* fl;
    int i;
    SDL_Point col;

    for (Ball* ptball : lsBalls)
    {
        b = ptball;

        for (Shot* pt : lsShoots)
        {
            sh = pt;
            if (!b->isDead() && !sh->getPlayer()->isDead())
            {
                if (b->collision(sh))
                {
                    sh->onBallHit(b);  // Polymorphic call - handles weapon-specific behavior
                    sh->getPlayer()->addScore(objectScore(b->diameter));

                    // Fire BALL_HIT event
                    GameEventData event(GameEventType::BALL_HIT);
                    event.ballHit.ball = b;
                    event.ballHit.shot = sh;
                    event.ballHit.shooter = sh->getPlayer();
                    EVENT_MGR.trigger(event);

                    b->kill();
                }
            }
        }

        FloorColision flc[2];
        int cont = 0;
        int moved = 0;

        for (Floor* pt : lsFloor)
        {
            fl = pt;
            col = b->collision(fl);

            if (col.x)
            {
                if (cont && flc[0].floor == fl)
                {
                    b->setDirX(-b->dirX);
                    moved = 1;
                    break;
                }
                if (cont < 2)
                {
                    flc[cont].point.x = col.x;
                    flc[cont].floor = fl;
                    cont++;
                }
            }
            if (col.y)
            {
                if (cont && flc[0].floor == fl)
                {
                    b->setDirY(-b->dirY);
                    moved = 2;
                    break;
                }
                if (cont < 2)
                {
                    flc[cont].point.y = col.y;
                    flc[cont].floor = fl;
                    cont++;
                }
            }
        }
        if (cont == 1)
        {
            if (flc[0].point.x)
                b->setDirX(-b->dirX);
            else
                b->setDirY(-b->dirY);
        }
        else if (cont > 1)
        {
            decide(b, flc, moved);
        }

        // Only check player collisions if we are in Playing state
        if (currentState != SceneState::Playing)
            continue;

        for ( i = 0; i < 2; i++ )
        {
            if (gameinf.player[i])
                if (!gameinf.player[i]->isImmune() && !gameinf.player[i]->isDead())
                {
                    if (b->collision(gameinf.player[i].get()))
                    {
                        // Fire PLAYER_HIT event
                        GameEventData event(GameEventType::PLAYER_HIT);
                        event.playerHit.player = gameinf.player[i].get();
                        event.playerHit.ball = b;
                        EVENT_MGR.trigger(event);

                        gameinf.player[i]->kill();
                        // Frame is now set by PlayerDeadAction via event subscription
                    }
                }
        }
    }

    for (Shot* ptshoot : lsShoots)
    {
        sh = ptshoot;

        for (Floor* pt : lsFloor)
        {
            fl = pt;
            if (!sh->isDead())
                if (sh->collision(fl))
                {
                    sh->onFloorHit(fl);  // Polymorphic call - handles weapon-specific behavior
                }
        }
    }
}

void Scene::decide(Ball* b, FloorColision* fc, int moved)
{
    if (fc[0].floor->getId() == fc[1].floor->getId() || fc[0].point.y == fc[1].point.y)
    {
        if (fc[0].point.x)
            if (moved != 1) b->setDirX(-b->dirX);

        if (fc[0].point.y)
            if (moved != 2) b->setDirY(-b->dirY);
        return;
    }

    if (fc[0].floor->getId() == fc[1].floor->getId())
    {
        if (fc[0].floor->getId() == 0)
            if (moved != 2) b->setDirY(-b->dirY);
        if (fc[0].floor->getId() == 1)
            if (moved != 1) b->setDirX(-b->dirX);
    }
    else
    {
        if (fc[0].floor->getY() == fc[1].floor->getY())
            if (moved != 2) b->setDirY(-b->dirY);
            else
                if (moved != 1) b->setDirX(-b->dirX);
    }
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

void Scene::splitBall(Ball* ball)
{
    // Queue new smaller balls to be added after cleanup
    if (ball->getSize() < 3)
    {
        pendingBalls.push_back(new Ball(this, ball));
        pendingBalls.push_back(new Ball(this, ball));
        pendingBalls.back()->setDirX(1);
        pendingBalls[pendingBalls.size()-2]->setDirX(-1);
    }
    else if (lsBalls.size() == 1 && !stage->itemsleft)
    {
        win();
    }
}

void Scene::processBallDivisions()
{
    // Add queued balls after cleanup
    for (Ball* b : pendingBalls)
        lsBalls.push_back(b);
    pendingBalls.clear();
}

GameState* Scene::moveAll(float dt)
{
    Ball* ptb;
    Shot* pts;
    Floor* pfl;
    int i, res;

    moveTick = SDL_GetTicks();
    if (moveTick - moveLastTick > 1000)
    {
        fpsv = moveCount;
        moveCount = 0;
        moveLastTick = moveTick;
    }
    else
        moveCount++;

    if (goback)
    {
        goback = false;
        gameinf.isMenu() = true;
        return new Menu;
    }

    // Handle ready screen blinking sequence
    if (currentState == SceneState::Ready)
    {
        if ( !readyBlinkCount && !readyBlinkTimer )
        {
            // Process time=0 objects now that ready screen is shown
            checkSequence();
            LOG_DEBUG("Ready shown, processing time=0 stage objects.");
        }

        readyBlinkTimer++;

        // 200ms per blink at ~60fps = 12 frames per toggle
        if (readyBlinkTimer >= 12)
        {
            readyBlinkTimer = 0;
            readyVisible = !readyVisible;

            // Count a blink when going from visible to invisible
            if (!readyVisible)
            {
                readyBlinkCount++;

                // After 6 complete blinks, transition to Playing state
                if (readyBlinkCount >= 6)
                {
                    currentState = SceneState::Playing;

                    // Fire STAGE_STARTED event (countdown complete, gameplay begins)
                    GameEventData startedEvent(GameEventType::STAGE_STARTED);
                    startedEvent.stageStarted.stageId = stage->id;
                    EVENT_MGR.trigger(startedEvent);
                }
            }
        }

        // During ready screen, skip all game logic but allow exit
        return nullptr;
    }

    if (gameOver)
    {
        for (i = 0; i < 2; i++)
        {
            if (gameinf.getPlayer(i))
            {
                if (appInput.key(gameinf.getKeys()[i].shoot))
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
    }

    if (!levelClear)
    {
        for (i = 0; i < 2; i++)
        {
            if (gameinf.getPlayer(i))
            {
                if (!gameinf.getPlayer(i)->isDead() && gameinf.getPlayer(i)->isPlaying())
                {
                    if (appInput.key(gameinf.getKeys()[i].shoot)) { shoot(gameinf.getPlayer(i)); }
                    else if (appInput.key(gameinf.getKeys()[i].left)) gameinf.getPlayer(i)->moveLeft();
                    else if (appInput.key(gameinf.getKeys()[i].right)) gameinf.getPlayer(i)->moveRight();
                    else gameinf.getPlayer(i)->stop();
                }
                gameinf.getPlayer(i)->update(dt);
            }
        }
        if (gameOverCount == -2)
        {
            if (gameinf.getPlayer(PLAYER1) && !gameinf.getPlayer(PLAYER2))
            {
                if (!gameinf.getPlayer(PLAYER1)->isPlaying())
                {
                    gameOver = true;
                    gameOverCount = 10;
                    currentState = SceneState::GameOver;  // Prevent player collisions during game over
                    gameOverSubState = GameOverSubState::ContinueCountdown;  // Allow continue

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
                    gameOver = true;
                    gameOverCount = 10;
                    currentState = SceneState::GameOver;  // Prevent player collisions during game over
                    gameOverSubState = GameOverSubState::ContinueCountdown;  // Allow continue

                    // Fire GAME_OVER event (reason: both players dead)
                    GameEventData event(GameEventType::GAME_OVER);
                    event.gameOver.reason = 1;  // Both players dead
                    EVENT_MGR.trigger(event);

                    CloseMusic();
                    OpenMusic("assets/music/gameover.ogg");
                    PlayMusic();
                }
            }
        }
    }
    else
    {
        // During level clear, still update players for victory animation
        for (i = 0; i < 2; i++)
        {
            if (gameinf.getPlayer(i))
            {
                if (gameinf.getPlayer(i)->isPlaying())
                {
                    gameinf.getPlayer(i)->update(dt);
                    // Ensure win frame is set (in case not using victory animation)
                    if (!gameinf.getPlayer(i)->isDead())
                    {
                        // Only override frame if not using custom animations
                        // Victory animation will override this automatically
                    }
                }
            }
        }
    }

    checkColisions();

    // === PHASE 1: Movement ===
    // Update all objects without modifying lists
    for (Ball* pt : lsBalls)
    {
        ptb = pt;
        ptb->update(dt);
    }

    for (Shot* pt : lsShoots)
    {
        pts = pt;
        pts->update(dt);
    }

    for (Floor* pt : lsFloor)
    {
        pfl = pt;
        pfl->update(dt);
    }

    // === PHASE 2: Cleanup ===
    cleanupDeadObjects(lsBalls);  // Now using standard pattern!
    cleanupDeadObjects(lsShoots);
    cleanupDeadObjects(lsFloor);
    
    // Add new balls created from splits
    processBallDivisions();

    if (dSecond < 60) dSecond++;
    else
    {
        dSecond = 0;
        if (timeRemaining > 0)
        {
            if (!pStageClear && !gameOver)
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
        else
        {
            if (timeRemaining == 0)
            {
                gameOver = true;
                gameOverCount = 10;
                currentState = SceneState::GameOver;  // Prevent player collisions during game over
                gameOverSubState = GameOverSubState::ContinueCountdown;  // Allow continue

                // Fire GAME_OVER event (reason: time expired)
                GameEventData event(GameEventType::GAME_OVER);
                event.gameOver.reason = 2;  // Time expired
                EVENT_MGR.trigger(event);

                gameinf.player[PLAYER1]->setPlaying(false);
                if (gameinf.player[PLAYER2])
                    gameinf.player[PLAYER2]->setPlaying(false);
                timeRemaining = -1;
            }
        }

        timeLine++;
        if (gameOver)
        {
            if (gameOverCount >= 0)
            {
                gameOverCount--;
                // Transition to Definitive when countdown expires
                if (gameOverCount < 0 && gameOverSubState == GameOverSubState::ContinueCountdown)
                {
                    gameOverSubState = GameOverSubState::Definitive;
                    LOG_INFO("Game Over: Countdown expired, transitioning to Definitive");
                }
            }
        }
    }

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
            currentState = SceneState::Ready;
            readyBlinkCount = 0;
            readyBlinkTimer = 0;
            readyVisible = true;
        }
    }
    else if (currentState == SceneState::Playing) // Only check sequence during playing state
    {
        checkSequence();
    }

    return nullptr;
}

int Scene::release()
{
    // Clean up entities
    for (Ball* ball : lsBalls)
        delete ball;
    lsBalls.clear();
    
    for (Shot* shot : lsShoots)
        delete shot;
    lsShoots.clear();
    
    for (Floor* floor : lsFloor)
        delete floor;
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
    for (Ball* b : lsBalls)
    {
        int x = (int)b->getX();
        int y = (int)b->getY();
        int d = b->getDiameter();
        appGraph.rectangle(x, y, x + d, y + d);
    }

    // Draw shot collision areas
    // Different visualization for Harpoon vs Gun
    for (Shot* s : lsShoots)
    {
        if (!s->isDead())
        {
            int xPos = (int)s->getX();
            int yPos = (int)s->getY();
            int yInit = (int)s->getYInit();

            // Check if this is a GunShot
            GunShot* gunShot = dynamic_cast<GunShot*>(s);

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
    for (Floor* f : lsFloor)
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
        for (Ball* ball : lsBalls)
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

    textOverlay.addTextF("GameOver=%s  LevelClear=%s",
            gameOver ? "YES" : "NO",
            levelClear ? "YES" : "NO");
}

int Scene::drawAll()
{
    Ball* ptb;
    Shot* pts;
    Floor* pfl;
    StageResources& res = gameinf.getStageRes();

    drawBackground();

    for (Floor* pt : lsFloor)
    {
        pfl = pt;
        draw(pfl);
    }

    // Draw shots using polymorphic draw() method
    for (Shot* pt : lsShoots)
    {
        pts = pt;
        pts->draw(&appGraph);  // Polymorphic call
    }

    drawMark();
    drawScore();
    appGraph.draw(&res.time, 320 - res.time.getWidth() / 2, MAX_Y + 3);
    appGraph.draw(&fontNum[FONT_BIG], timeRemaining, 300, MAX_Y + 25);

    if (gameinf.getPlayer(PLAYER1)->isVisible() && gameinf.getPlayer(PLAYER1)->isPlaying())
        draw(gameinf.getPlayer(PLAYER1));

    if (gameinf.getPlayer(PLAYER2))
        if (gameinf.getPlayer(PLAYER2)->isVisible() && gameinf.getPlayer(PLAYER2)->isPlaying())
            draw(gameinf.getPlayer(PLAYER2));

    for (Ball* pt : lsBalls)
    {
        ptb = pt;
        draw(ptb);
    }

    if (gameOver)
    {
        appGraph.draw(&res.gameover, 100, 125);
        if (gameOverSubState == GameOverSubState::ContinueCountdown)
        {
            // Show continue prompt and countdown
            appGraph.draw(&res.continu, 130, 200);
            appGraph.draw(&fontNum[FONT_HUGE], gameOverCount, 315, 300);
        }
        // GameOverSubState::Definitive shows only "GAME OVER" text, no continue
    }

    if (pStageClear) pStageClear->drawAll();

    // Draw ready screen if in Ready state and visible (blinking)
    if (currentState == SceneState::Ready && readyVisible)
    {
        // Center the ready sprite on screen
        int x = (640 - res.ready.getWidth()) / 2;
        int y = (416 - res.ready.getHeight()) / 2;
        appGraph.draw(&res.ready, x, y);
    }

    // Draw bounding boxes if debug mode is enabled
    if (boundingBoxes)
    {
        drawBoundingBoxes();
    }

    // Finalize render (debug overlay, console, flip)
    finalizeRender();

    drawTick = SDL_GetTicks();
    if (drawTick - drawLastTick > 1000)
    {
        fps = drawCount;
        drawCount = 0;
        drawLastTick = drawTick;
    }
    else
        drawCount++;

    return 1;
}
