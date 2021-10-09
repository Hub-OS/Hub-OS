#include "bnPaletteSwap.h"
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnEntity.h"
#include "bnCharacter.h"

PaletteSwap::PaletteSwap(Entity * owner) : Component(owner), enabled(true)
{
  asCharacter = dynamic_cast<Character*>(owner);
}

PaletteSwap::~PaletteSwap()
{
}

void PaletteSwap::OnUpdate(double _elapsed)
{
}

void PaletteSwap::Inject(BattleSceneBase&)
{
}

void PaletteSwap::LoadPaletteTexture(std::string path)
{
  palette = ResourceHandle().Textures().LoadTextureFromFile(path);
  asCharacter->SetPalette(palette);
}

void PaletteSwap::CopyFrom(PaletteSwap& other)
{
  enabled = other.enabled;
  base = other.base;
  palette = other.palette;
  Apply();
}

void PaletteSwap::SetPalette(const std::shared_ptr<sf::Texture>& palette)
{
  this->palette = palette;
  asCharacter->SetPalette(palette);
}

void PaletteSwap::SetBase(const std::shared_ptr<sf::Texture>& base)
{
  this->base = base;
  asCharacter->SetPalette(base);
}

void PaletteSwap::Revert()
{
  SetPalette(base);
}

void PaletteSwap::Enable(bool enabled)
{
  PaletteSwap::enabled = enabled;

  // Apply it on this call
  // Don't wait for the next frame (update())
  // Otherwise blocky effects occur
  //if (enabled) {
  //  Apply();
  //}
  //else {
  //  GetOwner()->SetShader(nullptr);
  //}
}

void PaletteSwap::Apply()
{
  if (std::shared_ptr<sf::Texture> p = enabled ? palette : base; p && asCharacter) {
    asCharacter->SetPalette(p);
  }
}

const bool PaletteSwap::IsEnabled() const
{
  return enabled;
}
