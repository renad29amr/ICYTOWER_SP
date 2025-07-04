#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>
#include "file.h"
#include "clock.h"
#include "head.h"
#include "view.h"

using namespace sf;
using namespace std;

const float WIDTH = 1920, HEIGHT = 1080;
const float VIEW_PAUSE_DURATION = 0.1;

RenderWindow window(VideoMode(WIDTH, HEIGHT), "icyTower", Style::Close | Style::Fullscreen);

View view;

bool isGround = false, showStartMenu = false, showWelcome = true;

// ----> blocks constants <----
const int blocksNum = 50;
int score = 0, floors = 0, Xleft, Xright, nonIntersectedBlocks = 0;

// ---->Level variables<----
int currentLevel = 1;

// ----> Life time <----
const int max_lives = 3;
int lives = max_lives;
Vector2f lastSafePosition;

// ---->Wider Blocks Effect<----
bool widerBlocksActive = false;
Clock widerBlocksTimer;

// ----> hurry up variables <----
bool showhurryUp = false;
float hurryUpTime = 0.0f;

// ----> sweet variables <----
float sweetTime = 0.0f;
bool showSweet = false;

// ----> highScore variables <----
float highScoreTime = 0.0f;
bool showHighScore = false;
bool highScoreShownThisGame = false;

// ----> boom variables <----
bool showBoom = false;
Vector2f boomPosition;
float boomTime = 0.0f;

// ----> File <----
string userName = "";

// Textures
Texture tex_background, tex_background2, tex_background3, tex_ground, tex_wall_right, tex_wall_left, tex_player, tex_block, tex_block2, tex_block3,
tex_interface, tex_hand, tex_menu, tex_heads, tex_gameover, tex_enterName, tex_winner, tex_newstage, tex_highscoreMenu, tex_barrier, tex_barrier2,
tex_ice, tex_clock, tex_clockHand, tex_instructions, tex_star, tex_credits, tex_sweet, tex_thankyou, tex_superJump, tex_extraLife, tex_loseLife,
tex_widerBlocks, tex_welcome, tex_bomb, tex_boom, tex_hurryUp, tex_cracks, tex_highScore;

// Sprites
Sprite ground, wall, wall2, interface, hand, menu, head, gameover, enterName, winner, newstage,highscoreMenu, ice[max_lives], 
wallClock, clockHand, instructions, star, credits, sweet, thankyou, welcome, bomb, boom, hurryUp, cracks, highScore;

// Sounds
SoundBuffer buffer_menu_choose, buffer_menu_change, buffer_jump, buffer_sound_gameover, buffer_sound_cheer, buffer_sound_rotate,
buffer_sound_gonna_fall, buffer_sound_sweet, buffer_sound_loseLife, buffer_sound_superJump, buffer_sound_extraLife, buffer_sound_winner,
buffer_sound_widerBlocks, buffer_sound_bye, buffer_sound_bomb, buffer_sound_spark, buffer_sound_hurryUp, buffer_sound_highScore;

Sound menu_choose, menu_change, jump_sound, sound_gameover, sound_cheer, sound_rotate, sound_gonna_fall, sound_sweet, sound_loseLife,
sound_superJump, sound_extraLife, sound_winner, sound_widerBlocks, sound_bye, sound_bomb, sound_spark, sound_hurryUp, sound_highScore;

Music background_music;

struct BLOCKS
{
    Sprite blocksSprite;
    int direction, level;
    float moveSpeed, leftBound, rightBound, xscale, originalXscale, originalXposition;
    bool isMoving, isIntersected;

    BLOCKS(Texture& texblocks, float xposition, float yposition, int level)
    {
        blocksSprite.setTexture(texblocks);
        blocksSprite.setOrigin(0, 0);
        originalXposition = xposition;
        originalXscale = (rand() % 2) + (level == 1 ? 1.75 : level == 2 ? 2 : 2.25);

        if (widerBlocksActive)
        {
            xscale = (Xright - Xleft) / static_cast<float>(texblocks.getSize().x);
            blocksSprite.setPosition(Xleft - 100, yposition);
            blocksSprite.setScale(xscale + 10, 1);
            isMoving = false;
        }

        else
        {
            xscale = originalXscale;
            blocksSprite.setPosition(xposition, yposition);
            blocksSprite.setScale(xscale, 1);

            if (level == 1)
            {
                isMoving = false;
            }

            else if (level == 2)
            {
                isMoving = (rand() % 2) == 0;
            }

            else
            {
                isMoving = true;
            }

            if (isMoving)
            {
                moveSpeed = (rand() % 50) + 100;
                leftBound = xposition - (rand() % 100 + 10);
                rightBound = xposition + (rand() % 100 + 10);
                direction = (rand() % 2) == 0 ? 1 : -1;
            }
        }

        isIntersected = false;
        this->level = level;
    }
};

BLOCKS* currentBlock = nullptr;
int lastBlockIndex = -1;

struct Barrier
{
    Sprite sprite;
    bool spawned;
    int targetLevel; // Level after which this barrier appears
    float xScale, yScale;

    Barrier(Texture& texture, int level, float xScaleFactor, float yScaleFactor)
    {
        sprite.setTexture(texture);
        targetLevel = level;
        xScale = xScaleFactor;
        yScale = yScaleFactor;
        sprite.setScale(xScale, yScale);
        spawned = false;
    }

    void setPosition(float x, float y)
    {
        sprite.setPosition(x, y);
    }
};

const int NUM_BARRIERS = 2;
Barrier* barriers[NUM_BARRIERS];

struct Feature
{
    Sprite sprite;
    string type; // "superJump", "extraLife", "loseLife" , "widerBlocks"  , "bomb"
    bool active;

    Feature(Texture& texture, string featureType, float x, float y)
    {
        sprite.setTexture(texture);
        sprite.setPosition(x, y);
        sprite.setScale(0.75f, 0.75f);
        type = featureType;
        active = true;
    }
};

struct Players
{
    Sprite sprite;
    const float moveSpeed = 300.0f, jump_strength = -10000.0f, gravity = 25000.0f;
    float velocity_x, velocity_y, frameTimer, rotationAngle;
    int frameIndex;
    bool rotatedInAir, onBarrier = true, onBarrier2 = true, isRotating, isMovingleft, isMovingright,hasSuperJump;

    Players()
    {
        velocity_x = 0;
        velocity_y = 0;
        frameIndex = 0;
        frameTimer = 0;
        rotationAngle = 0.0f;
        rotatedInAir = false;
        isRotating = false;
        isMovingleft = false;
        isMovingright = false;
        hasSuperJump = false;
        sprite.setTexture(tex_player);
        sprite.setTextureRect(IntRect(0, 0, 38, 71));
        sprite.setPosition(960, 650);
        sprite.setScale(2.4f, 2.4f);
    }

    void handleMovement(float deltatime)
    {
        frameTimer += deltatime;

        if (Keyboard::isKeyPressed(Keyboard::Right))
        {
            sprite.setScale(2.4, 2.4);
            velocity_x = moveSpeed;
            isMovingright = true;

            if (frameTimer >= 0.1f)
            {
                frameIndex = (frameIndex + 1) % 4;
                frameTimer = 0;
            }

            sprite.setTextureRect(IntRect(frameIndex * 38, 0, 38, 71));
        }

        else if (Keyboard::isKeyPressed(Keyboard::Left))
        {
            sprite.setScale(-2.4, 2.4);
            velocity_x = -moveSpeed;
            isMovingleft = true;

            if (frameTimer >= 0.1f)
            {
                frameIndex = (frameIndex + 1) % 4;
                frameTimer = 0;
            }

            sprite.setTextureRect(IntRect(frameIndex * 38, 0, 38, 71));
        }

        else
        {
            velocity_x = 0;
            if (isGround && frameIndex != 11 && frameIndex != 9)
            {
                frameIndex = 0;
                sprite.setTextureRect(IntRect(0, 0, 38, 71));
            }
        }

        if (!isRotating)
        {
            sprite.move(velocity_x * deltatime, 0);
        }
    }

    void jump(float deltatime)
    {
        static float jumpTimer = 0.0f;
        const float maxJumpTime = 0.3f, jumpAcceleration = -200000.0f;
        const float superJumpStrength = -2500.0f;

        if (Keyboard::isKeyPressed(Keyboard::Space) && isGround)
        {
            jump_sound.play();
            jumpTimer = 0.0f;
            isGround = false;
            velocity_y = jump_strength;

            if (hasSuperJump)
            {
                hasSuperJump = false;
            }
        }

        if (!isGround)
        {
            jumpTimer += deltatime;

            if (jumpTimer < maxJumpTime && Keyboard::isKeyPressed(Keyboard::Space))
            {
                velocity_y += jumpAcceleration * deltatime;
            }

            velocity_y += gravity * deltatime;

            float maxJumpSpeed = hasSuperJump ? superJumpStrength : -1500.0f;
            if (velocity_y < maxJumpSpeed)
            {
                velocity_y = maxJumpSpeed;
            }

            const float maxFallSpeed = 1000.0f;
            if (velocity_y > maxFallSpeed)
            {
                velocity_y = maxFallSpeed;
            }

            if (velocity_y < 0)
            {
                if (isMovingright && frameIndex != 6 && frameIndex != 9)
                {
                    frameIndex = 6;
                    sprite.setScale(2.4, 2.4);
                    sprite.setTextureRect(IntRect(frameIndex * 38, 0, 38, 71));
                }

                else if (isMovingleft && frameIndex != 6 && frameIndex != 9)
                {
                    frameIndex = 6;
                    sprite.setScale(-2.4, 2.4);
                    sprite.setTextureRect(IntRect(frameIndex * 38, 0, 38, 71));
                }

                else
                {
                    if (frameIndex != 5 && frameIndex != 9)
                    {
                        frameIndex = 5;
                        sprite.setTextureRect(IntRect(frameIndex * 38, 0, 38, 71));
                    }
                }
            }

            else
            {
                if (frameIndex != 8 && frameIndex != 9)
                {
                    frameIndex = 8;
                    sprite.setTextureRect(IntRect(frameIndex * 38, 0, 38, 71));
                }
            }
        }
    }

