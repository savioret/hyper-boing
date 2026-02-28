#pragma once

#include "../game/weapontype.h"
#include "../game/collisionsystem.h"
#include "eventmanager.h"
#include <memory>
#include <vector>
#include "../core/gameobject.h"
#include "../core/multianimsprite.h"
#include "../core/animspritesheet.h"
#include "../core/graph.h"

class Scene;
class Action;
class Ladder;
class Sprite;

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
    IDLE,       // Standing still (default pose)
    WALKING,    // Walking animation (from Aseprite JSON)
    CLIMBING,   // On ladder, moving up/down (climbing animation)
    WAKING_UP,  // Just reached ladder top, transitioning to platform
    SHOOTING,   // Shooting pose
    VICTORY,    // Victory celebration (from Aseprite JSON)
    DEAD        // Death animation (uses Action system)
};

/**
 * Player class
 *
 * Player, as its name suggests, contains information about the sprite
 * identifying the player, movement and action keys, score, lives,
 * weapons, and internal management data.
 *
 * Inherits from IGameObject for lifecycle management and position.
 * Uses composition for sprite management via MultiAnimSprite.
 */
class Player : public IGameObject
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

    // Pickup power-up states
    bool hasShield;   // Survives 1 hit when true

    // Render properties (from Sprite2D, now composed)
    RenderProps renderProps;
    bool visible = true; // Can be set to false for temporary invisibility (e.g., flashing when hit)

    // Current animation state
    PlayerState currentState;

    // Multi-animation sprite holder (manages walk, victory, climb animations + fallback)
    MultiAnimSprite sprite;

    // Walk animation (loaded from Aseprite JSON)
    std::unique_ptr<AnimSpriteSheet> walkAnim;

    // Victory animation (loaded from Aseprite JSON)
    std::unique_ptr<AnimSpriteSheet> victoryAnim;

    // Climbing animation (loaded from Aseprite JSON with StateMachineAnim)
    std::unique_ptr<AnimSpriteSheet> climbAnim;

    // Death animation using Action system
    std::unique_ptr<Action> deathAction;
    EventManager::ListenerHandle playerDiedHandle;
    EventManager::ListenerHandle levelClearHandle;
    EventManager::ListenerHandle stageLoadedHandle;

    // Ladder/climbing state
    Ladder* currentLadder;   // Ladder player is currently climbing (nullptr if not climbing)
    float climbSpeed;        // Climb speed in pixels per frame
    bool climbingMoving;     // True when player is actively moving up/down on ladder

    // Physics/gravity state
    float yVelocity;         // Current vertical velocity (positive = falling)
    bool grounded;           // Is player standing on something?
    static constexpr float GRAVITY = 0.3f;            // Gravity acceleration per frame
    static constexpr float MAX_FALL_SPEED = 7.0f;     // Terminal velocity
    static constexpr float GROUND_SNAP_TOLERANCE = 1.0f;  // Distance tolerance for snapping to ground

    // Fallback animation frame indices (used when Aseprite JSON is not available)
    static constexpr int ANIM_WALK = 0;
    static constexpr int ANIM_SHOOT = 5;
    static constexpr int ANIM_SHOOT_POSE = ANIM_SHOOT + 1;
    static constexpr int ANIM_WIN = 7;
    static constexpr int ANIM_DEAD = 8;

public:
    Player(int id);
    ~Player();

    // IGameObject lifecycle (isDead() is inherited, kill() has Player-specific logic)
    void kill() override;
    void onDeath() override;

    // Render property accessors
    void setFlipH(bool flip)
    {
        renderProps.flipH = flip;
    }

    bool getFlipH() const
    {
        return renderProps.flipH;
    }

    void setRotation(double rot)
    {
        renderProps.rotation = rot;
    }

    double getRotation() const
    {
        return renderProps.rotation;
    }

    // void setScale(float s) { renderProps.scale = s; }
    // float getScale() const { return renderProps.scale; }
    // void setAlpha(int a) { renderProps.alpha = a / 255.0f; }
    // int getAlpha() const { return (int)(renderProps.alpha * 255); }
    //void setVisible(bool v) { visible = v; }
    bool isVisible() const { return visible; }

    // Rendering
    void draw(Graph* graph);

    void init();
    // void setFrame(int frame); // Inherited from Sprite2D
    void update(float dt, Scene* scene);
    void updatePhysics(float dt, Scene* scene);
    void addScore(int num);
    void moveLeft();
    void moveRight();
    void stop();

    // Climbing methods
    void startClimbing(Ladder* ladder);
    void stopClimbing();
    void climbUp();
    void climbDown();
    bool isClimbing() const { return currentState == PlayerState::CLIMBING; }
    bool canEnterLadder(Ladder* ladder) const;
    Ladder* getCurrentLadder() const { return currentLadder; }
    float getClimbSpeed() const { return climbSpeed; }

    // Physics getters
    bool isGrounded() const { return grounded; }
    float getYVelocity() const { return yVelocity; }

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
    int getId() const { return id; }
    int getScore() const { return score; }
    int getLives() const { return lives; }
    bool isPlaying() const { return playing; }
    bool isImmune() const { return immuneCounter > 0; }
    int getNumShoots() const { return numShoots; }
    int getIdWeapon() const { return idWeapon; }
    //Sprite* getSprite() const { return getActiveSprite(); } // Returns actual rendered sprite
    FacingDirection getFacing() const { return facing; }
    int getWidth() const { return sprite.getWidth(); }
    int getHeight() const { return sprite.getHeight(); }
    int getFrame() const { return sprite.getFallbackFrame(); } // For debug display

    /**
     * Get the actual sprite being rendered (accounts for walk/victory modes).
     * Used for collision detection and bounding boxes.
     */
    AnimSpriteSheet* getActiveAnim() const;

    /**
     * Get collision box in top-left coordinate space for AABB collision detection.
     * Player position is stored in bottom-middle coordinates, but collision
     * detection uses top-left AABB. This method handles the conversion.
     */
    CollisionBox getCollisionBox() const override;

    /**
     * Change player animation state with proper transitions
     */
    void setState(PlayerState newState);

    /**
     * Get current animation state
     */
    PlayerState getState() const { return currentState; }

    // Setters
    void setPlaying(bool p) { playing = p; }
    void setLives(int l) { lives = l; }
    void setImmuneCounter(int counter) { immuneCounter = counter; }
    void setMaxShoots(int n) { maxShoots = n; }

    // Shield and power-up methods
    bool getShield() const { return hasShield; }
    void setShield(bool shield) { hasShield = shield; }

    // For access during refactoring
    friend class Scene;
    friend class Shoot;
    friend class Ball;
};