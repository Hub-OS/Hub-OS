#pragma once
#include "bnPlayer.h"
#include "bnCardAction.h"

class SpriteProxyNode;

class Megaman : public Player {
public:
  Megaman();
  ~Megaman();

  virtual void OnUpdate(float elapsed);
  CardAction* OnExecuteBusterAction() override final;
  CardAction* OnExecuteChargedBusterAction() override final;
  CardAction* OnExecuteSpecialAction() override final;
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
    sf::Sprite overlay;
    SpriteProxyNode* attachment;
    Animation attachmentAnim;

  public:
    SpecialAction(Character* owner);
    ~SpecialAction();

    void OnUpdate(float _elapsed);

    // Inherited via CardAction
    void Execute() override;
    void EndAction() override;
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

class ElecCross : public PlayerForm {
public:
  ElecCross();
  ~ElecCross();
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