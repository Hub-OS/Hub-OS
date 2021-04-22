#include "bnPaletteSwap.h"
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnEntity.h"

PaletteSwap::PaletteSwap(Entity * owner) : Component(owner), enabled(true)
{
  paletteSwap = ResourceHandle().Shaders().GetShader(ShaderType::PALETTE_SWAP);
  paletteSwap->setUniform("texture", sf::Shader::CurrentTexture);
}

PaletteSwap::~PaletteSwap()
{
}

void PaletteSwap::OnUpdate(double _elapsed)
{
  if (!enabled) return;
  if (GetOwner()->GetShader().Get() == paletteSwap) return;
  GetOwner()->SetShader(paletteSwap);
}

void PaletteSwap::Inject(BattleSceneBase&)
{
}

void PaletteSwap::LoadPaletteTexture(std::string path)
{
  palette = ResourceHandle().Textures().LoadTextureFromFile(path);
  paletteSwap->setUniform("palette", *palette);

}

void PaletteSwap::CopyFrom(PaletteSwap& other)
{
  base = other.base;
  palette = other.palette;
  Apply();
}

void PaletteSwap::SetTexture(const std::shared_ptr<sf::Texture>& texture)
{
  palette = texture;
  paletteSwap->setUniform("palette", *palette);
}

void PaletteSwap::SetBase(const std::shared_ptr<sf::Texture>& base)
{
  this->base = base;
  paletteSwap->setUniform("palette", *base);
}

void PaletteSwap::Revert()
{
  SetTexture(base);
}

void PaletteSwap::Enable(bool enabled)
{
  PaletteSwap::enabled = enabled;

  // Apply it on this call
  // Don't wait for the next frame (update())
  // Otherwise blocky effects occur
  if (enabled) {
    Apply();
  }
  else {
    GetOwner()->SetShader(nullptr);
  }
}

void PaletteSwap::Apply()
{
  GetOwner()->SetShader(paletteSwap);
}

const bool PaletteSwap::IsEnabled() const
{
  return enabled;
}
