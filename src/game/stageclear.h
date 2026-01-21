#pragma once

class Scene;
class Sprite;

/**
 * StageClearBitmaps struct
 * Contains sprites for the stage clear sequence.
 */
struct StageClearBitmaps
{
    Sprite title1;
    Sprite title2;
    Sprite roof;
};

/**
 * StageClear class
 *
 * Class contained in Scene (the game module).
 * The object created in Scene is a pointer initialized to null that is 
 * instantiated when needed and destroyed in the same way.
 *
 * The reason for creating this class was to prevent the Scene code 
 * from becoming too dense and to make it more readable, as this 
 * sequence requires many counters and state variables.
 */
class StageClear
{
private:
    StageClearBitmaps bmp;
    Scene* scene;

    int xt1, yt1;
    int xt2, yt2;
    int xnum, ynum;

    int yr1, yr2;

    int cscore[2];

    bool endMove;
    bool endCount;
    bool endClose;
    bool endOpening;
    bool movingOut;
    bool isClosing;
    bool isOpening;
    bool finish;
    
    int targetStage;  // For console-triggered level skips (0 = normal progression)

public:
    StageClear(Scene* scn, int targetStageNum = 0);
    ~StageClear();

    void drawAll();
    int moveAll();
    int init();
    int release();
    
    int getTargetStage() const { return targetStage; }

    // Friend class to allow Scene to access private members if needed
    friend class Scene;
};