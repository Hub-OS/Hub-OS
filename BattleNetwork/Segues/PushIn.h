#pragma once
#include <Swoosh/Segue.h>
#include <Swoosh/Ease.h>

using namespace swoosh;

template<int direction>
class PushIn : public Segue {
private:
    sf::Texture temp, temp2;
    bool loaded;

public:

    virtual void onDraw(sf::RenderTexture& surface) {
        double elapsed = getElapsed().asMilliseconds();
        double duration = getDuration().asMilliseconds();
        double alpha = ease::linear(elapsed, duration, 1.0);

        surface.clear(this->getLastActivityBGColor());
        this->drawLastActivity(surface);

        surface.display(); // flip and ready the buffer

#ifdef __ANDROID__
        if(!loaded) {
                temp = sf::Texture(surface.getTexture()); // Make a copy of the source texture
                temp.flip(true);
        }
#else
        temp = sf::Texture(surface.getTexture()); // Make a copy of the source texture
#endif

        sf::Sprite left(temp);

        int lr = 0;
        int ud = 0;

        if (direction == 0) lr = -1;
        if (direction == 1) lr = 1;
        if (direction == 2) ud = -1;
        if (direction == 3) ud = 1;

        left.setPosition((float)(lr * alpha * left.getTexture()->getSize().x), (float)(ud * alpha * left.getTexture()->getSize().y));

        surface.clear(this->getNextActivityBGColor());

        this->drawNextActivity(surface);

        surface.display(); // flip and ready the buffer


#ifdef __ANDROID__
        if(!loaded) {
                temp2 = sf::Texture(surface.getTexture());
                temp2.flip(true);
                loaded = true; // both textures are loaded by now
        }
#else
       temp2 = sf::Texture(surface.getTexture());
#endif

        sf::Sprite right(temp2);

        right.setPosition((float)(-lr * (1.0-alpha) * right.getTexture()->getSize().x), (float)(-ud * (1.0-alpha) * right.getTexture()->getSize().y));

        surface.draw(left);
        surface.draw(right);
    }

    PushIn(sf::Time duration, Activity* last, Activity* next) : Segue(duration, last, next) {
        /* ... */
        loaded = false;
    }

    virtual ~PushIn() { }
};