    void playerRotation(float deltatime, vector<BLOCKS>& blocksList, int level, bool& viewpaused, Clock& viewtimer, bool& win, bool resetRotation = false)
    {
        if (lives ==0 || win) return;

        static const float FAST_VIEW_SPEED = 700.0f;
        static float jumpHeight = 200.0f, jumpDirection = 1.0f, jumpDuration = 0.7f, jumpTime = 0.0f;
        static bool wallJumping = false, fastViewActive = false;
        static Vector2f jumpStart, jumpEnd;

        if (resetRotation)
        {
            jumpTime = 0.0f;
            wallJumping = false;
            fastViewActive = false;
            jumpStart = Vector2f(0, 0);
            jumpEnd = Vector2f(0, 0);
            return;
        }

        if (!isGround && !wallJumping && !isRotating)
        {
            bool rightWall = sprite.getGlobalBounds().intersects(wall2.getGlobalBounds());
            bool leftWall = sprite.getGlobalBounds().intersects(wall.getGlobalBounds());

            if (rightWall || leftWall)
            {
                int targetIndex = -1;
                float targetY = sprite.getPosition().y;

                // Normal wall jump
                if (lastBlockIndex >= 0 && lastBlockIndex < static_cast<int>(blocksList.size()) - 1)
                {
                    targetIndex = min((int)blocksList.size() - 1, lastBlockIndex + 3);
                    targetY = blocksList[targetIndex].blocksSprite.getPosition().y;
                }

                for (int i = 0; i < NUM_BARRIERS; ++i)
                {
                    Barrier* barrier = barriers[i];
                    bool isOnBarrier = (i == 0 && onBarrier) || (i == 1 && onBarrier2);

                    if (barrier->spawned && isOnBarrier &&
                        sprite.getPosition().y + sprite.getGlobalBounds().height <= barrier->sprite.getPosition().y)
                    {
                        int lastLevelIndex = -1;
                        int prevLevel = (i == 0) ? 1 : 2;
                        int targetLevel = prevLevel + 1;

                        for (auto it = blocksList.rbegin(); it != blocksList.rend(); ++it)
                        {
                            if (it->level == prevLevel)
                            {
                                lastLevelIndex = distance(blocksList.begin(), it.base()) - 1;
                                break;
                            }
                        }

                        if (lastLevelIndex >= 0 && lastLevelIndex + 4 < static_cast<int>(blocksList.size()))
                        {
                            targetIndex = lastLevelIndex + 3;
                            if (blocksList[targetIndex].level == targetLevel)
                            {
                                targetY = blocksList[targetIndex].blocksSprite.getPosition().y;
                            }
                            else
                            {
                                targetIndex = -1;
                            }
                        }

                        if (i == 0) onBarrier = false;
                        else if (i == 1) onBarrier2 = false;
                    }
                }

                // Work only if target is above player
                if (targetIndex >= 0 && targetY < sprite.getPosition().y)
                {
                    jumpEnd = blocksList[targetIndex].blocksSprite.getPosition();
                    jumpEnd.y -= sprite.getGlobalBounds().height - 12;
                    jumpEnd.x += blocksList[targetIndex].blocksSprite.getGlobalBounds().width / 2;

                    sound_rotate.play();
                    sound_sweet.play();
                    sweet.setPosition(view.getCenter().x - 70, view.getCenter().y - 80);
                    sweet.setScale(1.0f, 1.0f);
                    sweetTime = 0.0f;
                    showSweet = true;
                    jumpStart = sprite.getPosition();
                    wallJumping = true;
                    jumpTime = 0.0f;
                    jumpDuration = 0.7f;
                    jumpDirection = rightWall ? 1.0f : -1.0f;
                    sprite.setScale(jumpDirection * 2.4f, 2.4f);
                    jump_sound.play();
                    fastViewActive = true;
                }
            }
        }

        if (wallJumping)
        {
            jumpTime += deltatime;
            float t = min(jumpTime / jumpDuration, 1.0f);
            frameIndex = 9;
            sprite.setTextureRect(IntRect(frameIndex * 38, 0, 70, 71));

            float arcY = -4 * jumpHeight * t * (1 - t);
            Vector2f pos = (1 - t) * jumpStart + t * jumpEnd;
            pos.y += arcY;

            sprite.setRotation(jumpDirection * 360.0f * t);
            sprite.setPosition(pos);

            float viewY = view.getCenter().y;
            float playerY = sprite.getPosition().y;
            float targetViewY = playerY + HEIGHT / 3;
            float minViewY = jumpEnd.y + HEIGHT / 3;

            if (targetViewY < minViewY)
            {
                viewY = minViewY;
            }

            else
            {
                viewY += (targetViewY - viewY) * 5.0f * deltatime;
            }

            view.setCenter(Vector2f(view.getCenter().x, viewY));
            adjustViewAspectRatio(WIDTH / HEIGHT);
            window.setView(view);

            if (t >= 1.0f)
            {
                wallJumping = false;
                sprite.setRotation(0);
                frameIndex = 0;
                sprite.setTextureRect(IntRect(0, 0, 38, 71));
                isRotating = false;
                rotatedInAir = false;
                fastViewActive = false;
                viewpaused = true;
                viewtimer.restart();
            }
            return;
        }

        if (isGround)
        {
            if (frameIndex == 9 || sprite.getRotation() != 0)
            {
                frameIndex = 0;
                sprite.setTextureRect(IntRect(0, 0, 38, 71));
                sprite.setRotation(0);
                rotationAngle = 0.0f;
                rotatedInAir = false;
            }
        }

    }
};

struct Background
{
    Sprite sprite;
    Texture& texture;
    int level;

    Background(Texture& texture, int levelNum) : texture(texture), level(levelNum)
    {
        sprite.setTexture(texture);
        sprite.setScale(static_cast<float>(WIDTH) / texture.getSize().x,
            static_cast<float>(HEIGHT) / texture.getSize().y);
    }

    void setPosition(float x, float y)
    {
        sprite.setPosition(x, y);
    }

    void update(int currentLevel, Vector2f camPos)
    {
        if (this->level == currentLevel)
        {
            setPosition(camPos.x - WIDTH / 2, camPos.y - HEIGHT / 2);
        }
    }
};

vector<Background> backgrounds;

void generationBlocks(vector<BLOCKS>& blocksList, Players& player, int level)
{
    float y;
    Texture& tex_blocks = (level == 1) ? tex_block : (level == 2) ? tex_block2 : tex_block3;

    int numBlocks = blocksNum * level;
    float verticalSpacing = (level == 1) ? 175 : (level == 2) ? 215 : 250;

    if (level == 1 || blocksList.empty())
    {
        blocksList.clear();
        y = HEIGHT - 300;
    }

    else
    {
        float highestY = HEIGHT;
        for (const auto& block : blocksList)
        {
            if (block.level == level - 1)
            {
                highestY = min(highestY, block.blocksSprite.getPosition().y);
            }
        }
        y = highestY - verticalSpacing - 200;
    }

    for (int i = 0; i < numBlocks; i++)
    {
        y -= verticalSpacing + (rand() % 100);
        float x = (rand() % (Xright - Xleft + 400)) + Xleft;

        if (widerBlocksActive)
        {
            x = Xleft;
        }

        blocksList.push_back(BLOCKS(tex_blocks, x, y, level));
    }
}

void generationFeatures(vector<BLOCKS>& blocksList, Players& player, Text& Score, vector<Feature>& featuresList, int level)
{
    static Clock featureTimer;
    const float spawnInterval = 1.5f; // Spawn attempt every 1.5 seconds
    const float spawnProbability = 0.3f; // 30% chance to spawn a feature

    if (featureTimer.getElapsedTime().asSeconds() >= spawnInterval)
    {
        featureTimer.restart();
        if ((rand() % 100) / 100.0f < spawnProbability)
        {
            float x, y;
            string type;
            Texture* texture;

            int featureType = rand() % 4;

            x = (rand() % (Xright - Xleft - 100)) + Xleft + 50;
            y = player.sprite.getPosition().y - (rand() % 300 + 200);

            switch (featureType)
            {
            case 0:
                type = "superJump";
                texture = &tex_superJump;
                break;

            case 1:
                type = "extraLife";
                texture = &tex_extraLife;
                break;

            case 2:
                type = "loseLife";
                texture = &tex_loseLife;
                break;

            case 3:
                type = "widerBlocks";
                texture = &tex_widerBlocks;
                break;

            default:
                return;
            }

            featuresList.push_back(Feature(*texture, type, x, y));
        }
    }
}

void updateMovingBlocks(vector<BLOCKS>& blocksList, float deltatime)
{
    for (auto& block : blocksList)
    {
        if (block.isMoving && !widerBlocksActive)
        {
            block.blocksSprite.move(2 * block.moveSpeed * block.direction * deltatime, 0);

            float currentX = block.blocksSprite.getPosition().x;
            if (currentX <= block.leftBound)
            {
                block.direction = 1;
            }

            else if (currentX >= block.rightBound)
            {
                block.direction = -1;
            }
        }
    }
}

void handleWiderBlocksEffect(vector<BLOCKS>& blocksList, int currentLevel, Players& player)
{
    if (widerBlocksActive && widerBlocksTimer.getElapsedTime().asSeconds() >= 2.0f)
    {
        widerBlocksActive = false;

        for (auto& block : blocksList)
        {
            block.xscale = block.originalXscale;
            block.blocksSprite.setScale(block.xscale, 1);
            block.blocksSprite.setPosition(block.originalXposition, block.blocksSprite.getPosition().y);
            block.blocksSprite.setOrigin(0, 0);

            if (block.level > 1)
            {
                block.isMoving = true;
                block.moveSpeed = (rand() % 50) + 100;
                block.leftBound = block.originalXposition - (rand() % 100 + 10);
                block.rightBound = block.originalXposition + (rand() % 100 + 10);
                block.direction = (rand() % 2) == 0 ? 1 : -1;
            }

            else
            {
                block.isMoving = false;
            }
        }
    }
}

