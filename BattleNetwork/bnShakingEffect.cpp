#include "bnShakingEffect.h"
#include "bnEntity.h"
#include "battlescene/bnBattleSceneBase.h"

ShakingEffect::ShakingEffect(std::weak_ptr<Entity> owner) : 
  shakeDur(0.35f),
  stress(3),
  shakeProgress(0),
  startPos(owner.lock()->getPosition()),
  bscene(nullptr),
  isShaking(false),
  Component(owner, Component::lifetimes::ui)
{
}

ShakingEffect::~ShakingEffect()
{
}

void ShakingEffect::OnUpdate(double _elapsed)
{
  auto owner = GetOwner();
  shakeProgress += _elapsed;

  if (owner && shakeProgress <= shakeDur) {
    // Drop off to zero by end of shake
    double currStress = stress * (1.0 - (shakeProgress / shakeDur));

    int randomAngle = static_cast<int>(shakeProgress) * (rand() % 360);
    randomAngle += (150 + (rand() % 60));

    auto shakeOffset = sf::Vector2f(std::sin(static_cast<float>(randomAngle * currStress)), std::cos(static_cast<float>(randomAngle * currStress)));

    owner->setPosition(startPos + shakeOffset);
  }
  else {
    Eject();
  }
}

void ShakingEffect::Inject(BattleSceneBase&bscene)
{
  bscene.Inject(shared_from_base<ShakingEffect>());
  ShakingEffect::bscene = &bscene;
}
