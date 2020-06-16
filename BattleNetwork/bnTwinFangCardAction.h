#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class TwinFangCardAction : public CardAction {
private:
<<<<<<< HEAD
=======
  sf::Sprite twinfang;
  SpriteProxyNode* attachment;
  Animation attachmentAnim;
>>>>>>> development
  int damage;
public:
  TwinFangCardAction(Character& owner, int damage);
  ~TwinFangCardAction();
<<<<<<< HEAD
  void OnExecute();
  void OnEndAction() override;
=======
  void OnUpdate(float _elapsed) override;
  void EndAction() override;
  void Execute() override;
>>>>>>> development
};