void initializeObject()
{
    tex_background.loadFromFile("images//background.jpg");
    tex_background2.loadFromFile("images//background2.png");
    tex_background3.loadFromFile("images//background3.png");
    tex_ground.loadFromFile("images//ground.jpg");
    tex_wall_left.loadFromFile("images//wall left.png");
    tex_wall_right.loadFromFile("images//wall right.png");
    tex_player.loadFromFile("images//player.png");
    tex_block.loadFromFile("images//block.png");
    tex_block2.loadFromFile("images//block2.png");
    tex_block3.loadFromFile("images//block3.png");
    tex_interface.loadFromFile("images//mainMenu.png");
    tex_hand.loadFromFile("images//hand.png");
    tex_menu.loadFromFile("images//pauseMenu.png");
    tex_heads.loadFromFile("images//heads.png");
    tex_gameover.loadFromFile("images//gameover.png");
    tex_enterName.loadFromFile("images//enterName.png");
    tex_winner.loadFromFile("images//winner.png");
    tex_newstage.loadFromFile("images//newstage.png");
    tex_highscoreMenu.loadFromFile("images//highscoreMenu.png");
    tex_barrier.loadFromFile("images//barrier.png");
    tex_barrier2.loadFromFile("images//barrier2.png");
    tex_ice.loadFromFile("images//ice.png");
    tex_clock.loadFromFile("images//clock.png");
    tex_clockHand.loadFromFile("images//clockHand.png");
    tex_instructions.loadFromFile("images//instructions.png");
    tex_credits.loadFromFile("images//credits.png");
    tex_star.loadFromFile("images//star.png");
    tex_sweet.loadFromFile("images//sweet.png");
    tex_superJump.loadFromFile("images//superjump.png");
    tex_extraLife.loadFromFile("images//extraLife.png");
    tex_loseLife.loadFromFile("images//loseLife.png");
    tex_widerBlocks.loadFromFile("images//widerBlocks.png");
    tex_thankyou.loadFromFile("images//thankYou.png");
    tex_welcome.loadFromFile("images//Welcome.png");
    tex_bomb.loadFromFile("images//bomb.png");
    tex_boom.loadFromFile("images//BOOM.png");
    tex_hurryUp.loadFromFile("images//hurryUp.png");
    tex_cracks.loadFromFile("images//cracks.png");
    tex_highScore.loadFromFile("images//highScore.png");

    // Sounds
    background_music.openFromFile("Sound//backgroundMusic.wav");
    buffer_menu_choose.loadFromFile("Sound//menu_choose.ogg");
    buffer_menu_change.loadFromFile("Sound//menu_change.ogg");
    buffer_jump.loadFromFile("Sound//sound_jump.ogg");
    buffer_sound_gonna_fall.loadFromFile("Sound//sound_gonna_fall.ogg");
    buffer_sound_gameover.loadFromFile("Sound//sound_gameover.opus");
    buffer_sound_cheer.loadFromFile("Sound//sound_cheer.opus");
    buffer_sound_rotate.loadFromFile("Sound//sound_falling.wav");
    buffer_sound_sweet.loadFromFile("Sound//sound_sweet.ogg");
    buffer_sound_superJump.loadFromFile("Sound//sound_superJump.wav");
    buffer_sound_extraLife.loadFromFile("Sound//sound_extraLife.wav");
    buffer_sound_loseLife.loadFromFile("Sound//sound_loseLife.wav");
    buffer_sound_winner.loadFromFile("Sound//sound_winner.wav");
    buffer_sound_widerBlocks.loadFromFile("Sound//sound_widerBlock.wav");
    buffer_sound_bye.loadFromFile("Sound//sound_bye.wav");
    buffer_sound_bomb.loadFromFile("Sound//sound_bomb.wav");
    buffer_sound_spark.loadFromFile("Sound//sound_spark.wav");
    buffer_sound_hurryUp.loadFromFile("Sound//sound_hurryUp.wav");
    buffer_sound_highScore.loadFromFile("Sound//sound_highScore.ogg");

    menu_choose.setBuffer(buffer_menu_choose);
    menu_change.setBuffer(buffer_menu_change);
    jump_sound.setBuffer(buffer_jump);
    sound_gameover.setBuffer(buffer_sound_gameover);
    sound_cheer.setBuffer(buffer_sound_cheer);
    sound_rotate.setBuffer(buffer_sound_rotate);
    sound_gonna_fall.setBuffer(buffer_sound_gonna_fall);
    sound_sweet.setBuffer(buffer_sound_sweet);
    sound_superJump.setBuffer(buffer_sound_superJump);
    sound_extraLife.setBuffer(buffer_sound_extraLife);
    sound_loseLife.setBuffer(buffer_sound_loseLife);
    sound_winner.setBuffer(buffer_sound_winner);
    sound_widerBlocks.setBuffer(buffer_sound_widerBlocks);
    sound_bye.setBuffer(buffer_sound_bye);
    sound_bomb.setBuffer(buffer_sound_bomb);
    sound_spark.setBuffer(buffer_sound_spark);
    sound_hurryUp.setBuffer(buffer_sound_hurryUp);
    sound_highScore.setBuffer(buffer_sound_highScore);

    // Lives
    for (int i = 0; i < max_lives; i++)
    {
        ice[i].setTexture(tex_ice);
        ice[i].setScale(2.5, 2.5);
    }

    welcome.setTexture(tex_welcome);
    welcome.setScale(static_cast<float>(WIDTH) / tex_interface.getSize().x + .32,
        static_cast<float>(HEIGHT) / tex_interface.getSize().y + .28);
    welcome.setPosition(0, 0);

    enterName.setTexture(tex_enterName);
    enterName.setPosition(950, 630);
    enterName.setScale(2.5, 2.7);

    // Clock
    wallClock.setTexture(tex_clock);
    wallClock.setScale(1.5, 1.5);
    wallClock.setPosition(0, 50);

    clockHand.setTexture(tex_clockHand);
    clockHand.setPosition(65, 135);
    clockHand.setScale(1.2, 1.2);
    FloatRect clockhandBounds = clockHand.getLocalBounds();
    clockHand.setOrigin(clockhandBounds.width / 2, clockhandBounds.height - 10);
    clockHand.setRotation(0);

    star.setTexture(tex_star);
    star.setScale(1.35, 1.35);
    star.setPosition(7, 375);

    sweet.setTexture(tex_sweet);
    sweet.setPosition(620, 120);

    highScore.setTexture(tex_highScore);

    hurryUp.setTexture(tex_hurryUp);
    hurryUp.setPosition(view.getCenter().x - 70, view.getCenter().y - 80);

    menu.setTexture(tex_menu);
    menu.setScale(2, 2.5);

    gameover.setTexture(tex_gameover);
    gameover.setPosition(620, 70);
    gameover.setScale(1.3, 1.3);

    winner.setTexture(tex_winner);
    winner.setPosition(620, -120);
    winner.setScale(1.25, 1.25);

    newstage.setTexture(tex_newstage);
    newstage.setScale(1, 2);

    cracks.setTexture(tex_cracks);
    cracks.setScale(1, 1);

    hand.setTexture(tex_hand);
    hand.setScale(1.5, 1.5);

    boom.setTexture(tex_boom);
    boom.setScale(1, 2);

    interface.setTexture(tex_interface);
    interface.setScale(static_cast<float>(WIDTH) / tex_interface.getSize().x,
        static_cast<float>(HEIGHT) / tex_interface.getSize().y);
    interface.setPosition(0, 0);

    instructions.setTexture(tex_instructions);
    instructions.setScale(static_cast<float>(WIDTH) / tex_instructions.getSize().x,
        static_cast<float>(HEIGHT) / tex_instructions.getSize().y);
    instructions.setPosition(0, 0);

    credits.setTexture(tex_credits);
    credits.setScale(static_cast<float>(WIDTH) / tex_credits.getSize().x,
        static_cast<float>(HEIGHT) / tex_credits.getSize().y);
    credits.setPosition(0, 0);

    backgrounds.push_back(Background(tex_background, 1));
    backgrounds.push_back(Background(tex_background2, 2));
    backgrounds.push_back(Background(tex_background3, 3));

    ground.setTexture(tex_ground);
    ground.setPosition(0, 860);
    ground.setScale(3, 1.5);

    wall.setTexture(tex_wall_left);
    wall.setPosition(0, 0);
    wall.setScale(1.5, 3);
    Xleft = wall.getGlobalBounds().width + 50;

    wall2.setTexture(tex_wall_right);
    wall2.setPosition(1760, 0);
    wall2.setScale(1.5, 3);
    Xright = wall2.getGlobalBounds().width + 810;

    barriers[0] = new Barrier(tex_barrier, 1, 5.5f, 1.5f);
    barriers[1] = new Barrier(tex_barrier2, 2, 2.5f, 1.5f);

    thankyou.setTexture(tex_thankyou);
    thankyou.setScale(static_cast<float>(WIDTH) / tex_interface.getSize().x + .5,
        static_cast<float>(HEIGHT) / tex_interface.getSize().y + .5);
    thankyou.setPosition(0, 0);
}

void reset(Players& player, vector<BLOCKS>& blocksList, Text& Score, bool& viewpaused, bool& win, Clock& rotation, Clock& viewtimer, Clock& Timer, RectangleShape& OclockFill, vector<Feature>& featuresList)
{
    // Reset game variables
    score = 0;
    floors = 0;
    lives = max_lives;
    currentLevel = 1;

    for (int i = 0; i < NUM_BARRIERS; ++i)
    {
        barriers[i]->spawned = false;
    }

    widerBlocksActive = false;
    win = false;
    viewpaused = false;
    isGround = true;
    showSweet = false;
    showhurryUp = false;
    showBoom = false;
    showHighScore = false;
    highScoreShownThisGame = false;
    highScoreTime = 0;
    boomTime = 0;
    sweetTime = 0.0f;
    hurryUpTime = 0.0f;
    clockFill = 0.0f;

    nonIntersectedBlocks = 0;
    lastBlockIndex = -1;
    currentBlock = nullptr;

    // Reset player state
    player.velocity_x = 0.0f;
    player.velocity_y = 0.0f;
    player.frameIndex = 0;
    player.frameTimer = 0.0f;
    player.rotationAngle = 0.0f;
    player.rotatedInAir = false;
    player.isRotating = false;
    player.isMovingleft = false;
    player.isMovingright = false;
    player.hasSuperJump = false;
    player.onBarrier = true;
    player.onBarrier2 = true;
    player.sprite.setPosition(960, 750);
    player.sprite.setScale(2.4f, 2.4f);
    player.sprite.setTextureRect(IntRect(0, 0, 38, 71));
    player.sprite.setRotation(0.0f);
    lastSafePosition = player.sprite.getPosition();
    rotation.restart();
    player.playerRotation(0.0f, blocksList, currentLevel, viewpaused, viewtimer, win, true);

    // Reset score display
    Score.setString("Score: " + to_string(score));
    Score.setFillColor(Color::White);

    // Reset clocks
    Timer.restart();
    FillTime.restart();
    rotation.restart();
    viewtimer.restart();
    transitionClock.restart();
    widerBlocksTimer.restart();

    // Reset clock fill bar
    OclockFill.setSize(Vector2f(26, 0));
    OclockFill.setPosition(49, 380);

    // Reset background
    for (auto& bg : backgrounds)
    {
        bg.setPosition(0, 0);
        if (bg.level == 1)
        {
            bg.sprite.setTexture(bg.texture);
        }
    }

    // Reset view
    view.setCenter(Vector2f(960, 540));
    view.setSize(WIDTH, HEIGHT);
    adjustViewAspectRatio(WIDTH / HEIGHT);
    window.setView(view);

    // Reset ground and walls
    ground.setPosition(0, 860);
    wall.setPosition(0, 0);
    wall2.setPosition(1760, 0);

    // Clear and regenerate blocks for both levels
    blocksList.clear();
    featuresList.clear();
    generationBlocks(blocksList, player, 1);
    generationBlocks(blocksList, player, 2);
    generationBlocks(blocksList, player, 3);

    // Stop all sounds
    sound_rotate.stop();
    sound_gonna_fall.stop();
    sound_gameover.stop();
    sound_cheer.stop();
    sound_sweet.stop();
    sound_superJump.stop();
    sound_extraLife.stop();
    sound_loseLife.stop();
    sound_winner.stop();
    sound_widerBlocks.stop();
    sound_bomb.stop();
    sound_spark.stop();
    sound_highScore.stop();
}

