#include "bnForte.h"
#include "bnExplodeState.h"
#include "bnField.h"
#include "bnBuster.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnGame.h"
#include "bnLogger.h"
#include "bnVulcanCardAction.h"
#include "bnBusterCardAction.h"
#include <Swoosh/Ease.h>

const float COPY_DROP_COOLDOWN = 0.15f; // in seconds

const std::string RESOURCE_PATH = "resources/navis/forte/forte.animation";

std::shared_ptr<CardAction> Forte::OnExecuteBusterAction()
{
  return std::make_shared<BusterCardAction>(shared_from_base<Character>(), false, 1*GetAttackLevel());
}

std::shared_ptr<CardAction> Forte::OnExecuteChargedBusterAction()
{
  return std::make_shared<VulcanCardAction>(shared_from_base<Character>(), 10*GetAttackLevel());
}

std::shared_ptr<CardAction> Forte::OnExecuteSpecialAction() {
  return Player::OnExecuteSpecialAction();
}

Forte::Forte() : Player()
{
  chargeEffect.setPosition(0, -40.0f);
  SetName("Bass");
  SetLayer(0);
  team = Team::red;
  SetElement(Element::none);

  animationComponent->SetPath(RESOURCE_PATH);
  animationComponent->Reload();

  setTexture(Textures().GetTexture(TextureType::NAVI_FORTE_ATLAS));

  SetHealth(1000);

  SetFloatShoe(true);

  //aura->setPosition(0, -20.0f);

  dropCooldown = COPY_DROP_COOLDOWN;

  // Bass slides around lookin pretty slick
  SlideWhenMoving(true, frames(4));

  chargeEffect.SetFullyChargedColor(sf::Color::Magenta);

  SetAnimation("PLAYER_IDLE");
  FinishConstructor();
}

Forte::~Forte()
{
}

const float Forte::GetHeight() const
{
  return 80.0f;
}

void Forte::OnUpdate(double _elapsed)
{
  dropCooldown -= _elapsed;

  // Continue with the parent class routine
  Player::OnUpdate(_elapsed);

  // We are moving
  if (IsMoving()) {
    if (dropCooldown <= 0) {
      auto fx = std::make_shared<MoveEffect>(field);

      if (GetTeam() == Team::blue) {
        const auto& scale = fx->getScale();
        fx->setScale(-scale.x, scale.y);
      }

      field->AddEntity(fx, *GetTile());
    }
    else {
      dropCooldown = COPY_DROP_COOLDOWN;
    }
  }
}

void Forte::OnDelete()
{
  Player::OnDelete();
}

void Forte::OnSpawn(Battle::Tile& start)
{
  CreateComponent<Aura>(Aura::Type::AURA_100, shared_from_base<Character>())->Persist(false);
}

int Forte::MoveEffect::counter = 0;

Forte::MoveEffect::MoveEffect(Field* field) : 
  elapsed(0), 
  index(0), 
  Artifact()
{
  setTexture(Textures().GetTexture(TextureType::NAVI_FORTE_ATLAS));

  SetLayer(1);

  setScale(2.f, 2.f);

  animationComponent = CreateComponent<AnimationComponent>(weak_from_this());
  animationComponent->SetPath(RESOURCE_PATH);
  animationComponent->Reload();
  animationComponent->SetAnimation("COPY");

  index = ++counter;

  animationComponent->SetFrame((counter % 2));
}

Forte::MoveEffect::~MoveEffect()
{
}

void Forte::MoveEffect::OnUpdate(double _elapsed)
{
  elapsed += _elapsed;
  auto delta = 1.0 - swoosh::ease::linear(elapsed, 0.1, 1.0);

  SetAlpha(static_cast<int>(delta*125));

  if (delta <= 0.0) {
    Delete();
  }
}

void Forte::MoveEffect::OnDelete()
{
  Remove();
}
