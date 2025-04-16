#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include<Vector>
#include <random> 

using namespace sf;
using namespace std;

// Constants
const int WIDTH = 1000;
const int HEIGHT = 1000;
const float  moveSpeed = 200.0f;
const float jump_strength = -10000.0f;
const float gravity = 25000.0f;

//---->blocks constants<----
int Xleft, Xright;
float blockWidth = 180.0f;
float blockHeight = 35.0f;
float xscale;
const int blocksNum = 5;

float lastPosition;
int score = 0;
int floors = 0;

//---->booleans<----
bool isGround = false;
bool once = true;
bool isMovingleft = 0;
bool isMovingright = 0;

// Textures
Texture tex_background, tex_ground, tex_wall_right, tex_wall_left, tex_player, tex_blocks, tex_interface, tex_hand, tex_pauseMenu;

// Sprites
Sprite background, ground, wall, wall2, interface, hand, pause_menu;
 
//Sounds

SoundBuffer buffer_menu_choose, buffer_menu_change, buffer_jump;

Sound menu_choose,menu_change, jump_sound;
Music background_music;


// Blocks struct
struct BLOCKS
{
    Sprite blocksSprite;
    BLOCKS(Texture& texblocks, float xposition, float yposition)
    {

        xscale = (rand() % 2) + 1.5;
        blocksSprite.setTexture(texblocks);
        blocksSprite.setPosition(xposition, yposition);
        blocksSprite.setScale(xscale, 1);
    }
};


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

        if (!tex_player.loadFromFile("player.png"))
        {
            throw std::runtime_error("Failed to load player texture");
        }

        sprite.setTexture(tex_player);
        sprite.setTextureRect(IntRect(0, 0, 38, 71));
        sprite.setOrigin(21, 71);
        sprite.setPosition(500, 650);
        sprite.setScale(2.4f, 2.4f);
    }

    void handleMovement(float deltatime)
    {
        frameTimer += deltatime; // Accumulate time

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
            velocity_x = 0;
            if (isGround)
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
        const float maxJumpTime = 0.3f;  // Allow gradual jump for 0.2 seconds
        const float jumpAcceleration = -200000.0f; // Smooth upwards force

        if (Keyboard::isKeyPressed(Keyboard::Space) && once && isGround)
        {
            jump_sound.play();
            jumpTimer = 0.0f;
            isGround = false;
            once = false;
            velocity_y = jump_strength; // Initial impulse (small boost)
        }

        if (!isGround)
        {
            jumpTimer += deltatime;

            // Apply upward force gradually for a short duration
            if (jumpTimer < maxJumpTime && Keyboard::isKeyPressed(Keyboard::Space))
            {
                velocity_y += jumpAcceleration * deltatime;  // Smooth boost
            }

            velocity_y += gravity * deltatime;  // Continue applying gravity

            // Apply max jump speed to avoid too strong jumps
            const float maxJumpSpeed = -1200.0f;
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
            if (velocity_y < 0) // Going up
            {
                if (isMovingright)
                {
                    if (frameIndex != 6)
                    {
                        frameIndex = 6;
                        sprite.setScale(2.4, 2.4); // Ensure facing right
                        sprite.setTextureRect(IntRect(frameIndex * 38, 0, 38, 71));
                    }
                }
                else if (isMovingleft)
                {
                    if (frameIndex != 6)
                    {
                        frameIndex = 6;
                        sprite.setScale(-2.4, 2.4); // Ensure facing left
                        sprite.setTextureRect(IntRect(frameIndex * 38, 0, 38, 71));
                    }
                }
                else
                {
                    if (frameIndex != 5)
                    {
                        frameIndex = 5; // Straight jump
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

void initializeObject(RenderWindow& window)
{
    // Load textures
    if (!tex_background.loadFromFile("background.jpg")) return;
    if (!tex_ground.loadFromFile("ground.jpg")) return;
    if (!tex_wall_left.loadFromFile("wall left.png")) return;
    if (!tex_wall_right.loadFromFile("wall flipped right.png")) return;
    if (!tex_player.loadFromFile("player.png")) return;
    if (!tex_blocks.loadFromFile("block.png")) return;
    if (!tex_interface.loadFromFile("mainMenu.png")) return;
    if (!tex_hand.loadFromFile("hand.png")) return;
    if (!tex_pauseMenu.loadFromFile("pauseMenu.png")) return;
    if (!buffer_menu_choose.loadFromFile("menu_choose.ogg"))return;
    if (!buffer_menu_change.loadFromFile("menu_change.ogg"))return;
    if (!buffer_jump.loadFromFile("jump.ogg"))return;
    if (!background_music.openFromFile("backgroundMusic.wav"))return;
    //Sounds
    menu_choose.setBuffer(buffer_menu_choose);
    menu_change.setBuffer(buffer_menu_change);
    jump_sound.setBuffer(buffer_jump);
    

    pause_menu.setTexture(tex_pauseMenu);
    pause_menu.setPosition(150, 200);
    pause_menu.setScale(1.5, 2);

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
    ground.setPosition(-50, 800);
    ground.setScale(1.5, 1.5);

    wall.setTexture(tex_wall_left);
    wall.setPosition(0, 0);
    wall.setScale(1.5, 3);
    Xleft = wall.getGlobalBounds().width + 50;

    wall2.setTexture(tex_wall_right);
    wall2.setPosition(880, 0);
    wall2.setScale(1.5, 3);
    Xright = wall2.getGlobalBounds().width + 450;

}

void generationBlocks(Texture& texblocks, vector<BLOCKS>& blocksList)
{
    float xposition, yposition = 1000 - 200; // 375 is the height of the ground

    for (int i = 0; i < blocksNum; i++)
    {
        yposition -= 150 + (rand() % 60);    //  changing distance between blocks
        float xposition = (rand() % (Xright - Xleft + 1)) + Xleft;
        blocksList.emplace_back(texblocks, xposition, yposition);

    }
}



bool soundOptions(RenderWindow& window, Sprite hand, Font font, Sound& menu_change, Sound& menu_choose,Music& background_music)
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

    // Slider bars
    RectangleShape soundsSliderBar(Vector2f(sliderBarWidth, sliderBarHeight));
    soundsSliderBar.setFillColor(Color::Black);
    soundsSliderBar.setPosition(500, 260); // 

    RectangleShape musicSliderBar(Vector2f(sliderBarWidth, sliderBarHeight));
    musicSliderBar.setFillColor(Color::Black);
    musicSliderBar.setPosition(500, 360); 

    // Slider knobs
    CircleShape soundsSliderKnob(10); 
    soundsSliderKnob.setFillColor(Color::Red);
    soundsSliderKnob.setPosition(500 + (soundsVolume / 100.0f) * sliderBarWidth -7, 255); 

    CircleShape musicSliderKnob(10); 
    musicSliderKnob.setFillColor(Color::Red);
    musicSliderKnob.setPosition(500 + (musicVolume / 100.0f) * sliderBarWidth - 7, 355); // Initial position

    // Menu texts
    Text text_Sounds("SOUNDS", font, 50);
    text_Sounds.setStyle(Text::Bold);
    text_Sounds.setCharacterSize(35);
    text_Sounds.setFillColor(Color::Black);
    text_Sounds.setPosition(300, 250);

    Text text_Music("MUSIC", font, 50);
    text_Music.setStyle(Text::Bold);
    text_Music.setCharacterSize(35);
    text_Music.setFillColor(Color::Black);
    text_Music.setPosition(300, 350);

    Text text_Back("BACK", font, 50);
    text_Back.setStyle(Text::Bold);
    text_Back.setCharacterSize(35);
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
                soundsSliderKnob.setPosition(500 + (soundsVolume / 100.0f) * sliderBarWidth - 7, 255);

                menu_change.setVolume(soundsVolume);
                menu_choose.setVolume(soundsVolume);
                jump_sound.setVolume(soundsVolume);
            }
            if (Keyboard::isKeyPressed(Keyboard::Right))
            {
                soundsVolume = min(100, soundsVolume + 1); 
                soundsSliderKnob.setPosition(500 + (soundsVolume / 100.0f) * sliderBarWidth - 7, 255);

                menu_change.setVolume(soundsVolume);
                menu_choose.setVolume(soundsVolume);
                jump_sound.setVolume(soundsVolume);
            }
        }
        else if (menuSelection == 1) // ---> Music
        {
            if (Keyboard::isKeyPressed(Keyboard::Left))
            {
                musicVolume = max(0, musicVolume - 1); // decrease volume
                musicSliderKnob.setPosition(500 + (musicVolume / 100.0f) * sliderBarWidth - 7, 355);
                background_music.setVolume(musicVolume); // update music volume
            }
            if (Keyboard::isKeyPressed(Keyboard::Right))
            {
                musicVolume = min(100, musicVolume + 1); 
                musicSliderKnob.setPosition(500 + (musicVolume / 100.0f) * sliderBarWidth - 7, 355);
                background_music.setVolume(musicVolume); 
            }
        }

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

        window.display();
    }
    return false;
}

bool startMenu(RenderWindow& window, Sprite hand, Sprite interface, Font font,Text text_start, Text text_sound,Text text_exit)
{

    int  menuSelection = 0; // Start with "START" selected
    bool isPressed = false; // To track if a key is being held
    hand.setPosition(500, 615);

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
                    hand.setPosition(500, 620 + 50 * menuSelection);
                }

                if (event.key.code == Keyboard::Down)
                {
                    menu_change.play();
                    menuSelection = (menuSelection + 1) % 3;
                    hand.setPosition(500, 620 + 50 * menuSelection);
                }

                if (event.key.code == Keyboard::Enter)
                {
                    menu_choose.play();
                    if (menuSelection == 0) // ---> Start
                    {
                        return true;
                    }
                    if (menuSelection == 1) // ---> Start
                    {
                        soundOptions(window, hand, font, menu_change, menu_choose, background_music);
                    }

                    else if (menuSelection == 2) // ----> Exit
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

        text_start.setFillColor(menuSelection == 0 ? Color::Red : Color::Black);
        text_start.setOutlineColor(menuSelection == 0 ? Color::Yellow : Color::Transparent);
        text_start.setOutlineThickness(menuSelection == 0 ? 2 : 0);

        text_sound.setFillColor(menuSelection == 1 ? Color::Red : Color::Black);
        text_sound.setOutlineColor(menuSelection == 1 ? Color::Yellow : Color::Transparent);
        text_sound.setOutlineThickness(menuSelection == 1 ? 2 : 0);
        text_sound.setPosition(571, 670);

        text_exit.setFillColor(menuSelection == 2 ? Color::Red : Color::Black);
        text_exit.setOutlineColor(menuSelection == 2 ? Color::Yellow : Color::Transparent);
        text_exit.setOutlineThickness(menuSelection == 2 ? 2 : 0);
        text_exit.setPosition(571, 725);

        window.clear();
        window.draw(interface);
        window.draw(text_start);
        window.draw(text_sound);
        window.draw(text_exit);
        window.draw(hand);
        window.display();
    }
    return false;
}

