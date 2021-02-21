#include "bnMegaman.h"
#include "bnField.h"
#include "bnShaderResourceManager.h"
#include "bnBusterCardAction.h"
#include "bnCrackShotCardAction.h"
#include "bnFireBurnCardAction.h"
#include "bnTornadoCardAction.h"
#include "bnLightningCardAction.h"
#include "bnTomahawkSwingCardAction.h"
#include "bnDefenseStatusGuard.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#include "bnWind.h"
#include "bnPaletteSwap.h"

Megaman::Megaman() : Player() {
  auto basePallete = Textures().LoadTextureFromFile("resources/navis/megaman/forms/base.palette.png");
  PaletteSwap* pswap = CreateComponent<PaletteSwap>(this, basePallete);

  SetHealth(900);
  SetName("Megaman");
  setTexture(Textures().GetTexture(TextureType::NAVI_MEGAMAN_ATLAS));

  AddForm<TenguCross>()->SetUIPath("resources/navis/megaman/forms/tengu_entry.png");
  AddForm<HeatCross>()->SetUIPath("resources/navis/megaman/forms/heat_entry.png");
  AddForm<ElecCross>()->SetUIPath("resources/navis/megaman/forms/elec_entry.png");
  AddForm<TomahawkCross>()->SetUIPath("resources/navis/megaman/forms/hawk_entry.png");
}

Megaman::~Megaman()
{
}

void Megaman::OnUpdate(double elapsed)
{
  Player::OnUpdate(elapsed);
}

CardAction* Megaman::OnExecuteBusterAction()
{
  return new BusterCardAction(*this, false, 1*GetAttackLevel());
}

CardAction* Megaman::OnExecuteChargedBusterAction()
{
  if (activeForm) {
    return activeForm->OnChargedBusterAction(*this);
  }
  else {
    return new BusterCardAction(*this, true, 10*GetAttackLevel());
  }
}

CardAction* Megaman::OnExecuteSpecialAction() {
  if (activeForm) {
    return activeForm->OnSpecialAction(*this);
  }

  return Player::OnExecuteSpecialAction();
}

///////////////////////////////////////////////
//            CROSSES / FORMS                //
///////////////////////////////////////////////

// class TenguCross

TenguCross::TenguCross()
{
  loaded = false;
}

TenguCross::~TenguCross()
{
  delete overlay;
}

void TenguCross::OnActivate(Player& player)
{
  ResourceHandle handle;

  overlayAnimation = Animation("resources/navis/megaman/forms/tengu_cross.animation");
  overlayAnimation.Load();
  auto cross = handle.Textures().LoadTextureFromFile("resources/navis/megaman/forms/tengu_cross.png");
  overlay = new SpriteProxyNode();
  overlay->setTexture(cross);
  overlay->SetLayer(-1);
  overlay->EnableParentShader(false);

  parentAnim = player.GetFirstComponent<AnimationComponent>();
  auto pswap = player.GetFirstComponent<PaletteSwap>();

  pswap->LoadPaletteTexture("resources/navis/megaman/forms/tengu.palette.png");

  loaded = true;

  OnUpdate(0, player);

  player.AddNode(overlay);
  player.SetAirShoe(true);

  parentAnim->AddToOverrideList(&overlayAnimation, overlay->getSprite());
}

void TenguCross::OnDeactivate(Player & player)
{
  player.RemoveNode(overlay);
  auto pswap = player.GetFirstComponent<PaletteSwap>();
  pswap->Revert();

  parentAnim->RemoveFromOverrideList(&overlayAnimation);
}

void TenguCross::OnUpdate(double elapsed, Player& player)
{
  parentAnim->SyncAnimation(overlayAnimation);
  overlayAnimation.Refresh(overlay->getSprite());

  overlay->setColor(player.getColor());

  // update node position in the animation
  auto baseOffset = parentAnim->GetPoint("Head");
  auto origin = player.getOrigin();
  baseOffset = baseOffset - origin;

  overlay->setPosition(baseOffset);
}

CardAction* TenguCross::OnChargedBusterAction(Player& player)
{
  return new BusterCardAction(player, true, 20*player.GetAttackLevel()+40);
}

CardAction* TenguCross::OnSpecialAction(Player& player)
{
  // class TenguCross::SpecialAction is a CardAction implementation
  return new TenguCross::SpecialAction(player);
}

frame_time_t TenguCross::CalculateChargeTime(unsigned chargeLevel)
{
  /**
  * These values include the 10i+ initial frames
  * 1 - 100i
  * 2 - 90i
  * 3 - 80i
  * 4 - 75i
  * 5 - 70i
  */
  switch (chargeLevel) {
  case 1:
    return frames(90);
  case 2:
    return frames(80);
  case 3:
    return frames(70);
  case 4:
    return frames(65);
  }

  return frames(60);
}

// class HeatCross

HeatCross::HeatCross()
{
  loaded = false;
}

