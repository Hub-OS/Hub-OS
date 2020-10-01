#include "bnPaletteSwap.h"
#include "bnTextureResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnEntity.h"

PaletteSwap::PaletteSwap(Entity * owner, std::shared_ptr<sf::Texture> base_palette) : Component(owner), base(base_palette), enabled(true)
{
  paletteSwap = ShaderResourceManager::GetInstance().GetShader(ShaderType::PALETTE_SWAP);
  paletteSwap->setUniform("palette", *base);
  paletteSwap->setUniform("texture", sf::Shader::CurrentTexture);
}

PaletteSwap::~PaletteSwap()
{
}

void PaletteSwap::OnUpdate(float _elapsed)
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
  palette = TextureResourceManager::GetInstance().LoadTextureFromFile(path);
  paletteSwap->setUniform("palette", *palette);

}

void PaletteSwap::SetTexture(const std::shared_ptr<sf::Texture>& texture)
{
  palette = texture;
  paletteSwap->setUniform("palette", *palette);
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
    GetOwner()->SetShader(paletteSwap);
  }
}
