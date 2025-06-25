#pragma once
#include <SFML/Graphics.hpp>

using namespace sf;

extern RenderWindow window;
extern View view;

void adjustViewAspectRatio( float desiredAspectRatio)
{
    float windowWidth = static_cast<float>(window.getSize().x);
    float windowHeight = static_cast<float>(window.getSize().y);
    float windowAspectRatio = windowWidth / windowHeight;

    FloatRect viewport(0, 0, 1, 1);

    if (windowAspectRatio > desiredAspectRatio)
    {
        float viewportWidth = desiredAspectRatio / windowAspectRatio;
        viewport.left = (1.0f - viewportWidth) / 2.0f;
        viewport.width = viewportWidth;
    }

    else
    {
        float viewportHeight = windowAspectRatio / desiredAspectRatio;
        viewport.top = (1.0f - viewportHeight) / 2.0f;
        viewport.height = viewportHeight;
    }

    view.setViewport(viewport);
    window.setView(view);
}