HeatCross::~HeatCross()
{
  delete overlay;
}

void HeatCross::OnActivate(Player& player)
{
  ResourceHandle handle;

  overlayAnimation = Animation("resources/navis/megaman/forms/heat_cross.animation");
  overlayAnimation.Load();
  auto cross = handle.Textures().LoadTextureFromFile("resources/navis/megaman/forms/heat_cross.png");
  overlay = new SpriteProxyNode();
  overlay->setTexture(cross);
  overlay->SetLayer(-1);
  overlay->EnableParentShader(false);

  parentAnim = player.GetFirstComponent<AnimationComponent>();
  auto pswap = player.GetFirstComponent<PaletteSwap>();

  pswap->LoadPaletteTexture("resources/navis/megaman/forms/heat.palette.png");

  player.SetElement(Element::fire);

  loaded = true;

  OnUpdate(0, player);
  player.AddNode(overlay);

  parentAnim->AddToOverrideList(&overlayAnimation, overlay->getSprite());

}

void HeatCross::OnDeactivate(Player & player)
{
  player.RemoveNode(overlay);
  auto pswap = player.GetFirstComponent<PaletteSwap>();
  pswap->Revert();

  player.SetElement(Element::none);

  parentAnim->RemoveFromOverrideList(&overlayAnimation);

}

void HeatCross::OnUpdate(double elapsed, Player& player)
{
  overlay->setColor(player.getColor());

  parentAnim->SyncAnimation(overlayAnimation);
  overlayAnimation.Refresh(overlay->getSprite());

  // update node position in the animation
  auto baseOffset = parentAnim->GetPoint("Head");
  auto& origin = player.getSprite().getOrigin();
  baseOffset = baseOffset - origin;

  overlay->setPosition(baseOffset);
}

CardAction* HeatCross::OnChargedBusterAction(Player& player)
{
  auto* action = new FireBurnCardAction(player, FireBurn::Type::_2, 20 * player.GetAttackLevel() + 30);
  action->CrackTiles(false);
  return action;
}

CardAction* HeatCross::OnSpecialAction(Player& player)
{
  return player.Player::OnExecuteSpecialAction();
}

frame_time_t HeatCross::CalculateChargeTime(unsigned chargeLevel)
{
  /**
  * These values include the 10i+ initial frames
  * charge 1 - 70i
  * 2 - 60i
  * 3 - 50i
  * 4 - 45i
  * 5 - 40i
  */
  switch (chargeLevel) {
  case 1:
    return frames(60);
  case 2:
    return frames(50);
  case 3:
    return frames(40);
  case 4:
    return frames(35);
  }

  return frames(30);
}

// class TomahawkCross

TomahawkCross::TomahawkCross()
{
  loaded = false;
  statusGuard = new DefenseStatusGuard();
}

TomahawkCross::~TomahawkCross()
{
  delete overlay;
  delete statusGuard;
}

void TomahawkCross::OnActivate(Player& player)
{
  ResourceHandle handle;

  overlayAnimation = Animation("resources/navis/megaman/forms/hawk_cross.animation");
  overlayAnimation.Load();
  auto cross = handle.Textures().LoadTextureFromFile("resources/navis/megaman/forms/hawk_cross.png");
  overlay = new SpriteProxyNode();
  overlay->setTexture(cross);
  overlay->SetLayer(-1);
  overlay->EnableParentShader(false);

  parentAnim = player.GetFirstComponent<AnimationComponent>();
  auto pswap = player.GetFirstComponent<PaletteSwap>();

  pswap->LoadPaletteTexture("resources/navis/megaman/forms/hawk.palette.png");

  loaded = true;

  OnUpdate(0, player);
  player.AddNode(overlay);

  parentAnim->AddToOverrideList(&overlayAnimation, overlay->getSprite());

  player.AddDefenseRule(statusGuard);
}

void TomahawkCross::OnDeactivate(Player & player)
{
  player.RemoveNode(overlay);
  auto pswap = player.GetFirstComponent<PaletteSwap>();
  pswap->Revert();

  parentAnim->RemoveFromOverrideList(&overlayAnimation);
  player.SetAirShoe(false);

  player.RemoveDefenseRule(statusGuard);
}

void TomahawkCross::OnUpdate(double elapsed, Player& player)
{
  overlay->setColor(player.getColor());

  parentAnim->SyncAnimation(overlayAnimation);
  overlayAnimation.Refresh(overlay->getSprite());

  // update node position in the animation
  auto baseOffset = parentAnim->GetPoint("Head");
  auto& origin = player.getSprite().getOrigin();
  baseOffset = baseOffset - origin;

  overlay->setPosition(baseOffset);
}

CardAction* TomahawkCross::OnChargedBusterAction(Player& player)
{
  return new TomahawkSwingCardAction(player, 20 * player.GetAttackLevel() + 40);
}

