#include "bnRoll.h"
#include "bnExplodeState.h"
#include "bnField.h"
#include "bnBuster.h"
#include "bnRecoverCardAction.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnGame.h"
#include "bnLogger.h"
#include "bnBusterCardAction.h"

const std::string RESOURCE_PATH = "resources/navis/roll/roll.animation";

Roll::Roll() : Player()
{
  SetName("Roll");
  chargeEffect.setPosition(0, -30.0f);
  chargeEffect.SetFullyChargedColor(sf::Color::Yellow);

  SetLayer(0);
  SetTeam(Team::red);
  SetElement(Element::plus);

  animationComponent->SetPath(RESOURCE_PATH);
  animationComponent->Reload();

  setTexture(Textures().GetTexture(TextureType::NAVI_ROLL_ATLAS));

  SetHealth(1000);

  SetFloatShoe(true);
}

const float Roll::GetHeight() const
{
  return 140.0f;
}

CardAction * Roll::OnExecuteSpecialAction()
{
  return Player::OnExecuteSpecialAction();
}

CardAction* Roll::OnExecuteBusterAction()
{
  return new BusterCardAction(this, false, 1*GetAttackLevel());
}

CardAction* Roll::OnExecuteChargedBusterAction()
{
  return new BusterCardAction(this, true, 10*GetAttackLevel());
}