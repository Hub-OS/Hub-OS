#pragma once
#include "bnCardAction.h"

class WindRackCardAction : public CardAction {
  SpriteProxyNode* attachment{ nullptr }, *hilt{ nullptr };
  Animation attachmentAnim, hiltAnim;
  int damage{};
  std::string newDefault, newHilt; // hacky test
public:
  WindRackCardAction(Character& actor, int damage);
  ~WindRackCardAction();

  void ReplaceRack(SpriteProxyNode* node, const Animation& newAnim);

  // Inherited via CardAction
  void OnExecute(Character* user) override;
  void OnActionEnd() override;
  void OnAnimationEnd() override;
};