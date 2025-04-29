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

// Constants
const int WIDTH = 1000;
const int HEIGHT = 1000;
const float moveSpeed = 200.0f;
const float jump_strength = -10000.0f;
const float gravity = 25000.0f;
const float VIEW_PAUSE_DURATION = 0.1;
const float maxFilltime = 10.0f;

//---->blocks constants<----
const int blocksNum = 20;
int Xleft, Xright;
int score = 0;
int floors = 0;
float blockWidth = 180.0f;
float blockHeight = 35.0f;
float xscale;
float lastPosition;
float clockFill = 0.0f;

//---->booleans<----
bool isGround = false;
bool isMovingleft = 0;
bool isMovingright = 0;
bool showMenu = false;
bool win = false;
bool viewpaused = false;
bool showHighscore = false;
bool level2Generated = false; // New flag to track level 2 generation

// Level variables
int currentLevel = 1;
const int max_levels = 2;
bool barrierSpawned = false;

// Life time
const int max_lives = 3;
int lives = max_lives;
Vector2f lastSafePosition; // Tracks where player was last safely standing

RenderWindow window(VideoMode(WIDTH, HEIGHT), "Icy Tower SFML");

// Clocks
Clock viewtimer;
Clock transitionClock;
Clock Timer;
static Clock rotation;
static Clock FillTime;
// Textures
Texture tex_background, tex_ground, tex_wall_right, tex_wall_left, tex_player, tex_block, tex_interface, tex_hand, tex_pauseMenu, tex_heads, tex_gameover, tex_enterName, tex_winner, tex_newstage, tex_highscore, tex_block2, tex_background2, tex_barrier, tex_ice, tex_clock, tex_clockhand;

// Sprites
Sprite background, ground, wall, wall2, interface, hand, pause_menu, head, gameover, enterName, winner, newstage, highscore, levelBarrier, ice[max_lives], clock1, clockhand;

// Declare the view object
View view;

// Sounds
SoundBuffer buffer_menu_choose, buffer_menu_change, buffer_jump, buffer_sound_gameover, buffer_sound_cheer, buffer_sound_falling, buffer_sound_gonna_fall;
Sound menu_choose, menu_change, jump_sound, sound_gameover, sound_cheer, sound_falling, sound_gonna_fall;
Music background_music;

string userName = "";
RectangleShape OclockFill;

// Player Struct
struct Players
{
    Sprite sprite;
    float velocity_x;
    float velocity_y;
    int frameIndex;
    float frameTimer;

    Players()
    {
        velocity_x = 0;
        velocity_y = 0;
        frameIndex = 0;
        frameTimer = 0;

        if (!tex_player.loadFromFile("player.png")) return;

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
            isMovingright = 1;

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
            isMovingleft = 1;

            if (frameTimer >= 0.1f)
            {
                frameIndex = (frameIndex + 1) % 4;
                frameTimer = 0;
            }

            sprite.setTextureRect(IntRect(frameIndex * 38, 0, 38, 71));
        }

        else
        {
            velocity_x = 0;//resets the frame

            if (isGround && frameIndex != 11)
            {

                frameIndex = 0;
                sprite.setTextureRect(IntRect(0, 0, 38, 71));
            }
        }

        sprite.move(velocity_x * deltatime, 0);
    }

    void jump(float deltatime)
    {
        static float jumpTimer = 0.0f;
        const float maxJumpTime = 0.3f;
        const float jumpAcceleration = -200000.0f;

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

            // Apply upward force gradually for a short duration
            if (jumpTimer < maxJumpTime && Keyboard::isKeyPressed(Keyboard::Space))
            {
                velocity_y += jumpAcceleration * deltatime;
            }

            velocity_y += gravity * deltatime;

            // Apply max jump speed to avoid too strong jumps
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

            // Set jump animations only when needed
            if (velocity_y < 0)
            {
                if (isMovingright)
                {
                    if (frameIndex != 6)
                    {
                        frameIndex = 6;
                        sprite.setScale(2.4, 2.4);
                        sprite.setTextureRect(IntRect(frameIndex * 38, 0, 38, 71));
                    }
                }

                else if (isMovingleft)
                {
                    if (frameIndex != 6)
                    {
                        frameIndex = 6;
                        sprite.setScale(-2.4, 2.4);
                        sprite.setTextureRect(IntRect(frameIndex * 38, 0, 38, 71));
                    }
                }

                else
                {
                    if (frameIndex != 5)
                    {
                        frameIndex = 5;
                        sprite.setTextureRect(IntRect(frameIndex * 38, 0, 38, 71));
                    }
                }
            }
            else // Falling
            {
                if (frameIndex != 8)
                {
                    frameIndex = 8;
                    sprite.setTextureRect(IntRect(frameIndex * 38, 0, 38, 71));
                }
            }
        }
    }

};

