
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
const float jump_strength = -4000.0f;
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

// Textures
Texture tex_background, tex_ground, tex_wall_right, tex_wall_left, tex_player, tex_blocks, tex_interface;

// Sprites
Sprite background, ground, wall, wall2, Myinterface;

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
        sprite.setTextureRect(IntRect(0, 0, 42, 71));
        sprite.setOrigin(21, 71);
        sprite.setPosition(500, 650);
        sprite.setScale(2.4f, 2.4f);
    }

    void handleMovement(float deltatime) {
        frameTimer += deltatime; // Accumulate time

        if (Keyboard::isKeyPressed(Keyboard::Right))
        {
            sprite.setScale(2.4, 2.4);
            velocity_x = moveSpeed;

            if (frameTimer >= 0.1f)
            {
                frameIndex = (frameIndex + 1) % 4;
                frameTimer = 0;
            }
            sprite.setTextureRect(IntRect(frameIndex * 37, 0, 42, 71));
        }
        else if (Keyboard::isKeyPressed(Keyboard::Left))
        {
            sprite.setScale(-2.4, 2.4);
            velocity_x = -moveSpeed;

            if (frameTimer >= 0.1f)
            {
                frameIndex = (frameIndex + 1) % 4;
                frameTimer = 0;
            }
            sprite.setTextureRect(IntRect(frameIndex * 37, 0, 42, 71));
        }
        else
        {
            velocity_x = 0;
            if (isGround)
            {
                frameIndex = 0;
                sprite.setTextureRect(IntRect(0, 0, 42, 71));
            }
        }

        sprite.move(velocity_x * deltatime, 0);
    }

    void jump(float deltatime)
    {
        if (Keyboard::isKeyPressed(Keyboard::Space) && once && isGround)
        {
            velocity_y = jump_strength;
            isGround = false;
            once = false;
        }

        if (!isGround)
        {
            velocity_y += gravity * deltatime;
            frameIndex = 5;
            sprite.setTextureRect(IntRect(frameIndex * 37, 0, 42, 71));

            if (velocity_x > 0)
            {
                frameIndex = 6;
                sprite.setTextureRect(IntRect(frameIndex * 37, 0, 42, 71));
            }

            if (velocity_x < 0)
            {
                frameIndex = 6;
                sprite.setTextureRect(IntRect(frameIndex * -37, 0, 42, 71));
            }
        }

        if (velocity_y > 0)
        {
            frameIndex = 8;
            sprite.setTextureRect(IntRect(frameIndex * 37, 0, 42, 71));
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

    Myinterface.setTexture(tex_interface);
    Myinterface.setScale(1.25, 1.6);
    Myinterface.setPosition(window.getSize().x / 8, window.getSize().y / 8);

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
// text in interface  
void text_display(RenderWindow& window)
{
    Font font;
    font.loadFromFile("RushDriver-Italic.otf");
    Text text("START \n   GAME!!", font, 50);
    text.setStyle(Text::Bold | Text::Italic);
    text.setCharacterSize(50);
    text.setFillColor(Color::Black);
    text.setPosition(550, 650);
}


void draw(RenderWindow& window, Players& player, vector<BLOCKS>& blocksList)
{
    window.draw(Myinterface);
    window.draw(background);
    for (auto& block : blocksList)
    {
        window.draw(block.blocksSprite);
    }
    window.draw(ground);
    window.draw(wall);
    window.draw(wall2);
    window.draw(player.sprite);
    //window.draw(text);

}


int main()
{
    
    RenderWindow window(VideoMode(WIDTH, HEIGHT), "Icy Tower SFML");

    window.setFramerateLimit(60);

    initializeObject(window);
    Players player;
    Clock clock;
    vector <BLOCKS> blocksList;

    player.sprite.setOrigin(player.sprite.getLocalBounds().width / 2, 0);

    random_device rd; // more than one 
    srand(rd());
    generationBlocks(tex_blocks, blocksList);

    lastPosition = player.sprite.getPosition().y;

    while (window.isOpen())
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
        text_display(window);
        draw(window, player, blocksList);
        window.display();
    }
}
