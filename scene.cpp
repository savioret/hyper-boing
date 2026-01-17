#include <cstdio>
#include <cstring>
#include <memory>
#include "pang.h"
#include "appdata.h"
#include "appconsole.h"
#include <SDL.h>
#include <SDL_render.h>
#include <SDL_image.h>

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

Scene::Scene(Stage* stg, std::unique_ptr<StageClear> pstgclr)
    : levelClear(false), pStageClear(std::move(pstgclr)), gameOver(false), gameOverCount(-2),
      stage(stg), change(0), dSecond(0), timeRemaining(0), timeLine(0),
      moveTick(0), moveLastTick(0), moveCount(0),
      drawTick(0), drawLastTick(0), drawCount(0)
{
    gameinf.isMenu() = false;
    if (pStageClear) pStageClear->scene = this;
}

int Scene::init()
{
    GameState::init();

    char cadena[MAX_PATH];

    change = 0;
    levelClear = false;
    gameOver = false;
    gameOverCount = -2;
    timeLine = 0;
    dSecond = 0;
    timeRemaining = stage->timelimit;

    gameinf.getPlayers()[PLAYER1]->setX((float)stage->xpos[PLAYER1]);
    if (gameinf.getPlayers()[PLAYER2])
        gameinf.getPlayers()[PLAYER2]->setX((float)stage->xpos[PLAYER2]);

    CloseMusic();

    initBitmaps();

    std::snprintf(cadena, sizeof(cadena), "music/%s", stage->music);
    OpenMusic(cadena);
    PlayMusic();
    return 1;
}

/**
 * Loads all the necessary bitmaps for the game scene.
 * This includes balls, background, weapons, and UI elements.
 */
int Scene::initBitmaps()
{
    int i = 0;
    char cadena[MAX_PATH];

    int offs[10] = { 0, 22, 44, 71, 93, 120, 148, 171, 198, 221 };
    int offs1[10] = { 0, 13, 18, 31, 44, 58, 70, 82, 93, 105 };
    int offs2[10] = { 0, 49, 86, 134, 187, 233, 277, 327, 374, 421 };

    bmp.redball[0].init(&appGraph, "graph/ball-rd1.png");
    bmp.redball[1].init(&appGraph, "graph/ball-rd2.png");
    bmp.redball[2].init(&appGraph, "graph/ball-rd3.png");
    bmp.redball[3].init(&appGraph, "graph/ball-rd4.png");
    for (i = 0; i < 4; i++)
        appGraph.setColorKey(bmp.redball[i].getBmp(), 0x00FF00);

    bmp.miniplayer[PLAYER1].init(&appGraph, "graph/miniplayer1.png");
    bmp.miniplayer[PLAYER2].init(&appGraph, "graph/miniplayer2.png");
    appGraph.setColorKey(bmp.miniplayer[PLAYER1].getBmp(), 0x00FF00);
    appGraph.setColorKey(bmp.miniplayer[PLAYER2].getBmp(), 0x00FF00);

    bmp.lives[PLAYER1].init(&appGraph, "graph/lives1p.png");
    bmp.lives[PLAYER2].init(&appGraph, "graph/lives2p.png");
    appGraph.setColorKey(bmp.lives[PLAYER1].getBmp(), 0x00FF00);
    appGraph.setColorKey(bmp.lives[PLAYER2].getBmp(), 0x00FF00);

    bmp.shoot[0].init(&appGraph, "graph/weapon1.png");
    bmp.shoot[1].init(&appGraph, "graph/weapon2.png");
    bmp.shoot[2].init(&appGraph, "graph/weapon3.png");
    for (i = 0; i < 3; i++)
        appGraph.setColorKey(bmp.shoot[i].getBmp(), 0x00FF00);

    bmp.mark[0].init(&appGraph, "graph/ladrill1.png");
    bmp.mark[1].init(&appGraph, "graph/ladrill1u.png");
    bmp.mark[2].init(&appGraph, "graph/ladrill1d.png");
    bmp.mark[3].init(&appGraph, "graph/ladrill1l.png");
    bmp.mark[4].init(&appGraph, "graph/ladrill1r.png");

    for (i = 0; i < 5; i++)
        appGraph.setColorKey(bmp.mark[i].getBmp(), 0x00FF00);

    bmp.floor[0].init(&appGraph, "graph/floor1.png");
    appGraph.setColorKey(bmp.floor[0].getBmp(), 0x00FF00);
    bmp.floor[1].init(&appGraph, "graph/floor2.png");
    appGraph.setColorKey(bmp.floor[1].getBmp(), 0x00FF00);

    bmp.time.init(&appGraph, "graph/tiempo.png");
    appGraph.setColorKey(bmp.time.getBmp(), 0xFF0000);

    bmp.gameover.init(&appGraph, "graph/gameover.png", 16, 16);
    //appGraph.setColorKey(bmp.gameover.getBmp(), 0x00FF00);

    bmp.continu.init(&appGraph, "graph/continue.png", 16, 16);
    //appGraph.setColorKey(bmp.continu.getBmp(), 0x00FF00);

    std::snprintf(cadena, sizeof(cadena), "graph/%s", stage->back);
    bmp.back.init(&appGraph, cadena, 16, 16);
    appGraph.setColorKey(bmp.back.getBmp(), 0x00FF00);

    bmp.fontnum[0].init(&appGraph, "graph/fontnum1.png", 0, 0);
    appGraph.setColorKey(bmp.fontnum[0].getBmp(), 0xFF0000);
    fontNum[0].init(&bmp.fontnum[0]);
    fontNum[0].setValues(offs);
    bmp.fontnum[1].init(&appGraph, "graph/fontnum2.png", 0, 0);
    appGraph.setColorKey(bmp.fontnum[1].getBmp(), 0xFF0000);
    fontNum[1].init(&bmp.fontnum[1]);
    fontNum[1].setValues(offs1);
    bmp.fontnum[2].init(&appGraph, "graph/fontnum3.png", 0, 0);
    appGraph.setColorKey(bmp.fontnum[2].getBmp(), 0x00FF00);
    fontNum[2].init(&bmp.fontnum[2]);
    fontNum[2].setValues(offs2);

    return 1;
}

