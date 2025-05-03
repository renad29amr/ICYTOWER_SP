#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <string>
#include <sstream>
#include <fstream>

using namespace sf;
using namespace std;

const int WIDTH = 1000, HEIGHT = 1000;
const float VIEW_PAUSE_DURATION = 0.1, VIEW_SPEED_1 = -550, VIEW_SPEED_2 = -500;
bool isGround = false , showStartMenu = false;

RenderWindow window(VideoMode(WIDTH, HEIGHT), "Icy Tower SFML");
View view;

// --->blocks constants<---
const int blocksNum = 15;
int score = 0, floors = 0, Xleft, Xright, nonIntersectedBlocks = 0;

// ----->Level variables<-----
int currentLevel = 1;
bool barrierSpawned = false, doneLevel1 = false, level2Generated = false;

// --->Life time<---
const int max_lives = 3;
int lives = max_lives;
Vector2f lastSafePosition; 

// --->Clocks<----
Clock transitionClock;
static Clock FillTime;
float clockFill = 0.0f;

// Textures
Texture tex_background, tex_ground, tex_wall_right, tex_wall_left, tex_player, tex_block, tex_interface, tex_hand, tex_pauseMenu, tex_heads, tex_gameover,
tex_enterName, tex_winner, tex_newstage, tex_highscore, tex_block2, tex_background2, tex_barrier, tex_ice, tex_clock, tex_clockhand, tex_instructions, tex_star;

// Sprites
Sprite background, ground, wall, wall2, interface, hand, pause_menu, head, gameover, enterName, winner, newstage, highscore, levelBarrier, ice[max_lives], clock1, clockhand, instructions, star;

// Sounds
SoundBuffer buffer_menu_choose, buffer_menu_change, buffer_jump, buffer_sound_gameover, buffer_sound_cheer, buffer_sound_rotate, buffer_sound_gonna_fall;
Sound menu_choose, menu_change, jump_sound, sound_gameover, sound_cheer, sound_rotate, sound_gonna_fall;
Music background_music;

// ------> File <------
string userName = "";

struct User
{
    string user_name;
    int user_score;
};

const int MAX_USERS = 100;
User user_arr[MAX_USERS];
int user_count = 0;

void loadUserData()
{
    ifstream myFile("user_data.txt");
    string name;
    int score;

    while (myFile >> name >> score)
    {
        if (!name.empty() && user_count < MAX_USERS)
        {
            user_arr[user_count] = { name, score };
            user_count++;
        }
    }
    myFile.close();
}

void saveUserData()
{
    sort(user_arr, user_arr + user_count, [](const User& a, const User& b)
        {
            return a.user_score > b.user_score;
        }
    );

    ofstream myFile("user_data.txt");

    for (int i = 0; i < user_count; i++)
    {
        myFile << user_arr[i].user_name << " " << user_arr[i].user_score << endl;
    }

    myFile.close();
}

void updateOrAddUserScore(const string& name, int score)
{
    bool userExists = false;

    for (int i = 0; i < user_count; i++)
    {
        if (user_arr[i].user_name == name)
        {
            userExists = true;
            if (score > user_arr[i].user_score)
            {
                user_arr[i].user_score = score;
            }
            return;
        }
    }

    if (!userExists && user_count < MAX_USERS)
    {
        user_arr[user_count] = { name, score };
        user_count++;
    }
}

string copyUserScore()
{
    stringstream s;

    for (int i = 0; i < user_count; i++)
    {
        s << i + 1 << ". " << user_arr[i].user_name << " - " << user_arr[i].user_score << "\n";
    }

    return s.str();
}

void clocktimer(float deltatime)
{
    static const float decayDuration = 10.0f; 

    if (clockFill > 0.0f)
    {
        float elapsed = FillTime.getElapsedTime().asSeconds();
        if (elapsed <= decayDuration)
        {
            clockFill = 1.0f - (elapsed / decayDuration);
        }
        else
        {
            clockFill = 0.0f; 
        }
    }
}

struct BLOCKS
{
    Sprite blocksSprite;
    int direction, level;
    float moveSpeed, leftBound, rightBound, xscale;
    bool isMoving, isIntersected;
   
    BLOCKS(Texture& texblocks, float xposition, float yposition, int level)
    {
        xscale = (rand() % 2) + (level == 1 ? 1.5 : 1.25);
        blocksSprite.setTexture(texblocks);
        blocksSprite.setPosition(xposition, yposition);
        blocksSprite.setScale(xscale, 1);
        isIntersected = false;

        if (level == 2)
        {
            if (isMoving)
            {
                moveSpeed = (rand() % 50) + 100;
                leftBound = xposition - (rand() % 100 + 50);
                rightBound = xposition + (rand() % 100 + 50);
                direction = (rand() % 2) == 0 ? 1 : -1;
            }
        }
        else
        {
            isMoving = false;
        }
    }
};

BLOCKS* currentBlock = nullptr;
int lastBlockIndex = -1;

void view_movement(float& deltatime)
{
    if (currentLevel == 1)
    {
        view.move(0, VIEW_SPEED_1 * deltatime);

    }

    else
    {
        view.move(0, VIEW_SPEED_2 * deltatime);
    }
}

struct Players
{
    Sprite sprite; 
    const float moveSpeed = 200.0f, jump_strength = -10000.0f, gravity = 25000.0f;
    float velocity_x, velocity_y, frameTimer, rotationAngle;
    int frameIndex;
    bool rotatedInAir, onBarrier = true, isRotating, isMovingleft, isMovingright;
 
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
        sprite.setTexture(tex_player);
        sprite.setTextureRect(IntRect(0, 0, 38, 71));
        sprite.setOrigin(21, 71);
        sprite.setPosition(500, 650);
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
  
        if (Keyboard::isKeyPressed(Keyboard::Space) && isGround)
        {
            jump_sound.play();
            jumpTimer = 0.0f;
            isGround = false;
            velocity_y = jump_strength;
        }

