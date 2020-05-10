#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;

class CrackShotCardAction : public CardAction {
private:
  SpriteProxyNode* attachment;
  Animation attachmentAnim;
  int damage;
public:
  CrackShotCardAction(Character& owner, int damage);
  ~CrackShotCardAction();
  void OnEndAction() override;
  void OnExecute() override;
};