CardAction* TomahawkCross::OnSpecialAction(Player& player)
{
  return player.Player::OnExecuteSpecialAction();
}

frame_time_t TomahawkCross::CalculateChargeTime(unsigned chargeLevel)
{
  /**
  * These values include the 10i+ initial frames
  * 1 - 120i
  * 2 - 110i
  * 3 - 100i
  * 4 - 95i
  * 5 - 90i
  */

  switch (chargeLevel) {
  case 1:
    return frames(110);
  case 2:
    return frames(100);
  case 3:
    return frames(90);
  case 4:
    return frames(85);
  }

  return frames(80);
}

// class ElecCross

ElecCross::ElecCross()
{
  loaded = false;
}

ElecCross::~ElecCross()
{
  delete overlay;
}

void ElecCross::OnActivate(Player& player)
{
  overlayAnimation = Animation("resources/navis/megaman/forms/elec_cross.animation");
  overlayAnimation.Load();
  auto cross = player.Textures().LoadTextureFromFile("resources/navis/megaman/forms/elec_cross.png");
  overlay = new SpriteProxyNode();
  overlay->setTexture(cross);
  overlay->SetLayer(-1);
  overlay->EnableParentShader(false);

  parentAnim = player.GetFirstComponent<AnimationComponent>();
  auto pswap = player.GetFirstComponent<PaletteSwap>();

  pswap->LoadPaletteTexture("resources/navis/megaman/forms/elec.palette.png");

  player.SetElement(Element::elec);

  loaded = true;

  OnUpdate(0, player);
  player.AddNode(overlay);

  parentAnim->AddToOverrideList(&overlayAnimation, overlay->getSprite());

}

void ElecCross::OnDeactivate(Player& player)
{
  player.RemoveNode(overlay);
  auto pswap = player.GetFirstComponent<PaletteSwap>();
  pswap->Revert();

  player.SetElement(Element::none);

  parentAnim->RemoveFromOverrideList(&overlayAnimation);

}

void ElecCross::OnUpdate(double elapsed, Player& player)
{
  overlay->setColor(player.getColor());

  parentAnim->SyncAnimation(overlayAnimation);
  overlayAnimation.Refresh(overlay->getSprite());

  // update node position in the animation
  auto baseOffset = parentAnim->GetPoint("Head");
  auto& origin = player.getSprite().getOrigin();
  baseOffset = baseOffset - origin;

  overlay->setPosition(baseOffset);
}

CardAction* ElecCross::OnChargedBusterAction(Player& player)
{
  auto* action = new LightningCardAction(player, 20 * player.GetAttackLevel() + 40);
  action->SetStun(false);
  return action;
}

CardAction* ElecCross::OnSpecialAction(Player& player)
{
  return player.Player::OnExecuteSpecialAction();
}

frame_time_t ElecCross::CalculateChargeTime(unsigned chargeLevel)
{
  /**
  * These numbers include the 10i+ startup frames
  * 1 - 90i
  * 2 - 80i
  * 3 - 70i
  * 4 - 65i
  * 5 - 60i
  */

  switch (chargeLevel) {
  case 1:
    return frames(80);
  case 2:
    return frames(70);
  case 3:
    return frames(60);
  case 4:
    return frames(55);
  }

  return frames(50);
}

////////////////////////////////////////////////////////////////
//             SPECIAL ABILITY IMPLEMENTATIONS                //
////////////////////////////////////////////////////////////////

#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.3 }

#define FRAMES FRAME1, FRAME2, FRAME3

// class TenguCross
TenguCross::SpecialAction::SpecialAction(Character& owner) : 
  CardAction(owner, "PLAYER_SWORD") {
  overlay.setTexture(*owner.getTexture());
  attachment = new SpriteProxyNode(overlay);

  attachment->SetLayer(-1);
  attachment->EnableParentShader(true);

  OverrideAnimationFrames({ FRAMES });

  attachmentAnim = Animation(owner.GetFirstComponent<AnimationComponent>()->GetFilePath());
  attachmentAnim.SetAnimation("HAND");

  AddAttachment(owner, "hilt", *attachment).UseAnimation(attachmentAnim);
}

TenguCross::SpecialAction::~SpecialAction()
{
}

void TenguCross::SpecialAction::OnExecute()
{
  auto owner = GetOwner();
  auto team = owner->GetTeam();
  auto field = owner->GetField();

  // On throw frame, spawn projectile
  auto onThrow = [this, team, field]() -> void {
    auto wind = new Wind(team);
    field->AddEntity(*wind, 6, 1);

    wind = new Wind(team);
    field->AddEntity(*wind, 6, 2);

    wind = new Wind(team);
    field->AddEntity(*wind, 6, 3);
  };

  AddAnimAction(3, onThrow);
}

void TenguCross::SpecialAction::OnEndAction()
{
  Eject();
}

void TenguCross::SpecialAction::OnAnimationEnd()
{
}