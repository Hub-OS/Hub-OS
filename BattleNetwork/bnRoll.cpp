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
  
  // First sprite on the screen should be default player stance
  SetAnimation("PLAYER_IDLE");

  FinishConstructor();
}

const float Roll::GetHeight() const
{
  return 70.0f;
}

std::shared_ptr<CardAction> Roll::OnExecuteSpecialAction()
{
  return Player::OnExecuteSpecialAction();
}

std::shared_ptr<CardAction> Roll::OnExecuteBusterAction()
{
  auto character = shared_from_base<Character>();
  return std::make_shared<BusterCardAction>(character, false, 1*GetAttackLevel());
}

std::shared_ptr<CardAction> Roll::OnExecuteChargedBusterAction()
{
  auto character = shared_from_base<Character>();
  return std::make_shared<BusterCardAction>(character, true, 10*GetAttackLevel());
}