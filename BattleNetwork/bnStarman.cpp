#include "bnStarman.h"
#include "bnExplodeState.h"
#include "bnField.h"
#include "bnBuster.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnEngine.h"
#include "bnLogger.h"

#define RESOURCE_PATH "resources/navis/starman/starman.animation"

Starman::Starman() : Player()
{
  // Most of this is pretty redundant
  // But left as example
  name = "Starman";
  SetLayer(0);
  team = Team::RED;
  this->SetElement(Element::CURSOR);

  animationComponent.Setup(RESOURCE_PATH);
  animationComponent.Reload();

  textureType = TextureType::NAVI_STARMAN_ATLAS;
  setTexture(*TEXTURES.GetTexture(textureType));

  // Starman has FloatShoe enabled
  SetFloatShoe(true);
}
