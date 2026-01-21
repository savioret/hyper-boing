#pragma once

class Scene;
class Sprite;

/**
 * Player class
 *
 * Player, as its name suggests, contains information about the sprite
 * identifying the player, movement and action keys, score, lives,
 * weapons, and internal management data.
 */
class Player
{
private:
    Sprite* sprite;
    float xPos, yPos;
    int sx, sy;
    int xDir, yDir; // used for death animation
    int lives;
    int score;
    int id;  // 0 = player 1, 1 = player 2
    int idWeapon;
    int maxShoots;
    int numShoots;
    int shotCounter;
    int shotInterval; // time between shots
    int animSpeed;
    int animCounter;
    int moveIncrement; // displacement increment when walking
    int frame;
    bool dead;
    bool playing;
    int immuneCounter; // for when just revived
    bool visible;

public:
    Player(int sx, int sy, int id);
    Player(int id);
    ~Player();

    void init();
    void setFrame(int frame);
    void update();
    void addScore(int num);
    void moveLeft();
    void moveRight();
    void stop();
    
    bool canShoot() const;
    bool isDead() const { return dead; }
    void shoot();
    void looseShoot();
    void kill();
    void revive();

    // Getters
    float getX() const { return xPos; }
    float getY() const { return yPos; }
    int getWidth() const { return sx; }
    int getHeight() const { return sy; }
    int getId() const { return id; }
    int getScore() const { return score; }
    int getLives() const { return lives; }
    bool isPlaying() const { return playing; }
    bool isVisible() const { return visible; }
    bool isImmune() const { return immuneCounter > 0; }
    int getNumShoots() const { return numShoots; }
    int getIdWeapon() const { return idWeapon; }
    int getFrame() const { return frame; }
    Sprite* getSprite() const { return sprite; }

    // Setters
    void setX(float x) { xPos = x; }
    void setY(float y) { yPos = y; }
    void setPlaying(bool p) { playing = p; }
    void setVisible(bool v) { visible = v; }
    
    // For access during refactoring
    friend class Scene;
    friend class Shoot;
    friend class Ball;
};