#pragma once
#include <SFML/Graphics.hpp>

using namespace sf;

extern Texture tex_heads;
extern Sprite head;
extern RenderWindow window;

void heads()
{
    static Clock clock;
    static int faceframeindex = 0;
    const float angles[3] = { -11, 0, 11 };
    float timer = clock.getElapsedTime().asSeconds(), maxtimer = 0.4f;

    head.setTexture(tex_heads);
    head.setOrigin(Vector2f(104, 256));
    head.setScale(2.3f, 2.3f);
    head.setPosition(1520, 590);

    if (timer > maxtimer)
    {
        faceframeindex = (faceframeindex + 1) % 3;
        head.setRotation(angles[faceframeindex]);
        clock.restart();
    }

    head.setTextureRect(IntRect(faceframeindex * 208, 0, 208, 512));
    window.draw(head);
}
