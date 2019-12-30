#include "bnMegaman.h"
#include "bnShaderResourceManager.h"
#include "bnBusterChipAction.h"
#include "bnCrackShotChipAction.h"
#include "bnFireBurnChipAction.h"
#include "bnTornadoChipAction.h"
#include "bnChipAction.h"
#include "bnSpriteSceneNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#include "bnWind.h"
#include "bnPaletteSwap.h"

Megaman::Megaman() : Player() {

  auto base_palette = TEXTURES.LoadTextureFromFile("resources/navis/megaman/forms/base.palette.png");
  PaletteSwap* pswap = new PaletteSwap(this, *base_palette);
  RegisterComponent(pswap);
  delete base_palette;

  SetHealth(900);
  SetName("Megaman");
  setTexture(*TEXTURES.GetTexture(TextureType::NAVI_MEGAMAN_ATLAS));

  this->AddForm<TenguCross>()->SetUIPath("resources/navis/megaman/forms/tengu_entry.png");
  this->AddForm<HeatCross>()->SetUIPath("resources/navis/megaman/forms/heat_entry.png");
  this->AddForm<TomahawkCross>()->SetUIPath("resources/navis/megaman/forms/hawk_entry.png");
}

Megaman::~Megaman()
{
}

void Megaman::OnUpdate(float elapsed)
{
  Player::OnUpdate(elapsed);
}

ChipAction* Megaman::ExecuteBusterAction()
{
  return new BusterChipAction(this, false, 1);
}

ChipAction* Megaman::ExecuteChargedBusterAction()
{
  if (activeForm) {
    return activeForm->OnChargedBusterAction(*this);
  }
  else {
    return new BusterChipAction(this, true, 10);
  }
}

ChipAction* Megaman::ExecuteSpecialAction() {
  if (activeForm) {
    return activeForm->OnSpecialAction(*this);
  }

  return nullptr;
}

// CROSSES / FORMS

TenguCross::TenguCross()
{
  parentAnim = nullptr;
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
  overlay = new SpriteSceneNode();
  overlay->setTexture(*cross);
  overlay->SetLayer(-1);
  overlay->EnableParentShader(false);

  parentAnim = player.GetFirstComponent<AnimationComponent>();
  auto pswap = player.GetFirstComponent<PaletteSwap>();

  pswap->LoadPaletteTexture("resources/navis/megaman/forms/tengu.palette.png");

  loaded = true;

  OnUpdate(0.01f, player);

  player.AddNode(overlay);

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
  overlayAnimation.Refresh(*overlay);

  overlay->setColor(player.getColor());

  // update node position in the animation
  auto baseOffset = parentAnim->GetPoint("Head");
  auto origin = player.getOrigin();
  baseOffset = baseOffset - origin;

  overlay->setPosition(baseOffset);
}

ChipAction* TenguCross::OnChargedBusterAction(Player& player)
{
  return new BusterChipAction(&player, true, 10);
}

ChipAction* TenguCross::OnSpecialAction(Player& player)
{
  return new TenguCross::SpecialAction(&player);
}

// HEAT CROSS

HeatCross::HeatCross()
{
  parentAnim = nullptr;
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
  overlay = new SpriteSceneNode();
  overlay->setTexture(*cross);
  overlay->SetLayer(-1);
  overlay->EnableParentShader(false);

  parentAnim = player.GetFirstComponent<AnimationComponent>();
  auto pswap = player.GetFirstComponent<PaletteSwap>();

  pswap->LoadPaletteTexture("resources/navis/megaman/forms/heat.palette.png");

  player.SetElement(Element::FIRE);

  loaded = true;

  OnUpdate(0.01f, player);
  player.AddNode(overlay);

  parentAnim->AddToOverrideList(&overlayAnimation);

}

void HeatCross::OnDeactivate(Player & player)
{
  player.RemoveNode(overlay);
  auto pswap = player.GetFirstComponent<PaletteSwap>();
  pswap->Revert();

  player.SetElement(Element::NONE);

  parentAnim->RemoveFromOverrideList(&overlayAnimation);

}

