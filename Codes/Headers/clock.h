#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

using namespace sf;

Clock transitionClock;
static Clock FillTime;
float clockFill = 0.0f;

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
