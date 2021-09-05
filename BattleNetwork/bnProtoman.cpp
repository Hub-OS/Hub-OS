#include "bnProtoman.h"

#include "bnExplodeState.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnDrawWindow.h"
#include "bnLogger.h"
#include "bnBusterCardAction.h"
#include "bnReflectCardAction.h"
#include "bnWideSwordCardAction.h"
#include <Swoosh/Ease.h>

const float COPY_DROP_COOLDOWN = 0.15f; // in seconds

const std::string RESOURCE_PATH = "resources/navis/protoman/protoman.animation";

std::shared_ptr<CardAction> Protoman::OnExecuteBusterAction()
{
  auto character = shared_from_base<Character>();
  return std::make_shared<BusterCardAction>(character, false, 1*GetAttackLevel());
}

std::shared_ptr<CardAction> Protoman::OnExecuteChargedBusterAction()
{
  auto character = shared_from_base<Character>();
  return std::make_shared<WideSwordCardAction>(character, 20*GetAttackLevel());
}

std::shared_ptr<CardAction> Protoman::OnExecuteSpecialAction() {
  auto character = shared_from_base<Character>();
  auto action = std::make_shared<ReflectCardAction>(character, 20, ReflectShield::Type::red);
  action->SetLockout({
    CardAction::LockoutType::async,
    seconds_cast<double>(frames(80))
  });
  action->SetDuration(frames(21));

  return action;
}

Protoman::Protoman() : Player()
{
  chargeEffect.setPosition(0, -20.0f);
  SetName("Protoman");
  SetLayer(0);
  team = Team::red;
  SetElement(Element::none);

  animationComponent->SetPath(RESOURCE_PATH);
  animationComponent->Reload();

  setTexture(Textures().LoadTextureFromFile("resources/navis/protoman/navi_proto_atlas.png"));

  SetHealth(1000);

  chargeEffect.SetFullyChargedColor(sf::Color::White);

  SetAnimation("PLAYER_IDLE");

  FinishConstructor();
}

Protoman::~Protoman()
{
}

const float Protoman::GetHeight() const
{
  return 50.0f;
}

void Protoman::OnUpdate(double _elapsed)
{
  // Continue with the parent class routine
  Player::OnUpdate(_elapsed);
}

void Protoman::OnDelete()
{
  Player::OnDelete();
}