void Scene::addBall(int x, int y, int size, int top, int dirX, int dirY, int id)
{
    Ball* ball = new Ball(this, x, y, size, dirX, dirY, top, id);
    lsBalls.insert((MListData*)ball);
}

void Scene::addItem(int x, int y, int id)
{
    Item* item = new Item(x, y, id);
    lsItems.insert((MListData*)item);
}

void Scene::addFloor(int x, int y, int id)
{
    Floor* floor = new Floor(this, x, y, id);
    lsFloor.insert((MListData*)floor);
}

void Scene::addShoot(Player* pl)
{
    Shoot* shootInstance = new Shoot(this, pl);
    lsShoots.insert((MListData*)shootInstance);
}

void Scene::shoot(Player* pl)
{
    if (pl->canShoot())
    {
        pl->shoot();
        addShoot(pl);
    }
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

        lsBalls.insert((MListData*)ball1);
        lsBalls.insert((MListData*)ball2);

        res = 0;
    }
    else
    {
        if (lsBalls.getDimension() == 1 && !stage->itemsleft)
        {
            win();
        }
    }

    lsBalls.deleteNode(lsBalls.find(ball));

    return res;
}

int Scene::objectScore(int id)
{
    return 1000 / id;
}

void Scene::win()
{
    CloseMusic();
    OpenMusic("music/win.ogg");
    PlayMusic();
    levelClear = true;

    pStageClear = std::make_unique<StageClear>(this);
}

