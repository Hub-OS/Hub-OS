#include "bnRoll.h"
#include "bnExplodeState.h"
#include "bnField.h"
#include "bnBuster.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnEngine.h"
#include "bnLogger.h"

#define RESOURCE_PATH "resources/navis/roll/roll.animation"

Roll::Roll(void) : Player()
{
  name = "Roll";
  SetLayer(0);
  team = Team::RED;
  this->SetElement(Element::BREAK);

  animationComponent.Setup(RESOURCE_PATH);
  animationComponent.Reload();

  textureType = TextureType::NAVI_ROLL_ATLAS;
  setTexture(*TEXTURES.GetTexture(textureType));

  this->SetHealth(400);

  SetFloatShoe(true);
}