void adjustVolume(int& soundsVolume)
{
    menu_change.setVolume(soundsVolume);
    menu_choose.setVolume(soundsVolume);
    jump_sound.setVolume(soundsVolume);
    sound_cheer.setVolume(soundsVolume);
    sound_gameover.setVolume(soundsVolume);
    sound_rotate.setVolume(soundsVolume);
    sound_gonna_fall.setVolume(soundsVolume);
    sound_sweet.setVolume(soundsVolume);
    sound_superJump.setVolume(soundsVolume);
    sound_extraLife.setVolume(soundsVolume);
    sound_loseLife.setVolume(soundsVolume);
    sound_winner.setVolume(soundsVolume);
    sound_widerBlocks.setVolume(soundsVolume);
    sound_bye.setVolume(soundsVolume);
    sound_bomb.setVolume(soundsVolume);
    sound_spark.setVolume(soundsVolume);
    sound_hurryUp.setVolume(soundsVolume);
    sound_highScore.setVolume(soundsVolume);
}

bool soundOptions(Font font)
{
    int menuSelection = 0;
    bool isPressed = false;
    hand.setPosition(600, 300);

    int soundsVolume = menu_change.getVolume();
    int musicVolume = background_music.getVolume();

    float sliderBarWidth = 200.0f;
    float sliderBarHeight = 10.0f;

    RectangleShape soundsSliderBar(Vector2f(sliderBarWidth, sliderBarHeight));
    soundsSliderBar.setFillColor(Color::Black);
    soundsSliderBar.setPosition(1000, 330);

    RectangleShape musicSliderBar(Vector2f(sliderBarWidth, sliderBarHeight));
    musicSliderBar.setFillColor(Color::Black);
    musicSliderBar.setPosition(1000, 450);

    CircleShape soundsSliderKnob(10);
    soundsSliderKnob.setFillColor(Color::Red);
    soundsSliderKnob.setPosition(1000 + (soundsVolume / 100.0f) * sliderBarWidth - 7, 325);

    CircleShape musicSliderKnob(10);
    musicSliderKnob.setFillColor(Color::Red);
    musicSliderKnob.setPosition(1000 + (musicVolume / 100.0f) * sliderBarWidth - 7, 445);

    Text text_Sounds("SOUNDS", font);
    text_Sounds.setCharacterSize(50);
    text_Sounds.setFillColor(Color::Black);
    text_Sounds.setPosition(700, 300);

    Text text_Music("MUSIC", font);
    text_Music.setCharacterSize(50);
    text_Music.setFillColor(Color::Black);
    text_Music.setPosition(700, 420);

    Text text_Back("BACK", font);
    text_Back.setCharacterSize(50);
    text_Back.setFillColor(Color::Black);
    text_Back.setPosition(700, 540);

    while (window.isOpen())
    {
        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
            {
                window.close();
            }

            if (event.type == Event::KeyPressed && !isPressed)
            {
                isPressed = true;
                if (event.key.code == Keyboard::Up)
                {
                    menu_change.play();
                    menuSelection = (menuSelection - 1 + 3) % 3;
                    hand.setPosition(600, 300 + 120 * menuSelection);
                }

                if (event.key.code == Keyboard::Down)
                {
                    menu_change.play();
                    menuSelection = (menuSelection + 1) % 3;
                    hand.setPosition(600, 300 + 120 * menuSelection);
                }

                if (event.key.code == Keyboard::Enter)
                {
                    menu_choose.play();
                    if (menuSelection == 2)
                    {
                        return false;
                    }
                }
            }

            if (event.type == Event::KeyReleased)
            {
                isPressed = false;
            }
        }

        if (menuSelection == 0)
        {
            if (Keyboard::isKeyPressed(Keyboard::Left))
            {
                soundsVolume = max(0, soundsVolume - 1);
                soundsSliderKnob.setPosition(1000 + (soundsVolume / 100.0f) * sliderBarWidth - 7, 325);
                adjustVolume(soundsVolume);
            }

            if (Keyboard::isKeyPressed(Keyboard::Right))
            {
                soundsVolume = min(100, soundsVolume + 1);
                soundsSliderKnob.setPosition(1000 + (soundsVolume / 100.0f) * sliderBarWidth - 7, 325);
                adjustVolume(soundsVolume);
            }
        }

        else if (menuSelection == 1)
        {
            if (Keyboard::isKeyPressed(Keyboard::Left))
            {
                musicVolume = max(0, musicVolume - 1);
                musicSliderKnob.setPosition(1000 + (musicVolume / 100.0f) * sliderBarWidth - 7, 445);
                background_music.setVolume(musicVolume);
            }

            if (Keyboard::isKeyPressed(Keyboard::Right))
            {
                musicVolume = min(100, musicVolume + 1);
                musicSliderKnob.setPosition(1000 + (musicVolume / 100.0f) * sliderBarWidth - 7, 445);
                background_music.setVolume(musicVolume);
            }
        }

        window.setView(window.getDefaultView());

        text_Sounds.setFillColor(menuSelection == 0 ? Color::Red : Color::Black);
        text_Sounds.setOutlineColor(menuSelection == 0 ? Color::Yellow : Color::Transparent);
        text_Sounds.setOutlineThickness(menuSelection == 0 ? 2 : 0);

        text_Music.setFillColor(menuSelection == 1 ? Color::Red : Color::Black);
        text_Music.setOutlineColor(menuSelection == 1 ? Color::Yellow : Color::Transparent);
        text_Music.setOutlineThickness(menuSelection == 1 ? 2 : 0);

        text_Back.setFillColor(menuSelection == 2 ? Color::Red : Color::Black);
        text_Back.setOutlineColor(menuSelection == 2 ? Color::Yellow : Color::Transparent);
        text_Back.setOutlineThickness(menuSelection == 2 ? 2 : 0);

        window.clear();

        for (auto& bg : backgrounds)
        {
            if (bg.level == 1)
            {
                bg.setPosition(0, 0);
                window.draw(bg.sprite);
                break;
            }
        }

        window.draw(menu);
        window.draw(soundsSliderBar);
        window.draw(soundsSliderKnob);
        window.draw(musicSliderBar);
        window.draw(musicSliderKnob);
        window.draw(text_Sounds);
        window.draw(text_Music);
        window.draw(text_Back);
        window.draw(hand);
        window.setView(view);
        window.display();
    }
    return false;
}

