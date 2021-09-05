#include "bnTomahawkman.h"

#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnDrawWindow.h"
#include "bnLogger.h"
#include "bnBusterCardAction.h"
#include "bnTomahawkSwingCardAction.h"
#include <Swoosh/Ease.h>

const float COPY_DROP_COOLDOWN = 0.15f; // in seconds

const std::string RESOURCE_PATH = "resources/navis/tomahawk/tomahawk.animation";

std::shared_ptr<CardAction> Tomahawkman::OnExecuteBusterAction()
{
  auto character = shared_from_base<Character>();
  return std::make_shared<BusterCardAction>(character, false, 1*GetAttackLevel());
}

std::shared_ptr<CardAction> Tomahawkman::OnExecuteChargedBusterAction()
{
  auto character = shared_from_base<Character>();
  return std::make_shared<BusterCardAction>(character, true, 10*GetAttackLevel());
}

std::shared_ptr<CardAction> Tomahawkman::OnExecuteSpecialAction() {
  auto character = shared_from_base<Character>();
  return std::make_shared<TomahawkSwingCardAction>(character, 10*GetAttackLevel() + 10);
}

Tomahawkman::Tomahawkman() : Player()
{
  chargeEffect.setPosition(0, -20.0f);
  SetName("Tomahawkman");
  SetLayer(0);
  team = Team::red;
  SetElement(Element::none);

  animationComponent->SetPath(RESOURCE_PATH);
  animationComponent->Reload();

  setTexture(Textures().LoadTextureFromFile("resources/navis/tomahawk/navi_tomahawk_atlas.png"));

  SetHealth(1000);

  chargeEffect.SetFullyChargedColor(sf::Color::Green);

  SetAnimation("PLAYER_IDLE");
  FinishConstructor();
}

Tomahawkman::~Tomahawkman()
{
}

const float Tomahawkman::GetHeight() const
{
  ;
  return 50.0f;
}

void Tomahawkman::OnUpdate(double _elapsed)
{
  // Continue with the parent class routine
  Player::OnUpdate(_elapsed);
}

void Tomahawkman::OnDelete()
{
  Player::OnDelete();
}
