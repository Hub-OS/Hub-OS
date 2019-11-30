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
  AnimationComponent* animationComponent;

public:

  AnimatedCharacter(Rank _rank = Rank::_1);
  virtual ~AnimatedCharacter();

  virtual const float GetHitHeight() const =0;
  virtual void SetAnimation(string _state, std::function<void()> onFinish = std::function<void()>());
  virtual void SetCounterFrame(int frame);
  virtual void OnFrameCallback(int frame, std::function<void()> onEnter, std::function<void()> onLeave = nullptr, bool doOnce = false);
};