bool startMenu(Font font, Text text_start, Text text_sound, Text text_highscore, Text text_exit, Clock& Timer, Clock& rotation)
{
    int menuSelection = 0;
    bool isPressed = false, showHighscoreMenu = false, showInstructions = false, showCredits = false;

    hand.setPosition(1000, 670);

    Font nameFont;
    nameFont.loadFromFile("Fonts//Arial.ttf");

    Text displayName("", nameFont);
    displayName.setCharacterSize(35);
    displayName.setFillColor(Color::Black);
    displayName.setPosition(1040, 950);

    Text displayText;
    displayText.setFont(font);
    displayText.setCharacterSize(65);
    displayText.setFillColor(Color::White);
    displayText.setPosition(50, 900);

    Text error;
    error.setFont(font);
    error.setCharacterSize(60);
    error.setFillColor(Color::Red);
    error.setPosition(50, 900);
    error.setString("");

    Text cursor("|", nameFont);
    cursor.setCharacterSize(30);
    cursor.setFillColor(Color::Black);
    cursor.setPosition(displayName.getPosition().x, displayName.getPosition().y - 2);
    bool showCursor = true;
    Clock cursorClock;

    while (window.isOpen())
    {
        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
            {
                window.close();
            }

            if (showWelcome)
            {
                background_music.stop();
                window.clear();
                window.draw(welcome);
                window.display();

                RectangleShape innerBar(Vector2f(0, 100));
                innerBar.setPosition(365.75, 782.75);
                innerBar.setFillColor(Color::White);

                int progress = 0;
                const int maxProgress = 1200;
                Clock progressClock;

                while (progress < maxProgress)
                {
                    if (progressClock.getElapsedTime().asMilliseconds() > 10)
                    {
                        progress += 15;
                        innerBar.setSize(Vector2f(progress, 100));
                        progressClock.restart();
                    }

                    window.clear();
                    window.draw(welcome);
                    window.draw(innerBar);
                    window.display();
                }

                showWelcome = false;
            }

            if (!showStartMenu)
            {
                background_music.play();
                if (event.type == Event::TextEntered)
                {
                    if (event.text.unicode == '\b' && !userName.empty())
                    {
                        userName.pop_back();
                    }

                    else if (event.text.unicode >= 'a' && event.text.unicode <= 'z' ||
                        event.text.unicode >= 'A' && event.text.unicode <= 'Z' ||
                        event.text.unicode >= '0' && event.text.unicode <= '9')
                    {
                        userName += static_cast<char>(event.text.unicode);
                    }
                }

                displayText.setString("Welcome " + userName + " :)");

                if (event.type == Event::KeyPressed && event.key.code == Keyboard::Enter)
                {
                    if (userName.empty())
                    {
                        error.setString("You must enter your name!");
                    }

                    else
                    {
                        showStartMenu = true;
                        error.setString("");
                    }
                }
            }

            else
            {
                if (event.type == Event::KeyPressed && !isPressed && !showHighscoreMenu && !showInstructions && !showCredits)
                {
                    isPressed = true;
                    if (event.key.code == Keyboard::Up)
                    {
                        menu_change.play();
                        menuSelection = (menuSelection - 1 + 6) % 6;
                        hand.setPosition(1000, 670 + 60 * menuSelection);
                    }

                    if (event.key.code == Keyboard::Down)
                    {
                        menu_change.play();
                        menuSelection = (menuSelection + 1) % 6;
                        hand.setPosition(1000, 660 + 60 * menuSelection);
                    }

                    if (event.key.code == Keyboard::Enter)
                    {
                        menu_choose.play();
                        if (menuSelection == 0)
                        {
                            Timer.restart();
                            FillTime.restart();
                            rotation.restart();
                            return true;
                        }

                        if (menuSelection == 1)
                        {
                            menu.setPosition(500, 200);
                            soundOptions(font);
                            hand.setPosition(1000, 660 + 60 * menuSelection);
                        }

                        if (menuSelection == 2)
                        {
                            showHighscoreMenu = true;
                        }

                        if (menuSelection == 3)
                        {
                            showInstructions = true;
                        }

                        if (menuSelection == 4)
                        {
                            showCredits = true;
                        }

                        else if (menuSelection == 5)
                        {
                            background_music.stop();
                            window.clear();
                            sound_bye.play();
                            window.draw(thankyou);
                            window.display();
                            sleep(seconds(2));
                            window.close();
                        }
                    }
                }

                if (event.type == Event::KeyReleased)
                {
                    isPressed = false;
                }
            }
        }

        if (cursorClock.getElapsedTime().asSeconds() > 0.5f)
        {
            showCursor = !showCursor;
            cursorClock.restart();
        }

        float cursorX = displayName.getPosition().x + displayName.getLocalBounds().width;
        cursor.setPosition(cursorX, displayName.getPosition().y + 2);
        window.setView(window.getDefaultView());

        window.clear();
        window.draw(interface);
        window.draw(enterName);
        displayName.setString(userName);
        window.draw(displayName);

        if (Keyboard::isKeyPressed(Keyboard::Escape))
        {
            showHighscoreMenu = false;
            showInstructions = false;
            showCredits = false;
        }

        if (showHighscoreMenu)
        {
            window.clear();
            highscoreMenu.setTexture(tex_highscoreMenu);
            highscoreMenu.setPosition(650, 0);
            highscoreMenu.setScale(2, 2.2);

            Text text;
            text.setFont(font);
            text.setCharacterSize(50);
            text.setFillColor(Color::Black);
            text.setPosition(700, 180);
            text.setString(copyUserScore());

            for (auto& bg : backgrounds)
            {
                if (bg.level == 1)
                {
                    bg.setPosition(0, 0);
                    window.draw(bg.sprite);
                    break;
                }
            }

            window.draw(highscoreMenu);
            window.draw(text);
            window.display();
            continue;
        }

        if (showInstructions)
        {
            window.clear();
            window.draw(instructions);
            window.display();
            continue;
        }

        if (showCredits)
        {
            window.clear();
            window.draw(credits);
            window.display();
            continue;
        }

        if (!showStartMenu && showCursor)
        {
            window.draw(cursor);
        }

        heads();

        if (showStartMenu)
        {
            text_start.setFillColor(menuSelection == 0 ? Color::Red : Color::Black);
            text_start.setOutlineColor(menuSelection == 0 ? Color::Yellow : Color::Transparent);
            text_start.setOutlineThickness(menuSelection == 0 ? 5 : 0);

            text_sound.setFillColor(menuSelection == 1 ? Color::Red : Color::Black);
            text_sound.setOutlineColor(menuSelection == 1 ? Color::Yellow : Color::Transparent);
            text_sound.setOutlineThickness(menuSelection == 1 ? 5 : 0);
            text_sound.setPosition(1082, 725);

            text_highscore.setFillColor(menuSelection == 2 ? Color::Red : Color::Black);
            text_highscore.setOutlineColor(menuSelection == 2 ? Color::Yellow : Color::Transparent);
            text_highscore.setOutlineThickness(menuSelection == 2 ? 5 : 0);
            text_highscore.setPosition(1082, 780);

            Text text_instructions("INSTRUCTIONS", font);
            text_instructions.setCharacterSize(40);
            text_instructions.setFillColor(menuSelection == 3 ? Color::Red : Color::Black);
            text_instructions.setOutlineColor(menuSelection == 3 ? Color::Yellow : Color::Transparent);
            text_instructions.setOutlineThickness(menuSelection == 3 ? 5 : 0);
            text_instructions.setPosition(1082, 835);

            Text text_credits("CREDITS", font);
            text_credits.setCharacterSize(40);
            text_credits.setFillColor(menuSelection == 4 ? Color::Red : Color::Black);
            text_credits.setOutlineColor(menuSelection == 4 ? Color::Yellow : Color::Transparent);
            text_credits.setOutlineThickness(menuSelection == 4 ? 5 : 0);
            text_credits.setPosition(1082, 890);

            text_exit.setFillColor(menuSelection == 5 ? Color::Red : Color::Black);
            text_exit.setOutlineColor(menuSelection == 5 ? Color::Yellow : Color::Transparent);
            text_exit.setOutlineThickness(menuSelection == 5 ? 5 : 0);
            text_exit.setCharacterSize(40);
            text_exit.setPosition(1082, 950);

            window.clear();
            window.draw(interface);
            window.draw(text_start);
            window.draw(text_sound);
            window.draw(text_highscore);
            window.draw(text_instructions);
            window.draw(text_credits);
            window.draw(text_exit);
            window.draw(hand);
            heads();
            window.draw(displayText);
        }

        else
        {
            if (!error.getString().isEmpty())
            {
                window.draw(error);
            }
        }

        window.display();
    }
    return false;
}

bool pauseMenu(Players& player, Font font, Text text_sound, Text text_exit, Text text_start, vector<BLOCKS>& blocksList, Text text_play_again, Text& Score, bool& viewpaused, bool& win, Clock& rotation, Clock& viewtimer, Clock& Timer, RectangleShape& OclockFill, vector<Feature>& featuresList)
{
    int menuSelection = 0;
    bool isPressed = false;

    menu.setPosition(500, 200);
    hand.setPosition(600, 320);

    Text text_resume("RESUME", font);
    text_resume.setCharacterSize(60);
    text_resume.setFillColor(Color::Black);
    text_resume.setPosition(700, 300);

    while (window.isOpen())
    {
        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
            {
                window.close();
            }

            if (event.type == Event::KeyPressed && !isPressed)
            {
                isPressed = true;
                if (event.key.code == Keyboard::Up)
                {
                    menu_change.play();
                    menuSelection = (menuSelection - 1 + 4) % 4;
                    hand.setPosition(600, 320 + 100 * menuSelection);
                }

                if (event.key.code == Keyboard::Down)
                {
                    menu_change.play();
                    menuSelection = (menuSelection + 1) % 4;
                    hand.setPosition(600, 320 + 100 * menuSelection);
                }

                if (event.key.code == Keyboard::Enter)
                {
                    menu_choose.play();
                    if (menuSelection == 0)
                    {
                        viewpaused = true;
                        viewtimer.restart();
                        window.setView(view);
                        return true;
                    }

                    else if (menuSelection == 1)
                    {
                        reset(player, blocksList, Score, viewpaused, win, rotation, viewtimer, Timer, OclockFill, featuresList);
                        return true;
                    }

                    else if (menuSelection == 2)
                    {
                        soundOptions(font);
                    }

                    else if (menuSelection == 3)
                    {
                        showWelcome = false;
                        reset(player, blocksList, Score, viewpaused, win, rotation, viewtimer, Timer, OclockFill, featuresList);
                        return false;
                    }
                }
            }

            if (event.type == Event::KeyReleased)
            {
                isPressed = false;
            }
        }

        window.setView(window.getDefaultView());

        text_resume.setFillColor(menuSelection == 0 ? Color::Red : Color::Black);
        text_resume.setOutlineColor(menuSelection == 0 ? Color::Yellow : Color::Transparent);
        text_resume.setOutlineThickness(menuSelection == 0 ? 2 : 0);

        text_play_again.setFillColor(menuSelection == 1 ? Color::Red : Color::Black);
        text_play_again.setOutlineColor(menuSelection == 1 ? Color::Yellow : Color::Transparent);
        text_play_again.setOutlineThickness(menuSelection == 1 ? 2 : 0);
        text_play_again.setPosition(700, 410);

        text_sound.setFillColor(menuSelection == 2 ? Color::Red : Color::Black);
        text_sound.setOutlineColor(menuSelection == 2 ? Color::Yellow : Color::Transparent);
        text_sound.setOutlineThickness(menuSelection == 2 ? 2 : 0);
        text_sound.setCharacterSize(60);
        text_sound.setPosition(700, 510);

        text_exit.setFillColor(menuSelection == 3 ? Color::Red : Color::Black);
        text_exit.setOutlineColor(menuSelection == 3 ? Color::Yellow : Color::Transparent);
        text_exit.setOutlineThickness(menuSelection == 3 ? 2 : 0);
        text_exit.setCharacterSize(60);
        text_exit.setPosition(700, 610);

        window.clear();

        for (auto& bg : backgrounds)
        {
            if (bg.level == 1)
            {
                bg.setPosition(0, 0);
                window.draw(bg.sprite);
                break;
            }
        }

        window.draw(menu);
        window.draw(text_resume);
        window.draw(text_play_again);
        window.draw(text_sound);
        window.draw(text_exit);
        window.draw(hand);
        window.display();
    }
    return false;
}

