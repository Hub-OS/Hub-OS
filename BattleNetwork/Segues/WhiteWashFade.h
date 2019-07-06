#pragma once
#include <Swoosh/Segue.h>
#include <Swoosh/Ease.h>

using namespace swoosh;

class WhiteWashFade : public Segue {
    sf::Texture* white;

public:
    virtual void onDraw(sf::RenderTexture& surface) {
        double elapsed = getElapsed().asMilliseconds();
        double duration = getDuration().asMilliseconds();
        double alpha = ease::wideParabola(elapsed, duration, 1.0);

        if (elapsed <= duration * 0.5)
            this->drawLastActivity(surface);
        else
            this->drawNextActivity(surface);

        sf::RectangleShape whiteout;
        whiteout.setSize(sf::Vector2f((float)surface.getSize().x, (float)surface.getSize().y));
        whiteout.setFillColor(sf::Color(255, 255, 255, (sf::Uint8)(alpha*255)));
        whiteout.setTexture(white, true);
        surface.draw(whiteout);
    }

    WhiteWashFade(sf::Time duration, Activity* last, Activity* next) : Segue(duration, last, next) {
        white = new sf::Texture();

        sf::Image buffer;
        buffer.create(1, 1, sf::Color::White);

        // Load an sf::Texture with the image data
        white->loadFromImage(buffer);
        white->setRepeated(true);
    }

    virtual ~WhiteWashFade() { delete white; }
};
