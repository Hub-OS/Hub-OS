#include "bnMegaman.h"
#include "bnShaderResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnBusterChipAction.h"
#include "bnCrackShotChipAction.h"

Megaman::Megaman() : Player() {
  palette = *TextureResourceManager::GetInstance().LoadTextureFromFile("resources/navis/megaman/forms/base.palette.png");
  paletteSwap = ShaderResourceManager::GetInstance().GetShader(ShaderType::PALETTE_SWAP);
  paletteSwap->setUniform("palette", palette);
  paletteSwap->setUniform("texture", sf::Shader::CurrentTexture);

  SetHealth(900);
  SetName("Megaman");
  setTexture(*TEXTURES.GetTexture(TextureType::NAVI_MEGAMAN_ATLAS));

  this->AddForm<TenguCross>()->SetUIPath("resources/navis/megaman/forms/tengu_entry.png");
  this->AddForm<TenguCross>()->SetUIPath("resources/navis/megaman/forms/hawk_entry.png");
  this->AddForm<TenguCross>()->SetUIPath("resources/navis/megaman/forms/heat_entry.png");

}

Megaman::~Megaman()
{
}

void Megaman::OnUpdate(float elapsed)
{
  SetShader(paletteSwap);

  Player::OnUpdate(elapsed);

  if (activeForm) {
    palette = *TextureResourceManager::GetInstance().LoadTextureFromFile("resources/navis/megaman/forms/palette.png");
    paletteSwap->setUniform("palette", palette);
  }
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

  loaded = true;
}

void TenguCross::OnDeactivate(Player & player)
{
  player.RemoveNode(overlay);
}

void TenguCross::OnUpdate(float elapsed, Player& player)
{
  if (parentAnim->GetAnimationString() != overlayAnimation.GetAnimationString()) {
    overlayAnimation.SetAnimation(parentAnim->GetAnimationString());
  }

  overlayAnimation.Update(elapsed, *overlay);

  // update node position in the animation
  auto baseOffset = parentAnim->GetPoint("Head");
  auto origin = player.operator sf::Sprite &().getOrigin();
  baseOffset = baseOffset - origin;

  overlay->setPosition(baseOffset);

  // TODO: do once for demo and then refactor this setup
  if (loaded) {
    loaded = false;
    player.AddNode(overlay);
  }
}

void TenguCross::OnChargedBusterAction(Player& player)
{
  player.RegisterComponent(new CrackShotChipAction(&player, 30));
}

void TenguCross::OnSpecialAction(Player& player)
{
}
