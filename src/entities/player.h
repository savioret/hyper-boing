#pragma once

#include "../game/weapontype.h"
#include "eventmanager.h"
#include <memory>
#include "../core/gameobject.h"
#include "../core/sprite2d.h"
#include "../core/animcontroller.h"
#include "../core/spritesheet.h"

/**
 * Collision box in top-left coordinate space for AABB collision detection.
 * Used by Ball and other entities to check collision with Player.
 */
struct CollisionBox {
    int x, y, w, h;
};

class Scene;
class Action;
class Graph;

/**
 * Player facing direction
 */
enum class FacingDirection
{
    RIGHT,  // Player is facing right
    LEFT    // Player is facing left
};

/**
 * Player animation states
 */
enum class PlayerState
{
    IDLE,      // Standing still (default pose)
    WALKING,   // Walking animation (from Aseprite JSON)
    SHOOTING,  // Shooting pose
    VICTORY,   // Victory celebration (from Aseprite JSON)
    DEAD       // Death animation (uses Action system)
};

/**
 * Player class
 *
 * Player, as its name suggests, contains information about the sprite
 * identifying the player, movement and action keys, score, lives,
 * weapons, and internal management data.
 *
 * Inherits from both IGameObject (for lifecycle management) and Sprite2D
 * (for rendering). Position is delegated to Sprite2D as the single source of truth.
 */
class Player : public IGameObject, public Sprite2D
{
private:
    int xDir, yDir; // used for death animation (legacy, may be removed)
    FacingDirection facing;  // Direction player is facing
    int lives;
    int score;
    int id;  // 0 = player 1, 1 = player 2
    int idWeapon;  // Kept for backward compatibility, not used
    WeaponType currentWeapon;
    int maxShoots;
    int numShoots;
    int shotCounter;
    int shotInterval; // time between shots
    int animSpeed;
    int animCounter;
    int moveIncrement; // displacement increment when walking
    bool playing;
    int immuneCounter; // for when just revived

    // Current animation state
    PlayerState currentState;

    // Animation controller for state-based animations
    std::unique_ptr<StateMachineAnim> animController;

    // Walk animation (loaded from Aseprite JSON)
    std::unique_ptr<SpriteSheet> walkSheet;
    std::unique_ptr<IAnimController> walkAnim;

    // Victory animation (loaded from Aseprite JSON)
    std::unique_ptr<SpriteSheet> victorySheet;
    std::unique_ptr<IAnimController> victoryAnim;

    // Death animation using Action system
    std::unique_ptr<Action> deathAction;
    EventManager::ListenerHandle playerDiedHandle;
    EventManager::ListenerHandle levelClearHandle;
    EventManager::ListenerHandle stageLoadedHandle;

public:
    Player(int id);
    ~Player();

    // IGameObject position delegation to Sprite2D (single source of truth)
    void setPos(float x, float y) override { Sprite2D::setPos(x, y); }
    void setX(float x) override { Sprite2D::setX(x); }
    void setY(float y) override { Sprite2D::setY(y); }
    float getX() const override { return Sprite2D::getX(); }
    float getY() const override { return Sprite2D::getY(); }

    // IGameObject lifecycle (isDead() is inherited, kill() has Player-specific logic)
    void kill() override;
    void onDeath() override;

    // Rendering
    void draw(Graph* graph);

    void init();
    // void setFrame(int frame); // Inherited from Sprite2D
    void update(float dt);
    void addScore(int num);
    void moveLeft();
    void moveRight();
    void stop();

    bool canShoot() const;
    void shoot();
    void looseShoot();
    void revive();

    // Event handlers
    void onPlayerHit();
    void onLevelClear();
    void onStageLoaded();

    // Weapon management
    WeaponType getWeapon() const { return currentWeapon; }
    void setWeapon(WeaponType type);

    // Getters
    // getX, getY are overridden (delegate to Sprite2D)
    // getWidth, getHeight, getFrame are inherited from Sprite2D
    // isDead is inherited from IGameObject
    int getId() const { return id; }
    int getScore() const { return score; }
    int getLives() const { return lives; }
    bool isPlaying() const { return playing; }
    bool isImmune() const { return immuneCounter > 0; }
    int getNumShoots() const { return numShoots; }
    int getIdWeapon() const { return idWeapon; }
    Sprite* getSprite() const { return getActiveSprite(); } // Returns actual rendered sprite
    FacingDirection getFacing() const { return facing; }

    /**
     * Get the actual sprite being rendered (accounts for walk/victory modes).
     * Used for collision detection and bounding boxes.
     */
    Sprite* getActiveSprite() const;

    /**
     * Get collision box in top-left coordinate space for AABB collision detection.
     * Player position is stored in bottom-middle coordinates, but collision
     * detection uses top-left AABB. This method handles the conversion.
     */
    CollisionBox getCollisionBox() const;

    /**
     * Change player animation state with proper transitions
     */
    void setState(PlayerState newState);

    /**
     * Get current animation state
     */
    PlayerState getState() const { return currentState; }

    // Setters
    // setX, setY are inherited
    void setPlaying(bool p) { playing = p; }
    
    // For access during refactoring
    friend class Scene;
    friend class Shoot;
    friend class Ball;
};