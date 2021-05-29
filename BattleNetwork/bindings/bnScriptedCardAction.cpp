#include "bnScriptedCardAction.h"

ScriptedCardAction::ScriptedCardAction(Character& actor, const std::string& state) :
  CardAction(actor, state)
{

}

ScriptedCardAction::~ScriptedCardAction() {

}

void ScriptedCardAction::Update(double elapsed) {
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

void ScriptedCardAction::OnExecute(Character* user) {
  if (onExecute) {
    onExecute(*this, user);
  }
}