void reset(RenderWindow& window, Players& player, vector<BLOCKS>& blocksList, int &score, int &floors)
{
    score = 0;
    floors = 0;
    blocksList.clear(); 
    generationBlocks(tex_blocks, blocksList); 
    player.sprite.setPosition(500, 650); 
    player.velocity_x = 0; 
    player.velocity_y = 0; 
    player.frameIndex = 0; 
    player.sprite.setTextureRect(IntRect(0, 0, 38, 71)); 
    
    lastPosition = 800;
}



bool pauseMenu(RenderWindow& window, Players& player, Sprite interface ,Sprite hand, Font font, Text text_sound, Text text_exit, Text text_start, vector<BLOCKS>& blocksList)
{
    int menuSelection = 0; 
    bool isPressed = false; 
    hand.setPosition(310, 240);

    
    Text text_resume("RESUME", font, 50);
    text_resume.setStyle(Text::Bold);
    text_resume.setCharacterSize(35);
    text_resume.setFillColor(Color::Black);
    text_resume.setPosition(420, 250);

    Text text_play_again("PLAY AGAIN", font, 50);
    text_play_again.setStyle(Text::Bold);
    text_play_again.setCharacterSize(35);
    text_play_again.setFillColor(Color::Black);
    text_play_again.setPosition(380, 350);

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
                        return true; 
                    }
                    else if (menuSelection == 1) // ---> Play Again
                    {
                        reset( window, player, blocksList,  score,  floors);
                        return true; 
                    }
                    else if (menuSelection == 2) // ---> Sound
                    {
                        soundOptions(window, hand, font, menu_change, menu_choose,background_music);
                        
                    }
                    else if (menuSelection == 3) // ---> Exit to Main Menu
                    {
                        reset(window, player, blocksList, score, floors);
                        return false;
                    }
                }
            }
            if (event.type == Event::KeyReleased)
            {
                isPressed = false;
            }
        }

        // Update text colors based on selection
        text_resume.setFillColor(menuSelection == 0 ? Color::Red : Color::Black);
        text_resume.setOutlineColor(menuSelection == 0 ? Color::Yellow : Color::Transparent);
        text_resume.setOutlineThickness(menuSelection == 0 ? 2 : 0);

        text_play_again.setFillColor(menuSelection == 1 ? Color::Red : Color::Black);
        text_play_again.setOutlineColor(menuSelection == 1 ? Color::Yellow : Color::Transparent);
        text_play_again.setOutlineThickness(menuSelection == 1 ? 2 : 0);

        text_sound.setFillColor(menuSelection == 2 ? Color::Red : Color::Black);
        text_sound.setOutlineColor(menuSelection == 2 ? Color::Yellow : Color::Transparent);
        text_sound.setOutlineThickness(menuSelection == 2 ? 2 : 0);
        text_sound.setPosition(430, 450);

        text_exit.setFillColor(menuSelection == 3 ? Color::Red : Color::Black);
        text_exit.setOutlineColor(menuSelection == 3 ? Color::Yellow : Color::Transparent);
        text_exit.setOutlineThickness(menuSelection == 3 ? 2 : 0);
        text_exit.setPosition(450, 550);

        // Draw the pause menu
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
    //return false;
}

