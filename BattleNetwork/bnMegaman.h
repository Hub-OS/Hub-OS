#pragma once
#include "bnPlayer.h"
#include "bnCardAction.h"

class SpriteProxyNode;

class Megaman final : public Player {
public:
  Megaman();
  ~Megaman();

  virtual void OnUpdate(float elapsed);

  CardAction* OnExecuteBusterAction() override;
  CardAction* OnExecuteChargedBusterAction() override;
  CardAction* OnExecuteSpecialAction() override;
};

class TenguCross final : public PlayerForm {
public:
  TenguCross();
  ~TenguCross();
  void OnUpdate(float elapsed, Player&) override;
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

  class SpecialAction : public CardAction {
    sf::Sprite overlay;
    SpriteProxyNode* attachment{ nullptr };
    Animation attachmentAnim;

  public:
    SpecialAction(Character* owner);
    ~SpecialAction();

    void OnUpdate(float _elapsed);

    // Inherited via CardAction
    void Execute() override;
    void EndAction() override;
    void OnAnimationEnd() override;
  };

};

class HeatCross final : public PlayerForm {
public:
  HeatCross();
  ~HeatCross();
  void OnUpdate(float elapsed, Player&) override;
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
};

class DefenseRule; // forward declare

class TomahawkCross final : public PlayerForm {
public:
  TomahawkCross();
  ~TomahawkCross();
  void OnUpdate(float elapsed, Player&) override;
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
};

class ElecCross final : public PlayerForm {
public:
  ElecCross();
  ~ElecCross();
  void OnUpdate(float elapsed, Player&) override;
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
};