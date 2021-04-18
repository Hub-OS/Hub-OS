#pragma once
#include "bnPlayer.h"
#include "bnCardAction.h"

class SpriteProxyNode;

class Megaman final : public Player {
public:
  Megaman();
  ~Megaman();

  virtual void OnUpdate(double elapsed);

  CardAction* OnExecuteBusterAction() override;
  CardAction* OnExecuteChargedBusterAction() override;
  CardAction* OnExecuteSpecialAction() override;
};

class TenguCross final : public PlayerForm {
public:
  TenguCross();
  ~TenguCross();
  void OnUpdate(double elapsed, Player&) override;
  void OnActivate(Player& player) override;
  void OnDeactivate(Player& player) override;
  CardAction* OnChargedBusterAction(Player&) override;
  CardAction* OnSpecialAction(Player&) override;
  frame_time_t CalculateChargeTime(unsigned chargeLevel) override;
private:
  bool loaded;
  AnimationComponent* parentAnim{ nullptr };
  Animation overlayAnimation, fanAnimation;
  SpriteProxyNode* overlay{ nullptr };
  AnimationComponent::SyncItem sync;

  class SpecialAction : public CardAction {
  public:
    SpecialAction(Character& owner);
    ~SpecialAction();

    // Inherited via CardAction
    void OnExecute() override;
    void OnEndAction() override;
    void OnAnimationEnd() override;
  };
};

class HeatCross final : public PlayerForm {
public:
  HeatCross();
  ~HeatCross();
  void OnUpdate(double elapsed, Player&) override;
  void OnActivate(Player& player) override;
  void OnDeactivate(Player& player) override;
  CardAction* OnChargedBusterAction(Player&) override;
  CardAction* OnSpecialAction(Player&) override;
  frame_time_t CalculateChargeTime(unsigned chargeLevel) override;
private:
  bool loaded;
  AnimationComponent* parentAnim{ nullptr };
  Animation overlayAnimation;
  SpriteProxyNode* overlay{ nullptr };
  AnimationComponent::SyncItem sync;
};

class DefenseRule; // forward declare

class TomahawkCross final : public PlayerForm {
public:
  TomahawkCross();
  ~TomahawkCross();
  void OnUpdate(double elapsed, Player&) override;
  void OnActivate(Player& player) override;
  void OnDeactivate(Player& player) override;
  CardAction* OnChargedBusterAction(Player&) override;
  CardAction* OnSpecialAction(Player&) override;
  frame_time_t CalculateChargeTime(unsigned chargeLevel) override;
private:
  DefenseRule* statusGuard{ nullptr };
  bool loaded;
  AnimationComponent* parentAnim{ nullptr };
  Animation overlayAnimation;
  SpriteProxyNode* overlay{ nullptr };
  AnimationComponent::SyncItem sync;
};

class ElecCross final : public PlayerForm {
public:
  ElecCross();
  ~ElecCross();
  void OnUpdate(double elapsed, Player&) override;
  void OnActivate(Player& player) override;
  void OnDeactivate(Player& player) override;
  CardAction* OnChargedBusterAction(Player&) override;
  CardAction* OnSpecialAction(Player&) override;
  frame_time_t CalculateChargeTime(unsigned chargeLevel) override;
private:
  bool loaded;
  AnimationComponent* parentAnim{ nullptr };
  Animation overlayAnimation;
  SpriteProxyNode* overlay{ nullptr };
  AnimationComponent::SyncItem sync;
};