void draw(RenderWindow& window, Players& player, vector<BLOCKS>& blocksList)
{
    window.draw(background);
    for (auto& block : blocksList)
    {
        window.draw(block.blocksSprite);
    }
    window.draw(ground);
    window.draw(wall);
    window.draw(wall2);
    window.draw(player.sprite);
}

int main()
{
    
    RenderWindow window(VideoMode(WIDTH, HEIGHT), "Icy Tower SFML");

    window.setFramerateLimit(60);

    initializeObject(window);
    background_music.setLoop(true);
    background_music.play();
    Font font;
    font.loadFromFile("RushDriver-Italic.otf");
    
    Text text_start("START", font, 50);
    text_start.setStyle(Text::Bold);
    text_start.setCharacterSize(35);
    text_start.setFillColor(Color::Black);
    text_start.setPosition(571, 620);

    Text text_exit("EXIT", font, 50);
    text_exit.setStyle(Text::Bold);
    text_exit.setCharacterSize(35);
    text_exit.setFillColor(Color::Black);

    Text text_sound("SOUND", font, 50);
    text_sound.setStyle(Text::Bold);
    text_sound.setCharacterSize(35);
    text_sound.setFillColor(Color::Black);

    bool start_game = startMenu(window, hand, interface, font,text_start, text_sound,text_exit);


    if (start_game)
    {
        
        Players player;
        Clock clock;
        vector <BLOCKS> blocksList;

        player.sprite.setOrigin(player.sprite.getLocalBounds().width / 2, 0);

        random_device rd; // more than one 
        srand(rd());
        generationBlocks(tex_blocks, blocksList);

        lastPosition = player.sprite.getPosition().y;
        //bool running = true;
        while ( window.isOpen())
        {
            float deltatime = clock.restart().asSeconds();

            Event event;
            while (window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed)
                {
                    window.close();
                }
            }

            if (Keyboard::isKeyPressed(Keyboard::Escape) || Keyboard::isKeyPressed(Keyboard::P))
            {
                bool resumeGame = pauseMenu(window, player, interface, hand, font, text_sound, text_exit, text_start, blocksList);
                if (!resumeGame)
                {
                    startMenu(window, hand, interface, font,text_start, text_sound,text_exit);
                }
            }

            //Collision detection with the ground
            if (player.sprite.getGlobalBounds().intersects(ground.getGlobalBounds()))
            {
                player.sprite.setPosition(player.sprite.getPosition().x, ground.getPosition().y + 15 - player.sprite.getGlobalBounds().height);
                isGround = true;
                player.velocity_y = 0;
                once = true;

                // Reset to idle if player is not moving
                if (player.velocity_x == 0)
                {
                    player.frameIndex = 0;
                    player.sprite.setTextureRect(IntRect(0, 0, 42, 71));
                }
            }

            //Collision detection with the left wall
            if (player.sprite.getGlobalBounds().intersects(wall.getGlobalBounds()))
            {
                player.sprite.setPosition(wall.getGlobalBounds().left + wall.getGlobalBounds().width + 50, player.sprite.getPosition().y);  // Position on the wall
                player.velocity_x = 0;
            }

            //Collision detection with the right wall
            else if (player.sprite.getGlobalBounds().intersects(wall2.getGlobalBounds()))
            {
                player.sprite.setPosition(wall2.getGlobalBounds().left - player.sprite.getGlobalBounds().width + 50, player.sprite.getPosition().y);  // Position on the wall
                player.velocity_x = 0;
            }

            // Collision detection with blocks ------- Track the block the player is currently standing on
            static BLOCKS* currentBlock = nullptr;

            for (auto& block : blocksList)
            {
                if (player.sprite.getGlobalBounds().intersects(block.blocksSprite.getGlobalBounds()))
                {
                    // Check if the player is falling onto the block
                    if (player.velocity_y > 0 &&
                        (player.sprite.getPosition().y + player.sprite.getGlobalBounds().height - 50
                            <= block.blocksSprite.getPosition().y))
                    {

                        // Place player on top of the block
                        player.sprite.setPosition(player.sprite.getPosition().x,
                            block.blocksSprite.getPosition().y - player.sprite.getGlobalBounds().height + 12);

                        // Stop falling
                        isGround = true;
                        if (player.sprite.getPosition().y < lastPosition)
                        {
                            floors++;
                            score += 10;
                            cout << "Score" << score << endl;
                            cout << "Floors: " << floors << endl;
                            lastPosition = player.sprite.getPosition().y;
                        }
                        player.velocity_y = 0;
                        once = true;

                        // Store the current block
                        currentBlock = &block;
                    }
                }
            }

            // Check if the player has stepped off the block
            if (currentBlock)
            {
                float blockLeft = currentBlock->blocksSprite.getPosition().x;
                float blockRight = blockLeft + currentBlock->blocksSprite.getGlobalBounds().width;

                float playerLeft = player.sprite.getPosition().x - (player.sprite.getGlobalBounds().width / 2);
                float playerRight = player.sprite.getPosition().x + (player.sprite.getGlobalBounds().width / 2);

                // If the player's center is outside the block's bounds, start falling
                if (playerRight < blockLeft || playerLeft > blockRight)
                {
                    isGround = false;
                    currentBlock = nullptr; // Player is no longer on a block
                }
            }

            player.jump(deltatime);

            player.sprite.move(player.velocity_x * deltatime, player.velocity_y * deltatime);
            player.handleMovement(deltatime);

            window.clear();

            draw(window, player, blocksList);
            window.display();
        }
    }
}
