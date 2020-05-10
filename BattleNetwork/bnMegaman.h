#pragma once
#include "bnPlayer.h"
#include "bnCardAction.h"

class SpriteProxyNode;

class Megaman : public Player {
public:
  Megaman();
  ~Megaman();

  virtual void OnUpdate(float elapsed);
  CardAction* ExecuteBusterAction() final;
  CardAction* ExecuteChargedBusterAction() final;
  CardAction* ExecuteSpecialAction() final;
};

class TenguCross : public PlayerForm {
public:
  TenguCross();
  ~TenguCross();
  void OnUpdate(float elapsed, Player&);
  void OnActivate(Player& player);
  void OnDeactivate(Player& player);
  CardAction* OnChargedBusterAction(Player&);
  CardAction* OnSpecialAction(Player&);

private:
  bool loaded;
  AnimationComponent* parentAnim;
  Animation overlayAnimation;
  SpriteProxyNode* overlay;

  class SpecialAction : public CardAction {
    SpriteProxyNode* attachment;
    Animation attachmentAnim;

  public:
    SpecialAction(Character& owner);
    ~SpecialAction();

    // Inherited via CardAction
    void OnExecute() override;
    void OnEndAction() override;
  };

};

class HeatCross : public PlayerForm {
public:
  HeatCross();
  ~HeatCross();
  void OnUpdate(float elapsed, Player&);
  void OnActivate(Player& player);
  void OnDeactivate(Player& player);
  CardAction* OnChargedBusterAction(Player&);
  CardAction* OnSpecialAction(Player&);
private:
  bool loaded;
  AnimationComponent* parentAnim;
  Animation overlayAnimation;
  SpriteProxyNode* overlay;
};

class TomahawkCross : public PlayerForm {
public:
  TomahawkCross();
  ~TomahawkCross();
  void OnUpdate(float elapsed, Player&);
  void OnActivate(Player& player);
  void OnDeactivate(Player& player);
  CardAction* OnChargedBusterAction(Player&);
  CardAction* OnSpecialAction(Player&);
private:
  bool loaded;
  AnimationComponent* parentAnim;
  Animation overlayAnimation;
  SpriteProxyNode* overlay;
};