bool gameOver(Players& player, Font font, Text text_play_again, Text text_exit, vector<BLOCKS>& blocksList, Text& Score, Text& timerText, bool& viewpaused, bool win, Clock& rotation, Clock& viewtimer, Clock& Timer, RectangleShape& OclockFill, vector<Feature>& featuresList)
{
    int menuSelection = 0;
    bool isPressed = false;

    hand.setPosition(700, 650);
    menu.setPosition(500, 300);

    View gameView = window.getView();

    Text Floor("Floors: " + to_string(floors), font);
    Floor.setCharacterSize(60);
    Floor.setFillColor(Color::White);

    updateOrAddUserScore(userName, score); 
    saveUserData();

    while (window.isOpen())
    {
        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
            {
                window.close();
            }

            if (event.type == Event::KeyPressed && !isPressed)
            {
                isPressed = true;
                if (event.key.code == Keyboard::Up)
                {
                    menu_change.play();
                    menuSelection = (menuSelection - 1 + 2) % 2;
                    hand.setPosition(700, 650 + 100 * menuSelection);
                }

                if (event.key.code == Keyboard::Down)
                {
                    menu_change.play();
                    menuSelection = (menuSelection + 1) % 2;
                    hand.setPosition(700, 650 + 100 * menuSelection);
                }

                if (event.key.code == Keyboard::Enter)
                {
                    menu_choose.play();
                    if (menuSelection == 0)
                    {
                        reset(player, blocksList, Score, viewpaused, win, rotation, viewtimer, Timer, OclockFill, featuresList);
                        return true;
                    }
                    else if (menuSelection == 1)
                    {
                        showWelcome = false;
                        reset(player, blocksList, Score, viewpaused, win, rotation, viewtimer, Timer, OclockFill, featuresList);
                        return false;
                    }
                }
            }

            if (event.type == Event::KeyReleased)
            {
                isPressed = false;
            }
        }

        Score.setFillColor(Color::Black);
        Score.setPosition(780, 350);

        Floor.setFillColor(Color::Black);
        Floor.setPosition(780, 450);

        timerText.setFillColor(Color::Black);
        timerText.setPosition(780, 550);

        text_play_again.setFillColor(menuSelection == 0 ? Color::Red : Color::Black);
        text_play_again.setOutlineColor(menuSelection == 0 ? Color::Yellow : Color::Transparent);
        text_play_again.setOutlineThickness(menuSelection == 0 ? 2 : 0);
        text_play_again.setCharacterSize(55);
        text_play_again.setPosition(780, 650);

        text_exit.setFillColor(menuSelection == 1 ? Color::Red : Color::Black);
        text_exit.setOutlineColor(menuSelection == 1 ? Color::Yellow : Color::Transparent);
        text_exit.setOutlineThickness(menuSelection == 1 ? 2 : 0);
        text_exit.setCharacterSize(60);
        text_exit.setPosition(780, 750);

        window.clear();
        window.setView(gameView);

        for (auto& bg : backgrounds)
        {
            if (bg.level == currentLevel)
            {
                window.draw(bg.sprite);
            }
        }

        for (const auto& block : blocksList)
        {
            window.draw(block.blocksSprite);
        }

        for (int i = 0; i < NUM_BARRIERS; ++i)
        {
            if (barriers[i]->spawned)
            {
                window.draw(barriers[i]->sprite);
            }
        }

        window.draw(wall);
        window.draw(wall2);
        window.draw(player.sprite);
        window.setView(window.getDefaultView());
        window.draw(gameover);
        window.draw(menu);
        window.draw(text_play_again);
        window.draw(text_exit);
        window.draw(Score);
        window.draw(Floor);
        window.draw(timerText);
        window.draw(hand);
        window.display();
    }
    return false;
}

bool winMenu(Players& player, Font font, Text text_play_again, Text text_exit, vector<BLOCKS>& blocksList, Text& Score, Text& timerText, vector<Feature>& featuresList)
{
    sound_winner.play();
    sound_cheer.play();

    int menuSelection = 0;
    bool isPressed = false;

    View gameView = window.getView();

    hand.setPosition(700, 650);
    menu.setPosition(500, 300);
    newstage.setPosition(0, -550);

    Text Floor("Floors: " + to_string(floors), font);
    Floor.setCharacterSize(60);
    Floor.setFillColor(Color::White);

    updateOrAddUserScore(userName, score);
    saveUserData();

    while (window.isOpen())
    {
        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
            {
                window.close();
            }

            if (event.type == Event::KeyPressed && !isPressed)
            {
                isPressed = true;
                if (event.key.code == Keyboard::Up)
                {
                    menu_change.play();
                    menuSelection = (menuSelection - 1 + 2) % 2;
                    hand.setPosition(700, 650 + 100 * menuSelection);
                }

                if (event.key.code == Keyboard::Down)
                {
                    menu_change.play();
                    menuSelection = (menuSelection + 1) % 2;
                    hand.setPosition(700, 650 + 100 * menuSelection);
                }

                if (event.key.code == Keyboard::Enter)
                {
                    menu_choose.play();
                    if (menuSelection == 0)
                    {
                        return true;
                    }
                    else if (menuSelection == 1)
                    {
                        showWelcome = false;
                        return false;
                    }
                }
            }

            if (event.type == Event::KeyReleased)
            {
                isPressed = false;
            }
        }

        Score.setFillColor(Color::Black);
        Score.setPosition(780, 350);

        Floor.setFillColor(Color::Black);
        Floor.setPosition(780, 450);

        timerText.setFillColor(Color::Black);
        timerText.setPosition(780, 550);

        text_play_again.setFillColor(menuSelection == 0 ? Color::Red : Color::Black);
        text_play_again.setOutlineColor(menuSelection == 0 ? Color::Yellow : Color::Transparent);
        text_play_again.setOutlineThickness(menuSelection == 0 ? 2 : 0);
        text_play_again.setPosition(780, 650);

        text_exit.setFillColor(menuSelection == 1 ? Color::Red : Color::Black);
        text_exit.setOutlineColor(menuSelection == 1 ? Color::Yellow : Color::Transparent);
        text_exit.setOutlineThickness(menuSelection == 1 ? 2 : 0);
        text_exit.setCharacterSize(60);
        text_exit.setPosition(780, 750);

        window.clear();
        window.setView(gameView);

        for (auto& bg : backgrounds)
        {
            if (bg.level == currentLevel)
            {
                window.draw(bg.sprite);
            }
        }

        for (const auto& block : blocksList)
        {
            window.draw(block.blocksSprite);
        }

        window.draw(wall);
        window.draw(wall2);
        window.draw(player.sprite);
        window.setView(window.getDefaultView());
        window.draw(winner);
        window.draw(menu);
        window.draw(text_play_again);
        window.draw(text_exit);
        window.draw(Score);
        window.draw(Floor);
        window.draw(timerText);
        window.draw(hand);
        window.draw(newstage);
        newstage.move(0, 10);
        window.display();
    }
    return false;
}

void featureCollision(vector<BLOCKS>& blocksList, Players& player, Text& Score, Text& text_skipped, Font& font, Text& text_play_again, Text& text_exit, Text& text_start, Text& text_sound, Text& text_highscore, Text& timerText, bool& viewpaused, bool win, Clock& rotation, Clock& viewtimer, Clock& Timer, RectangleShape& OclockFill, vector<Feature>& featuresList)
{
    for (auto& feature : featuresList)
    {
        if (feature.active && player.sprite.getGlobalBounds().intersects(feature.sprite.getGlobalBounds()))
        {
            feature.active = false;
            if (feature.type == "bomb")
            {
                sound_bomb.play();
                showBoom = true;
                boomPosition = feature.sprite.getPosition();
                boomTime = 0.0f;
                blocksList.clear();
                cracks.setPosition(boomPosition.x, boomPosition.y - 50);
                cracks.setScale(1.5f, 1.5f);
            }

            else if (feature.type == "superJump")
            {
                player.hasSuperJump = true;
                sound_superJump.play();
            }

            else if (feature.type == "extraLife")
            {
                sound_extraLife.play();
                if (lives < max_lives)
                    lives++;
            }

            else if (feature.type == "loseLife")
            {
                sound_loseLife.play();
                if (lives > 1)
                {
                    lives--;
                }

                else
                {
                    sound_gameover.play();
                    bool resumeGame = gameOver(player, font, text_play_again, text_exit, blocksList, Score, timerText, viewpaused, win, rotation, viewtimer, Timer, OclockFill, featuresList);
                    if (!resumeGame)
                    {
                        startMenu(font, text_start, text_sound, text_highscore, text_exit, Timer, rotation);
                    }
                    win = false;
                }
            }

            else if (feature.type == "widerBlocks")
            {
                sound_widerBlocks.play();
                widerBlocksActive = true;
                widerBlocksTimer.restart();

                for (auto& block : blocksList)
                {
                    block.xscale = (Xright - Xleft) / static_cast<float>(block.blocksSprite.getTexture()->getSize().x);
                    block.blocksSprite.setOrigin(0, 0);
                    block.blocksSprite.setScale(block.xscale + 10, 1);
                    block.blocksSprite.setPosition(Xleft - 150, block.blocksSprite.getPosition().y);
                    block.isMoving = false;
                }
            }
        }
    }
}

void barrierCollision(vector<BLOCKS>& blocksList, Players& player)
{
    int currentBlockIndex = -1;

    for (int i = 0; i < NUM_BARRIERS; ++i)
    {
        Barrier* barrier = barriers[i];
        if (!barrier->spawned && currentLevel >= barrier->targetLevel)
        {
            float highestY = HEIGHT;
            for (auto it = blocksList.rbegin(); it != blocksList.rend(); ++it)
            {
                if (it->level == barrier->targetLevel)
                {
                    highestY = min(highestY, it->blocksSprite.getPosition().y);
                    break;
                }
            }

            barrier->setPosition(100, highestY - 300);
            barrier->spawned = true;
        }

        if (barrier->spawned && player.sprite.getGlobalBounds().intersects(barrier->sprite.getGlobalBounds()))
        {
            if (player.velocity_y > 0 && (player.sprite.getPosition().y + player.sprite.getGlobalBounds().height - 50 <= barrier->sprite.getPosition().y))
            {
                sound_rotate.stop();
                sound_gonna_fall.stop();
                lastSafePosition = player.sprite.getPosition();
                lastSafePosition.y = barrier->sprite.getPosition().y - player.sprite.getGlobalBounds().height + 12;
                player.sprite.setPosition(player.sprite.getPosition().x, barrier->sprite.getPosition().y - player.sprite.getGlobalBounds().height + 12);
                isGround = true;
                player.velocity_y = 0;
                currentBlock = nullptr;
                currentBlockIndex = -1;
                lastBlockIndex = -1;
                if (i == 0) player.onBarrier = true;
                else if (i == 1) player.onBarrier2 = true;
            }
        }

        // Check if player has passed above the barrier
        else if (barrier->spawned && player.sprite.getPosition().y + player.sprite.getGlobalBounds().height < barrier->sprite.getPosition().y)
        {
            if (i == 0 && currentLevel == 1)
            {
                sound_cheer.play();
                currentLevel = 2;
                transitionClock.restart();
            }

            else if (i == 1 && currentLevel == 2)
            {
                sound_cheer.play();
                currentLevel = 3;
                transitionClock.restart();
            }
        }
    }
}

