#include "Timer.h"
#include "Activity.h"
#include <SFML/Graphics.hpp>

namespace swoosh {
  class swoosh::ActivityController;

  class Segue : public swoosh::Activity {
    friend class swoosh::ActivityController;

  private:
    swoosh::Activity* last;
    swoosh::Activity* next;
    sf::Time duration;
    swoosh::Timer timer;
    
    // Hack to make this lib header-only
    void (ActivityController::*setActivityViewFunc)(sf::RenderTexture& surface, swoosh::Activity* activity);
    void (ActivityController::*resetViewFunc)(sf::RenderTexture& surface);

  protected:
    const sf::Time getDuration() { return duration; }
    const sf::Time getElapsed() { return timer.getElapsed(); }

    void drawLastActivity(sf::RenderTexture& surface) {
      if (last) {
        (controller.*setActivityViewFunc)(surface, last);
        last->onDraw(surface);
        (controller.*resetViewFunc)(surface);
      }
    }

    void drawNextActivity(sf::RenderTexture& surface) {
      (controller.*setActivityViewFunc)(surface, next);
      next->onDraw(surface);
      (controller.*resetViewFunc)(surface);
    }

  public:
    virtual void onStart() final { next->onEnter();  last->onLeave(); timer.reset(); }

    virtual void onUpdate(double elapsed) final {
      if (last) last->onUpdate(elapsed);
      next->onUpdate(elapsed);
    }

    virtual void onLeave() final { timer.pause(); }
    virtual void onExit() final { ; }
    virtual void onEnter() final { ; }
    virtual void onResume() final { timer.start(); }
    virtual void onDraw(sf::RenderTexture& surface) = 0;
    virtual void onEnd() final { last->onExit(); }

    Segue() = delete;
    Segue(sf::Time duration, swoosh::Activity* last, swoosh::Activity* next) : duration(duration), last(last), next(next), Activity(next->controller) { /* ... */ }
    virtual ~Segue() { }
  };
}