void Scene::checkColisions()
{
    MListNode* pt;
    MListNode* ptball = lsBalls.getFirstNode();
    MListNode* ptshoot;
    Ball* b;
    Shoot* sh;
    Floor* fl;
    int i;
    SDL_Point col;

    while (ptball)
    {
        b = (Ball*)ptball->data;

        pt = lsShoots.getFirstNode();
        while (pt)
        {
            sh = (Shoot*)pt->data;
            if (!b->hitStatus && !sh->getPlayer()->isDead())
                if (b->collision(sh))
                {
                    sh->kill();
                    sh->getPlayer()->looseShoot();
                    sh->getPlayer()->addScore(objectScore(b->diameter));
                    b->kill();
                }
            pt = lsShoots.getNextNode(pt);
        }

        FloorColision flc[2];
        int cont = 0;
        int moved = 0;

        pt = lsFloor.getFirstNode();
        while (pt)
        {
            fl = (Floor*)pt->data;
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
            pt = lsFloor.getNextNode(pt);
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

        for (i = 0; i < 2; i++)
        {
            if (gameinf.player[i])
                if (!gameinf.player[i]->isImmune() && !gameinf.player[i]->isDead())
                {
                    if (b->collision(gameinf.player[i].get()))
                    {
                        gameinf.player[i]->kill();
                        gameinf.player[i]->setFrame(ANIM_DEAD);
                    }
                }
        }

        ptball = lsBalls.getNextNode(ptball);
    }

    ptshoot = lsShoots.getFirstNode();
    while (ptshoot)
    {
        sh = (Shoot*)ptshoot->data;

        pt = lsFloor.getFirstNode();
        while (pt)
        {
            fl = (Floor*)pt->data;
            if (!sh->isDead())
                if (sh->collision(fl))
                {
                    sh->kill();
                    sh->getPlayer()->looseShoot();
                }
            pt = lsFloor.getNextNode(pt);
        }

        ptshoot = lsShoots.getNextNode(ptshoot);
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
            if (obj.extra.use)
                addBall(obj.x, obj.y, obj.extra.ex1, obj.extra.ex2, obj.extra.ex3, obj.extra.ex4, obj.extra.ex5);
            else
                addBall(obj.x, obj.y);
            break;
        case OBJ_FLOOR:
            addFloor(obj.x, obj.y, obj.extra.ex1);
            break;
        }
    } while (obj.id != OBJ_NULL);
}

