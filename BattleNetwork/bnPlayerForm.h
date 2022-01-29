#pragma once
#include "bnDrawWindow.h"
#include "bnAnimation.h"
#include "bnSpriteProxyNode.h"
#include <string>

class Player;
class CardAction;

class PlayerForm {
  bool elementalDecross{ true };
public:
  PlayerForm() = default;
  virtual ~PlayerForm() { ; }
  virtual void OnActivate(std::shared_ptr<Player>) = 0;
  virtual void OnDeactivate(std::shared_ptr<Player>) = 0;
  virtual void OnUpdate(double elapsed, std::shared_ptr<Player>) = 0;
  virtual std::shared_ptr<CardAction> OnChargedBusterAction(std::shared_ptr<Player>) = 0;
  virtual std::shared_ptr<CardAction> OnSpecialAction(std::shared_ptr<Player>) = 0;
  virtual std::optional<frame_time_t> CalculateChargeTime(const unsigned) = 0;
  void SetElementalDecross(bool state) { elementalDecross = state; }
  const bool WillElementalHitDecross() const { return elementalDecross; }
};

class PlayerFormMeta {
private:
  size_t index{};
  PlayerForm* form{ nullptr };
  std::function<void()> loadFormClass; /*!< Deffered form loading. Only load form class when needed */
  std::string path;

public:
  PlayerFormMeta(size_t index);
  virtual ~PlayerFormMeta() { }

  template<class T> PlayerFormMeta& SetFormClass();

  void SetUIPath(std::string path);

  const std::string GetUIPath() const;

  void ActivateForm(std::shared_ptr<Player> player);

  virtual PlayerForm* BuildForm();

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