void blocksCollision(vector<BLOCKS>& blocksList, Players& player, Text& Score, Text& text_skipped, Font& font, Text& text_play_again, Text& text_exit, Text& text_start, Text& text_sound, Text& text_highscore,  Text& timerText, bool& viewpaused, bool win, Clock& rotation, Clock& viewtimer, Clock& Timer, vector<Feature>& featuresList)
{
    static int blockLandingCount = 0; // Tracks number of block landings
    static int targetBlockCount = 0;  // Random number of blocks before next bomb
    static bool bombPending = false;  // Indicates if a bomb spawn is pending

    bool landed = false;
    int currentBlockIndex = -1;

    for (size_t i = 0; i < blocksList.size(); ++i)
    {
        auto& block = blocksList[i];
        if (player.sprite.getGlobalBounds().intersects(block.blocksSprite.getGlobalBounds()))
        {
            if (player.velocity_y > 0 && (player.sprite.getPosition().y + player.sprite.getGlobalBounds().height - 50 <= block.blocksSprite.getPosition().y))
            {
                lastSafePosition = player.sprite.getPosition();
                lastSafePosition.y = block.blocksSprite.getPosition().y - player.sprite.getGlobalBounds().height + 12;
                player.sprite.setPosition(player.sprite.getPosition().x, block.blocksSprite.getPosition().y - player.sprite.getGlobalBounds().height + 12);
                isGround = true;
                sound_rotate.stop();
                sound_gonna_fall.stop();
                currentBlock = &blocksList[i];
                currentBlockIndex = i;
                landed = true;

                // Bomb collision 
                if (block.level == 3 && currentLevel == 3)
                {
                    blockLandingCount++;
                    if (!bombPending)
                    {
                        targetBlockCount = 2 + (rand() % 2);
                        bombPending = true;
                    }

                    if (blockLandingCount >= targetBlockCount)
                    {
                        vector<BLOCKS*> level3BlocksInRange;
                        float playerY = player.sprite.getPosition().y;
                        float minY = playerY - 500;
                        float maxY = playerY - 200;

                        for (size_t j = 0; j < blocksList.size() - 1; ++j)
                        {
                            if (blocksList[j].level == 3 && blocksList[j].blocksSprite.getPosition().y >= minY && blocksList[j].blocksSprite.getPosition().y <= maxY)
                            {
                                level3BlocksInRange.push_back(&blocksList[j]);
                            }
                        }

                        if (!level3BlocksInRange.empty())
                        {
                            BLOCKS* targetBlock = level3BlocksInRange[rand() % level3BlocksInRange.size()];
                            float x = targetBlock->blocksSprite.getPosition().x + (targetBlock->blocksSprite.getGlobalBounds().width / 2);
                            float y = targetBlock->blocksSprite.getPosition().y - 100;
                            featuresList.push_back(Feature(tex_bomb, "bomb", x, y));
                            sound_spark.play();
                        }

                        blockLandingCount = 0;
                        bombPending = false;
                    }
                }

                if (block.level == 2 && currentLevel == 1)
                {
                    currentLevel = 2;
                    transitionClock.restart();
                }

                else if (block.level == 3 && currentLevel == 2)
                {
                    currentLevel = 3;
                    transitionClock.restart();
                }

                if (!block.isIntersected)
                {
                    block.isIntersected = true;
                    floors++;
                    score += (block.level == 1) ? 10 : (block.level == 2) ? 15 : 20;
                    Score.setString("Score: " + to_string(score));
                }

                if (lastBlockIndex != -2 && currentBlockIndex != lastBlockIndex && lastBlockIndex < static_cast<int>(blocksList.size()))
                {
                    int startIndex = min(lastBlockIndex, currentBlockIndex);
                    int endIndex = max(lastBlockIndex, currentBlockIndex);

                    for (int j = startIndex + 1; j < endIndex; j++)
                    {
                        if (!blocksList[j].isIntersected)
                        {
                            blocksList[j].isIntersected = true;
                            nonIntersectedBlocks++;
                            text_skipped.setString(to_string(nonIntersectedBlocks));
                            score += (blocksList[j].level == 1) ? 20 : (block.level == 2) ? 30: 40;
                            floors++;

                            if (nonIntersectedBlocks >= 1)
                            {
                                clockFill = 1.0f;
                                FillTime.restart();
                            }
                        }
                    }
                    Score.setString("Score: " + to_string(score));
                }
                lastBlockIndex = currentBlockIndex;
                player.velocity_y = 0;
            }
        }
    }

    // Handle falling off current block
    if (!landed && currentBlock)
    {
        float blockLeft = currentBlock->blocksSprite.getPosition().x;
        float blockRight = blockLeft + currentBlock->blocksSprite.getGlobalBounds().width;
        float playerLeft = player.sprite.getPosition().x - (player.sprite.getGlobalBounds().width / 2);
        float playerRight = player.sprite.getPosition().x + (player.sprite.getGlobalBounds().width / 2);
        float edgeThreshold = 40.0f;

        bool atLeftEdge = (playerRight > blockLeft && playerRight <= blockLeft + edgeThreshold);
        bool atRightEdge = (playerLeft < blockRight && playerLeft >= blockRight - edgeThreshold);

        if (isGround && (atLeftEdge || atRightEdge))
        {
            if (player.frameIndex != 11)
            {
                player.frameIndex = 11;
                player.sprite.setTextureRect(IntRect(player.frameIndex * 38, 0, 38, 71));
                player.sprite.setScale(atLeftEdge ? -2.4 : 2.4, 2.4);
                sound_gonna_fall.play();
            }
        }

        if (playerRight < blockLeft || playerLeft > blockRight)
        {
            isGround = false;
            sound_gonna_fall.stop();
            currentBlock = nullptr;
        }
    }
}

void drawTransitionEffect(Font font, Vector2f camPos, int currentLevel, float deltaTime, Clock& transitionClock, Barrier* barrier)
{
    float transitionTime = transitionClock.getElapsedTime().asSeconds();
    const float transitionDuration = 1.5f;

    if (transitionTime < transitionDuration)
    {
        float startY = barrier->sprite.getPosition().y - 500;
        float endY = startY + 600.0f;
        newstage.setPosition(0, startY + (transitionTime / transitionDuration) * (endY - startY));

        Text transitionText("Level " + to_string(currentLevel), font);
        transitionText.setCharacterSize(100);
        transitionText.setFillColor(Color::White);
        transitionText.setOutlineColor(Color::Black);
        transitionText.setOutlineThickness(10);
        transitionText.setPosition(camPos.x - transitionText.getGlobalBounds().width / 2, camPos.y - transitionText.getGlobalBounds().height / 2);

        window.draw(transitionText);
        window.draw(newstage);
    }

}

void draw(Players& player, vector<BLOCKS>& blocksList, Text& Score, Font font, Text& timerText, Text& text_skipped, float& deltatime, Clock& rotation, Clock& Timer, RectangleShape& OclockFill, vector<Feature>& featuresList, bool& viewpaused, bool win, Clock& viewtimer, Text text_play_again, Text text_exit, Text text_start, Text text_sound, Text text_highscore)
{
    window.setView(view);

    Vector2f camPos = view.getCenter();
    wall.setPosition(camPos.x - WIDTH / 2, camPos.y - HEIGHT / 2);
    wall2.setPosition(1760 + camPos.x - WIDTH / 2 + 40, camPos.y - HEIGHT / 2);
    Score.setPosition(camPos.x - WIDTH / 2 + 200, camPos.y - HEIGHT / 2);

    float elapsedTime = rotation.getElapsedTime().asSeconds();
    float rotationPeriod = 60.0f;
    float angle = (elapsedTime / rotationPeriod) * 360.0f;

    clockHand.setRotation(angle);

    int time = Timer.getElapsedTime().asSeconds();
    int minutes = time / 60;
    int seconds = time % 60;

    timerText.setPosition(200 + camPos.x - WIDTH / 2, camPos.y - HEIGHT / 2 + 950);
    string timeString = "Time: ";
    timerText.setFillColor(Color::White);

    if (minutes < 10) timeString += "0";
    timeString += to_string(minutes);
    timeString += ":";

    if (seconds < 10) timeString += "0";
    timeString += to_string(seconds);
    timerText.setString(timeString);

    Text levelText("Level: " + to_string(currentLevel), font);
    levelText.setCharacterSize(60);
    levelText.setFillColor(Color::White);
    levelText.setPosition(camPos.x - WIDTH / 2 + 1450, camPos.y - HEIGHT / 2);
    text_skipped.setString(to_string(nonIntersectedBlocks));

    for (auto& bg : backgrounds)
    {
        bg.update(currentLevel, camPos);
        if (bg.level == currentLevel)
        {
            window.draw(bg.sprite);
        }
    }

    if (currentLevel >= 2 && currentLevel <= 3)
    {
        Barrier* barrier = (currentLevel == 2) ? barriers[0] : barriers[1];
        drawTransitionEffect(font, camPos, currentLevel, deltatime, transitionClock, barrier);
    }

    for (int i = 0; i < lives; i++)
    {
        ice[i].setPosition(camPos.x - WIDTH / 2 + 800 + (i * 100), camPos.y - HEIGHT / 2 - 10);
        window.draw(ice[i]);
    }

    for (int i = 0; i < NUM_BARRIERS; ++i)
    {
        if (barriers[i]->spawned)
        {
            window.draw(barriers[i]->sprite);
        }
    }

    for (auto& block : blocksList)
    {
        window.draw(block.blocksSprite);
    }

    for (auto& feature : featuresList)
    {
        if (feature.active)
        {
            window.draw(feature.sprite);
        }
    }

    if (showhurryUp)
    {
        hurryUpTime += deltatime;

        Vector2f pos = hurryUp.getPosition();
        pos.y -= 30.0f * deltatime;
        hurryUp.setPosition(pos);

        int fadeTime = static_cast<int>(255 * (1.0f - hurryUpTime / 1.0f));

        float scale = 2.0f + 0.3f * hurryUpTime;
        hurryUp.setScale(scale, scale);

        if (hurryUpTime >= 1.0f)
        {
            showhurryUp = false;
        }

        window.draw(hurryUp);
    }

    if (showBoom)
    {
        showSweet = false;
        sound_sweet.stop();

        featuresList.clear();
        boomTime += deltatime;

        Vector2f boomPos = boomPosition;
        float boomScale = 5.0f + 1.0f * boomTime;
        boom.setPosition(boomPos.x - boom.getGlobalBounds().width / 2, boomPos.y - boom.getGlobalBounds().height / 2);
        boom.setScale(boomScale, boomScale);
        int boomFade = static_cast<int>(255 * (1.0f - boomTime / 1.0f));
        if (boomFade < 0) boomFade = 0;
        boom.setColor(Color(255, 255, 255, boomFade));
        window.draw(boom);

        Vector2f windowCenter = window.mapPixelToCoords(Vector2i(window.getSize().x / 2, window.getSize().y / 2));
        cracks.setPosition(windowCenter.x - cracks.getGlobalBounds().width / 2, windowCenter.y - cracks.getGlobalBounds().height / 2);
        float cracksScale = 1.5f + 0.5f * boomTime;
        cracks.setScale(cracksScale, cracksScale);
        int cracksFade = static_cast<int>(255 * (1.0f - boomTime / 1.5f));
        if (cracksFade < 0) cracksFade = 0;
        cracks.setColor(Color(255, 255, 255, cracksFade));
        window.draw(cracks);
        lives = 0;

        if (boomTime >= 1.75f)
        {
            showBoom = false;
        }
    }

    window.draw(ground);
    window.draw(wall);
    window.draw(wall2);
    window.draw(player.sprite);
    window.draw(Score);
    window.draw(levelText);
    window.draw(timerText);

    if (showSweet)
    {
        sweetTime += deltatime;
        Vector2f pos = sweet.getPosition();
        pos.y -= 30.0f * deltatime;
        sweet.setPosition(pos);

        int fadeTime = static_cast<int>(255 * (1.0f - sweetTime / 1.0f));
        float scale = 2.0f + 0.3f * sweetTime;
        sweet.setScale(scale, scale);

        if (sweetTime >= 1.0f)
        {
            showSweet = false;
        }

        window.draw(sweet);
    }

    updateOrAddUserScore(userName, score);

    if (showHighScore)
    {
        showSweet = false;
        sound_sweet.stop();
        showhurryUp = false;
        sound_hurryUp.stop();
        highScoreTime += deltatime;
        Vector2f pos = highScore.getPosition();
        pos.y -= 500.0f * deltatime;
        pos.x = 500;
        highScore.setPosition(pos);

        int fadeTime = static_cast<int>(255 * (1.0f - highScoreTime / 1.0f));
        float scale = 1.0f + 0.2f * highScoreTime;
        highScore.setScale(scale, scale);

        if (highScoreTime >= 3.0f)
        {
            showHighScore = false;
        }

        window.draw(highScore);
    }

    window.setView(window.getDefaultView());
    window.draw(wallClock);
    window.draw(star);
    window.draw(clockHand);
    OclockFill.setSize(Vector2f(26, 152 * clockFill));
    OclockFill.setPosition(49, 380 - 152 * clockFill);
    window.draw(OclockFill);
    window.draw(text_skipped);
    window.setView(view);
}