void* Scene::moveAll()
{
    MListNode* ptprev, * pt = lsBalls.getFirstNode();
    Ball* ptb;
    Shoot* pts;
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

    if (gameOver)
    {
        for (i = 0; i < 2; i++)
        {
            if (gameinf.getPlayers()[i])
            {
                if (appInput.key(gameinf.getKeys()[i].shoot))
                {
                    if (gameOverCount >= 0)
                    {
                        gameinf.getPlayers()[PLAYER1]->init();
                        if (gameinf.getPlayers()[PLAYER2])
                            gameinf.getPlayers()[PLAYER2]->init();
                        gameinf.initStages();
                        return new Scene(stage);
                    }
                    else
                    {
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
            if (gameinf.getPlayers()[i])
            {
                if (!gameinf.getPlayers()[i]->isDead() && gameinf.getPlayers()[i]->isPlaying())
                {
                    if (appInput.key(gameinf.getKeys()[i].shoot)) { shoot(gameinf.getPlayers()[i]); }
                    else if (appInput.key(gameinf.getKeys()[i].left)) gameinf.getPlayers()[i]->moveLeft();
                    else if (appInput.key(gameinf.getKeys()[i].right)) gameinf.getPlayers()[i]->moveRight();
                    else gameinf.getPlayers()[i]->stop();
                }
                gameinf.getPlayers()[i]->update();
            }
        }
        if (gameOverCount == -2)
        {
            if (gameinf.getPlayers()[PLAYER1] && !gameinf.getPlayers()[PLAYER2])
            {
                if (!gameinf.getPlayers()[PLAYER1]->isPlaying())
                {
                    gameOver = true;
                    gameOverCount = 10;
                    CloseMusic();
                    OpenMusic("music/gameover.ogg");
                    PlayMusic();
                }
            }
            else if (gameinf.getPlayers()[PLAYER1] && gameinf.getPlayers()[PLAYER2])
            {
                if (!gameinf.getPlayers()[PLAYER1]->isPlaying() && !gameinf.getPlayers()[PLAYER2]->isPlaying())
                {
                    gameOver = true;
                    gameOverCount = 10;
                    CloseMusic();
                    OpenMusic("music/gameover.ogg");
                    PlayMusic();
                }
            }
        }
    }
    else
    {
        for (i = 0; i < 2; i++)
            if (gameinf.getPlayers()[i])
                if (gameinf.getPlayers()[i]->isPlaying())
                    gameinf.getPlayers()[i]->setFrame(ANIM_WIN);
    }

    checkColisions();

    while (pt)
    {
        ptb = (Ball*)pt->data;
        ptb->move();
        pt = lsBalls.getNextNode(pt);
        if (ptb->isHit()) divideBall(ptb);
    }

    pt = lsShoots.getFirstNode();
    while (pt)
    {
        pts = (Shoot*)pt->data;
        pts->move();
        ptprev = pt;
        pt = lsShoots.getNextNode(pt);
        if (pts->isDead())
            lsShoots.deleteNode(ptprev);
    }

    pt = lsFloor.getFirstNode();
    while (pt)
    {
        pfl = (Floor*)pt->data;
        pfl->update();
        pt = lsFloor.getNextNode(pt);
    }

    if (dSecond < 60) dSecond++;
    else
    {
        dSecond = 0;
        if (timeRemaining > 0)
        {
            if (!pStageClear && !gameOver) timeRemaining--;
        }
        else
        {
            if (timeRemaining == 0)
            {
                gameOver = true;
                gameOverCount = 10;
                gameinf.player[PLAYER1]->setPlaying(false);
                if (gameinf.player[PLAYER2])
                    gameinf.player[PLAYER2]->setPlaying(false);
                timeRemaining = -1;
            }
        }

        timeLine++;
        if (gameOver)
        {
            if (gameOverCount >= 0) gameOverCount--;
        }
    }

    if (pStageClear)
    {
        res = pStageClear->moveAll();
        if (res == -1)
        {
            if (stage->id < gameinf.getNumStages())
            {
                gameinf.getCurrentStage() = stage->id + 1;
                return new Scene(&gameinf.getStages()[stage->id], std::move(pStageClear));
            }
            else
                return new Menu();
        }
        if (res == 0)
        {
            pStageClear.reset();
        }
    }
    else checkSequence();

    return nullptr;
}

int Scene::release()
{
    lsBalls.release();
    lsShoots.release();
    lsFloor.release();

    bmp.back.release();
    bmp.floor[0].release();
    bmp.floor[1].release();
    bmp.fontnum[0].release();
    bmp.fontnum[1].release();
    bmp.miniplayer[PLAYER1].release();
    bmp.miniplayer[PLAYER2].release();
    bmp.lives[PLAYER1].release();
    bmp.lives[PLAYER2].release();
    bmp.time.release();
    bmp.gameover.release();
    bmp.continu.release();

    for (int i = 0; i < 5; i++)
        bmp.mark[i].release();
    for (int i = 0; i < 4; i++)
        bmp.redball[i].release();
    for (int i = 0; i < 3; i++)
        bmp.shoot[i].release();

    CloseMusic();

    return 1;
}

void Scene::drawBackground()
{
    appGraph.draw(&bmp.back, 0, 0);
}

void Scene::draw(Ball* b)
{
    appGraph.draw(b->getSprite(), (int)b->getX(), (int)b->getY());
}

void Scene::draw(Player* pl)
{
    appGraph.draw(pl->getSprite(), (int)pl->getX(), (int)pl->getY());
}

void Scene::draw(Shoot* sht)
{
    appGraph.draw(sht->getSprite(0), (int)sht->getX(), (int)sht->getY());

    for (int i = (int)sht->getY() + sht->getSprite(0)->getHeight(); i < MAX_Y; i += sht->getSprite(1)->getHeight())
        appGraph.draw(sht->getSprite(1 + sht->getTail()), (int)sht->getX(), i);
}

void Scene::draw(Floor* fl)
{
    appGraph.draw(&bmp.floor[fl->getId()], fl->getX(), fl->getY());
}

void Scene::drawScore()
{
    if (gameinf.getPlayers()[PLAYER1]->isPlaying())
    {
        appGraph.draw(&fontNum[1], gameinf.getPlayers()[PLAYER1]->getScore(), 80, RES_Y - 55);
        appGraph.draw(&bmp.miniplayer[PLAYER1], 20, MAX_Y + 7);
        for (int i = 0; i < gameinf.getPlayers()[PLAYER1]->getLives(); i++)
        {
            appGraph.draw(&bmp.lives[PLAYER1], 80 + 26 * i, MAX_Y + 30);
        }
    }

    if (gameinf.getPlayers()[PLAYER2])
        if (gameinf.getPlayers()[PLAYER2]->isPlaying())
        {
            appGraph.draw(&bmp.miniplayer[PLAYER2], 400, MAX_Y + 7);
            appGraph.draw(&fontNum[1], gameinf.getPlayers()[PLAYER2]->getScore(), 460, RES_Y - 55);
            for (int i = 0; i < gameinf.getPlayers()[PLAYER2]->getLives(); i++)
            {
                appGraph.draw(&bmp.lives[PLAYER2], 460 + 26 * i, MAX_Y + 30);
            }
        }
}

void Scene::drawMark()
{
    for (int j = 0; j < 640; j += 16)
    {
        appGraph.draw(&bmp.mark[2], j, 0);
        appGraph.draw(&bmp.mark[1], j, MAX_Y + 1);
        appGraph.draw(&bmp.mark[0], j, MAX_Y + 17);
        appGraph.draw(&bmp.mark[0], j, MAX_Y + 33);
        appGraph.draw(&bmp.mark[2], j, MAX_Y + 49);
    }

    for (int j = 0; j < 416; j += 16)
    {
        appGraph.draw(&bmp.mark[4], 0, j);
        appGraph.draw(&bmp.mark[3], MAX_X + 1, j);
    }

    appGraph.draw(&bmp.mark[0], 0, 0);
    appGraph.draw(&bmp.mark[0], MAX_X + 1, 0);
    appGraph.draw(&bmp.mark[0], 0, MAX_Y + 1);
    appGraph.draw(&bmp.mark[0], MAX_X + 1, MAX_Y + 1);
}

void Scene::drawDebugOverlay()
{
    AppData& appData = AppData::instance();
    
    if (!appData.debugMode) return;
    GameState::drawDebugOverlay();

    char cadena[256];
    int y = 80;
    int lineHeight = 20;
    
    if (appData.getPlayers()[PLAYER1])
    {
        std::sprintf(cadena, "P1: Score=%d Lives=%d Shoots=%d", 
                appData.getPlayers()[PLAYER1]->getScore(),
                appData.getPlayers()[PLAYER1]->getLives(),
                appData.getPlayers()[PLAYER1]->getNumShoots());
        appData.graph.text(cadena, 20, y);
        y += lineHeight;
    }
    
    MListNode* pt = lsBalls.getFirstNode();
    if (pt)
    {
        Ball* ptb = (Ball*)pt->data;
        std::sprintf(cadena, "Ball: y=%.1f size=%d diameter=%d", 
                ptb->getY(), ptb->getSize(), ptb->getDiameter());
        appData.graph.text(cadena, 20, y);
        y += lineHeight;
    }
    
    std::sprintf(cadena, "Objects: Balls=%d Shoots=%d Floors=%d", 
            lsBalls.getDimension(),
            lsShoots.getDimension(),
            lsFloor.getDimension());
    appData.graph.text(cadena, 20, y);
    y += lineHeight;
    
    std::sprintf(cadena, "Stage: %d  Time=%d  Timeline=%d", 
            stage->id, timeRemaining, timeLine);
    appData.graph.text(cadena, 20, y);
    y += lineHeight;
    
    std::sprintf(cadena, "GameOver=%s  LevelClear=%s", 
            gameOver ? "YES" : "NO",
            levelClear ? "YES" : "NO");
    appData.graph.text(cadena, 20, y);
}

int Scene::drawAll()
{
    MListNode* pt = lsBalls.getFirstNode();
    Ball* ptb;
    Shoot* pts;
    Floor* pfl;

    drawBackground();

    pt = lsFloor.getFirstNode();
    while (pt)
    {
        pfl = (Floor*)pt->data;
        draw(pfl);
        pt = lsFloor.getNextNode(pt);
    }

    pt = lsShoots.getFirstNode();
    while (pt)
    {
        pts = (Shoot*)pt->data;
        draw(pts);
        pt = lsShoots.getNextNode(pt);
    }

    drawMark();
    drawScore();
    appGraph.draw(&bmp.time, 320 - bmp.time.getWidth() / 2, MAX_Y + 3);
    appGraph.draw(&fontNum[FONT_BIG], timeRemaining, 300, MAX_Y + 20);

    if (gameinf.getPlayers()[PLAYER1]->isVisible() && gameinf.getPlayers()[PLAYER1]->isPlaying())
        draw(gameinf.getPlayers()[PLAYER1]);

    if (gameinf.getPlayers()[PLAYER2])
        if (gameinf.getPlayers()[PLAYER2]->isVisible() && gameinf.getPlayers()[PLAYER2]->isPlaying())
            draw(gameinf.getPlayers()[PLAYER2]);

    pt = lsBalls.getFirstNode();
    while (pt)
    {
        ptb = (Ball*)pt->data;
        draw(ptb);
        pt = lsBalls.getNextNode(pt);
    }

    if (gameOver)
    {
        appGraph.draw(&bmp.gameover, 100, 125);
        if (gameOverCount >= 0)
        {
            appGraph.draw(&bmp.continu, 130, 200);
            appGraph.draw(&fontNum[FONT_HUGE], gameOverCount, 315, 300);
        }
    }

    if (pStageClear) pStageClear->drawAll();
    
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
