#include "bnRoll.h"
#include "bnExplodeState.h"
#include "bnField.h"
#include "bnBuster.h"
#include "bnRecoverCardAction.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnEngine.h"
#include "bnLogger.h"
#include "bnBusterCardAction.h"

const std::string RESOURCE_PATH = "resources/navis/roll/roll.animation";

Roll::Roll() : Player()
{
  name = "Roll";
  chargeEffect.setPosition(0, -30.0f);
  chargeEffect.SetFullyChargedColor(sf::Color::Yellow);

  SetLayer(0);
  team = Team::RED;
  this->SetElement(Element::PLUS);

  animationComponent->SetPath(RESOURCE_PATH);
  animationComponent->Reload();

  setTexture(TEXTURES.GetTexture(TextureType::NAVI_ROLL_ATLAS));

  this->SetHealth(1500);

  SetFloatShoe(true);
}

const float Roll::GetHeight() const
{
  return 140.0f;
}

CardAction * Roll::ExecuteSpecialAction()
{
  return new RecoverCardAction(this, 20);
}

CardAction* Roll::ExecuteBusterAction()
{
  return new BusterCardAction(this, false, 1);
}

CardAction* Roll::ExecuteChargedBusterAction()
{
  return new BusterCardAction(this, true, 20);
}