void HeatCross::OnUpdate(float elapsed, Player& player)
{
  overlay->setColor(player.getColor());

  parentAnim->SyncAnimation(overlayAnimation);
  overlayAnimation.Refresh(*overlay);

  // update node position in the animation
  auto baseOffset = parentAnim->GetPoint("Head");
  auto origin = player.operator sf::Sprite &().getOrigin();
  baseOffset = baseOffset - origin;

  overlay->setPosition(baseOffset);
}

ChipAction* HeatCross::OnChargedBusterAction(Player& player)
{
  return new FireBurnChipAction(&player, FireBurn::Type::_2, 60);
}

ChipAction* HeatCross::OnSpecialAction(Player& player)
{
  return nullptr;
}

// TOMAHAWK CROSS


TomahawkCross::TomahawkCross()
{
  parentAnim = nullptr;
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
  overlay = new SpriteSceneNode();
  overlay->setTexture(*cross);
  overlay->SetLayer(-1);
  overlay->EnableParentShader(false);

  parentAnim = player.GetFirstComponent<AnimationComponent>();
  auto pswap = player.GetFirstComponent<PaletteSwap>();

  pswap->LoadPaletteTexture("resources/navis/megaman/forms/hawk.palette.png");

  loaded = true;

  OnUpdate(0.01f, player);
  player.AddNode(overlay);

  parentAnim->AddToOverrideList(&overlayAnimation);

}

void TomahawkCross::OnDeactivate(Player & player)
{
  player.RemoveNode(overlay);
  auto pswap = player.GetFirstComponent<PaletteSwap>();
  pswap->Revert();

  parentAnim->RemoveFromOverrideList(&overlayAnimation);

}

void TomahawkCross::OnUpdate(float elapsed, Player& player)
{
  overlay->setColor(player.getColor());

  parentAnim->SyncAnimation(overlayAnimation);
  overlayAnimation.Refresh(*overlay);

  // update node position in the animation
  auto baseOffset = parentAnim->GetPoint("Head");
  auto origin = player.operator sf::Sprite &().getOrigin();
  baseOffset = baseOffset - origin;

  overlay->setPosition(baseOffset);
}

ChipAction* TomahawkCross::OnChargedBusterAction(Player& player)
{
  return new CrackShotChipAction(&player, 30);
}

ChipAction* TomahawkCross::OnSpecialAction(Player& player)
{
  return nullptr;
}

// SPECIAL ABILITY IMPLEMENTATIONS
#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.3 }

#define FRAMES FRAME1, FRAME2, FRAME3

TenguCross::SpecialAction::SpecialAction(Character* owner) : ChipAction(owner, "PLAYER_SWORD", &attachment, "HILT") {
  overlay.setTexture(*owner->getTexture());
  this->attachment = new SpriteSceneNode(overlay);
  this->attachment->SetLayer(-1);
  this->attachment->EnableParentShader(true);

  this->OverrideAnimationFrames({ FRAMES });

  attachmentAnim = Animation(owner->GetFirstComponent<AnimationComponent>()->GetFilePath());
  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("HAND");
}

TenguCross::SpecialAction::~SpecialAction()
{
}

void TenguCross::SpecialAction::OnUpdate(float _elapsed)
{
  ChipAction::OnUpdate(_elapsed);
  attachmentAnim.Update(_elapsed, *this->attachment);
}

void TenguCross::SpecialAction::Execute()
{
  auto owner = GetOwner();
  auto team = owner->GetTeam();
  auto field = owner->GetField();

  owner->AddNode(this->attachment);
  attachmentAnim.Update(0, *this->attachment);

  // On throw frame, spawn projectile
  auto onThrow = [this, owner, team, field]() -> void {
    auto wind = new Wind(field, team);
    field->AddEntity(*wind, 6, 1);

    wind = new Wind(field, team);
    field->AddEntity(*wind, 6, 2);

    wind = new Wind(field, team);
    field->AddEntity(*wind, 6, 3);
  };

  this->AddAction(3, onThrow);
}

void TenguCross::SpecialAction::EndAction()
{
  this->GetOwner()->RemoveNode(attachment);
  GetOwner()->FreeComponentByID(this->GetID());
  delete this;
}
