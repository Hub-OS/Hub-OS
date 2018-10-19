#pragma once
#include "bnActivity.h"
#include <SFML/Graphics.hpp>
#include "bnActivityManager.h"

template<sf::Time(*s)(int val), int val = 0>
struct Duration
{
  static sf::Time value() { return (*s)(val); }
};

class Segue : public Activity {
  friend class ActivityManager;

private:
  Activity* last;
  Activity* next;
  sf::Time duration;
  Timer timer;

public:
  virtual void OnStart() final { next->OnStart(); timer.Reset(); }

  virtual void OnUpdate(double _elapsed) final {
    if (last) last->OnUpdate(_elapsed);
    next->OnUpdate(_elapsed);

    if (timer.GetElapsed() >= duration.asMilliseconds()) {
      ActivityManager::GetInstance().EndSegue(this);
    }
  }

  virtual void OnLeave() final { timer.Pause();  }
  virtual void OnResume() final { timer.Start(); }
  virtual void OnDraw(sf::RenderTexture& surface) = 0;
  virtual void OnEnd() final { if (last) { last->OnEnd(); delete last; } }

  Segue() = delete;
  Segue(sf::Time duration, Activity* last, Activity* next) : duration(duration), last(last), next(next), Activity() { /* ... */ }
  virtual ~Segue() { if (last) { delete last; } next = last = nullptr; }
};