        if (!isGround)
        {
            jumpTimer += deltatime;

            if (jumpTimer < maxJumpTime && Keyboard::isKeyPressed(Keyboard::Space))
            {
                velocity_y += jumpAcceleration * deltatime;
            }

            velocity_y += gravity * deltatime;
            
            float maxJumpSpeed = -1500.0f;
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

    void playerRotation(float deltatime, View& view, vector<BLOCKS>& blocksList, int level , bool& viewpaused)
    {
        static const float FAST_VIEW_SPEED = 800.0f;
        static float jumpHeight = 200.0f, jumpDirection = 1.0f, jumpDuration = 0.7f, jumpTime = 0.0f;
        static bool wallJumping = false, fastViewActive = false;
        static Vector2f jumpStart, jumpEnd;

        if (!isGround && !wallJumping && !isRotating)
        {
            bool rightWall = sprite.getGlobalBounds().intersects(wall2.getGlobalBounds());
            bool leftWall = sprite.getGlobalBounds().intersects(wall.getGlobalBounds());

            if (rightWall || leftWall)
            {
                int startIndex = lastBlockIndex;
                int targetIndex = min((int)blocksList.size() - 1, startIndex + 3);

                if (barrierSpawned && sprite.getPosition().y + sprite.getGlobalBounds().height <= levelBarrier.getPosition().y && onBarrier)
                {
                    for (size_t i = blocksNum; i < blocksList.size() - 2; i++)
                    {
                        if (blocksList[i].level == 2)
                        {
                            targetIndex = i + 3;
                            break;
                        }
                    }
                    onBarrier = false;
                }

                if (targetIndex > startIndex)
                {
                    sound_rotate.play();
                    jumpStart = sprite.getPosition();
                    jumpEnd = blocksList[targetIndex].blocksSprite.getPosition();
                    jumpEnd.y -= sprite.getGlobalBounds().height - 12;
                    jumpEnd.x += blocksList[targetIndex].blocksSprite.getGlobalBounds().width / 2;
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
            sprite.setTextureRect(IntRect(frameIndex * 50, 0, 60, 71));
            // Parabolic arc
            float arcY = -4 * jumpHeight * t * (1 - t);
            Vector2f pos = (1 - t) * jumpStart + t * jumpEnd;
            pos.y += arcY;

            sprite.setRotation(jumpDirection * 360.0f * t);
            sprite.setPosition(pos);

            if (t < 0.5f && fastViewActive)
            {
                view.move(0, FAST_VIEW_SPEED * deltatime);
            }

            else
            {
                if (fastViewActive)
                {
                    fastViewActive = false; 
                }

                if (!viewpaused && sprite.getPosition().y < HEIGHT / 3)
                {
                    view_movement(deltatime);
                }
            }

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

void generationBlocks(vector<BLOCKS>& blocksList, Players& player, int level)
{
    float y;
    Texture& tex_blocks = (level == 1) ? tex_block : tex_block2;
    int numBlocks = (level == 1) ? blocksNum : blocksNum * 2;
    float verticalSpacing = (level == 1) ? 200 : 250;

    if (level == 1)
    {
        blocksList.clear();
        y = HEIGHT - 200;
    }

    else
    {
        y = (barrierSpawned ? levelBarrier.getPosition().y : blocksList.back().blocksSprite.getPosition().y);
    }

    for (int i = 0; i < numBlocks; i++)
    {
        y -= verticalSpacing + (rand() % 100);
        float x = (level == 1) ? (rand() % (Xright - Xleft + 1)) + Xleft : (rand() % (Xright - Xleft - 200)) + Xleft + 100;;
        blocksList.push_back(BLOCKS(tex_blocks, x, y, level));
        blocksList[i].level = level;
    }
}

void updateMovingBlocks(vector<BLOCKS>& blocksList, float deltaTime)
{
    for (auto& block : blocksList)
    {
        if (block.isMoving)
        {
            block.blocksSprite.move(2 * block.moveSpeed * block.direction * deltaTime, 0);

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

void collisionANDlevelTransition(vector<BLOCKS>& blocksList, Players& player, Text& Score, Text& text_skipped)
{
    if (player.sprite.getPosition().y + player.sprite.getGlobalBounds().height <= blocksList.back().blocksSprite.getPosition().y && !barrierSpawned)
    {
        levelBarrier.setPosition(88, blocksList.back().blocksSprite.getPosition().y - 300);
        barrierSpawned = true;
        if (!level2Generated)
        {
            generationBlocks(blocksList, player, 2);
            level2Generated = true;
            transitionClock.restart();
        }
    }
    if (player.sprite.getPosition().y < levelBarrier.getPosition().y - 200 && barrierSpawned)
    {
        currentLevel = 2;
        doneLevel1 = true;
    }
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

                currentBlock = &block;
                currentBlockIndex = i;
                landed = true;

                if (!block.isIntersected)
                {
                    block.isIntersected = true;

                    if (currentLevel == 1)
                    {
                        score += 10;
                    }
                    else
                    {
                        score += 15;
                    }
                    Score.setString("Score: " + to_string(score));
                }

                if (lastBlockIndex != -1 && currentBlockIndex != lastBlockIndex && lastBlockIndex < static_cast<int>(blocksList.size()))
                {
                    int startIndex = min(lastBlockIndex, currentBlockIndex);
                    int endIndex = max(lastBlockIndex, currentBlockIndex);

                    int floorsSkipped = abs(currentBlockIndex - lastBlockIndex);
                    floors += floorsSkipped;
                    for (int j = startIndex + 1; j < endIndex; j++)
                    {
                        if (!blocksList[j].isIntersected)
                        {
                            blocksList[j].isIntersected = true;
                            nonIntersectedBlocks++;
                            text_skipped.setString(to_string(nonIntersectedBlocks));

                            if (currentLevel == 1)
                            {
                                score += 20;
                            }
                            else
                            {
                                score += 25;
                            }
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

    if (barrierSpawned && player.sprite.getGlobalBounds().intersects(levelBarrier.getGlobalBounds()))
    {
        if (player.velocity_y > 0 && (player.sprite.getPosition().y + player.sprite.getGlobalBounds().height - 50 <= levelBarrier.getPosition().y))
        {
            sound_rotate.stop();
            sound_gonna_fall.stop();
            player.sprite.setPosition(player.sprite.getPosition().x, levelBarrier.getPosition().y - player.sprite.getGlobalBounds().height + 12);
            isGround = true;
            player.velocity_y = 0;
            currentBlock = nullptr;
            currentBlockIndex = -1;
            lastBlockIndex = -1;
        }
    }

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
                if (atLeftEdge)
                    player.sprite.setScale(-2.4, 2.4);
                else
                    player.sprite.setScale(2.4, 2.4);

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

void initializeObject()
{
    tex_background.loadFromFile("background.jpg");
    tex_background2.loadFromFile("background2.png");
    tex_ground.loadFromFile("ground.jpg");
    tex_wall_left.loadFromFile("wall left.png");
    tex_wall_right.loadFromFile("wall flipped right.png");
    tex_player.loadFromFile("player.png");
    tex_block.loadFromFile("block.png");
    tex_block2.loadFromFile("block2.png");
    tex_interface.loadFromFile("mainMenu.png");
    tex_hand.loadFromFile("hand.png");
    tex_pauseMenu.loadFromFile("pauseMenu.png");
    tex_heads.loadFromFile("heads.png");
    tex_gameover.loadFromFile("gameover.png");
    tex_enterName.loadFromFile("enterName.png");
    tex_winner.loadFromFile("winner.png");
    tex_newstage.loadFromFile("newstage.png");
    tex_highscore.loadFromFile("highscore.png");
    tex_barrier.loadFromFile("barrier.png");
    tex_ice.loadFromFile("ice.png");
    tex_clock.loadFromFile("clock2.png");
    tex_clockhand.loadFromFile("clock 1.png");
    tex_instructions.loadFromFile("instructions.png");
    tex_star.loadFromFile("star.png");
    background_music.openFromFile("backgroundMusic.wav");
    buffer_menu_choose.loadFromFile("menu_choose.ogg");
    buffer_menu_change.loadFromFile("menu_change.ogg");
    buffer_jump.loadFromFile("jump.ogg");
    buffer_sound_gonna_fall.loadFromFile("gonna_fall.ogg");
    buffer_sound_gameover.loadFromFile("sound_gameover.opus");
    buffer_sound_cheer.loadFromFile("sound_cheer.opus");
    buffer_sound_rotate.loadFromFile("sound_falling.wav");

    //Sounds
    menu_choose.setBuffer(buffer_menu_choose);
    menu_change.setBuffer(buffer_menu_change);
    jump_sound.setBuffer(buffer_jump);
    sound_gameover.setBuffer(buffer_sound_gameover);
    sound_cheer.setBuffer(buffer_sound_cheer);
    sound_rotate.setBuffer(buffer_sound_rotate);
    sound_gonna_fall.setBuffer(buffer_sound_gonna_fall);

    for (int i = 0; i < max_lives; i++)
    {
        ice[i].setTexture(tex_ice);
        ice[i].setScale(2.5, 2.5);
    }

    enterName.setTexture(tex_enterName);
    enterName.setPosition(140, 500);
    enterName.setScale(2, 2.2);

    clock1.setTexture(tex_clock);
    clock1.setScale(1.5, 1.5);
    clock1.setPosition(0, 50);
    clockhand.setTexture(tex_clockhand);
    clockhand.setPosition(65, 135);
    clockhand.setScale(1.2, 1.2);
    FloatRect clockhandBounds = clockhand.getLocalBounds();
    clockhand.setOrigin(clockhandBounds.width / 2, clockhandBounds.height - 10);
    clockhand.setRotation(0);
   
    star.setTexture(tex_star);
    star.setScale(1.2, 1.2);
    star.setPosition(10, 375);

    pause_menu.setTexture(tex_pauseMenu);
    pause_menu.setScale(1.5, 2);

    gameover.setTexture(tex_gameover);
    gameover.setPosition(150, 70);
    gameover.setScale(1.3, 1.3);

    winner.setTexture(tex_winner);
    winner.setPosition(250, -30);
    winner.setScale(1, 1);

    newstage.setTexture(tex_newstage);
    newstage.setScale(1, 2);

    hand.setTexture(tex_hand);
    hand.setScale(1.5, 1.5);

    interface.setTexture(tex_interface);
    interface.setScale((float)(window.getSize().x) / tex_background.getSize().x,
       (float)(window.getSize().y) / tex_background.getSize().y);
    interface.setPosition(0, 0);

    instructions.setTexture(tex_instructions);
    instructions.setScale((float)(window.getSize().x) / tex_instructions.getSize().x,
        (float)(window.getSize().y) / tex_instructions.getSize().y);
    instructions.setPosition(0, 0);

    background.setTexture(tex_background);
    background.setScale((float)(window.getSize().x) / tex_background.getSize().x,
        (float)(window.getSize().y) / tex_background.getSize().y);

    ground.setTexture(tex_ground);
    ground.setPosition(0, 800);
    ground.setScale(1.5, 1.5);

    wall.setTexture(tex_wall_left);
    wall.setPosition(0, 0);
    wall.setScale(1.5, 3);
    Xleft = wall.getGlobalBounds().width + 50;

    wall2.setTexture(tex_wall_right);
    wall2.setPosition(880, 0);
    wall2.setScale(1.5, 3);
    Xright = wall2.getGlobalBounds().width + 450;

    levelBarrier.setTexture(tex_barrier);
    levelBarrier.setScale(2.25f, 1.5f);
}

void heads()
{
    static Clock clock;
    static int faceframeindex = 0;
    static const float angles[3] = { -11, 0, 11 };
    float timer = clock.getElapsedTime().asSeconds(), maxtimer = 0.4f;
    
    head.setTexture(tex_heads);
    head.setOrigin(Vector2f(104, 256));
    head.setScale(1.9f, 1.9f);
    head.setPosition(800, 500);

    if (timer > maxtimer)
    {
        faceframeindex = (faceframeindex + 1) % 3;
        head.setRotation(angles[faceframeindex]);
        clock.restart();
    }

    head.setTextureRect(IntRect(faceframeindex * 208, 0, 208, 512));
    window.draw(head);
}

void reset(Players& player, vector<BLOCKS>& blocksList, Text& Score, bool& viewpaused, bool& win , Clock& rotation, Clock& viewtimer, Clock& Timer, RectangleShape& OclockFill)
{
    score = 0;
    floors = 0;
    lives = max_lives;
    currentLevel = 1;
    barrierSpawned = false;
    level2Generated = false;
    win = false;
    viewpaused = false;
    isGround = true;
    clockFill = 0.0f;
    lastBlockIndex = -1;
    currentBlock = nullptr;
    nonIntersectedBlocks = 0;
    doneLevel1 = false;

    player.velocity_x = 0.0f;
    player.velocity_y = 0.0f;
    player.frameIndex = 0;
    player.frameTimer = 0.0f;
    player.onBarrier = true;
    player.sprite.setPosition(500, 650);
    player.sprite.setScale(2.4f, 2.4f);
    player.sprite.setTextureRect(IntRect(0, 0, 38, 71));
    lastSafePosition = player.sprite.getPosition();

    player.isRotating = false;
    player.rotatedInAir = false;
    player.rotationAngle = 0.0f;
    player.sprite.setRotation(0.0f);

    Score.setString("Score: " + to_string(score));
    Score.setFillColor(Color::White);

    Timer.restart();
    FillTime.restart();
    rotation.restart();
    viewtimer.restart();
    transitionClock.restart();

    OclockFill.setSize(Vector2f(26, 0));
    OclockFill.setPosition(49, 380);

    background.setTexture(tex_background);
    background.setScale(static_cast<float>(window.getSize().x) / tex_background.getSize().x,
        static_cast<float>(window.getSize().y) / tex_background.getSize().y);
    background.setPosition(0, 0);

    view.setCenter(Vector2f(500, 520));
    view.setSize(WIDTH, HEIGHT);

    ground.setPosition(0, 800);
    wall.setPosition(0, 0);
    wall2.setPosition(880, 0);

    blocksList.clear();
    generationBlocks(blocksList, player, currentLevel);
    window.setView(view);
}

bool soundOptions(Font font)
{
    int menuSelection = 0;
    bool isPressed = false;
    hand.setPosition(220, 240);

    int soundsVolume = menu_change.getVolume();
    int musicVolume = background_music.getVolume();

    float sliderBarWidth = 200.0f;
    float sliderBarHeight = 10.0f;

    RectangleShape soundsSliderBar(Vector2f(sliderBarWidth, sliderBarHeight));
    soundsSliderBar.setFillColor(Color::Black);
    soundsSliderBar.setPosition(500, 270);

    RectangleShape musicSliderBar(Vector2f(sliderBarWidth, sliderBarHeight));
    musicSliderBar.setFillColor(Color::Black);
    musicSliderBar.setPosition(500, 370);

    CircleShape soundsSliderKnob(10);
    soundsSliderKnob.setFillColor(Color::Red);
    soundsSliderKnob.setPosition(500 + (soundsVolume / 100.0f) * sliderBarWidth - 7, 265);

    CircleShape musicSliderKnob(10);
    musicSliderKnob.setFillColor(Color::Red);
    musicSliderKnob.setPosition(500 + (musicVolume / 100.0f) * sliderBarWidth - 7, 365);

    Text text_Sounds("SOUNDS", font);
    text_Sounds.setCharacterSize(40);
    text_Sounds.setFillColor(Color::Black);
    text_Sounds.setPosition(300, 250);

    Text text_Music("MUSIC", font);
    text_Music.setCharacterSize(40);
    text_Music.setFillColor(Color::Black);
    text_Music.setPosition(300, 350);

    Text text_Back("BACK", font);
    text_Back.setCharacterSize(40);
    text_Back.setFillColor(Color::Black);
    text_Back.setPosition(300, 450);

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
                    hand.setPosition(220, 240 + 100 * menuSelection);
                }

                if (event.key.code == Keyboard::Down)
                {
                    menu_change.play();
                    menuSelection = (menuSelection + 1) % 3;
                    hand.setPosition(220, 240 + 100 * menuSelection);
                }

                if (event.key.code == Keyboard::Enter)
                {
                    menu_choose.play();
                    if (menuSelection == 2) // ---> Back
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

        if (menuSelection == 0) // -----> Sound
        {
            if (Keyboard::isKeyPressed(Keyboard::Left))
            {
                soundsVolume = max(0, soundsVolume - 1);
                soundsSliderKnob.setPosition(500 + (soundsVolume / 100.0f) * sliderBarWidth - 7, 265);
                menu_change.setVolume(soundsVolume);
                menu_choose.setVolume(soundsVolume);
                jump_sound.setVolume(soundsVolume);
                sound_cheer.setVolume(soundsVolume);
                sound_gameover.setVolume(soundsVolume);
                sound_rotate.setVolume(soundsVolume);
                sound_gonna_fall.setVolume(soundsVolume);
            }

            if (Keyboard::isKeyPressed(Keyboard::Right))
            {
                soundsVolume = min(100, soundsVolume + 1);
                soundsSliderKnob.setPosition(500 + (soundsVolume / 100.0f) * sliderBarWidth - 7, 265);
                menu_change.setVolume(soundsVolume);
                menu_choose.setVolume(soundsVolume);
                jump_sound.setVolume(soundsVolume);
                sound_cheer.setVolume(soundsVolume);
                sound_gameover.setVolume(soundsVolume);
                sound_rotate.setVolume(soundsVolume);
                sound_gonna_fall.setVolume(soundsVolume);
            }
        }

        else if (menuSelection == 1) // ---> Music
        {
            if (Keyboard::isKeyPressed(Keyboard::Left))
            {
                musicVolume = max(0, musicVolume - 1);
                musicSliderKnob.setPosition(500 + (musicVolume / 100.0f) * sliderBarWidth - 7, 365);
                background_music.setVolume(musicVolume);
            }

            if (Keyboard::isKeyPressed(Keyboard::Right))
            {
                musicVolume = min(100, musicVolume + 1);
                musicSliderKnob.setPosition(500 + (musicVolume / 100.0f) * sliderBarWidth - 7, 365);
                background_music.setVolume(musicVolume);
            }
        }

        background.setPosition(0, 0);
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
        window.draw(background);
        window.draw(pause_menu);
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

bool startMenu(Font font, Text text_start, Text text_sound, Text text_highscore, Text text_exit, Clock& Timer)
{
    int menuSelection = 0;
    bool isPressed = false, showHighscore = false, showInstructions = false;
    
    hand.setPosition(500, 620);

    Font nameFont;
    nameFont.loadFromFile("Arial.ttf");

    Text displayName("", nameFont);
    displayName.setCharacterSize(30);
    displayName.setFillColor(Color::Black);
    displayName.setPosition(210, 758);

    Text displayText;
    displayText.setFont(font);
    displayText.setCharacterSize(40);
    displayText.setFillColor(Color::White);
    displayText.setPosition(50, 850);

    Text error;
    error.setFont(font);
    error.setCharacterSize(30);
    error.setFillColor(Color::Red);
    error.setPosition(50, 900);
    error.setString("");

    Text cursor("|", nameFont);
    cursor.setCharacterSize(25);
    cursor.setFillColor(Color::Black);
    cursor.setPosition(displayName.getPosition().x, displayName.getPosition().y + 2);
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

            if (!showStartMenu)
            {
                if (event.type == Event::TextEntered)
                {
                    if (event.text.unicode == '\b' && !userName.empty())
                    {
                        userName.pop_back();
                    }

                    else if (event.text.unicode < 128 && event.text.unicode != '\r')
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
                if (event.type == Event::KeyPressed && !isPressed && !showHighscore && !showInstructions)
                {
                    isPressed = true;
                    if (event.key.code == Keyboard::Up)
                    {
                        menu_change.play();
                        menuSelection = (menuSelection - 1 + 5) % 5;
                        hand.setPosition(500, 620 + 60 * menuSelection);
                    }

                    if (event.key.code == Keyboard::Down)
                    {
                        menu_change.play();
                        menuSelection = (menuSelection + 1) % 5;
                        hand.setPosition(500, 610 + 60 * menuSelection);
                    }

                    if (event.key.code == Keyboard::Enter)
                    {
                        menu_choose.play();
                        if (menuSelection == 0) //--->start
                        {
                            Timer.restart();
                            FillTime.restart();
                            return true;
                        }

                        if (menuSelection == 1)//--->sound
                        {
                            pause_menu.setPosition(150, 200);
                            soundOptions(font);
                            hand.setPosition(500, 610 + 60 * menuSelection);
                        }

                        if (menuSelection == 2)//--->highscore
                        {
                            showHighscore = true;
                        }

                        if (menuSelection == 3)//--->instructions
                        {
                            showInstructions = true;
                        }

                        else if (menuSelection == 4)//---->exit
                        {
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

        background.setPosition(0, 0);
        window.setView(window.getDefaultView());

        window.clear();
        window.draw(interface);
        window.draw(enterName);
        displayName.setString(userName);
        window.draw(displayName);

        if (Keyboard::isKeyPressed(Keyboard::Escape))
        {
            showHighscore = false;
            showInstructions = false;
        }

        if (showHighscore)
        {
            window.clear();
            highscore.setTexture(tex_highscore);
            highscore.setPosition(200, 0);
            highscore.setScale(2, 2);

            Text text;
            text.setFont(font);
            text.setCharacterSize(50);
            text.setFillColor(Color::Black);
            text.setPosition(250, 180);
            text.setString(copyUserScore());

            window.draw(background);
            window.draw(highscore);
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

        if (!showStartMenu && showCursor)
        {
            window.draw(cursor);
        }

        heads();

        if (showStartMenu)
        {
            text_start.setFillColor(menuSelection == 0 ? Color::Red : Color::Black);
            text_start.setOutlineColor(menuSelection == 0 ? Color::Yellow : Color::Transparent);
            text_start.setOutlineThickness(menuSelection == 0 ? 4 : 0);

            text_sound.setFillColor(menuSelection == 1 ? Color::Red : Color::Black);
            text_sound.setOutlineColor(menuSelection == 1 ? Color::Yellow : Color::Transparent);
            text_sound.setOutlineThickness(menuSelection == 1 ? 4 : 0);
            text_sound.setPosition(571, 680);

            text_highscore.setFillColor(menuSelection == 2 ? Color::Red : Color::Black);
            text_highscore.setOutlineColor(menuSelection == 2 ? Color::Yellow : Color::Transparent);
            text_highscore.setOutlineThickness(menuSelection == 2 ? 4 : 0);
            text_highscore.setPosition(571, 735);

            Text text_instructions("INSTRUCTIONS", font);
            text_instructions.setCharacterSize(40);
            text_instructions.setFillColor(menuSelection == 3 ? Color::Red : Color::Black);
            text_instructions.setOutlineColor(menuSelection == 3 ? Color::Yellow : Color::Transparent);
            text_instructions.setOutlineThickness(menuSelection == 3 ? 4 : 0);
            text_instructions.setPosition(571, 790);

            text_exit.setFillColor(menuSelection == 4 ? Color::Red : Color::Black);
            text_exit.setOutlineColor(menuSelection == 4 ? Color::Yellow : Color::Transparent);
            text_exit.setOutlineThickness(menuSelection == 4 ? 4 : 0);
            text_exit.setPosition(571, 845);

            window.clear();
            window.draw(interface);
            window.draw(text_start);
            window.draw(text_sound);
            window.draw(text_highscore);
            window.draw(text_instructions);
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

bool pauseMenu(Players& player, Font font, Text text_sound, Text text_exit, Text text_start, vector<BLOCKS>& blocksList, Text text_play_again, Text& Score , bool& viewpaused, bool& win, Clock& rotation,Clock& viewtimer, Clock& Timer, RectangleShape& OclockFill)
{
    int menuSelection = 0;
    bool isPressed = false;

    pause_menu.setPosition(150, 200);
    hand.setPosition(200, 240);

    Text text_resume("RESUME", font);
    text_resume.setCharacterSize(40);
    text_resume.setFillColor(Color::Black);
    text_resume.setPosition(300, 250);

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
                    hand.setPosition(200, 240 + 100 * menuSelection);
                }

                if (event.key.code == Keyboard::Down)
                {
                    menu_change.play();
                    menuSelection = (menuSelection + 1) % 4;
                    hand.setPosition(200, 240 + 100 * menuSelection);
                }

                if (event.key.code == Keyboard::Enter)
                {
                    menu_choose.play();
                    if (menuSelection == 0) // ---> Resume
                    {
                        viewpaused = true;
                        viewtimer.restart();
                        window.setView(view);
                        return true;
                    }

                    else if (menuSelection == 1) // ---> Play Again
                    {
                        reset(player, blocksList, Score,viewpaused, win, rotation, viewtimer, Timer, OclockFill);
                        return true;
                    }

                    else if (menuSelection == 2) // ---> Sound
                    {
                        soundOptions(font);
                    }

                    else if (menuSelection == 3) // ---> Exit to Main Menu
                    {
                        reset(player, blocksList, Score,viewpaused, win,rotation, viewtimer, Timer, OclockFill);
                        return false;
                    }
                }
            }

            if (event.type == Event::KeyReleased)
            {
                isPressed = false;
            }
        }

        background.setPosition(0, 0);
        window.setView(window.getDefaultView());

        text_resume.setFillColor(menuSelection == 0 ? Color::Red : Color::Black);
        text_resume.setOutlineColor(menuSelection == 0 ? Color::Yellow : Color::Transparent);
        text_resume.setOutlineThickness(menuSelection == 0 ? 2 : 0);

        text_play_again.setFillColor(menuSelection == 1 ? Color::Red : Color::Black);
        text_play_again.setOutlineColor(menuSelection == 1 ? Color::Yellow : Color::Transparent);
        text_play_again.setOutlineThickness(menuSelection == 1 ? 2 : 0);
        text_play_again.setPosition(300, 350);

        text_sound.setFillColor(menuSelection == 2 ? Color::Red : Color::Black);
        text_sound.setOutlineColor(menuSelection == 2 ? Color::Yellow : Color::Transparent);
        text_sound.setOutlineThickness(menuSelection == 2 ? 2 : 0);
        text_sound.setPosition(300, 450);

        text_exit.setFillColor(menuSelection == 3 ? Color::Red : Color::Black);
        text_exit.setOutlineColor(menuSelection == 3 ? Color::Yellow : Color::Transparent);
        text_exit.setOutlineThickness(menuSelection == 3 ? 2 : 0);
        text_exit.setPosition(300, 550);

        window.clear();
        window.draw(background);
        window.draw(pause_menu);
        window.draw(text_resume);
        window.draw(text_play_again);
        window.draw(text_sound);
        window.draw(text_exit);
        window.draw(hand);
        window.display();
    }
    return false;
}

bool gameOver(Players& player, Font font, Text text_play_again, Text text_exit, vector<BLOCKS>& blocksList, Text& Score, Text& timerText, bool& viewpaused, bool win, Clock& rotation,Clock& viewtimer, Clock& Timer, RectangleShape& OclockFill)
{
    int menuSelection = 0;
    bool isPressed = false;

    hand.setPosition(200, 560);
    pause_menu.setPosition(150, 300);

    View gameView = window.getView();

    Text Floor("Floors: " + to_string(floors), font);
    Floor.setCharacterSize(50);
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
                    hand.setPosition(200, 560 + 70 * menuSelection);
                }

                if (event.key.code == Keyboard::Down)
                {
                    menu_change.play();
                    menuSelection = (menuSelection + 1) % 2;
                    hand.setPosition(200, 560 + 70 * menuSelection);
                }

                if (event.key.code == Keyboard::Enter)
                {
                    menu_choose.play();
                    if (menuSelection == 0) // ---> Play Again
                    {
                        reset(player, blocksList, Score, viewpaused, win, rotation, viewtimer, Timer, OclockFill);
                        return true;
                    }

                    else if (menuSelection == 1) // ---> Exit to Main Menu
                    {
                        reset(player, blocksList, Score,viewpaused, win, rotation, viewtimer, Timer, OclockFill);
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
        Score.setPosition(280, 350);

        Floor.setFillColor(Color::Black);
        Floor.setPosition(280, 420);

        timerText.setFillColor(Color::Black);
        timerText.setPosition(280, 490);

        text_play_again.setFillColor(menuSelection == 0 ? Color::Red : Color::Black);
        text_play_again.setOutlineColor(menuSelection == 0 ? Color::Yellow : Color::Transparent);
        text_play_again.setOutlineThickness(menuSelection == 0 ? 2 : 0);
        text_play_again.setPosition(280, 560);

        text_exit.setFillColor(menuSelection == 1 ? Color::Red : Color::Black);
        text_exit.setOutlineColor(menuSelection == 1 ? Color::Yellow : Color::Transparent);
        text_exit.setOutlineThickness(menuSelection == 1 ? 2 : 0);
        text_exit.setPosition(280, 630);

        window.clear();
        window.setView(gameView);
        window.draw(background);
        window.draw(wall);
        window.draw(wall2);
        window.draw(player.sprite);

        for (const auto& block : blocksList)
        {
            window.draw(block.blocksSprite);
        }

        window.setView(window.getDefaultView());
        window.draw(gameover);
        window.draw(pause_menu);
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

bool winMenu(Players& player, Font font, Text text_play_again, Text text_exit, vector<BLOCKS>& blocksList, Text& Score, Text& timerText)
{
    sound_cheer.play();
    int menuSelection = 0;
    bool isPressed = false;

    View gameView = window.getView();

    hand.setPosition(200, 560);
    pause_menu.setPosition(150, 300);
    newstage.setPosition(0, -550);

    Text Floor("Floors: " + to_string(floors), font);
    Floor.setCharacterSize(50);
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
                    hand.setPosition(200, 560 + 70 * menuSelection);
                }

                if (event.key.code == Keyboard::Down)
                {
                    menu_change.play();
                    menuSelection = (menuSelection + 1) % 2;
                    hand.setPosition(200, 560 + 70 * menuSelection);
                }

                if (event.key.code == Keyboard::Enter)
                {
                    menu_choose.play();
                    if (menuSelection == 0) // ---> Play Again
                    {
                        return true;
                    }

                    else if (menuSelection == 1) // ---> Exit to Main Menu
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

        Score.setFillColor(Color::Black);
        Score.setPosition(300, 350);

        Floor.setFillColor(Color::Black);
        Floor.setPosition(300, 420);

        timerText.setFillColor(Color::Black);
        timerText.setPosition(300, 490);

        text_play_again.setFillColor(menuSelection == 0 ? Color::Red : Color::Black);
        text_play_again.setOutlineColor(menuSelection == 0 ? Color::Yellow : Color::Transparent);
        text_play_again.setOutlineThickness(menuSelection == 0 ? 2 : 0);
        text_play_again.setPosition(300, 560);

        text_exit.setFillColor(menuSelection == 1 ? Color::Red : Color::Black);
        text_exit.setOutlineColor(menuSelection == 1 ? Color::Yellow : Color::Transparent);
        text_exit.setOutlineThickness(menuSelection == 1 ? 2 : 0);
        text_exit.setPosition(300, 630);

        window.clear();
        window.setView(gameView);
        window.draw(background);
        window.draw(wall);
        window.draw(wall2);
        window.draw(ground);
        window.draw(player.sprite);

        for (const auto& block : blocksList)
        {
            window.draw(block.blocksSprite);
        }

        window.setView(window.getDefaultView());
        window.draw(winner);
        window.draw(pause_menu);
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

void draw(Players& player, vector<BLOCKS>& blocksList, Text& Score, Font font, Text& timerText, Text& text_skipped, float& deltatime, Clock& rotation, Clock& Timer,RectangleShape& OclockFill)
{
    window.setView(view);

    Vector2f camPos = view.getCenter();
    wall.setPosition(camPos.x - WIDTH / 2, camPos.y - HEIGHT / 2);
    wall2.setPosition(880 + camPos.x - WIDTH / 2, camPos.y - HEIGHT / 2);
    Score.setPosition(camPos.x - WIDTH / 2 + 120, camPos.y - HEIGHT / 2);

    float elapsedTime = rotation.getElapsedTime().asSeconds();
    float rotationPeriod = 60.0f; 
    float angle = (elapsedTime / rotationPeriod) * 360.0f; 
    clockhand.setRotation(angle);

    int time = Timer.getElapsedTime().asSeconds();
    int minutes =time / 60;
    int seconds =time % 60;

    timerText.setPosition(50 + camPos.x - WIDTH / 2, camPos.y - HEIGHT / 2 + 850);
    string timeString = "Time: ";
    timerText.setFillColor(Color::White);
    if (minutes < 10) timeString += "0";
    timeString += to_string(minutes);
    timeString += ":";
    if (seconds < 10) timeString += "0";
    timeString += to_string(seconds);
    timerText.setString(timeString);

    Text levelText("Level: " + to_string(currentLevel), font);
    levelText.setCharacterSize(50);
    levelText.setFillColor(Color::White);
    levelText.setPosition(camPos.x - WIDTH / 2 + 680, camPos.y - HEIGHT / 2);

    text_skipped.setString(to_string(nonIntersectedBlocks));

    if (barrierSpawned && player.sprite.getPosition().y <= levelBarrier.getPosition().y)
    {
        background.setTexture(tex_background2);
        background.setScale((float)(window.getSize().x) / tex_background2.getSize().x,
            (float)(window.getSize().y) / tex_background2.getSize().y);
    }

    else
    {
        background.setTexture(tex_background);
        background.setScale((float)(window.getSize().x) / tex_background.getSize().x,
           (float)(window.getSize().y) / tex_background.getSize().y);
    }

    background.setPosition(camPos.x - WIDTH / 2, camPos.y - HEIGHT / 2);
    window.draw(background);

    for (int i = 0; i < lives; i++) {
        ice[i].setPosition(camPos.x - WIDTH / 2 + 300 + (i * 100), camPos.y - HEIGHT / 2 + 80);
        window.draw(ice[i]);
    }

    if (barrierSpawned)
    {
        window.draw(levelBarrier);
    }

    for (auto& block : blocksList)
    {
        window.draw(block.blocksSprite);
    }

    window.draw(ground);
    window.draw(wall);
    window.draw(wall2);
    window.draw(player.sprite);
    window.draw(Score);
    window.draw(levelText);
    window.draw(timerText);
    window.setView(window.getDefaultView());
    window.draw(clock1);
    window.draw(star);
    window.draw(clockhand);
    OclockFill.setSize(Vector2f(26, 152 * clockFill));
    OclockFill.setPosition(49, 380 - 152 * clockFill);
    window.draw(OclockFill);
    window.draw(text_skipped);
    window.setView(view);

    if (doneLevel1)
    {
        newstage.setPosition(0, levelBarrier.getPosition().y - 500);
        if (transitionClock.getElapsedTime().asSeconds() < 1.5f)
        {
            sound_cheer.play();
            Text transitionText("Level " + to_string(currentLevel), font);
            transitionText.setCharacterSize(100);
            transitionText.setFillColor(Color::White);
            transitionText.setOutlineColor(Color::Black);
            transitionText.setOutlineThickness(10);
            transitionText.setPosition(camPos.x - transitionText.getGlobalBounds().width / 2, camPos.y - transitionText.getGlobalBounds().height / 2);
            window.draw(transitionText);
            newstage.move(0, 10);
            window.draw(newstage);
            doneLevel1 = false;
        }

    }

}

int main()
{
    window.setFramerateLimit(60);
    initializeObject();
    loadUserData();
   
    //background_music.setLoop(true);
    //background_music.play();

    Font font;
    font.loadFromFile("RushDriver-Italic.otf");

    Text text_start("START", font);
    text_start.setCharacterSize(40);
    text_start.setFillColor(Color::Black);
    text_start.setPosition(571, 620);

    Text text_sound("SOUND", font);
    text_sound.setCharacterSize(40);
    text_sound.setFillColor(Color::Black);

    Text text_highscore("HIGHSCORE", font);
    text_highscore.setCharacterSize(40);
    text_highscore.setFillColor(Color::Black);

    Text text_exit("EXIT", font);
    text_exit.setCharacterSize(40);
    text_exit.setFillColor(Color::Black);

    Text text_play_again("PLAY AGAIN", font);
    text_play_again.setCharacterSize(40);
    text_play_again.setFillColor(Color::Black);
    Text Score("Score: " + to_string(score), font);

    Score.setCharacterSize(50);
    Score.setFillColor(Color::White);

    Text timerText;
    timerText.setFont(font);
    timerText.setCharacterSize(30);
    timerText.setFillColor(Color::White);
    timerText.setCharacterSize(50);
    timerText.setFillColor(Color::White);

    Text text_skipped;
    text_skipped.setFont(font);
    text_skipped.setCharacterSize(30);
    text_skipped.setFillColor(Color::Black);
    text_skipped.setPosition(45, 390);

    RectangleShape OclockFill;
    OclockFill.setSize(Vector2f(26, 152));
    OclockFill.setPosition(49, 380);
    OclockFill.setFillColor(Color::Red);
    Clock viewtimer, Timer;
    static Clock rotation;

    bool viewpaused = false, win = false;
    bool start_game = startMenu(font, text_start, text_sound, text_highscore, text_exit, Timer);
    const int max_levels = 2;
   
    if (start_game)
    {
        Players player;
        Clock clock;
        vector<BLOCKS> blocksList;
        random_device RANDOM;
        srand(RANDOM());

        player.sprite.setOrigin(player.sprite.getLocalBounds().width / 2, 0);
        generationBlocks(blocksList, player, currentLevel);
       
        view.setSize(WIDTH, HEIGHT);
        view.setCenter(Vector2f(500, 520));
        window.setView(view);

        while (window.isOpen())
        {
            float deltatime = clock.restart().asSeconds();
            if (!viewpaused && player.sprite.getPosition().y < HEIGHT / 3)
            {
                view_movement(deltatime);
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

            // Pause menu
            if (Keyboard::isKeyPressed(Keyboard::Escape) || Keyboard::isKeyPressed(Keyboard::P))
            {
                bool resumeGame = pauseMenu(player, font, text_sound, text_exit, text_start, blocksList, text_play_again, Score, viewpaused, win, rotation, viewtimer, Timer, OclockFill);
                if (!resumeGame)
                {
                    startMenu(font, text_start, text_sound, text_highscore, text_exit, Timer);
                }
            }

            // Game over
            float viewBottom = view.getCenter().y + HEIGHT / 2;
            if (player.sprite.getPosition().y > viewBottom)
            {
                sound_rotate.stop();
                sound_gonna_fall.stop();
                if (lives > 1)
                {
                    lives--;
                    player.sprite.setPosition(lastSafePosition);
                    player.velocity_x = 0;
                    player.velocity_y = 0;
                    player.frameIndex = 0;
                    player.sprite.setTextureRect(IntRect(0, 0, 38, 71));
                    player.onBarrier = true;
                    view.setCenter(Vector2f(500, lastSafePosition.y + 100));
                    window.setView(view);
                    viewpaused = true;
                    viewtimer.restart();
                }

                else
                {
                    sound_gameover.play();
                    bool resumeGame = gameOver(player, font, text_play_again, text_exit, blocksList, Score, timerText, viewpaused, win,rotation, viewtimer, Timer, OclockFill);

                    if (!resumeGame)
                    {
                        startMenu(font, text_start, text_sound, text_highscore, text_exit, Timer);
                    }
                    win = false;
                }
            }

            // Collision with the ground
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

            //playerRotation
            player.playerRotation(deltatime, view, blocksList, currentLevel, viewpaused);

            // Collision detection with the left wall
            if (player.sprite.getGlobalBounds().intersects(wall.getGlobalBounds()))
            {
                player.sprite.setPosition(wall.getGlobalBounds().left + wall.getGlobalBounds().width + 50, player.sprite.getPosition().y);
            }

            // Collision detection with the right wall
            else if (player.sprite.getGlobalBounds().intersects(wall2.getGlobalBounds()))
            {
                player.sprite.setPosition(wall2.getGlobalBounds().left - player.sprite.getGlobalBounds().width + 50, player.sprite.getPosition().y);
            }

            // Collision with blocks&level2 
            collisionANDlevelTransition(blocksList, player, Score, text_skipped);

            clocktimer(deltatime);

            if (currentLevel == 2 && !viewpaused)
            {
                updateMovingBlocks(blocksList, deltatime);
            }

            // Win condition
            if (currentLevel == max_levels && isGround)
            {
                if (player.sprite.getGlobalBounds().intersects(blocksList.back().blocksSprite.getGlobalBounds()) &&
                    player.sprite.getPosition().y + player.sprite.getGlobalBounds().height - 50 <= blocksList.back().blocksSprite.getPosition().y)
                {
                    player.sprite.setPosition(blocksList.back().blocksSprite.getPosition().x + blocksList.back().blocksSprite.getGlobalBounds().width / 2,
                        blocksList.back().blocksSprite.getPosition().y - player.sprite.getGlobalBounds().height + 12);
                    win = true;
                    isGround = true;

                    bool resumeGame = winMenu(player, font, text_play_again, text_exit, blocksList, Score, timerText);
                    if (!resumeGame)
                    {
                        reset(player, blocksList, Score, viewpaused, win,rotation, viewtimer, Timer, OclockFill);
                        startMenu(font, text_start, text_sound, text_highscore, text_exit, Timer);
                    }

                    else
                    {
                        reset(player, blocksList, Score, viewpaused, win,rotation, viewtimer, Timer, OclockFill);
                    }
                }
            }
            player.jump(deltatime);
            player.sprite.move(player.velocity_x * deltatime, player.velocity_y * deltatime);
            player.handleMovement(deltatime);

            window.clear();
            draw(player, blocksList, Score, font, timerText, text_skipped, deltatime,rotation, Timer, OclockFill);
            window.display();
        }
    }
    return 0;
}
