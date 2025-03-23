#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

using namespace sf;

// Constants
const int WIDTH = 809;
const int HEIGHT = 730;
const float  moveSpeed = 2.0f;
// Textures
Texture tex_b, tex_g, tex_w, tex_w2, tex_player;

// Sprites
Sprite background;
Sprite ground;
Sprite wall;
Sprite wall2;



// Player Struct
struct Players {
    Sprite sprite;
    int velocity_x;
    int velocity_y;

    // Constructor to load the player sprite
    Players() {
        velocity_x = 0;
        velocity_y = 0;

        // Load the player texture
        if (!tex_player.loadFromFile("player.png")) {
            throw std::runtime_error("Failed to load player texture");
        }

        sprite.setTexture(tex_player);
        sprite.setTextureRect(IntRect(0, 0, 58, 60)); // Crop sprite
        sprite.setPosition(300, 420);
        sprite.setScale(3.0f, 3.0f);
    }

    // Move function
    void move(float dx, float dy) {
        sprite.move(dx, dy);
    }
};



void initializeObject(RenderWindow& window) {
    // Load textures
    if (!tex_b.loadFromFile("background.jpg")) return;
    if (!tex_g.loadFromFile("ground.jpg")) return;
    if (!tex_w.loadFromFile("wall.jpg")) return;
    if (!tex_w2.loadFromFile("wall2.jpg")) return;
    if (!tex_player.loadFromFile("player.png")) return;


    background.setTexture(tex_b);
    background.setScale(static_cast<float>(window.getSize().x) / tex_b.getSize().x,
        static_cast<float>(window.getSize().y) / tex_b.getSize().y);
    background.setPosition(0, 0);

    ground.setTexture(tex_g);
    ground.setPosition(-50, 630);
    ground.setScale(1.5, 1.5);

    wall.setTexture(tex_w);
    wall.setPosition(0, 0);
    wall.setScale(1.5, 1.5);

    wall2.setTexture(tex_w2);
    wall2.setPosition(685, 0);
    wall2.setScale(1.5, 1.5);



}

void draw(RenderWindow& window, Players& player) {
    window.draw(background);
    window.draw(ground);
    window.draw(wall);
    window.draw(wall2);
    window.draw(player.sprite);
}

int main() {
    RenderWindow window(VideoMode(WIDTH, HEIGHT), "Icy Tower SFML");

    initializeObject(window);
    Players player;
    // Initialization
    bool isGround = false;
    int i = 0;
    
    


    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }


            
        if (event.type == Event::KeyPressed) {
            // Move right
            if (event.key.code == Keyboard::Right) {
                player.sprite.setScale(3, 3);
                player.velocity_x = moveSpeed;
                player.sprite.setTextureRect(IntRect(i * 39, 0, 39, 70));
                i++;
                i = i % 4;
            }
            // Move left
            if (event.key.code == Keyboard::Left) {
                player.sprite.setScale(-3, 3);
                player.velocity_x = -moveSpeed;
                player.sprite.setTextureRect(IntRect(i * 39, 0, 39, 70));
                i++;
                i = i % 4;
            }
        }













           
        window.clear();
        draw(window, player);
        window.display();
    }
}
