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
    onUpdate(*this, elapsed);
  }
}

void ScriptedCardAction::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
  // TODO
}

void ScriptedCardAction::OnAnimationEnd() {
  if (onAnimationEnd) {
    onAnimationEnd(*this);
  }
}

void ScriptedCardAction::OnActionEnd() {
  if (onActionEnd) {
    onActionEnd(*this);
  }
}

void ScriptedCardAction::OnExecute(std::shared_ptr<Character> user) {
  if (onExecute) {
    onExecute(*this, user);
  }
}

#endif