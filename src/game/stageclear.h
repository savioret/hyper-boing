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
 * @enum LevelClearSubState
 * @brief Substates for the stage completion sequence
 *
 * The level clear animation progresses through these states:
 * TextSlideIn -> ScoreCounting -> WaitingForInput -> CurtainClosing -> TextSlideOut -> CurtainOpening
 */
enum class LevelClearSubState
{
    TextSlideIn,      ///< "LEVEL COMPLETED" text sliding into screen position
    ScoreCounting,    ///< Score incrementing (can skip with fire button)
    WaitingForInput,  ///< Score finished, waiting for player to press fire button
    CurtainClosing,   ///< Red brick curtain closing from top/bottom
    TextSlideOut,     ///< Text sliding out of screen
    CurtainOpening    ///< Red brick curtain opening (transitions to Ready state)
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

    // Animation positions
    int xt1, yt1;      ///< Position of first title text
    int xt2, yt2;      ///< Position of second title text
    int xnum, ynum;    ///< Position of stage number
    int yr1, yr2;      ///< Position of red brick curtains (top/bottom)

    // Score display
    int cscore[2];     ///< Current displayed score for each player (increments during counting)

    // State machine
    LevelClearSubState currentSubState;  ///< Current substate of the level clear sequence

    int targetStage;  ///< For console-triggered level skips (0 = normal progression)

    /**
     * @brief Set the current substate and log transition
     */
    void setSubState(LevelClearSubState newState);

public:
    StageClear(Scene* scn, int targetStageNum = 0);
    ~StageClear();

    void drawAll();
    int moveAll();
    int init();
    int release();

    int getTargetStage() const { return targetStage; }
    LevelClearSubState getSubState() const { return currentSubState; }

    // Friend class to allow Scene to access private members if needed
    friend class Scene;
};