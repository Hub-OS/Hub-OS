#include "bnMegaman.h"
#include "bnShaderResourceManager.h"
#include "bnBusterCardAction.h"
#include "bnCrackShotCardAction.h"
#include "bnFireBurnCardAction.h"
#include "bnTornadoCardAction.h"
#include "bnLightningCardAction.h"
#include "bnTomahawkSwingCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#include "bnWind.h"
#include "bnPaletteSwap.h"

Megaman::Megaman() : Player() {
  auto basePallete = TEXTURES.LoadTextureFromFile("resources/navis/megaman/forms/base.palette.png");
  PaletteSwap* pswap = CreateComponent<PaletteSwap>(this, basePallete);

  SetHealth(300);
  SetName("Megaman");
  setTexture(TEXTURES.GetTexture(TextureType::NAVI_MEGAMAN_ATLAS));

  AddForm<TenguCross>()->SetUIPath("resources/navis/megaman/forms/tengu_entry.png");
  AddForm<HeatCross>()->SetUIPath("resources/navis/megaman/forms/heat_entry.png");
  AddForm<ElecCross>()->SetUIPath("resources/navis/megaman/forms/elec_entry.png");
  AddForm<TomahawkCross>()->SetUIPath("resources/navis/megaman/forms/hawk_entry.png");
}

Megaman::~Megaman()
{
}

void Megaman::OnUpdate(float elapsed)
{
  Player::OnUpdate(elapsed);
}

CardAction* Megaman::OnExecuteBusterAction()
{
  return new BusterCardAction(this, false, 1);
}

CardAction* Megaman::OnExecuteChargedBusterAction()
{
  if (activeForm) {
    return activeForm->OnChargedBusterAction(*this);
  }
  else {
    return new BusterCardAction(this, true, 10);
  }
}

CardAction* Megaman::OnExecuteSpecialAction() {
  if (activeForm) {
    return activeForm->OnSpecialAction(*this);
  }

  return nullptr;
}

// CROSSES / FORMS

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
  overlayAnimation = Animation("resources/navis/megaman/forms/tengu_cross.animation");
  overlayAnimation.Load();
  auto cross = TextureResourceManager::GetInstance().LoadTextureFromFile("resources/navis/megaman/forms/tengu_cross.png");
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

  parentAnim->AddToOverrideList(&overlayAnimation);
}

void TenguCross::OnDeactivate(Player & player)
{
  player.RemoveNode(overlay);
  auto pswap = player.GetFirstComponent<PaletteSwap>();
  pswap->Revert();

  parentAnim->RemoveFromOverrideList(&overlayAnimation);

}

void TenguCross::OnUpdate(float elapsed, Player& player)
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
  return new BusterCardAction(&player, true, 10);
}

CardAction* TenguCross::OnSpecialAction(Player& player)
{
  return new TenguCross::SpecialAction(&player);
}

// HEAT CROSS

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
  overlayAnimation = Animation("resources/navis/megaman/forms/heat_cross.animation");
  overlayAnimation.Load();
  auto cross = TextureResourceManager::GetInstance().LoadTextureFromFile("resources/navis/megaman/forms/heat_cross.png");
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

  parentAnim->AddToOverrideList(&overlayAnimation);

}

void HeatCross::OnDeactivate(Player & player)
{
  player.RemoveNode(overlay);
  auto pswap = player.GetFirstComponent<PaletteSwap>();
  pswap->Revert();

  player.SetElement(Element::none);

  parentAnim->RemoveFromOverrideList(&overlayAnimation);

}

void HeatCross::OnUpdate(float elapsed, Player& player)
{
  overlay->setColor(player.getColor());

  parentAnim->SyncAnimation(overlayAnimation);
  overlayAnimation.Refresh(overlay->getSprite());

  // update node position in the animation
  auto baseOffset = parentAnim->GetPoint("Head");
  auto origin = player.getSprite().getOrigin();
  baseOffset = baseOffset - origin;

  overlay->setPosition(baseOffset);
}

CardAction* HeatCross::OnChargedBusterAction(Player& player)
{
  auto* action = new FireBurnCardAction(&player, FireBurn::Type::_2, 60);
  action->CrackTiles(false);
  return action;
}

CardAction* HeatCross::OnSpecialAction(Player& player)
{
  return nullptr;
}

// TOMAHAWK CROSS

TomahawkCross::TomahawkCross()
{
  loaded = false;
}