// Blocks struct
struct BLOCKS
{
    Sprite blocksSprite;
    bool isMoving;
    float moveSpeed;
    float leftBound;
    float rightBound;
    int direction;
    int level; // Track which level this block belongs to

    BLOCKS(Texture& texblocks, float xposition, float yposition, int blockLevel)
    {
        level = blockLevel;
        xscale = (rand() % 2) + (level == 1 ? 1.5 : 1.5);
        blocksSprite.setTexture(texblocks);
        blocksSprite.setPosition(xposition, yposition);
        blocksSprite.setScale(xscale, 1);

        if (level == 2)
        {
            if (isMoving)
            {
                moveSpeed = (rand() % 50) + 50;
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

// User struct
struct User
{
    string user_name;
    int user_score;
};

const int MAX_USERS = 100;
User user_arr[MAX_USERS];
int user_count = 0;

// Function to load user data from file
void loadUserData()
{
    ifstream myFile("user_data.txt");

    if (!myFile.is_open())
    {
        cout << "No existing score file found. Creating a new one." << endl;
        return;
    }

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

// Function to save user data to file
void saveUserData()
{
    // Sort the user_arr array by user_score in descending order
    sort(user_arr, user_arr + user_count, [](const User& a, const User& b)
        {
            return a.user_score > b.user_score;
        }
    );

    // Open the file for writing
    ofstream myFile("user_data.txt");

    if (!myFile.is_open())
    {
        cout << "Error: Unable to open score file for writing." << endl;
        return;
    }

    // Write the sorted data to the file
    for (int i = 0; i < user_count; i++)
    {
        myFile << user_arr[i].user_name << " " << user_arr[i].user_score << endl;
    }

    // Close the file
    myFile.close();
}

// Function to update or add user score
void updateOrAddUserScore(const string& name, int score)
{
    if (name.empty())
    {
        cout << "Error: Username cannot be empty." << endl;
        return;
    }

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

// Copy user data
string copyUserScoreString()
{
    stringstream ss;

    for (int i = 0; i < user_count; i++)
    {
        ss << i + 1 << ". " << user_arr[i].user_name << " - " << user_arr[i].user_score << "\n";
    }

    return ss.str();
}

// Generate blocks for a level
void generationBlocks(vector<BLOCKS>& blockslist, Players& player, int level)
{
    float y;
    Texture& tex_blocks = (level == 1) ? tex_block : tex_block2;
    int numBlocks = (level == 1) ? blocksNum : blocksNum + 0;
    float verticalSpacing = (level == 1) ? 200 : 250;


    if (level == 1)
    {
        blockslist.clear();
        y = HEIGHT - 200;
    }

    else
    {
        // Start above the barrier or the highest block
        y = (barrierSpawned ? levelBarrier.getPosition().y : blockslist.back().blocksSprite.getPosition().y);
    }

    for (int i = 0; i < numBlocks; i++)
    {
        float horizontalSpacing = (level == 1) ? (rand() % (Xright - Xleft + 1)) + Xleft : (rand() % (Xright - Xleft - 200)) + Xleft + 100;
        y -= verticalSpacing + (rand() % 100); //  changing distance between blocks
        float x = horizontalSpacing;
        blockslist.push_back(BLOCKS(tex_blocks, x, y, level));
    }
}

// Update the blocks for level 2
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

// Collision function
void collision(vector<BLOCKS>& blockslist, Players& player, Font font, Text& Score, Text& text_start, Text& text_exit, Text& text_play_again, Text& text_highscore, Text& text_sound)
{
    static BLOCKS* currentBlock = nullptr;
    static int lastBlockIndex = -1; // Track the index of the last block the player was on

    // Check if player reached top to spawn barrier
    if (player.sprite.getPosition().y + player.sprite.getGlobalBounds().height <= blockslist.back().blocksSprite.getPosition().y && !barrierSpawned)
    {
        levelBarrier.setPosition(88, blockslist.back().blocksSprite.getPosition().y - 300);
        barrierSpawned = true;
    }

    // Block collision
    bool landed = false;
    int currentBlockIndex = -1;

    for (size_t i = 0; i < blockslist.size(); ++i)
    {
        auto& block = blockslist[i];
        if (player.sprite.getGlobalBounds().intersects(block.blocksSprite.getGlobalBounds()))
        {
            if (player.velocity_y > 0 && (player.sprite.getPosition().y + player.sprite.getGlobalBounds().height - 50 <= block.blocksSprite.getPosition().y))
            {
                lastSafePosition = player.sprite.getPosition();
                lastSafePosition.y = block.blocksSprite.getPosition().y - player.sprite.getGlobalBounds().height + 12;
                player.sprite.setPosition(player.sprite.getPosition().x, block.blocksSprite.getPosition().y - player.sprite.getGlobalBounds().height + 12);
                isGround = true;
                sound_falling.stop();
                sound_gonna_fall.stop();

                currentBlock = &block;
                currentBlockIndex = i;
                landed = true;

                // Update floors and score based on block index
                if (lastBlockIndex != -1 && currentBlockIndex != lastBlockIndex)
                {
                    int floorsSkipped = abs(currentBlockIndex - lastBlockIndex);
                    cout << "floorSkip: " << floorsSkipped << endl;
                    floors += floorsSkipped;
                    score += floorsSkipped * 10; // 10 points per floor
                    Score.setString("Score: " + to_string(score));
                }
                lastBlockIndex = currentBlockIndex;
                player.velocity_y = 0;
            }
        }
    }

    // Barrier collision
    if (barrierSpawned && player.sprite.getGlobalBounds().intersects(levelBarrier.getGlobalBounds()))
    {
        if (player.velocity_y > 0 && (player.sprite.getPosition().y + player.sprite.getGlobalBounds().height - 50 <= levelBarrier.getPosition().y))
        {
            sound_falling.stop();
            sound_gonna_fall.stop();
            player.sprite.setPosition(player.sprite.getPosition().x, levelBarrier.getPosition().y - player.sprite.getGlobalBounds().height + 12);
            isGround = true;
            player.velocity_y = 0;
            currentBlock = nullptr;
            currentBlockIndex = -1; // Reset block index for barrier
            lastBlockIndex = -1; // Reset last block index

            if (!level2Generated)
            {
                sound_cheer.play();
                currentLevel = 2;
                generationBlocks(blockslist, player, currentLevel);
                level2Generated = true;
                transitionClock.restart();
            }
        }
    }

    // If not landed, check if player has stepped off the block
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
            sound_falling.play();
            sound_gonna_fall.stop();
            currentBlock = nullptr;
        }
    }
}

void initializeObject()
{
    // load textures
    if (!tex_background.loadFromFile("background.jpg")) return;
    if (!tex_background2.loadFromFile("background2.png")) return;
    if (!tex_ground.loadFromFile("ground.jpg")) return;
    if (!tex_wall_left.loadFromFile("wall left.png")) return;
    if (!tex_wall_right.loadFromFile("wall flipped right.png")) return;
    if (!tex_player.loadFromFile("player.png")) return;
    if (!tex_block.loadFromFile("block.png")) return;
    if (!tex_block2.loadFromFile("block2.png")) return;
    if (!tex_interface.loadFromFile("mainMenu.png")) return;
    if (!tex_hand.loadFromFile("hand.png")) return;
    if (!tex_pauseMenu.loadFromFile("pauseMenu.png")) return;
    if (!tex_heads.loadFromFile("heads.png")) return;
    if (!tex_gameover.loadFromFile("gameover.png")) return;
    if (!tex_enterName.loadFromFile("enterName.png")) return;
    if (!tex_winner.loadFromFile("winner.png")) return;
    if (!tex_newstage.loadFromFile("newstage.png")) return;
    if (!tex_highscore.loadFromFile("highscore.png")) return;
    if (!tex_barrier.loadFromFile("barrier.png")) return;
    if (!tex_ice.loadFromFile("ice.png")) return;
    if (!tex_clock.loadFromFile("clock2.png")) return;
    if (!tex_clockhand.loadFromFile("clock 1.png")) return;

    if (!buffer_menu_choose.loadFromFile("menu_choose.ogg")) return;
    if (!buffer_menu_change.loadFromFile("menu_change.ogg")) return;
    if (!buffer_jump.loadFromFile("jump.ogg")) return;
    if (!buffer_sound_gonna_fall.loadFromFile("gonna_fall.ogg")) return;
    if (!buffer_sound_gameover.loadFromFile("sound_gameover.opus")) return;
    if (!buffer_sound_cheer.loadFromFile("sound_cheer.opus")) return;
    if (!buffer_sound_falling.loadFromFile("sound_falling.wav")) return;
    if (!background_music.openFromFile("backgroundMusic.wav")) return;

    //Sounds
    menu_choose.setBuffer(buffer_menu_choose);
    menu_change.setBuffer(buffer_menu_change);
    jump_sound.setBuffer(buffer_jump);
    sound_gameover.setBuffer(buffer_sound_gameover);
    sound_cheer.setBuffer(buffer_sound_cheer);
    sound_falling.setBuffer(buffer_sound_falling);
    sound_gonna_fall.setBuffer(buffer_sound_gonna_fall);

    for (int i = 0;i < max_lives;i++)
    {
        ice[i].setTexture(tex_ice);
        ice[i].setScale(2.5, 2.5);
    }

    enterName.setTexture(tex_enterName);
    enterName.setPosition(140, 500);
    enterName.setScale(2, 2.2);

    winner.setTexture(tex_winner);
    winner.setPosition(250, -30);
    winner.setScale(1, 1);

    clock1.setTexture(tex_clock);
    clock1.setScale(1.5, 1.5);
    clock1.setPosition(0, 50);

    clockhand.setTexture(tex_clockhand);
    clockhand.setPosition(65, 135);
    clockhand.setScale(1.2, 1.2);

    // Set the origin to the center of the clockhand texture
    FloatRect clockhandBounds = clockhand.getLocalBounds();
    clockhand.setOrigin(clockhandBounds.width / 2, clockhandBounds.height - 10);
    clockhand.setRotation(0);

    OclockFill.setSize(Vector2f(26, 0));
    OclockFill.setPosition(49, 380);
    OclockFill.setFillColor(Color::Red);


    pause_menu.setTexture(tex_pauseMenu);
    pause_menu.setScale(1.5, 2);

    gameover.setTexture(tex_gameover);
    gameover.setPosition(150, 70);
    gameover.setScale(1.3, 1.3);

    newstage.setTexture(tex_newstage);
    newstage.setScale(1, 2);

    hand.setTexture(tex_hand);
    hand.setScale(1.5, 1.5);

    interface.setTexture(tex_interface);
    interface.setScale(static_cast<float>(window.getSize().x) / tex_background.getSize().x,
        static_cast<float>(window.getSize().y) / tex_background.getSize().y);
    interface.setPosition(0, 0);

    background.setTexture(tex_background);
    background.setScale(static_cast<float>(window.getSize().x) / tex_background.getSize().x,
        static_cast<float>(window.getSize().y) / tex_background.getSize().y);
    background.setPosition(0, 0);

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

void heads(Texture& tex_heads, Sprite& head)
{
    static Clock clock;
    static int faceframeindex = 0;
    float timer = clock.getElapsedTime().asSeconds();
    float maxtimer = 0.4f;
    static const float angles[3] = { -11, 0, 11 };

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

void reset(Players& player, vector<BLOCKS>& blocksList, int& score, int& floors, Text& Score)
{
    // Reset score and floors
    score = 0;
    floors = 0;

    lives = max_lives;

    currentLevel = 1;
    barrierSpawned = false;
    level2Generated = false;

    win = false;
    viewpaused = false;
    Timer.restart();
    FillTime.restart();
    rotation.restart();
    Score.setString("Score: " + to_string(score));
    Score.setFillColor(Color::White);

    // Clear the blocks list and regenerate blocks
    blocksList.clear();
    generationBlocks(blocksList, player, currentLevel);
    player.sprite.setPosition(500, 650);

    // Reset player position and velocity
    player.velocity_x = 0;
    player.velocity_y = 0;
    player.frameIndex = 0;
    player.sprite.setTextureRect(IntRect(0, 0, 38, 71));

    // Reset ground detection and last position
    lastPosition = 800;
    isGround = true;

    background.setTexture(tex_background);

    // Reset the camera view
    view.setCenter(Vector2f(500, 520));
    window.setView(view);
}

bool soundOptions(Sprite hand, Font font, Sound& menu_change, Sound& menu_choose, Music& background_music)
{
    int menuSelection = 0;
    bool isPressed = false;
    hand.setPosition(220, 240);

    // Volume levels (range: 0 to 100)
    int soundsVolume = menu_change.getVolume();
    int musicVolume = background_music.getVolume();

    // Slider dimensions
    float sliderBarWidth = 200.0f;
    float sliderBarHeight = 10.0f;

    RectangleShape soundsSliderBar(Vector2f(sliderBarWidth, sliderBarHeight));
    soundsSliderBar.setFillColor(Color::Black);
    soundsSliderBar.setPosition(500, 270);

    RectangleShape musicSliderBar(Vector2f(sliderBarWidth, sliderBarHeight));
    musicSliderBar.setFillColor(Color::Black);
    musicSliderBar.setPosition(500, 370);

    // Slider knobs
    CircleShape soundsSliderKnob(10);
    soundsSliderKnob.setFillColor(Color::Red);
    soundsSliderKnob.setPosition(500 + (soundsVolume / 100.0f) * sliderBarWidth - 7, 265);

    CircleShape musicSliderKnob(10);
    musicSliderKnob.setFillColor(Color::Red);
    musicSliderKnob.setPosition(500 + (musicVolume / 100.0f) * sliderBarWidth - 7, 365);

    // Menu texts
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
                sound_falling.setVolume(soundsVolume);
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
                sound_falling.setVolume(soundsVolume);
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

        // Update text colors based on selection
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

bool startMenu(Sprite hand, Sprite interface, Sprite enterName, Font font, Text text_start, Text text_sound, Text text_highscore, Text text_exit, Texture& tex_heads, Sprite head, Texture& tex_pauseMenu, Texture& tex_highscore, Sprite& highscore)
{
    int menuSelection = 0;
    bool isPressed = false;

    hand.setPosition(500, 620);

    Font nameFont;
    nameFont.loadFromFile("Arial.ttf");

    Text displayName("", nameFont);
    displayName.setCharacterSize(30);
    displayName.setFillColor(sf::Color::Black);
    displayName.setPosition(210, 758);

    Text displayText;
    displayText.setFont(font);
    displayText.setCharacterSize(40);
    displayText.setFillColor(sf::Color::White);
    displayText.setPosition(50, 850);

    Text error;
    error.setFont(font);
    error.setCharacterSize(30);
    error.setFillColor(sf::Color::Red);
    error.setPosition(50, 900);
    error.setString("");

    // Cursor setup
    Text cursor("|", nameFont);
    cursor.setCharacterSize(25);
    cursor.setFillColor(sf::Color::Black);
    cursor.setPosition(displayName.getPosition().x, displayName.getPosition().y + 2); // Initial position
    bool showCursor = true; // Toggle for blinking effect
    Clock cursorClock; // Clock to control blinking

    while (window.isOpen())
    {
        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
            {
                window.close();
            }

            if (!showMenu)
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
                        // Display error message if name is empty
                        error.setString("You must enter your name!");
                    }

                    else
                    {
                        showMenu = true; // Proceed to the menu
                        error.setString("");
                    }
                }
            }

            else
            {
                if (event.type == Event::KeyPressed && !isPressed)
                {
                    isPressed = true;
                    if (event.key.code == Keyboard::Up)
                    {
                        menu_change.play();
                        menuSelection = (menuSelection - 1 + 4) % 4;
                        hand.setPosition(500, 630 + 50 * menuSelection);
                    }

                    if (event.key.code == Keyboard::Down)
                    {
                        menu_change.play();
                        menuSelection = (menuSelection + 1) % 4;
                        hand.setPosition(500, 630 + 50 * menuSelection);
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
                            soundOptions(hand, font, menu_change, menu_choose, background_music);
                        }

                        if (menuSelection == 2)//--->highscore
                        {
                            showHighscore = true;
                        }

                        else if (menuSelection == 3)//---->exit
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
        }

        if (showHighscore)
        {
            window.clear();
            highscore.setTexture(tex_highscore);
            highscore.setPosition(200, 0);
            highscore.setScale(2, 2);

            // Draw highscore text
            Text text;
            text.setFont(font);
            text.setCharacterSize(50);
            text.setFillColor(sf::Color::Black);
            text.setPosition(250, 180);
            text.setString(copyUserScoreString());

            window.draw(background);
            window.draw(highscore);
            window.draw(text);
            window.display();
            continue;
        }

        if (!showMenu && showCursor)
        {
            window.draw(cursor);
        }

        heads(tex_heads, head);

        if (showMenu)
        {
            // Show menu only after name entry
            text_start.setFillColor(menuSelection == 0 ? Color::Red : Color::Black);
            text_start.setOutlineColor(menuSelection == 0 ? Color::Yellow : Color::Transparent);
            text_start.setOutlineThickness(menuSelection == 0 ? 3 : 0);

            text_sound.setFillColor(menuSelection == 1 ? Color::Red : Color::Black);
            text_sound.setOutlineColor(menuSelection == 1 ? Color::Yellow : Color::Transparent);
            text_sound.setOutlineThickness(menuSelection == 1 ? 3 : 0);
            text_sound.setPosition(571, 680);

            text_highscore.setFillColor(menuSelection == 2 ? Color::Red : Color::Black);
            text_highscore.setOutlineColor(menuSelection == 2 ? Color::Yellow : Color::Transparent);
            text_highscore.setOutlineThickness(menuSelection == 2 ? 3 : 0);
            text_highscore.setPosition(571, 735);

            text_exit.setFillColor(menuSelection == 3 ? Color::Red : Color::Black);
            text_exit.setOutlineColor(menuSelection == 3 ? Color::Yellow : Color::Transparent);
            text_exit.setOutlineThickness(menuSelection == 3 ? 3 : 0);
            text_exit.setPosition(571, 790);

            window.clear();
            window.draw(interface);
            window.draw(text_start);
            window.draw(text_sound);
            window.draw(text_highscore);
            window.draw(text_exit);
            window.draw(hand);
            heads(tex_heads, head);
            window.draw(displayText);
        }
        else
        {
            // Draw error message if it exists
            if (!error.getString().isEmpty())
            {
                window.draw(error);
            }
        }
        window.display();
    }
    return false;
}

bool pauseMenu(Players& player, Sprite interface, Sprite hand, Font font, Text text_sound, Text text_exit, Text text_start, vector<BLOCKS>& blocksList, Text text_play_again, Text& Score)
{
    int menuSelection = 0;
    bool isPressed = false;

    pause_menu.setPosition(150, 200);
    hand.setPosition(310, 240);

    Text text_resume("RESUME", font);
    text_resume.setCharacterSize(40);
    text_resume.setFillColor(Color::Black);
    text_resume.setPosition(420, 250);

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
                    hand.setPosition(310, 240 + 100 * menuSelection);
                }

                if (event.key.code == Keyboard::Down)
                {
                    menu_change.play();
                    menuSelection = (menuSelection + 1) % 4;
                    hand.setPosition(310, 240 + 100 * menuSelection);
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
                        reset(player, blocksList, score, floors, Score);
                        return true;
                    }

                    else if (menuSelection == 2) // ---> Sound
                    {
                        soundOptions(hand, font, menu_change, menu_choose, background_music);
                    }

                    else if (menuSelection == 3) // ---> Exit to Main Menu
                    {
                        reset(player, blocksList, score, floors, Score);
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
        text_play_again.setPosition(380, 350);

        text_sound.setFillColor(menuSelection == 2 ? Color::Red : Color::Black);
        text_sound.setOutlineColor(menuSelection == 2 ? Color::Yellow : Color::Transparent);
        text_sound.setOutlineThickness(menuSelection == 2 ? 2 : 0);
        text_sound.setPosition(430, 450);

        text_exit.setFillColor(menuSelection == 3 ? Color::Red : Color::Black);
        text_exit.setOutlineColor(menuSelection == 3 ? Color::Yellow : Color::Transparent);
        text_exit.setOutlineThickness(menuSelection == 3 ? 2 : 0);
        text_exit.setPosition(450, 550);

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

bool gameOver(Players& player, Sprite hand, Texture& tex_gameover, Texture& tex_pauseMenu, Font& font, Text text_play_again, Text text_exit, vector<BLOCKS>& blocksList, Text& Score, Text& timerText)
{
    int menuSelection = 0;
    bool isPressed = false;

    hand.setPosition(200, 560);
    pause_menu.setPosition(150, 300);

    // Store the current game-specific view
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
                        reset(player, blocksList, score, floors, Score);
                        return true;
                    }

                    else if (menuSelection == 1) // ---> Exit to Main Menu
                    {
                        reset(player, blocksList, score, floors, Score);
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
        window.draw(ground);
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

bool winMenu(RenderWindow& window, Players& player, Sprite hand, Texture& tex_gameover, Texture& tex_pauseMenu, Font& font, Text text_play_again, Text text_exit, vector<BLOCKS>& blocksList, Text& Score)
{
    sound_cheer.play();

    int menuSelection = 0;
    bool isPressed = false;

    hand.setPosition(300, 530);
    pause_menu.setPosition(150, 300);
    newstage.setPosition(0, -550);

    // Store the current game-specific view
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
                    hand.setPosition(300, 530 + 100 * menuSelection);
                }

                if (event.key.code == Keyboard::Down)
                {
                    menu_change.play();
                    menuSelection = (menuSelection + 1) % 2;
                    hand.setPosition(300, 530 + 100 * menuSelection);
                }

                if (event.key.code == Keyboard::Enter)
                {
                    menu_choose.play();
                    if (menuSelection == 0) // ---> Play Again
                    {
                        reset(player, blocksList, score, floors, Score);
                        return true;
                    }

                    else if (menuSelection == 1) // ---> Exit to Main Menu
                    {
                        reset(player, blocksList, score, floors, Score);
                        return false;
                    }
                }
            }

            if (event.type == Event::KeyReleased)
            {
                isPressed = false;
            }
        }

        Floor.setFillColor(Color::Black);
        Floor.setPosition(400, 430);

        Score.setFillColor(Color::Black);
        Score.setPosition(400, 330);

        text_play_again.setFillColor(menuSelection == 0 ? Color::Red : Color::Black);
        text_play_again.setOutlineColor(menuSelection == 0 ? Color::Yellow : Color::Transparent);
        text_play_again.setOutlineThickness(menuSelection == 0 ? 2 : 0);
        text_play_again.setPosition(400, 530);

        text_exit.setFillColor(menuSelection == 1 ? Color::Red : Color::Black);
        text_exit.setOutlineColor(menuSelection == 1 ? Color::Yellow : Color::Transparent);
        text_exit.setOutlineThickness(menuSelection == 1 ? 2 : 0);
        text_exit.setPosition(450, 630);

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
        window.draw(hand);
        window.draw(newstage);
        newstage.move(0, 10);
        window.display();
    }
    return false;
}

void draw(Players& player, vector<BLOCKS>& blocksList, Text& Score, Font& font, Text& timerText)
{
    window.setView(view);

    Vector2f camPos = view.getCenter();
    wall.setPosition(camPos.x - WIDTH / 2, camPos.y - HEIGHT / 2);
    wall2.setPosition(880 + camPos.x - WIDTH / 2, camPos.y - HEIGHT / 2);
    Score.setPosition(camPos.x - WIDTH / 2 + 50, camPos.y - HEIGHT / 2 + 20);

    float elapsedTime = rotation.getElapsedTime().asSeconds();
    float rotationPeriod = 60.0f; // Time for one full rotation (e.g., 60 seconds)
    float angle = (elapsedTime / rotationPeriod) * 360.0f; // Calculate rotation angle
    clockhand.setRotation(angle);


    float time = Timer.getElapsedTime().asSeconds();
    int minutes = static_cast<int>(time) / 60;
    int seconds = static_cast<int>(time) % 60;

    timerText.setPosition(50 + camPos.x - WIDTH / 2, camPos.y - HEIGHT / 2 + 850);
    string timeString = "Time: ";
    if (minutes < 10) timeString += "0";
    timeString += std::to_string(minutes);
    timeString += ":";
    if (seconds < 10) timeString += "0";
    timeString += std::to_string(seconds);

    // Set text
    timerText.setString(timeString);

    Text levelText("Level: " + to_string(currentLevel), font);
    levelText.setCharacterSize(50);
    levelText.setFillColor(Color::White);
    levelText.setPosition(camPos.x - WIDTH / 2 + 600, camPos.y - HEIGHT / 2 + 20);

    clockFill = FillTime.getElapsedTime().asSeconds() / maxFilltime;

    if (clockFill > 1.0f)
    {
        clockFill = 0.0f;
        FillTime.restart();
    }

    // Draw background based on player position
    if (barrierSpawned && player.sprite.getPosition().y <= levelBarrier.getPosition().y)
    {
        background.setTexture(tex_background2);
        background.setScale(static_cast<float>(window.getSize().x) / tex_background2.getSize().x,
            static_cast<float>(window.getSize().y) / tex_background2.getSize().y);
    }

    else
    {
        background.setTexture(tex_background);
        background.setScale(static_cast<float>(window.getSize().x) / tex_background.getSize().x,
            static_cast<float>(window.getSize().y) / tex_background.getSize().y);
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

    window.draw(wall);
    window.draw(wall2);

    for (auto& block : blocksList)
    {
        window.draw(block.blocksSprite);
    }

    window.draw(ground);
    window.draw(player.sprite);
    window.draw(Score);
    window.draw(levelText);
    window.draw(timerText);
    window.setView(window.getDefaultView());
    window.draw(clock1);
    window.draw(clockhand);
    OclockFill.setSize(Vector2f(26, 152 * clockFill));
    OclockFill.setPosition(49, 380 - 152 * clockFill);
    window.draw(OclockFill);

    if (currentLevel > 1)
    {
        newstage.setPosition(0, levelBarrier.getPosition().y - 500);
        newstage.move(0, 10);
        window.draw(newstage);

        if (transitionClock.getElapsedTime().asSeconds() < 0.75f)
        {
            Text transitionText("Level " + to_string(currentLevel), font);
            transitionText.setCharacterSize(100);
            transitionText.setFillColor(Color::White);
            transitionText.setOutlineColor(Color::Black);
            transitionText.setOutlineThickness(10);
            transitionText.setPosition(camPos.x - transitionText.getGlobalBounds().width / 2, camPos.y - transitionText.getGlobalBounds().height / 2);
            window.draw(transitionText);
        }
    }
    window.setView(view);
}

int main()
{
    window.setFramerateLimit(60);
    initializeObject();
    loadUserData();

    /*background_music.setLoop(true);
    background_music.play();*/

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
    Score.setPosition(50, 900);

    Text timerText;
    timerText.setFont(font);
    timerText.setCharacterSize(30);
    timerText.setFillColor(sf::Color::White);
    timerText.setCharacterSize(50);
    timerText.setFillColor(Color::White);

    bool start_game = startMenu(hand, interface, enterName, font, text_start, text_sound, text_highscore, text_exit, tex_heads, head, tex_pauseMenu, tex_highscore, highscore);

    if (start_game)
    {
        Players player;
        Clock clock;
        vector<BLOCKS> blocksList;
        player.sprite.setOrigin(player.sprite.getLocalBounds().width / 2, 0);

        random_device rd;
        srand(rd());
        generationBlocks(blocksList, player, currentLevel);
        lastPosition = player.sprite.getPosition().y;

        view.setSize(WIDTH, HEIGHT);
        view.setCenter(Vector2f(500, 520));
        window.setView(view);

        while (window.isOpen())
        {
            float deltatime = clock.restart().asSeconds();
            if (!viewpaused && player.sprite.getPosition().y < HEIGHT / 3)
            {
                if (currentLevel == 1)
                {
                    view.move(0, -550 * deltatime);
                }
                else
                {
                    view.move(0, -500 * deltatime);
                }

                window.setView(view);
            }

            if (viewpaused && viewtimer.getElapsedTime().asSeconds() >= VIEW_PAUSE_DURATION)
            {
                viewpaused = false; // Resume view movement
            }

            Event event;
            while (window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed)
                {
                    window.close();
                }

            }

            // Pause menu
            if (Keyboard::isKeyPressed(Keyboard::Escape) || Keyboard::isKeyPressed(Keyboard::P))
            {
                bool resumeGame = pauseMenu(player, interface, hand, font, text_sound, text_exit, text_start, blocksList, text_play_again, Score);
                if (!resumeGame)
                {
                    startMenu(hand, interface, enterName, font, text_start, text_sound, text_highscore, text_exit, tex_heads, head, tex_pauseMenu, tex_highscore, highscore);
                }
            }

            // Game over
            float viewBottom = view.getCenter().y + HEIGHT / 2;
            if (player.sprite.getPosition().y > viewBottom)
            {
                sound_falling.stop();
                sound_gonna_fall.stop();
                if (lives > 1)
                {
                    // Player has remaining lives
                    lives--;
                    player.sprite.setPosition(lastSafePosition);
                    player.velocity_x = 0;
                    player.velocity_y = 0;
                    player.frameIndex = 0;
                    player.sprite.setTextureRect(IntRect(0, 0, 38, 71));
                    isGround = true;
                    view.setCenter(Vector2f(500, lastSafePosition.y + 100)); // Center view on player
                    window.setView(view);
                    viewpaused = true;
                    viewtimer.restart();
                }

                else
                {
                    // No lives left - game over
                    sound_gameover.play();
                    bool resumeGame = gameOver(player, hand, tex_gameover, tex_pauseMenu, font, text_play_again, text_exit, blocksList, Score, timerText);

                    if (!resumeGame)
                    {
                        startMenu(hand, interface, enterName, font, text_start, text_sound, text_highscore, text_exit, tex_heads, head, tex_pauseMenu, tex_highscore, highscore);
                    }
                    win = false;
                }
            }

            // Collision detection with the ground
            if (player.sprite.getGlobalBounds().intersects(ground.getGlobalBounds()))
            {
                sound_falling.stop();
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

            //Collision detection with the left wall
            if (player.sprite.getGlobalBounds().intersects(wall.getGlobalBounds()))
            {
                player.sprite.setPosition(wall.getGlobalBounds().left + wall.getGlobalBounds().width + 50, player.sprite.getPosition().y);
            }

            //Collision detection with the right wall
            else if (player.sprite.getGlobalBounds().intersects(wall2.getGlobalBounds()))
            {
                player.sprite.setPosition(wall2.getGlobalBounds().left - player.sprite.getGlobalBounds().width + 50, player.sprite.getPosition().y);
            }

            // Collision detection with blocks 
            collision(blocksList, player, font, Score, text_start, text_exit, text_play_again, text_highscore, text_sound);

            // Upditing blocks for level 2
            if (currentLevel == 2)
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

                    bool resumeGame = winMenu(window, player, hand, tex_gameover, tex_pauseMenu, font, text_play_again, text_exit, blocksList, Score);
                    if (!resumeGame)
                    {
                        startMenu(hand, interface, enterName, font, text_start, text_sound, text_highscore, text_exit, tex_heads, head, tex_pauseMenu, tex_highscore, highscore);
                    }
                    else
                    {
                        reset(player, blocksList, score, floors, Score);
                    }
                }
            }
            player.jump(deltatime);
            player.sprite.move(player.velocity_x * deltatime, player.velocity_y * deltatime);
            player.handleMovement(deltatime);

            window.clear();
            draw(player, blocksList, Score, font, timerText);
            window.display();
        }
    }
    return 0;
}
