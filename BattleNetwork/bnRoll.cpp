#include "bnRoll.h"
#include "bnExplodeState.h"
#include "bnField.h"
#include "bnBuster.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnEngine.h"
#include "bnLogger.h"

const std::string RESOURCE_PATH = "resources/navis/roll/roll.animation";

Roll::Roll() : Player()
{
  name = "Roll";
  SetLayer(0);
  team = Team::RED;
  this->SetElement(Element::PLUS);

  animationComponent->Setup(RESOURCE_PATH);
  animationComponent->Reload();

  setTexture(*TEXTURES.GetTexture(TextureType::NAVI_ROLL_ATLAS));

  this->SetHealth(1500);

  SetFloatShoe(true);

  SetBusterDamage(2);
  SetChargedBusterDamage(20);
}

const float Roll::GetHitHeight() const
{
  return 140.0f;
}
