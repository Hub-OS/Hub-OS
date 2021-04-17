#pragma once
#include "bnCardAction.h"

class WindRackCardAction : public CardAction {
  SpriteProxyNode* attachment{ nullptr }, *hilt{ nullptr };
  Animation attachmentAnim, hiltAnim;
  int damage{};
public:
  WindRackCardAction(Character& owner, int damage);
  ~WindRackCardAction();

  void ReplaceRack(SpriteProxyNode* node, const Animation& newAnim);

  // Inherited via CardAction
  void OnExecute() override;
  void OnEndAction() override;
  void OnAnimationEnd() override;
};