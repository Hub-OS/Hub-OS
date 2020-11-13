#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include "bnSpriteProxyNode.h"

#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class DefenseRule;

class HubBatchCardAction : public CardAction {
public:
  HubBatchCardAction(Character* owner);
  ~HubBatchCardAction();
  void OnUpdate(float _elapsed);
  void OnAnimationEnd() override;
  void EndAction();
  void Execute();
};

class HubBatchProgram : public Component {
  Animation anim;
  SpriteProxyNode effect;
  DefenseRule* superarmor{ nullptr };
public:
  HubBatchProgram(Character* owner);
  ~HubBatchProgram();
  void OnUpdate(float elapsed) override;
  void Inject(BattleSceneBase&) override;
};