int main()
{
    window.setFramerateLimit(60);
    initializeObject();
    loadUserData();

    background_music.setLoop(true);
    background_music.play();

    Font font;
    font.loadFromFile("Fonts//RushDriver-Italic.otf");

    Text text_start("START", font);
    text_start.setCharacterSize(40);
    text_start.setFillColor(Color::Black);
    text_start.setPosition(1082, 670);

    Text text_sound("SOUND", font);
    text_sound.setCharacterSize(40);
    text_sound.setFillColor(Color::Black);

    Text text_highscore("HIGHSCORE", font);
    text_highscore.setCharacterSize(40);
    text_highscore.setFillColor(Color::Black);

    Text text_exit("EXIT", font);
    text_exit.setFillColor(Color::Black);

    Text text_play_again("PLAY AGAIN", font);
    text_play_again.setCharacterSize(57);
    text_play_again.setFillColor(Color::Black);
    Text Score("Score: " + to_string(score), font);

    Score.setCharacterSize(60);
    Score.setFillColor(Color::White);

    Text timerText;
    timerText.setFont(font);
    timerText.setFillColor(Color::White);
    timerText.setCharacterSize(60);
    timerText.setFillColor(Color::White);

    Text text_skipped;
    text_skipped.setFont(font);
    text_skipped.setCharacterSize(31);
    text_skipped.setFillColor(Color::Black);
    text_skipped.setPosition(45, 390);

    RectangleShape OclockFill;
    OclockFill.setSize(Vector2f(26, 152));
    OclockFill.setPosition(49, 380);
    OclockFill.setFillColor(Color::Red);
    Clock viewtimer, Timer, rotation;

    bool viewpaused = false, win = false, showSuperJump = false;
    bool start_game = startMenu(font, text_start, text_sound, text_highscore, text_exit, Timer, rotation);
    const int max_levels = 3;

    if (start_game)
    {
        Players player;
        Clock clock;
        vector<BLOCKS> blocksList;
        vector<Feature> featuresList;
        random_device RANDOM;
        srand(RANDOM());

        player.sprite.setOrigin(player.sprite.getLocalBounds().width / 2, 0);

        generationBlocks(blocksList, player, 1);
        generationBlocks(blocksList, player, 2);
        generationBlocks(blocksList, player, 3);

        view.setSize(WIDTH,HEIGHT);
        view.setCenter(Vector2f(960, 520));
        adjustViewAspectRatio(WIDTH/ HEIGHT);
        window.setView(view);

        while (window.isOpen())
        {
            float deltatime = clock.restart().asSeconds();
            if (!viewpaused && player.sprite.getPosition().y < HEIGHT / 7)
            {
                view_movement(deltatime, currentLevel);
                adjustViewAspectRatio(WIDTH / HEIGHT);
                generationFeatures(blocksList, player, Score, featuresList, currentLevel);
                window.setView(view);
            }

            if (viewpaused && viewtimer.getElapsedTime().asSeconds() >= VIEW_PAUSE_DURATION)
            {
                viewpaused = false;
            }

            Event event;
            while (window.pollEvent(event))
            {
                if (event.type == Event::Closed)
                {
                    window.close();
                }
            }

            handleWiderBlocksEffect(blocksList, currentLevel, player);

            // Pause Menu
            if (Keyboard::isKeyPressed(Keyboard::Escape) || Keyboard::isKeyPressed(Keyboard::P))
            {
                bool resumeGame = pauseMenu(player, font, text_sound, text_exit, text_start, blocksList, text_play_again, Score, viewpaused, win, rotation, viewtimer, Timer, OclockFill, featuresList);
                if (!resumeGame)
                {
                    startMenu(font, text_start, text_sound, text_highscore, text_exit, Timer, rotation);
                }
            }

            // Game Over 
            float viewBottom = view.getCenter().y + HEIGHT / 2;

            if (player.sprite.getPosition().y > viewBottom)
            {
                sound_rotate.stop();
                sound_spark.stop();
                sound_gonna_fall.stop();
                if (lives > 1)
                {
                    lives--;
                    if (lives == 1)
                    {
                        sound_hurryUp.play();
                        hurryUp.setPosition(view.getCenter().x - 350, view.getCenter().y + 300);
                        hurryUp.setScale(1.0f, 1.0f);
                        hurryUpTime = 0.0f;
                        showhurryUp = true;
                    }

                    player.sprite.setPosition(lastSafePosition);
                    player.velocity_x = 0;
                    player.velocity_y = 0;
                    player.frameIndex = 0;
                    showHighScore = false;
                    player.sprite.setTextureRect(IntRect(0, 0, 38, 71));
                    view.setCenter(Vector2f(960, lastSafePosition.y + 100));
                    adjustViewAspectRatio(WIDTH / HEIGHT);
                    window.setView(view);
                    viewpaused = true;
                    viewtimer.restart();
                }

                else
                {
                    sound_gameover.play();
                    bool resumeGame = gameOver(player, font, text_play_again, text_exit, blocksList, Score, timerText, viewpaused, win, rotation, viewtimer, Timer, OclockFill, featuresList);
                    if (!resumeGame)
                    {
                        startMenu(font, text_start, text_sound, text_highscore, text_exit, Timer, rotation);
                    }
                    win = false;
                }
            }

            player.playerRotation(deltatime, blocksList, currentLevel, viewpaused, viewtimer, win);

            // Ground Intersection
            if (player.sprite.getGlobalBounds().intersects(ground.getGlobalBounds()))
            {
                sound_rotate.stop();
                sound_gonna_fall.stop();
                player.sprite.setPosition(player.sprite.getPosition().x, ground.getPosition().y + 15 - player.sprite.getGlobalBounds().height);
                isGround = true;
                player.velocity_y = 0;

                if (player.velocity_x == 0)
                {
                    player.frameIndex = 0;
                    player.sprite.setTextureRect(IntRect(0, 0, 38, 71));
                }
            }

            // Walls Intersection
            if (player.sprite.getGlobalBounds().intersects(wall.getGlobalBounds()))
            {
                player.sprite.setPosition(wall.getGlobalBounds().left + wall.getGlobalBounds().width + 50, player.sprite.getPosition().y);
            }
            else if (player.sprite.getGlobalBounds().intersects(wall2.getGlobalBounds()))
            {
                player.sprite.setPosition(wall2.getGlobalBounds().left - player.sprite.getGlobalBounds().width + 50, player.sprite.getPosition().y);
            }

            // Collisions 
            barrierCollision(blocksList, player);
            featureCollision(blocksList, player, Score, text_skipped, font, text_play_again, text_exit, text_start, text_sound, text_highscore,  timerText, viewpaused, win, rotation, viewtimer, Timer, OclockFill, featuresList);
            blocksCollision(blocksList, player, Score, text_skipped, font, text_play_again, text_exit, text_start, text_sound, text_highscore,  timerText, viewpaused, win, rotation, viewtimer, Timer, featuresList);
            
            clocktimer(deltatime);

            if (!viewpaused)
            {
                updateMovingBlocks(blocksList, deltatime);
            }

            // Win Condition
            if (currentLevel == max_levels && isGround)
            {
                for (auto it = blocksList.rbegin(); it != blocksList.rend(); ++it)
                {
                    if (it->level == 3)
                    {
                        if (player.sprite.getGlobalBounds().intersects(it->blocksSprite.getGlobalBounds()) &&
                            player.sprite.getPosition().y + player.sprite.getGlobalBounds().height - 50 <= it->blocksSprite.getPosition().y)
                        {
                            player.sprite.setPosition(it->blocksSprite.getPosition().x + it->blocksSprite.getGlobalBounds().width / 2,
                                it->blocksSprite.getPosition().y - player.sprite.getGlobalBounds().height + 12);
                            win = true;
                            isGround = true;

                            bool resumeGame = winMenu(player, font, text_play_again, text_exit, blocksList, Score, timerText, featuresList);
                            if (!resumeGame)
                            {
                                reset(player, blocksList, Score, viewpaused, win, rotation, viewtimer, Timer, OclockFill, featuresList);
                                startMenu(font, text_start, text_sound, text_highscore, text_exit, Timer, rotation);
                            }
                            else
                            {
                                reset(player, blocksList, Score, viewpaused, win, rotation, viewtimer, Timer, OclockFill, featuresList);
                            }
                            break;
                        }
                        break;
                    }
                }
            }

            player.jump(deltatime);
            player.sprite.move(player.velocity_x * deltatime, player.velocity_y * deltatime);
            player.handleMovement(deltatime);

            window.clear();
            draw(player, blocksList, Score, font, timerText, text_skipped, deltatime, rotation, Timer, OclockFill,  featuresList,
                viewpaused, win, viewtimer, text_play_again, text_exit, text_start, text_sound, text_highscore);

            window.display();
        }

        for (int i = 0; i < NUM_BARRIERS; ++i)
        {
            delete barriers[i];
        }
    }
    return 0;
}
