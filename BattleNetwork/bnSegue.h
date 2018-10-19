#pragma once
#include "bnActivity.h"
#include <SFML/Graphics.hpp>

template<sf::Time(*s)(int val), int val = 0>
struct Duration
{
  static sf::Time value() { return (*s)(val); }
};

class Segue : public Activity {
private:
  Activity* last;
  Activity* next;
  sf::Time duration;
  Timer timer;

public:
  virtual void OnStart() final { next->OnStart(); timer.Reset(); }
  virtual void OnUpdate(double _elapsed) = 0;
  virtual void OnLeave() final { timer.Pause();  }
  virtual void OnResume() final { timer.Start(); }
  virtual void OnDraw(sf::RenderTexture& surface) = 0;
  virtual void OnEnd() final { if (last) { last->OnEnd(); } }
  virtual void OnTimeOut() = 0;

  Segue() = delete;
  Segue(sf::Time duration, Activity* last, Activity* next) : duration(duration), last(last), next(next), Activity() { /* ... */ }
  virtual ~Segue() { if (last) { delete last; } next = last = nullptr; }
};