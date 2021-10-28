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

  if (update_func.valid())
  {
    auto result = CallLuaCallback(update_func, weakWrap, elapsed);

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
  if (animation_end_func.valid()) 
  {
    auto result = CallLuaCallback(animation_end_func, weakWrap);

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
    }
  }
}

void ScriptedCardAction::OnActionEnd() {
  if (action_end_func.valid()) 
  {
    auto result = CallLuaCallback(action_end_func, weakWrap);

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
    }
  }
}

void ScriptedCardAction::OnExecute(std::shared_ptr<Character> user) {
  if (execute_func.valid()) 
  {
    auto result = CallLuaCallback(execute_func, weakWrap, WeakWrapper(user));

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
    }
  }
}

#endif