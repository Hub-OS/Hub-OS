#pragma once
#pragma once
#include "bnAnimationComponent.h"
#include "bnAnimation.h"
#include "bnCharacter.h"

#include <string>
using std::string;

class AnimatedCharacter : public Character {
  friend class Field;

protected:
  AnimationComponent animationComponent;
  int hitHeight;

public:

  AnimatedCharacter(Rank _rank = Rank::_1);
  virtual ~AnimatedCharacter();

  virtual const bool Hit(int damage, HitProperties props = Entity::DefaultHitProperties);
  virtual const float GetHitHeight() const;
  virtual void Update(float _elapsed);
  virtual vector<Drawable*> GetMiscComponents();
  virtual void AddAnimation(string _state, FrameList _frameList, float _duration);
  virtual void SetAnimation(string _state, std::function<void()> onFinish = std::function<void()>());
  virtual void SetCounterFrame(int frame);
  virtual void OnFrameCallback(int frame, std::function<void()> onEnter, std::function<void()> onLeave = nullptr, bool doOnce = false);
};