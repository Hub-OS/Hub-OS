#pragma once
#include "bnAnimation.h"
#include "bnSpriteProxyNode.h"
#include <string>

class Player;
class CardAction;

class PlayerForm {
public:
  PlayerForm() = default;
  virtual ~PlayerForm() { ; }
  virtual void OnActivate(Player&) = 0;
  virtual void OnDeactivate(Player&) = 0;
  virtual void OnUpdate(float elapsed, Player&) = 0;
  virtual CardAction* OnChargedBusterAction(Player&) = 0;
  virtual CardAction* OnSpecialAction(Player&) = 0;
};

class PlayerFormMeta {
private:
  size_t index{};
  PlayerForm* form{ nullptr };
  std::function<void()> loadFormClass; /*!< Deffered form loading. Only load form class when needed */
  std::string path;

public:
  PlayerFormMeta(size_t index);

  template<class T> PlayerFormMeta& SetFormClass();

  void SetUIPath(std::string path);

  const std::string GetUIPath() const;

  void ActivateForm(Player& player);

  PlayerForm* BuildForm();

  const size_t GetFormIndex() const;
};

/**
 * @brief Sets the deferred type loader T
 *
 * Automatically sets battle texture and overworld texture from the net navi class
 * Automatically sets health from net navi class
 *
 * @return NaviMeta& object for chaining
 */
template<class T>
inline PlayerFormMeta& PlayerFormMeta::SetFormClass()
{
  loadFormClass = [this]() {
    form = new T();
  };

  return *this;
}
