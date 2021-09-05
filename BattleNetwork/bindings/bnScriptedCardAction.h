#pragma once
#ifdef BN_MOD_SUPPORT

#include <sol/sol.hpp>
#include <functional>
#include <SFML/Graphics.hpp>
#include "dynamic_object.h"
#include "../bnCardAction.h"
#include "../bnAnimation.h"

class SpriteProxyNode;
class Character;
class ScriptedCardAction : public CardAction, public dynamic_object {
public:
  ScriptedCardAction(std::shared_ptr<Character> actor, const std::string& state);
  ~ScriptedCardAction();

  void Update(double elapsed) override;
  void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

  void OnAnimationEnd() override;
  void OnActionEnd() override;
  void OnExecute(std::shared_ptr<Character> user) override;
  CardAction::Attachment& AddAttachment(std::shared_ptr<Character> character, const std::string& point, SpriteProxyNode& node);

  std::function<void(ScriptedCardAction&, double)> onUpdate;
  std::function<void(ScriptedCardAction&)> onAnimationEnd;
  std::function<void(ScriptedCardAction&)> onActionEnd;
  std::function<void(ScriptedCardAction&, std::shared_ptr<Character>)> onExecute;
};

#endif