#include "bnStarman.h"
#include "bnExplodeState.h"
#include "bnField.h"
#include "bnBuster.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnEngine.h"
#include "bnLogger.h"

#define RESOURCE_PATH "resources/navis/starman/starman.animation"

#define MOVE_ANIMATION_SPRITES 4
#define MOVE_ANIMATION_WIDTH 38
#define MOVE_ANIMATION_HEIGHT 58

#define SHOOT_ANIMATION_SPRITES 5
#define SHOOT_ANIMATION_WIDTH 75
#define SHOOT_ANIMATION_HEIGHT 58

Starman::Starman(void) : Player()
{
  name = "Starman";
  SetLayer(0);
  team = Team::RED;
  this->SetElement(Element::CURSOR);

  animationComponent.Setup(RESOURCE_PATH);
  animationComponent.Reload();

  textureType = TextureType::NAVI_STARMAN_ATLAS;
  setTexture(*TEXTURES.GetTexture(textureType));

  SetFloatShoe(true);
}
