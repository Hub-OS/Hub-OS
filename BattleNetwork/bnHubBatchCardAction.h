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
  HubBatchCardAction(std::shared_ptr<Character> actor);
  ~HubBatchCardAction();
  void Update(double _elapsed) override;
  void OnAnimationEnd() override;
  void OnActionEnd() override;
  void OnExecute(std::shared_ptr<Character> user) override;
};

class HubBatchProgram : public Component {
  Animation anim;
  SpriteProxyNode effect;
  std::shared_ptr<DefenseRule> superarmor{ nullptr };
public:
  HubBatchProgram(std::weak_ptr<Character> owner);
  ~HubBatchProgram();
  void OnUpdate(double elapsed) override;
  void Inject(BattleSceneBase&) override;
};