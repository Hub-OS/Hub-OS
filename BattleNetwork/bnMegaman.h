#pragma once
#include "bnPlayer.h"
#include "bnCardAction.h"

class SpriteProxyNode;

class Megaman final : public Player {
  std::shared_ptr<sf::Texture> basePalette;
public:
  Megaman();
  ~Megaman();

  virtual void OnUpdate(double elapsed);
  void OnSpawn(Battle::Tile& start) override;

  std::shared_ptr<CardAction> OnExecuteBusterAction() override;
  std::shared_ptr<CardAction> OnExecuteChargedBusterAction() override;
  std::shared_ptr<CardAction> OnExecuteSpecialAction() override;
};

class ForteCross final : public PlayerForm {
public:
  ForteCross();
  ~ForteCross();
  void OnUpdate(double elapsed, std::shared_ptr<Player>) override;
  void OnActivate(std::shared_ptr<Player> player) override;
  void OnDeactivate(std::shared_ptr<Player> player) override;
  std::shared_ptr<CardAction> OnChargedBusterAction(std::shared_ptr<Player>) override;
  std::shared_ptr<CardAction> OnSpecialAction(std::shared_ptr<Player>) override;
  frame_time_t CalculateChargeTime(unsigned chargeLevel) override;
private:
  std::shared_ptr<sf::Texture> prevTexture;
  std::string prevAnimation, prevState;
  /*class SpecialAction : public CardAction {
  public:
    SpecialAction(std::shared_ptr<Character> actor);
    ~SpecialAction();

    // Inherited via CardAction
    void OnExecute(std::shared_ptr<Character> user) override;
    void OnActionEnd() override;
    void OnAnimationEnd() override;
  };*/
};


class TenguCross final : public PlayerForm {
public:
  TenguCross();
  ~TenguCross();
  void OnUpdate(double elapsed, std::shared_ptr<Player>) override;
  void OnActivate(std::shared_ptr<Player> player) override;
  void OnDeactivate(std::shared_ptr<Player> player) override;
  std::shared_ptr<CardAction> OnChargedBusterAction(std::shared_ptr<Player>) override;
  std::shared_ptr<CardAction> OnSpecialAction(std::shared_ptr<Player>) override;
  frame_time_t CalculateChargeTime(unsigned chargeLevel) override;
private:
  std::shared_ptr<AnimationComponent> parentAnim{ nullptr };
  Animation overlayAnimation, fanAnimation;
  SpriteProxyNode* overlay{ nullptr };
  AnimationComponent::SyncItem sync;

  class SpecialAction : public CardAction {
  public:
    SpecialAction(std::shared_ptr<Character> actor);
    ~SpecialAction();

    // Inherited via CardAction
    void OnExecute(std::shared_ptr<Character> user) override;
    void OnActionEnd() override;
    void OnAnimationEnd() override;
  };
};

class HeatCross final : public PlayerForm {
public:
  HeatCross();
  ~HeatCross();
  void OnUpdate(double elapsed, std::shared_ptr<Player>) override;
  void OnActivate(std::shared_ptr<Player> player) override;
  void OnDeactivate(std::shared_ptr<Player> player) override;
  std::shared_ptr<CardAction> OnChargedBusterAction(std::shared_ptr<Player>) override;
  std::shared_ptr<CardAction> OnSpecialAction(std::shared_ptr<Player>) override;
  frame_time_t CalculateChargeTime(unsigned chargeLevel) override;
private:
  std::shared_ptr<AnimationComponent> parentAnim{ nullptr };
  Animation overlayAnimation;
  SpriteProxyNode* overlay{ nullptr };
  AnimationComponent::SyncItem sync;
};

class DefenseRule; // forward declare

class TomahawkCross final : public PlayerForm {
public:
  TomahawkCross();
  ~TomahawkCross();
  void OnUpdate(double elapsed, std::shared_ptr<Player>) override;
  void OnActivate(std::shared_ptr<Player> player) override;
  void OnDeactivate(std::shared_ptr<Player> player) override;
  std::shared_ptr<CardAction> OnChargedBusterAction(std::shared_ptr<Player>) override;
  std::shared_ptr<CardAction> OnSpecialAction(std::shared_ptr<Player>) override;
  frame_time_t CalculateChargeTime(unsigned chargeLevel) override;
private:
  std::shared_ptr<DefenseRule> statusGuard{ nullptr };
  std::shared_ptr<AnimationComponent> parentAnim{ nullptr };
  Animation overlayAnimation;
  SpriteProxyNode* overlay{ nullptr };
  AnimationComponent::SyncItem sync;
};

class ElecCross final : public PlayerForm {
public:
  ElecCross();
  ~ElecCross();
  void OnUpdate(double elapsed, std::shared_ptr<Player>) override;
  void OnActivate(std::shared_ptr<Player> player) override;
  void OnDeactivate(std::shared_ptr<Player> player) override;
  std::shared_ptr<CardAction> OnChargedBusterAction(std::shared_ptr<Player>) override;
  std::shared_ptr<CardAction> OnSpecialAction(std::shared_ptr<Player>) override;
  frame_time_t CalculateChargeTime(unsigned chargeLevel) override;
private:
  std::shared_ptr<AnimationComponent> parentAnim{ nullptr };
  Animation overlayAnimation;
  SpriteProxyNode* overlay{ nullptr };
  AnimationComponent::SyncItem sync;
};