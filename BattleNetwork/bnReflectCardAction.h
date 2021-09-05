#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include "bnReflectShield.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class ReflectCardAction : public CardAction {
  frame_time_t duration{frames(43)};
  int damage;
  ReflectShield::Type type;
public:
  ReflectCardAction(std::shared_ptr<Character> actor, int damage, ReflectShield::Type type);
  ~ReflectCardAction();
  void Update(double _elapsed) override;
  void OnAnimationEnd() override;
  void OnActionEnd() override;
  void OnExecute(std::shared_ptr<Character> user) override;
  void SetDuration(const frame_time_t& duration);
};
