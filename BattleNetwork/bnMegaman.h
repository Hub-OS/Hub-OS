#pragma once
#include "bnPlayer.h"
#include "bnChipAction.h"

class SpriteSceneNode;

class Megaman : public Player {
public:
  Megaman();
  ~Megaman();

  virtual void OnUpdate(float elapsed);
  ChipAction* ExecuteBusterAction() final;
  ChipAction* ExecuteChargedBusterAction() final;
  ChipAction* ExecuteSpecialAction() final;
};

class TenguCross : public PlayerForm {
public:
  TenguCross();
  ~TenguCross();
  void OnUpdate(float elapsed, Player&);
  void OnActivate(Player& player);
  void OnDeactivate(Player& player);
  ChipAction* OnChargedBusterAction(Player&);
  ChipAction* OnSpecialAction(Player&);

private:
  bool loaded;
  AnimationComponent* parentAnim;
  Animation overlayAnimation;
  SpriteSceneNode* overlay;

  class SpecialAction : public ChipAction {
    sf::Sprite overlay;
    SpriteSceneNode* attachment;
    Animation attachmentAnim;

  public:
    SpecialAction(Character* owner);
    ~SpecialAction();

    void OnUpdate(float _elapsed);

    // Inherited via ChipAction
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
  ChipAction* OnChargedBusterAction(Player&);
  ChipAction* OnSpecialAction(Player&);
private:
  bool loaded;
  AnimationComponent* parentAnim;
  Animation overlayAnimation;
  SpriteSceneNode* overlay;
};

class TomahawkCross : public PlayerForm {
public:
  TomahawkCross();
  ~TomahawkCross();
  void OnUpdate(float elapsed, Player&);
  void OnActivate(Player& player);
  void OnDeactivate(Player& player);
  ChipAction* OnChargedBusterAction(Player&);
  ChipAction* OnSpecialAction(Player&);
private:
  bool loaded;
  AnimationComponent* parentAnim;
  Animation overlayAnimation;
  SpriteSceneNode* overlay;
};