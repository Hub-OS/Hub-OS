#ifdef BN_MOD_SUPPORT
#include "bnScriptedCardAction.h"
#include "../bnCharacter.h"
ScriptedCardAction::ScriptedCardAction(std::shared_ptr<Character> actor, const std::string& state) :
  CardAction(actor, state)
{

}

ScriptedCardAction::~ScriptedCardAction() {

}

CardAction::Attachment& ScriptedCardAction::AddAttachment(std::shared_ptr<Character> character, const std::string& point, SpriteProxyNode& node) {
  return CardAction::AddAttachment(character, point, node);
}

void ScriptedCardAction::Update(double elapsed) {
  CardAction::Update(elapsed);
  if (onUpdate) {
    try {
      onUpdate(shared_from_base<ScriptedCardAction>(), elapsed);
    } catch(std::exception& e) {
      Logger::Log(e.what());
    }
  }
}

void ScriptedCardAction::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
  // TODO
}

void ScriptedCardAction::OnAnimationEnd() {
  if (onAnimationEnd) {
    try {
      onAnimationEnd(shared_from_base<ScriptedCardAction>());
    } catch(std::exception& e) {
      Logger::Log(e.what());
    }
  }
}

void ScriptedCardAction::OnActionEnd() {
  if (onActionEnd) {
    try {
      onActionEnd(shared_from_base<ScriptedCardAction>());
    } catch(std::exception& e) {
      Logger::Log(e.what());
    }
  }
}

void ScriptedCardAction::OnExecute(std::shared_ptr<Character> user) {
  if (onExecute) {
    try {
      onExecute(shared_from_base<ScriptedCardAction>(), WeakWrapper(user));
    } catch(std::exception& e) {
      Logger::Log(e.what());
    }
  }
}

#endif