#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class TornadoCardAction : public CardAction {
private:
  SpriteProxyNode* attachment;
  Animation attachmentAnim;
  bool armIsOut;
  int damage;
public:
  TornadoCardAction(Character& owner, int damage);
  ~TornadoCardAction();
  void OnUpdate(float _elapsed) override;
  void OnEndAction() override;
  void OnExecute() override;
}; 
