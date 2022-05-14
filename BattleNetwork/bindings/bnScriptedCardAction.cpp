#ifdef ONB_MOD_SUPPORT
#include "bnScriptedCardAction.h"
#include "../bnCharacter.h"
#include "../bnSolHelpers.h"
#include "../bnTextureResourceManager.h"

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

  if (on_update_func.valid())
  {
    auto result = CallLuaCallback(on_update_func, weakWrap, elapsed);

    if (result.is_error()) {
      Logger::Log(LogLevel::critical, result.error_cstr());
    }
  }
}

void ScriptedCardAction::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
  // TODO
}

std::optional<bool> ScriptedCardAction::CanMoveTo(Battle::Tile* next)
{
  if (can_move_to_func.valid())
  {
    auto result = CallLuaCallbackExpectingValue<bool>(can_move_to_func, next);

    if (result.is_error()) {
      Logger::Log(LogLevel::critical, result.error_cstr());
    }

    return result.value();
  }

  return {};
}

void ScriptedCardAction::SetBackgroundData(const std::filesystem::path& bgTexturePath, const std::filesystem::path& animPath, float velx, float vely)
{
  auto bg = std::make_shared<CustomBackground>(Textures().LoadFromFile(bgTexturePath), Animation(animPath), sf::Vector2f(velx, vely));
  this->SetCustomBackground(bg);
}

void ScriptedCardAction::SetBackgroundColor(const sf::Color& color)
{
  auto bg = std::make_shared<CustomBackground>(nullptr, Animation(), sf::Vector2f());
  bg->SetColor(color);
  this->SetCustomBackground(bg);
}

void ScriptedCardAction::OnAnimationEnd() {
  if (on_animation_end_func.valid()) 
  {
    auto result = CallLuaCallback(on_animation_end_func, weakWrap);

    if (result.is_error()) {
      Logger::Log(LogLevel::critical, result.error_cstr());
    }
  }
}

void ScriptedCardAction::OnActionEnd() {
  if (on_action_end_func.valid()) 
  {
    auto result = CallLuaCallback(on_action_end_func, weakWrap);

    if (result.is_error()) {
      Logger::Log(LogLevel::critical, result.error_cstr());
    }
  }
}

void ScriptedCardAction::OnExecute(std::shared_ptr<Character> user) {
  if (on_execute_func.valid()) 
  {
    auto result = CallLuaCallback(on_execute_func, weakWrap, WeakWrapper(user));

    if (result.is_error()) {
      Logger::Log(LogLevel::critical, result.error_cstr());
    }
  }
}

#endif