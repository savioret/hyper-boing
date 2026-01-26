#include "menutitle.h"
#include "../core/graph.h"
#include "../main.h" 

MenuTitle::MenuTitle(Graph* gr)
    : graph(gr)
{
    // Initialize default position
    xPos = 0.0f;
    yPos = 0.0f;
}

MenuTitle::~MenuTitle()
{
    r_title_boing.release();
    r_title_hyper.release();
    r_title_bg.release();
    r_title_bg_redball.release();
    r_title_bg_greenball.release();
    r_title_bg_blueball.release();
}

void MenuTitle::init()
{
    if (!graph) return;

    // Load resource sprites
    r_title_boing.init(graph, "assets/graph/ui/title_boing.png", 0, 0);
    r_title_hyper.init(graph, "assets/graph/ui/title_hyper.png", 0, 0);
    r_title_bg.init(graph, "assets/graph/ui/title_bg.png", 0, 0);

    r_title_bg_redball.init(graph, "assets/graph/ui/title_bg_redball.png", 0, 0);
    r_title_bg_greenball.init(graph, "assets/graph/ui/title_bg_greenball.png", 0, 0);
    r_title_bg_blueball.init(graph, "assets/graph/ui/title_bg_blueball.png", 0, 0);

    // Link actors to resources
    title_boing.addSprite(&r_title_boing);
    title_hyper.addSprite(&r_title_hyper);
    title_bg.addSprite(&r_title_bg);
    title_bg_redball.addSprite(&r_title_bg_redball);
    title_bg_greenball.addSprite(&r_title_bg_greenball);
    title_bg_blueball.addSprite(&r_title_bg_blueball);

    // Set initial sprite states (off-screen)
    title_boing.setY(-300.0f);
    title_hyper.setX(-400.0f);
    title_bg.setAlpha(0.0f);

    // Build and start animation
    buildAnimation();
    if (animation)
    {
        animation->start();
    }
}

void MenuTitle::buildAnimation()
{
    const int titleY = 5;
    const int centerX = (RES_X - title_hyper.getWidth()) / 2;
    const int bgCenterX = (RES_X - title_bg.getWidth()) / 2;
    const int bgCenterY = titleY + 60;

    // Calculate ball explosion center and final positions
    const float ballCenterX = bgCenterX + title_bg.getWidth() / 2.0f;
    const float ballCenterY = bgCenterY + title_bg.getHeight() / 2.0f;

    const float redBallFinalX = bgCenterX - 10.0f;
    const float redBallFinalY = bgCenterY + 20.0f;
    const float blueBallFinalX = bgCenterX + 230.0f;
    const float blueBallFinalY = bgCenterY + 120.0f;
    const float greenBallFinalX = bgCenterX + 350.0f;
    const float greenBallFinalY = bgCenterY + 5.0f;

    // Set sprite positions for those not using animated x/y
    title_bg.setPos(centerX, titleY + 60);
    title_hyper.setY(titleY + 20);
    title_boing.setX(bgCenterX + 30);

    // Create animation sequence
    animation = std::make_unique<ActionSequence>();

    // Phase 1: "BOING" drops from top (0.5s, EaseOut)
    animation->then(tweenTo(title_boing.yPtr(), 85.0f, 0.5f, Easing::EaseOut));

    // Phase 2: "HYPER" slides from left (0.5s, EaseOut)
    animation->then(tweenTo(title_hyper.xPtr(), (float)centerX - 30.0f, 0.5f, Easing::EaseOut));

    // Phase 3: Background fades in (0.3s, EaseOut)
    animation->then(tweenTo(title_bg.alphaPtr(), 255.0f, 0.3f, Easing::EaseOut));

    // Phase 4: Balls explode from center (0.4s, EaseOut, parallel)
    // Set initial positions at center
    title_bg_redball.setPos(ballCenterX, ballCenterY);
    title_bg_redball.setAlpha(255.0f);
    title_bg_redball.setScale(0.0f);
    title_bg_greenball.setPos(ballCenterX, ballCenterY);
    title_bg_greenball.setAlpha(255.0f);
    title_bg_greenball.setScale(0.0f);
    title_bg_blueball.setPos(ballCenterX, ballCenterY);
    title_bg_blueball.setAlpha(255.0f);
    title_bg_blueball.setScale(0.0f);

    // Animate balls to final positions in parallel
    auto ballExplosion = std::make_unique<ActionParallel>();
    ballExplosion->add(tween2D(title_bg_redball.xPtr(), title_bg_redball.yPtr(),
                               redBallFinalX, redBallFinalY, 0.4f, Easing::EaseOut));
    ballExplosion->add(tween2D(title_bg_greenball.xPtr(), title_bg_greenball.yPtr(),
                               greenBallFinalX, greenBallFinalY, 0.4f, Easing::EaseOut));
    ballExplosion->add(tween2D(title_bg_blueball.xPtr(), title_bg_blueball.yPtr(),
                               blueBallFinalX, blueBallFinalY, 0.4f, Easing::EaseOut));

    // Add scale animations (0 -> 1.0) in parallel with position
    ballExplosion->add(tweenTo(title_bg_redball.scalePtr(), 1.0f, 0.4f, Easing::EaseOut));
    ballExplosion->add(tweenTo(title_bg_greenball.scalePtr(), 1.0f, 0.4f, Easing::EaseOut));
    ballExplosion->add(tweenTo(title_bg_blueball.scalePtr(), 1.0f, 0.4f, Easing::EaseOut));

    animation->then(std::move(ballExplosion));
}

void MenuTitle::update(float dt)
{
    if (animation)
    {
        animation->update(dt);
    }
}

void MenuTitle::draw()
{
    if (!graph) return;

    // Render logic using Sprite2D properties
    const int titleY = 5;
    const int centerX = (RES_X - title_bg.getWidth()) / 2;
    
    // Ensure static elements have correct positions if they aren't animated
    // Note: Assuming setPos updates the actor state which is persisted
    title_bg.setPos((float)centerX, (float)(titleY + 60));
    
    // Background layer
    if (title_bg.getAlpha() > 0)
    {
        graph->draw(&title_bg);
    }

    // Colored balls
    if (title_bg_redball.getAlpha() > 0)
    {
        graph->draw(&title_bg_redball);
    }

    if (title_bg_blueball.getAlpha() > 0)
    {
        graph->draw(&title_bg_blueball);
    }

    if (title_bg_greenball.getAlpha() > 0)
    {
        graph->draw(&title_bg_greenball);
    }

    // "HYPER" text
    if (title_hyper.getX() >= -400)
    {
        graph->draw(&title_hyper);
    }

    // "BOING" text
    if (title_boing.getY() >= -300)
    {
        graph->draw(&title_boing);
    }
}

bool MenuTitle::isAnimationFinished() const
{
    return animation && animation->isFinished();
}
