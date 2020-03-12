#include "bnStarman.h"
#include "bnExplodeState.h"
#include "bnField.h"
#include "bnBuster.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnEngine.h"
#include "bnLogger.h"
#include "bnBusterCardAction.h"

#define RESOURCE_PATH "resources/navis/starman/starman.animation"

Starman::Starman() : Player()
{
  // Most of this is pretty redundant
  // But left as example
  name = "Starman";
  SetLayer(0);
  team = Team::RED;
  this->SetElement(Element::NONE);

  animationComponent->Setup(RESOURCE_PATH);
  animationComponent->Reload();

  setTexture(*TEXTURES.GetTexture(TextureType::NAVI_STARMAN_ATLAS));

  this->SetHealth(1000);

  // Starman has FloatShoe enabled
  SetFloatShoe(true);
}

Starman::~Starman() {

}

const float Starman::GetHeight() const
{
  return 140.0f;
}

CardAction* Starman::ExecuteBusterAction()
{
  return new BusterCardAction(this, false, 1);
}

CardAction* Starman::ExecuteChargedBusterAction()
{
  return new BusterCardAction(this, true, 10);
}

CardAction * Starman::ExecuteSpecialAction()
{
  return nullptr;
}