TomahawkCross::~TomahawkCross()
{
  delete overlay;
}

void TomahawkCross::OnActivate(Player& player)
{
  overlayAnimation = Animation("resources/navis/megaman/forms/hawk_cross.animation");
  overlayAnimation.Load();
  auto cross = TextureResourceManager::GetInstance().LoadTextureFromFile("resources/navis/megaman/forms/hawk_cross.png");
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

  parentAnim->AddToOverrideList(&overlayAnimation);
}

void TomahawkCross::OnDeactivate(Player & player)
{
  player.RemoveNode(overlay);
  auto pswap = player.GetFirstComponent<PaletteSwap>();
  pswap->Revert();

  parentAnim->RemoveFromOverrideList(&overlayAnimation);
  player.SetAirShoe(false);
}

void TomahawkCross::OnUpdate(float elapsed, Player& player)
{
  overlay->setColor(player.getColor());

  parentAnim->SyncAnimation(overlayAnimation);
  overlayAnimation.Refresh(overlay->getSprite());

  // update node position in the animation
  auto baseOffset = parentAnim->GetPoint("Head");
  auto origin = player.getSprite().getOrigin();
  baseOffset = baseOffset - origin;

  overlay->setPosition(baseOffset);
}

CardAction* TomahawkCross::OnChargedBusterAction(Player& player)
{
  return new TomahawkSwingCardAction(player);
}

CardAction* TomahawkCross::OnSpecialAction(Player& player)
{
  return nullptr;
}

// SPECIAL ABILITY IMPLEMENTATIONS
#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.3 }

#define FRAMES FRAME1, FRAME2, FRAME3

TenguCross::SpecialAction::SpecialAction(Character* owner) : 
  CardAction(*owner, "PLAYER_SWORD") {
  overlay.setTexture(*owner->getTexture());
  attachment = new SpriteProxyNode(overlay);
  attachment->SetLayer(-1);
  attachment->EnableParentShader(true);

  OverrideAnimationFrames({ FRAMES });

  attachmentAnim = Animation(owner->GetFirstComponent<AnimationComponent>()->GetFilePath());
  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("HAND");

  AddAttachment(*owner, "hilt", *attachment).UseAnimation(attachmentAnim);
}

TenguCross::SpecialAction::~SpecialAction()
{
}

void TenguCross::SpecialAction::OnUpdate(float _elapsed)
{
  CardAction::OnUpdate(_elapsed);
}

void TenguCross::SpecialAction::Execute()
{
  auto owner = GetOwner();
  auto team = owner->GetTeam();
  auto field = owner->GetField();

  // On throw frame, spawn projectile
  auto onThrow = [this, owner, team, field]() -> void {
    auto wind = new Wind(field, team);
    field->AddEntity(*wind, 6, 1);

    wind = new Wind(field, team);
    field->AddEntity(*wind, 6, 2);

    wind = new Wind(field, team);
    field->AddEntity(*wind, 6, 3);
  };

  AddAnimAction(3, onThrow);
}

void TenguCross::SpecialAction::EndAction()
{
  Eject();
}

void TenguCross::SpecialAction::OnAnimationEnd()
{
}


// ELEC CROSS

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
  auto cross = TextureResourceManager::GetInstance().LoadTextureFromFile("resources/navis/megaman/forms/elec_cross.png");
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

  parentAnim->AddToOverrideList(&overlayAnimation);

}

void ElecCross::OnDeactivate(Player& player)
{
  player.RemoveNode(overlay);
  auto pswap = player.GetFirstComponent<PaletteSwap>();
  pswap->Revert();

  player.SetElement(Element::none);

  parentAnim->RemoveFromOverrideList(&overlayAnimation);

}

void ElecCross::OnUpdate(float elapsed, Player& player)
{
  overlay->setColor(player.getColor());

  parentAnim->SyncAnimation(overlayAnimation);
  overlayAnimation.Refresh(overlay->getSprite());

  // update node position in the animation
  auto baseOffset = parentAnim->GetPoint("Head");
  auto origin = player.getSprite().getOrigin();
  baseOffset = baseOffset - origin;

  overlay->setPosition(baseOffset);
}

CardAction* ElecCross::OnChargedBusterAction(Player& player)
{
  auto* action = new LightningCardAction(&player, 50);
  action->SetStun(false);
  return action;
}

CardAction* ElecCross::OnSpecialAction(Player& player)
{
  return nullptr;
}