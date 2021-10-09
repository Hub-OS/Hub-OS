#ifdef BN_MOD_SUPPORT
#include "bnScriptedCardAction.h"
#include "../bnCharacter.h"
#include "../bnSolHelpers.h"

ScriptedCardAction::ScriptedCardAction(std::shared_ptr<Character> actor, const std::string& state) :
  CardAction(actor, state)
{

}

ScriptedCardAction::~ScriptedCardAction() {

}

void ScriptedCardAction::Init() {
  weakWrap = WeakWrapper(weak_from_base<ScriptedCardAction>());
}

void ScriptedCardAction::Update(double elapsed) {
  CardAction::Update(elapsed);

  if (entries["update_func"].valid()) 
  {
    auto result = CallLuaFunction(entries, "update_func", weakWrap, elapsed);

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
    }
  }
}

void ScriptedCardAction::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
  // TODO
}

void ScriptedCardAction::OnAnimationEnd() {
  if (entries["animation_end_func"].valid()) 
  {
    auto result = CallLuaFunction(entries, "animation_end_func", weakWrap);

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
    }
  }
}

void ScriptedCardAction::OnActionEnd() {
  if (entries["action_end_func"].valid()) 
  {
    auto result = CallLuaFunction(entries, "action_end_func", weakWrap);

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
    }
  }
}

void ScriptedCardAction::OnExecute(std::shared_ptr<Character> user) {
  if (entries["execute_func"].valid()) 
  {
    auto result = CallLuaFunction(entries, "execute_func", weakWrap, WeakWrapper(user));

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
    }
  }
}

#endif