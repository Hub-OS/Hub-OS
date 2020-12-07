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
  ReflectCardAction(Character* owner, int damage, ReflectShield::Type type);
  ~ReflectCardAction();
  void OnUpdate(float _elapsed);
  void OnAnimationEnd() override;
  void OnEndAction() override;
  void OnExecute() override;
  void SetDuration(const frame_time_t& duration);
};
