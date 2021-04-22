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
  HubBatchCardAction(Character& actor);
  ~HubBatchCardAction();
  void Update(double _elapsed) override;
  void OnAnimationEnd() override;
  void OnEndAction() override;
  void OnExecute(Character* user) override;
};

class HubBatchProgram : public Component {
  Animation anim;
  SpriteProxyNode effect;
  DefenseRule* superarmor{ nullptr };
public:
  HubBatchProgram(Character* owner);
  ~HubBatchProgram();
  void OnUpdate(double elapsed) override;
  void Inject(BattleSceneBase&) override;
};