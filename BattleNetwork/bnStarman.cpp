#include "bnStarman.h"
#include "bnExplodeState.h"
#include "bnField.h"
#include "bnBuster.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnGame.h"
#include "bnLogger.h"
#include "bnBusterCardAction.h"

#define RESOURCE_PATH "resources/navis/starman/starman.animation"

Starman::Starman() : Player()
{
  // Most of this is pretty redundant
  // But left as example
  SetName("Starman");
  SetLayer(0);
  SetTeam(Team::red);
  SetElement(Element::none);

  animationComponent->SetPath(RESOURCE_PATH);
  animationComponent->Reload();

  setTexture(Textures().GetTexture(TextureType::NAVI_STARMAN_ATLAS));

  SetHealth(1200);

  // Starman has FloatShoe enabled
  SetFloatShoe(true);
  SetAnimation("PLAYER_IDLE");

  FinishConstructor();
}

Starman::~Starman() {

}

const float Starman::GetHeight() const
{
  return 70.0f;
}

std::shared_ptr<CardAction> Starman::OnExecuteBusterAction()
{
  auto character = shared_from_base<Character>();
  return std::make_shared<BusterCardAction>(character, false, 1*GetAttackLevel());
}

std::shared_ptr<CardAction> Starman::OnExecuteChargedBusterAction()
{
  auto character = shared_from_base<Character>();
  return std::make_shared<BusterCardAction>(character, true, 10*GetAttackLevel());
}

std::shared_ptr<CardAction> Starman::OnExecuteSpecialAction()
{
  return Player::OnExecuteSpecialAction();
}
