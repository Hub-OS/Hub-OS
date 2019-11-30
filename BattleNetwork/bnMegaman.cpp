#include "bnMegaman.h"
#include "bnShaderResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnBusterChipAction.h"
#include "bnCrackShotChipAction.h"
#include "bnFireBurnChipAction.h"
#include "bnTornadoChipAction.h"
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

void Megaman::ExecuteBusterAction()
{
  this->RegisterComponent(new BusterChipAction(this, false, 1));
}

void Megaman::ExecuteChargedBusterAction()
{
  if (activeForm) {
    activeForm->OnChargedBusterAction(*this);
  }
  else {
    this->RegisterComponent(new BusterChipAction(this, true, 10));
  }
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
  overlay->EnableUseParentShader(false);

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
  auto origin = player.operator sf::Sprite &().getOrigin();
  baseOffset = baseOffset - origin;

  overlay->setPosition(baseOffset);
}

void TenguCross::OnChargedBusterAction(Player& player)
{
  player.RegisterComponent(new TornadoChipAction(&player, 30));
}

void TenguCross::OnSpecialAction(Player& player)
{
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
  overlay->EnableUseParentShader(false);

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

void HeatCross::OnChargedBusterAction(Player& player)
{
  player.RegisterComponent(new FireBurnChipAction(&player, FireBurn::Type::_2, 60));
}

void HeatCross::OnSpecialAction(Player& player)
{
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
  overlay->EnableUseParentShader(false);

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

void TomahawkCross::OnChargedBusterAction(Player& player)
{
  player.RegisterComponent(new CrackShotChipAction(&player, 30));
}

void TomahawkCross::OnSpecialAction(Player& player)
{
}
