#include "bnShakingEffect.h"
#include "bnEntity.h"
#include "battlescene/bnBattleSceneBase.h"

ShakingEffect::ShakingEffect(Entity * owner) : Component(owner, Component::lifetimes::ui), 
privOwner(owner),
shakeDur(0.35f),
stress(3),
shakeProgress(0),
startPos(privOwner->getPosition()),
bscene(nullptr)
{
}

ShakingEffect::~ShakingEffect()
{
}

void ShakingEffect::OnUpdate(float _elapsed)
{
  shakeProgress += _elapsed;

  if (shakeProgress <= shakeDur) {
    // Drop off to zero by end of shake
    double currStress = stress * (1.0 - (shakeProgress / shakeDur));

    int randomAngle = int(shakeProgress) * (rand() % 360);
    randomAngle += (150 + (rand() % 60));

    auto shakeOffset = sf::Vector2f(std::sin((float)randomAngle) * float(currStress), std::cos((float)randomAngle) * float(currStress));
    privOwner->setPosition(startPos + shakeOffset);
  }
  else {
    bscene->Eject(GetID());
  }
}

void ShakingEffect::Inject(BattleSceneBase&bscene)
{
  bscene.Inject(this);
  ShakingEffect::bscene = &bscene;
}
